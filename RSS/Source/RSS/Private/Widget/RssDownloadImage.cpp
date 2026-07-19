// Copyright Epic Games, Inc. All Rights Reserved.

#include "Widget/RssDownloadImage.h"
#include "Cache/RssImageCache.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture2DDynamic.h"
#include "HttpModule.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "IPAddress.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IHttpResponse.h"
#include "Modules/ModuleManager.h"
#include "SocketSubsystem.h"
#include "Subsystem/RSSImageSubsystem.h"

DECLARE_LOG_CATEGORY_EXTERN(RSSImageDownloaderLog, Log, All)

DEFINE_LOG_CATEGORY(RSSImageDownloaderLog)

namespace
{
	bool IsPublicAddress(const FString& AddressString)
	{
		FString Address = AddressString.ToLower();
		Address.RemoveFromStart(TEXT("["));
		Address.RemoveFromEnd(TEXT("]"));

		FIPv4Address IPv4;
		if (FIPv4Address::Parse(Address, IPv4))
		{
			return IPv4.A != 0 && IPv4.A != 10 && IPv4.A != 127 &&
				!(IPv4.A == 100 && IPv4.B >= 64 && IPv4.B <= 127) &&
				!(IPv4.A == 169 && IPv4.B == 254) &&
				!(IPv4.A == 172 && IPv4.B >= 16 && IPv4.B <= 31) &&
				!(IPv4.A == 192 && IPv4.B == 168) && IPv4.A < 224;
		}

		if (Address.StartsWith(TEXT("::ffff:")))
		{
			return IsPublicAddress(Address.RightChop(7));
		}

		return Address != TEXT("::") && Address != TEXT("::1") &&
			!Address.StartsWith(TEXT("fc")) && !Address.StartsWith(TEXT("fd")) &&
			!Address.StartsWith(TEXT("fe8")) && !Address.StartsWith(TEXT("fe9")) &&
			!Address.StartsWith(TEXT("fea")) && !Address.StartsWith(TEXT("feb")) &&
			!Address.StartsWith(TEXT("ff"));
	}
}

//----------------------------------------------------------------------//
// URssDownloadImage
//----------------------------------------------------------------------//


URssDownloadImage::URssDownloadImage(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		AddToRoot();
	}
}

URssDownloadImage* URssDownloadImage::DownloadImage(UObject* WorldContextObject, FString URL, bool bUseCache,
                                                    bool bPersistToCache)
{
	URssDownloadImage* DownloadTask = NewObject<URssDownloadImage>();
	DownloadTask->bUseCache = bUseCache;
	DownloadTask->bPersistToCache = bPersistToCache;

	if (bUseCache && WorldContextObject)
	{
		DownloadTask->StartWithCache(URL, WorldContextObject);
	}
	else
	{
		DownloadTask->Start(URL);
	}

	return DownloadTask;
}

URssDownloadImage* URssDownloadImage::DownloadImageNoCache(FString URL)
{
	URssDownloadImage* DownloadTask = NewObject<URssDownloadImage>();
	DownloadTask->bUseCache = false;
	DownloadTask->Start(URL);

	return DownloadTask;
}

void URssDownloadImage::WriteTexture_RenderThread(FTexture2DDynamicResource* TextureResource, TArray64<uint8>* RawData,
                                                  bool bUseSRGB)
{
#if !UE_SERVER
	check(IsInRenderingThread());

	if (TextureResource)
	{
		FRHITexture* TextureRHI = TextureResource->GetTexture2DRHI();

		int32 Width = TextureRHI->GetSizeX();
		int32 Height = TextureRHI->GetSizeY();

		uint32 DestStride = 0;
		uint8* DestData =
			reinterpret_cast<uint8*>(RHILockTexture2D(TextureRHI, 0, RLM_WriteOnly, DestStride, false, false));

		for (int32 y = 0; y < Height; y++)
		{
			uint8* DestPtr = &DestData[(static_cast<int64>(Height) - 1 - y) * DestStride];

			const FColor* SrcPtr = &((FColor*)(RawData->GetData()))[(static_cast<int64>(Height) - 1 - y) * Width];
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

	delete RawData;
#endif
}

void URssDownloadImage::Start(FString URL)
{
#if !UE_SERVER
	if (!IsSafeRemoteImageUrl(URL))
	{
		OnFail.Broadcast(nullptr);
		Cleanup();
		return;
	}

	// Create the Http request and add to pending request list
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->OnProcessRequestComplete().BindUObject(this, &URssDownloadImage::HandleImageRequest);
	HttpRequest->SetURL(URL);
	HttpRequest->SetVerb(TEXT("GET"));
	HttpRequest->ProcessRequest();
#else
	OnFail.Broadcast(nullptr);
	Cleanup();
#endif
}

void URssDownloadImage::HandleImageRequest(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
#if !UE_SERVER

	RemoveFromRoot();

	if (bSucceeded && HttpResponse.IsValid() && EHttpResponseCodes::IsOk(HttpResponse->GetResponseCode()) &&
		IsSafeRemoteImageUrl(HttpResponse->GetURL()))
	{
		IImageWrapperModule& ImageWrapperModule =
			FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		const TArray<uint8>& CompressedData = HttpResponse->GetContent();
		if (CompressedData.IsEmpty() || CompressedData.Num() > MaxCompressedImageBytes)
		{
			OnFail.Broadcast(nullptr);
			return;
		}
		const EImageFormat ImageFormat =
			ImageWrapperModule.DetectImageFormat(CompressedData.GetData(), CompressedData.Num());
		TSharedPtr<IImageWrapper> ImageWrappers[1];
		if (ImageFormat != EImageFormat::Invalid)
		{
			ImageWrappers[0] = ImageWrapperModule.CreateImageWrapper(ImageFormat);
		}

		for (auto ImageWrapper : ImageWrappers)
		{
			if (ImageWrapper.IsValid() &&
				ImageWrapper->SetCompressed(CompressedData.GetData(), CompressedData.Num()))
			{
				const int32 SourceWidth = ImageWrapper->GetWidth();
				const int32 SourceHeight = ImageWrapper->GetHeight();
				int64 RawBytes = 0;
				if (!ValidateImageDimensions(CompressedData.Num(), SourceWidth, SourceHeight, RawBytes))
				{
					continue;
				}

				TArray64<uint8>* RawData = new TArray64<uint8>();
				constexpr ERGBFormat InFormat = ERGBFormat::BGRA;
				if (ImageWrapper->GetRaw(InFormat, 8, *RawData))
				{
					int32 Width = bHalfImageSize ? SourceWidth / 2 : SourceWidth;
					int32 Height = bHalfImageSize ? SourceHeight / 2 : SourceHeight;

					if (UTexture2DDynamic* Texture = UTexture2DDynamic::Create(Width, Height))
					{
						Texture->SRGB = true;
						Texture->UpdateResource();

						if (FTexture2DDynamicResource* TextureResource =
							static_cast<FTexture2DDynamicResource*>(Texture->GetResource()))
						{
							ENQUEUE_RENDER_COMMAND(FWriteRawDataToTexture)(
								[TextureResource, RawData](FRHICommandListImmediate& RHICmdList)
								{
									WriteTexture_RenderThread(TextureResource, RawData);
								});
						}
						else
						{
							delete RawData;
						}

						// (RSSImageSubsystem.cpp), so calling Subsystem->OnDownloadSuccess() directly
						// was a double-fire. The direct call also crashed on the non-cache Start() path
						// where Subsystem is null.
						OnSuccess.Broadcast(Texture);
						return;
					}
				}
			}
		}
	}

	OnFail.Broadcast(nullptr);

#endif
}

void URssDownloadImage::StartWithCache(FString URL, UObject* WorldContextObject)
{
#if !UE_SERVER
	if (!IsSafeRemoteImageUrl(URL))
	{
		OnFail.Broadcast(nullptr);
		Cleanup();
		return;
	}

	CurrentUrl = URL;

	// Try to get from cache first
	CacheManager = URssImageCacheManager::Get(WorldContextObject);
	if (CacheManager)
	{
		// Check if already cached in memory
		UTexture2DDynamic* CachedTexture = nullptr;
		if (CacheManager->GetCachedImage(URL, CachedTexture))
		{
			UE_LOG(RSSImageDownloaderLog, Verbose, TEXT("Image found in memory cache: %s"), *URL);
			CacheManager->AddReference(URL);

			OnSuccess.Broadcast(CachedTexture);
			Cleanup();
			return;
		}

		// Bind to cache events
		bWaitingForCache = true;
		CacheManager->OnImageLoaded.AddDynamic(this, &URssDownloadImage::OnCacheImageLoaded);
		CacheManager->OnImageFailed.AddDynamic(this, &URssDownloadImage::OnCacheImageFailed);

		// Request from cache (will load from disk or download)
		CacheManager->RequestImage(URL, bPersistToCache);
		return;
	}

	// Fallback to direct download
	Start(URL);
#else
	OnFail.Broadcast(nullptr);
	Cleanup();
#endif
}

bool URssDownloadImage::IsSafeRemoteImageUrl(const FString& Url)
{
	if (Url.Len() > 2048 || Url.Contains(TEXT("@")))
	{
		return false;
	}

	FURL ParsedUrl(nullptr, *Url, TRAVEL_Absolute);
	const FString Host = ParsedUrl.Host.ToLower();
	if (!ParsedUrl.Valid || (ParsedUrl.Protocol != TEXT("http") && ParsedUrl.Protocol != TEXT("https")) ||
		Host.IsEmpty() || Host == TEXT("localhost") || Host.EndsWith(TEXT(".localhost")) ||
		Host.EndsWith(TEXT(".local")) || Host.EndsWith(TEXT(".internal")))
	{
		return false;
	}

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return false;
	}

	const FAddressInfoResult AddressInfo = SocketSubsystem->GetAddressInfo(
		*Host, nullptr, EAddressInfoFlags::Default, NAME_None);
	if (AddressInfo.ReturnCode != SE_NO_ERROR || AddressInfo.Results.IsEmpty())
	{
		return false;
	}

	for (const FAddressInfoResultData& Result : AddressInfo.Results)
	{
		if (!IsPublicAddress(Result.Address->ToString(false)))
		{
			return false;
		}
	}
	return true;
}

bool URssDownloadImage::ValidateImageDimensions(int64 CompressedBytes, int32 Width, int32 Height, int64& OutRawBytes)
{
	OutRawBytes = 0;
	if (CompressedBytes <= 0 || CompressedBytes > MaxCompressedImageBytes || Width <= 0 || Height <= 0 ||
		Width > MaxImageDimension || Height > MaxImageDimension || Width > MAX_int64 / Height)
	{
		return false;
	}

	const int64 Pixels = static_cast<int64>(Width) * Height;
	if (Pixels > MaxImagePixels || Pixels > MAX_int64 / 4)
	{
		return false;
	}

	OutRawBytes = Pixels * 4;
	return true;
}

void URssDownloadImage::OnCacheImageLoaded(const FString& Url, UTexture2DDynamic* Texture)
{
	if (!bWaitingForCache || Url != CurrentUrl)
	{
		return;
	}

	bWaitingForCache = false;

	// Unbind from cache events
	if (CacheManager)
	{
		CacheManager->OnImageLoaded.RemoveDynamic(this, &URssDownloadImage::OnCacheImageLoaded);
		CacheManager->OnImageFailed.RemoveDynamic(this, &URssDownloadImage::OnCacheImageFailed);
	}

	UE_LOG(RSSImageDownloaderLog, Verbose, TEXT("Image loaded via cache: %s"), *Url);

	OnSuccess.Broadcast(Texture);
	Cleanup();
}

void URssDownloadImage::OnCacheImageFailed(const FString& Url)
{
	if (!bWaitingForCache || Url != CurrentUrl)
	{
		return;
	}

	bWaitingForCache = false;

	// Unbind from cache events
	if (CacheManager)
	{
		CacheManager->OnImageLoaded.RemoveDynamic(this, &URssDownloadImage::OnCacheImageLoaded);
		CacheManager->OnImageFailed.RemoveDynamic(this, &URssDownloadImage::OnCacheImageFailed);
	}

	UE_LOG(RSSImageDownloaderLog, Warning, TEXT("Cache image load failed: %s"), *Url);

	OnFail.Broadcast(nullptr);
	Cleanup();
}

void URssDownloadImage::Cleanup()
{
	RemoveFromRoot();
	CacheManager = nullptr;
	Subsystem = nullptr;
}
