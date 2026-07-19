// Fill out your copyright notice in the Description page of Project Settings.

#include "Cache/RssImageCache.h"
#include "Async/Async.h"
#include "Compression/CompressedBuffer.h"
#include "Engine/Texture2DDynamic.h"
#include "HAL/PlatformFileManager.h"
#include "HttpModule.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"
#include "Widget/RssDownloadImage.h"

DEFINE_LOG_CATEGORY(RSSImageCacheLog);

TMap<TWeakObjectPtr<UWorld>, URssImageCacheManager*> URssImageCacheManager::Instances;

URssImageCacheManager::URssImageCacheManager()
{
}

URssImageCacheManager* URssImageCacheManager::Get(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		return nullptr;
	}

	TWeakObjectPtr<UWorld> WorldPtr(World);

	// Check if we already have an instance for this world
	if (URssImageCacheManager** FoundInstance = Instances.Find(WorldPtr))
	{
		if (*FoundInstance && IsValid(*FoundInstance))
		{
			return *FoundInstance;
		}
	}

	// Create new instance
	URssImageCacheManager* NewInstance = NewObject<URssImageCacheManager>(World);
	NewInstance->AddToRoot(); // Prevent GC
	Instances.Add(WorldPtr, NewInstance);

	// Auto-initialize with defaults
	FRssImageCacheConfig DefaultConfig;
	NewInstance->Initialize(DefaultConfig);

	return NewInstance;
}

void URssImageCacheManager::Initialize(const FRssImageCacheConfig& Config)
{
	if (bIsInitialized)
	{
		return;
	}

	CacheConfig = Config;

	// Create cache directory if it doesn't exist
	if (CacheConfig.bEnableDiskCache)
	{
		FString CachePath = FPaths::Combine(FPaths::ProjectSavedDir(), CacheConfig.CacheDirectory);
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		if (!PlatformFile.DirectoryExists(*CachePath))
		{
			PlatformFile.CreateDirectoryTree(*CachePath);
		}

		// Load disk cache index
		FString IndexPath = FPaths::Combine(CachePath, TEXT("cache_index.txt"));
		FString IndexContent;
		FScopeLock Lock(&DiskLock);
		if (FFileHelper::LoadFileToString(IndexContent, *IndexPath))
		{
			TArray<FString> Lines;
			IndexContent.ParseIntoArrayLines(Lines);
			for (const FString& Line : Lines)
			{
				FString Url, Hash;
				if (Line.Split(TEXT("|"), &Url, &Hash))
				{
					DiskCacheIndex.Add(Url, Hash);
				}
			}
		}

		// Calculate current disk usage
		CurrentDiskUsage = 0;
		for (const auto& Entry : DiskCacheIndex)
		{
			FString FilePath = GetCacheFilePath(Entry.Key);
			if (PlatformFile.FileExists(*FilePath))
			{
				CurrentDiskUsage += PlatformFile.FileSize(*FilePath);
			}
		}
	}

	bIsInitialized = true;
	UE_LOG(RSSImageCacheLog, Log, TEXT("Image cache initialized. Disk cache entries: %d, Disk usage: %lld bytes"),
	       DiskCacheIndex.Num(), CurrentDiskUsage);
}

void URssImageCacheManager::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}

	// Save disk cache index
	if (CacheConfig.bEnableDiskCache)
	{
		FString CachePath = FPaths::Combine(FPaths::ProjectSavedDir(), CacheConfig.CacheDirectory);
		FString IndexPath = FPaths::Combine(CachePath, TEXT("cache_index.txt"));

		FString IndexContent;
		{
			FScopeLock Lock(&DiskLock);
			for (const auto& Entry : DiskCacheIndex)
			{
				IndexContent += FString::Printf(TEXT("%s|%s\n"), *Entry.Key, *Entry.Value);
			}
		}
		FFileHelper::SaveStringToFile(IndexContent, *IndexPath);
	}

	// Clear memory cache
	{
		FScopeLock Lock(&CacheLock);
		MemoryCache.Empty();
		CurrentMemoryUsage = 0;
	}

	bIsInitialized = false;
	RemoveFromRoot();

	// Remove from instances
	for (auto It = Instances.CreateIterator(); It; ++It)
	{
		if (It->Value == this)
		{
			It.RemoveCurrent();
			break;
		}
	}
}

bool URssImageCacheManager::RequestImage(const FString& Url, bool bPersistent)
{
	if (Url.IsEmpty() || !URssDownloadImage::IsSafeRemoteImageUrl(Url))
	{
		return false;
	}

	// Check memory cache first
	{
		FScopeLock Lock(&CacheLock);
		if (FRssImageCacheEntry* Entry = MemoryCache.Find(Url))
		{
			if (Entry->IsValid())
			{
				Entry->LastAccessTime = FDateTime::Now();
				Entry->RefCount++;

				// Notify immediately
				TWeakObjectPtr<URssImageCacheManager> WeakThis(this);
				AsyncTask(ENamedThreads::GameThread,
				          [WeakThis, Url, Texture = Entry->Texture]()
				          {
					          if (!WeakThis.IsValid())
					          {
						          return;
					          }
					          WeakThis->OnImageLoaded.Broadcast(Url, Texture);
				          });
				return true;
			}
		}
	}

	// Check if already downloading
	{
		FScopeLock Lock(&DownloadLock);
		if (PendingDownloads.Contains(Url))
		{
			return true; // Already in progress
		}
	}

	// Try to load from disk cache
	bool bHasDiskEntry = false;
	if (CacheConfig.bEnableDiskCache)
	{
		FScopeLock Lock(&DiskLock);
		bHasDiskEntry = DiskCacheIndex.Contains(Url);
	}
	if (bHasDiskEntry)
	{
		// Load async from disk without extending this UObject's lifetime across world teardown.
		TWeakObjectPtr<URssImageCacheManager> WeakThis(this);
		AsyncTask(
			ENamedThreads::AnyBackgroundThreadNormalTask,
			[WeakThis, Url, bPersistent]()
			{
				if (!WeakThis.IsValid())
				{
					return;
				}

				TArray<uint8> RawData;
				if (WeakThis->LoadFromDiskCache(Url, RawData) &&
					RawData.Num() <= URssDownloadImage::MaxCompressedImageBytes)
				{
					// Decode image on background thread
					IImageWrapperModule& ImageWrapperModule =
						FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

					const EImageFormat ImageFormat =
						ImageWrapperModule.DetectImageFormat(RawData.GetData(), RawData.Num());
					TSharedPtr<IImageWrapper> ImageWrappers[1];
					if (ImageFormat != EImageFormat::Invalid)
					{
						ImageWrappers[0] = ImageWrapperModule.CreateImageWrapper(ImageFormat);
					}

					for (auto& ImageWrapper : ImageWrappers)
					{
						if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawData.GetData(), RawData.Num()))
						{
							const int32 Width = ImageWrapper->GetWidth();
							const int32 Height = ImageWrapper->GetHeight();
							int64 RawBytes = 0;
							if (!URssDownloadImage::ValidateImageDimensions(
								RawData.Num(), Width, Height, RawBytes))
							{
								continue;
							}

							TArray64<uint8> DecompressedData;
							if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, DecompressedData))
							{
								// Create texture on game thread
								AsyncTask(
									ENamedThreads::GameThread,
									[WeakThis, Url, Width, Height, RawBytes,
										DecompressedData = MoveTemp(DecompressedData), bPersistent]() mutable
									{
										if (!WeakThis.IsValid())
										{
											return;
										}
										UTexture2DDynamic* Texture = UTexture2DDynamic::Create(Width, Height);
										if (Texture)
										{
											Texture->SRGB = true;
											Texture->UpdateResource();

											// Write texture data
											FTexture2DDynamicResource* TextureResource =
												static_cast<FTexture2DDynamicResource*>(Texture->GetResource());
											if (TextureResource)
											{
												TArray64<uint8>* RawDataCopy =
													new TArray64<uint8>(MoveTemp(DecompressedData));
												ENQUEUE_RENDER_COMMAND(FWriteRawDataToTexture)(
													[TextureResource, RawDataCopy, Width,
														Height](FRHICommandListImmediate& RHICmdList)
													{
														FRHITexture* TextureRHI = TextureResource->GetTexture2DRHI();
														if (TextureRHI)
														{
															uint32 DestStride = 0;
															uint8* DestData = reinterpret_cast<uint8*>(
																RHILockTexture2D(TextureRHI, 0, RLM_WriteOnly,
																	DestStride, false, false));

															for (int32 y = 0; y < Height; y++)
															{
																uint8* DestPtr =
																	&DestData[(static_cast<int64>(Height) - 1 - y) *
																		DestStride];
																const FColor* SrcPtr =
																	&((FColor*)(RawDataCopy->GetData()))
																	[(static_cast<int64>(Height) - 1 - y) * Width];
																for (int32 x = 0; x < Width; x++)
																{
																	*DestPtr++ = SrcPtr->B;
																	*DestPtr++ = SrcPtr->G;
																	*DestPtr++ = SrcPtr->R;
																	*DestPtr++ = SrcPtr->A;
																	SrcPtr++;
																}
															}

															RHIUnlockTexture2D(TextureRHI, 0, false, false);
														}
														delete RawDataCopy;
													});
											}

											WeakThis->AddToMemoryCache(Url, Texture, RawBytes, bPersistent);
											WeakThis->OnImageLoaded.Broadcast(Url, Texture);
										}
										else
										{
											WeakThis->OnImageFailed.Broadcast(Url);
										}
									});
								return;
							}
						}
					}

					// Failed to decode
					AsyncTask(ENamedThreads::GameThread,
					          [WeakThis, Url]()
					          {
						          if (!WeakThis.IsValid())
						          {
							          return;
						          }
						          WeakThis->OnImageFailed.Broadcast(Url);
					          });
				}
				else
				{
					// Disk load failed, try download
					AsyncTask(ENamedThreads::GameThread,
					          [WeakThis, Url, bPersistent]()
					          {
						          if (!WeakThis.IsValid())
						          {
							          return;
						          }

						          // Remove from disk index since it's invalid
						          {
							          FScopeLock Lock(&WeakThis->DiskLock);
							          WeakThis->DiskCacheIndex.Remove(Url);
						          }

						          // Start download
						          WeakThis->StartDownload(Url, bPersistent);
					          });
				}
			});
		return true;
	}

	// Start download
	return StartDownload(Url, bPersistent);
}

bool URssImageCacheManager::StartDownload(const FString& Url, bool bPersistent)
{
	if (!URssDownloadImage::IsSafeRemoteImageUrl(Url))
	{
		OnImageFailed.Broadcast(Url);
		return false;
	}

	{
		FScopeLock Lock(&DownloadLock);
		if (PendingDownloads.Contains(Url))
		{
			return true;
		}
		PendingDownloads.Add(Url);
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(Url);
	HttpRequest->SetVerb(TEXT("GET"));

	TWeakObjectPtr<URssImageCacheManager> WeakThis(this);
	HttpRequest->OnProcessRequestComplete().BindLambda(
		[WeakThis, Url, bPersistent](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSucceeded)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			{
				FScopeLock Lock(&WeakThis->DownloadLock);
				WeakThis->PendingDownloads.Remove(Url);
			}

			if (bSucceeded && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()) &&
				URssDownloadImage::IsSafeRemoteImageUrl(Response->GetURL()))
			{
				TArray<uint8> RawData = Response->GetContent();
				if (RawData.IsEmpty() || RawData.Num() > URssDownloadImage::MaxCompressedImageBytes)
				{
					WeakThis->OnImageFailed.Broadcast(Url);
					return;
				}

				IImageWrapperModule& ImageWrapperModule =
					FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

				const EImageFormat ImageFormat =
					ImageWrapperModule.DetectImageFormat(RawData.GetData(), RawData.Num());
				TSharedPtr<IImageWrapper> ImageWrappers[1];
				if (ImageFormat != EImageFormat::Invalid)
				{
					ImageWrappers[0] = ImageWrapperModule.CreateImageWrapper(ImageFormat);
				}

				for (auto& ImageWrapper : ImageWrappers)
				{
					if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawData.GetData(), RawData.Num()))
					{
						const int32 Width = ImageWrapper->GetWidth();
						const int32 Height = ImageWrapper->GetHeight();
						int64 RawBytes = 0;
						if (!URssDownloadImage::ValidateImageDimensions(RawData.Num(), Width, Height, RawBytes))
						{
							continue;
						}

						TArray64<uint8> DecompressedData;
						if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, DecompressedData))
						{
							UTexture2DDynamic* Texture = UTexture2DDynamic::Create(Width, Height);
							if (Texture)
							{
								Texture->SRGB = true;
								Texture->UpdateResource();

								FTexture2DDynamicResource* TextureResource =
									static_cast<FTexture2DDynamicResource*>(Texture->GetResource());
								if (TextureResource)
								{
									TArray64<uint8>* RawDataCopy = new TArray64<uint8>(MoveTemp(DecompressedData));
									ENQUEUE_RENDER_COMMAND(FWriteRawDataToTexture)(
										[TextureResource, RawDataCopy, Width,
											Height](FRHICommandListImmediate& RHICmdList)
										{
											FRHITexture* TextureRHI = TextureResource->GetTexture2DRHI();
											if (TextureRHI)
											{
												uint32 DestStride = 0;
												uint8* DestData = reinterpret_cast<uint8*>(RHILockTexture2D(
													TextureRHI, 0, RLM_WriteOnly, DestStride, false, false));

												for (int32 y = 0; y < Height; y++)
												{
													uint8* DestPtr =
														&DestData[(static_cast<int64>(Height) - 1 - y) * DestStride];
													const FColor* SrcPtr =
														&((FColor*)(RawDataCopy->GetData()))
														[(static_cast<int64>(Height) - 1 - y) * Width];
													for (int32 x = 0; x < Width; x++)
													{
														*DestPtr++ = SrcPtr->B;
														*DestPtr++ = SrcPtr->G;
														*DestPtr++ = SrcPtr->R;
														*DestPtr++ = SrcPtr->A;
														SrcPtr++;
													}
												}

												RHIUnlockTexture2D(TextureRHI, 0, false, false);
											}
											delete RawDataCopy;
										});
								}

								WeakThis->AddToMemoryCache(Url, Texture, RawBytes, bPersistent);

								// Save to disk cache
								if (bPersistent && WeakThis->CacheConfig.bEnableDiskCache)
								{
									WeakThis->SaveToDiskCache(Url, RawData);
								}

								WeakThis->OnImageLoaded.Broadcast(Url, Texture);
								return;
							}
						}
					}
				}
			}

			WeakThis->OnImageFailed.Broadcast(Url);
		});

	if (HttpRequest->ProcessRequest())
	{
		return true;
	}

	{
		FScopeLock Lock(&DownloadLock);
		PendingDownloads.Remove(Url);
	}
	OnImageFailed.Broadcast(Url);
	return false;
}

bool URssImageCacheManager::GetCachedImage(const FString& Url, UTexture2DDynamic*& OutTexture)
{
	FScopeLock Lock(&CacheLock);
	if (FRssImageCacheEntry* Entry = MemoryCache.Find(Url))
	{
		if (Entry->IsValid())
		{
			Entry->LastAccessTime = FDateTime::Now();
			OutTexture = Entry->Texture;
			return true;
		}
	}
	OutTexture = nullptr;
	return false;
}

bool URssImageCacheManager::IsImageCached(const FString& Url) const
{
	{
		FScopeLock Lock(&CacheLock);
		if (const FRssImageCacheEntry* Entry = MemoryCache.Find(Url))
		{
			return Entry->IsValid();
		}
	}
	FScopeLock DiskScopeLock(&DiskLock);
	return DiskCacheIndex.Contains(Url);
}

bool URssImageCacheManager::IsImageLoading(const FString& Url) const
{
	FScopeLock Lock(&DownloadLock);
	return PendingDownloads.Contains(Url);
}

void URssImageCacheManager::AddReference(const FString& Url)
{
	FScopeLock Lock(&CacheLock);
	if (FRssImageCacheEntry* Entry = MemoryCache.Find(Url))
	{
		Entry->RefCount++;
		Entry->LastAccessTime = FDateTime::Now();
	}
}

void URssImageCacheManager::RemoveReference(const FString& Url)
{
	FScopeLock Lock(&CacheLock);
	if (FRssImageCacheEntry* Entry = MemoryCache.Find(Url))
	{
		Entry->RefCount = FMath::Max(0, Entry->RefCount - 1);
	}
}

int32 URssImageCacheManager::GetReferenceCount(const FString& Url) const
{
	FScopeLock Lock(&CacheLock);
	if (const FRssImageCacheEntry* Entry = MemoryCache.Find(Url))
	{
		return Entry->RefCount;
	}
	return 0;
}

void URssImageCacheManager::ForceRemoveFromCache(const FString& Url)
{
	{
		FScopeLock Lock(&CacheLock);
		if (FRssImageCacheEntry* Entry = MemoryCache.Find(Url))
		{
			CurrentMemoryUsage -= Entry->SizeInBytes;
			MemoryCache.Remove(Url);
		}
	}

	if (CacheConfig.bEnableDiskCache)
	{
		RemoveFromDiskCache(Url);
	}
}

void URssImageCacheManager::ClearUnreferencedImages()
{
	FScopeLock Lock(&CacheLock);

	TArray<FString> ToRemove;
	for (auto& Pair : MemoryCache)
	{
		if (Pair.Value.RefCount <= 0)
		{
			ToRemove.Add(Pair.Key);
		}
	}

	for (const FString& Url : ToRemove)
	{
		if (FRssImageCacheEntry* Entry = MemoryCache.Find(Url))
		{
			CurrentMemoryUsage -= Entry->SizeInBytes;
		}
		MemoryCache.Remove(Url);
	}

	UE_LOG(RSSImageCacheLog, Log, TEXT("Cleared %d unreferenced images from memory cache"), ToRemove.Num());
}

void URssImageCacheManager::ClearAllCache()
{
	{
		FScopeLock Lock(&CacheLock);
		MemoryCache.Empty();
		CurrentMemoryUsage = 0;
	}

	if (CacheConfig.bEnableDiskCache)
	{
		FString CachePath = FPaths::Combine(FPaths::ProjectSavedDir(), CacheConfig.CacheDirectory);
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		FScopeLock Lock(&DiskLock);
		for (const auto& Entry : DiskCacheIndex)
		{
			FString FilePath = GetCacheFilePath(Entry.Key);
			PlatformFile.DeleteFile(*FilePath);
		}

		DiskCacheIndex.Empty();
		CurrentDiskUsage = 0;
	}

	UE_LOG(RSSImageCacheLog, Log, TEXT("Cleared all cache"));
}

int64 URssImageCacheManager::GetCurrentMemoryUsage() const { return CurrentMemoryUsage; }

int64 URssImageCacheManager::GetCurrentDiskUsage() const
{
	FScopeLock Lock(&DiskLock);
	return CurrentDiskUsage;
}

void URssImageCacheManager::GetCacheStats(int32& OutMemoryCachedCount, int32& OutDiskCachedCount,
                                          int32& OutPendingDownloads) const
{
	{
		FScopeLock Lock(&CacheLock);
		OutMemoryCachedCount = MemoryCache.Num();
	}
	{
		FScopeLock Lock(&DiskLock);
		OutDiskCachedCount = DiskCacheIndex.Num();
	}
	{
		FScopeLock Lock(&DownloadLock);
		OutPendingDownloads = PendingDownloads.Num();
	}
}

void URssImageCacheManager::Tick(float DeltaTime)
{
	MaintenanceTimer += DeltaTime;
	if (MaintenanceTimer < MaintenanceInterval)
	{
		return;
	}
	MaintenanceTimer = 0.0f;

	// Check memory pressure
	CheckMemoryPressure();

	// Clean up expired entries
	FDateTime Now = FDateTime::Now();
	TArray<FString> ToRemove;

	{
		FScopeLock Lock(&CacheLock);
		for (auto& Pair : MemoryCache)
		{
			if (Pair.Value.RefCount <= 0)
			{
				FTimespan TimeSinceAccess = Now - Pair.Value.LastAccessTime;
				if (TimeSinceAccess.GetTotalSeconds() > CacheConfig.UnusedTimeoutSeconds)
				{
					ToRemove.Add(Pair.Key);
				}
			}
		}

		for (const FString& Url : ToRemove)
		{
			if (FRssImageCacheEntry* Entry = MemoryCache.Find(Url))
			{
				CurrentMemoryUsage -= Entry->SizeInBytes;
			}
			MemoryCache.Remove(Url);
		}
	}

	if (ToRemove.Num() > 0)
	{
		UE_LOG(RSSImageCacheLog, Verbose, TEXT("Expired %d cache entries"), ToRemove.Num());
	}
}

bool URssImageCacheManager::LoadFromDiskCache(const FString& Url, TArray<uint8>& OutData)
{
	FString FilePath = GetCacheFilePath(Url);
	return FFileHelper::LoadFileToArray(OutData, *FilePath);
}

bool URssImageCacheManager::SaveToDiskCache(const FString& Url, const TArray<uint8>& Data)
{
	FScopeLock Lock(&DiskLock);

	// Check disk cache size limit
	if (CurrentDiskUsage + Data.Num() > CacheConfig.MaxDiskCacheSize)
	{
		// Need to evict some entries
		int64 TargetSize = CacheConfig.MaxDiskCacheSize - Data.Num();
		// Simple eviction: remove oldest entries
		// For now, just skip saving
		UE_LOG(RSSImageCacheLog, Warning, TEXT("Disk cache full, skipping save for: %s"), *Url);
		return false;
	}

	FString FilePath = GetCacheFilePath(Url);
	FString Hash = GenerateUrlHash(Url);

	if (FFileHelper::SaveArrayToFile(Data, *FilePath))
	{
		DiskCacheIndex.Add(Url, Hash);
		CurrentDiskUsage += Data.Num();
		return true;
	}
	return false;
}

bool URssImageCacheManager::RemoveFromDiskCache(const FString& Url)
{
	FScopeLock Lock(&DiskLock);

	if (!DiskCacheIndex.Contains(Url))
	{
		return false;
	}

	FString FilePath = GetCacheFilePath(Url);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (PlatformFile.FileExists(*FilePath))
	{
		int64 FileSize = PlatformFile.FileSize(*FilePath);
		if (PlatformFile.DeleteFile(*FilePath))
		{
			CurrentDiskUsage -= FileSize;
			DiskCacheIndex.Remove(Url);
			return true;
		}
	}

	DiskCacheIndex.Remove(Url);
	return false;
}

FString URssImageCacheManager::GetCacheFilePath(const FString& Url) const
{
	FString Hash = GenerateUrlHash(Url);
	FString CachePath = FPaths::Combine(FPaths::ProjectSavedDir(), CacheConfig.CacheDirectory);
	return FPaths::Combine(CachePath, Hash + TEXT(".cache"));
}

FString URssImageCacheManager::GenerateUrlHash(const FString& Url) { return FMD5::HashAnsiString(*Url); }

void URssImageCacheManager::CheckMemoryPressure()
{
	if (CurrentMemoryUsage > CacheConfig.MaxMemoryCacheSize)
	{
		EvictLRUEntries(CacheConfig.MaxMemoryCacheSize * 0.75); // Target 75% of max
	}
}

void URssImageCacheManager::EvictLRUEntries(int64 TargetSize)
{
	FScopeLock Lock(&CacheLock);
	EvictLRUEntriesLocked(TargetSize);
}

void URssImageCacheManager::EvictLRUEntriesLocked(int64 TargetSize)
{
	// Collect entries sorted by last access time (oldest first)
	TArray<TPair<FString, FDateTime>> SortedEntries;
	for (auto& Pair : MemoryCache)
	{
		if (Pair.Value.RefCount <= 0) // Only evict unreferenced entries
		{
			SortedEntries.Add(TPair<FString, FDateTime>(Pair.Key, Pair.Value.LastAccessTime));
		}
	}

	SortedEntries.Sort(
		[](const TPair<FString, FDateTime>& A, const TPair<FString, FDateTime>& B)
		{
			return A.Value < B.Value; // Oldest first
		});

	// Remove entries until we're under target
	for (const auto& Entry : SortedEntries)
	{
		if (CurrentMemoryUsage <= TargetSize)
		{
			break;
		}

		if (FRssImageCacheEntry* CacheEntry = MemoryCache.Find(Entry.Key))
		{
			CurrentMemoryUsage -= CacheEntry->SizeInBytes;
			MemoryCache.Remove(Entry.Key);
		}
	}
}

void URssImageCacheManager::AddToMemoryCache(const FString& Url, UTexture2DDynamic* Texture, int64 SizeInBytes,
                                             bool bPersistent)
{
	FScopeLock Lock(&CacheLock);

	// Check if we need to make room
	if (CurrentMemoryUsage + SizeInBytes > CacheConfig.MaxMemoryCacheSize)
	{
		EvictLRUEntriesLocked(FMath::Max<int64>(0, CacheConfig.MaxMemoryCacheSize - SizeInBytes));
	}

	FRssImageCacheEntry Entry;
	Entry.Texture = Texture;
	Entry.Url = Url;
	// Active signs increment to 1 via AddReference (StartWithCache memory-hit or RequestImage cache-hit).
	// Previously RefCount = 1 meant init-loaded textures could never be evicted, defeating the LRU cap.
	Entry.RefCount = 0;
	Entry.LastAccessTime = FDateTime::Now();
	Entry.SizeInBytes = SizeInBytes;
	Entry.bIsPersistent = bPersistent;

	MemoryCache.Add(Url, Entry);
	CurrentMemoryUsage += SizeInBytes;

	UE_LOG(RSSImageCacheLog, Verbose, TEXT("Added to memory cache: %s (Size: %lld, Total: %lld)"), *Url, SizeInBytes,
	       CurrentMemoryUsage);
}
