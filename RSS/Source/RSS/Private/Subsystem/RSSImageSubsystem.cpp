// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Subsystem/RSSImageSubsystem.h"

#include "Blueprint/AsyncTaskDownloadImage.h"
#include "Cache/RssImageCache.h"
#include "Engine/Texture2DDynamic.h"
#include "HAL/PlatformFileManager.h"
#include "Interface/RssSignInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Serialization/RssJsonSerializer.h"

DECLARE_LOG_CATEGORY_EXTERN(RSSImageSubsystemLog, Log, All)

DEFINE_LOG_CATEGORY(RSSImageSubsystemLog)

ARSSImageSubsystem::ARSSImageSubsystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25;

	bReplicates = true;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnLocal;
}

void ARSSImageSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bSaveDataDirty)
	{
		bSaveDataDirty = false;
	}
	SaveCustomDataToFileInternal();

	if (CacheManager)
	{
		CacheManager->Shutdown();
		CacheManager = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ARSSImageSubsystem::BeginPlay()
{
	Super::BeginPlay();

	CacheManager = URssImageCacheManager::Get(this);

	FString Data;
	if (ReadCustomDataFromFile(Data))
	{

		FString TrimmedContent = Data.TrimStartAndEnd();
		bool bLoadedCatalog = false;
		bool bNeedsMigration = false;

		if (TrimmedContent.StartsWith(TEXT("{")))
		{

			if (FRssJsonSerializer::JsonToCustomData(Data, mInformation))
			{
				bLoadedCatalog = true;
			}
		}
		else if (!TrimmedContent.IsEmpty() && CheckString(Data, {"mAllowedInSigns", "mUrl"}))
		{

			mInformation = CustomInformationStringToSignData(this, Data);
			bLoadedCatalog = true;
			bNeedsMigration = true;
		}

		if (bLoadedCatalog)
		{
			GenerateMapFromInformationInVar();
			for (auto Info : mInformation.mInformations)
			{
				FInitRequests InitRequest;
				InitRequest.mUrl = Info.mUrl;
				InitRequest.mSignSizes = Info.mAllowedInSigns;
				mInitRequests.Add(InitRequest);
				UE_LOG(RSSImageSubsystemLog, Log, TEXT("Add URL: %s"), *Info.mUrl);
			}
			if (bNeedsMigration)
			{
				SaveCustomDataToFile();
			}
		}
	}

	bAllowToRunInitQuery = true;
	TriggerIconExtraction();
}

void ARSSImageSubsystem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CacheManager)
	{
		CacheManager->Tick(DeltaSeconds);
	}

	if (IsValid(IconExtractor))
	{
		IconExtractor->TickExtractionQueue(DeltaSeconds);
	}

	if (bSaveDataDirty)
	{
		mSaveDirtyAccumulator += DeltaSeconds;
		if (mSaveDirtyAccumulator >= mSaveDebounceTime)
		{
			bSaveDataDirty = false;
			mSaveDirtyAccumulator = 0.f;

			GenerateInformation();
			FString SerializedData = FRssJsonSerializer::CustomDataToJson(mInformation);
			FString FilePath = GetFilePath();
			if (!SerializedData.IsEmpty())
			{
				AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,
				          [SerializedData = MoveTemp(SerializedData), FilePath = MoveTemp(FilePath)]()
				          {
					          FFileHelper::SaveStringToFile(SerializedData, *FilePath);
				          });
			}
		}
	}

	mInitTimer += DeltaSeconds;

	if (!bRequestIsRunning && !bInitIsRunning && mRuntimeRequests.IsValidIndex(0))
	{
		FRssSignRequestData Request = mRuntimeRequests[0];
		if (IsValid(Request.mBuildable))
		{
			FRssSignData SignData = IRssSignInterface::Execute_GetSignData(Request.mBuildable);
			if (IsUrlValid(Request.mRequestUrl))
			{
				FRssCustomSignUrlData UrlData = GetDataFromUrl(Request.mRequestUrl);
				if (UrlData.mTexture)
				{
					if (!UrlData.mAllowedInSigns.Contains(SignData.mSignTypeSize))
					{
						UrlData.mAllowedInSigns.AddUnique(SignData.mSignTypeSize);
						mStoredImages.Add(Request.mRequestUrl, UrlData);
						bSaveDataDirty = true;
						mSaveDirtyAccumulator = 0.f;
					}

					mRuntimeRequests.RemoveAt(0);
					OnImageRequestStateChanged.Broadcast(Request.mRequestUrl, ERssImageRequestState::Succeeded);
					SendRequestBack(Request, UrlData.mTexture);
					return;
				}

				bRequestIsRunning = true;
				OnImageRequestStateChanged.Broadcast(Request.mRequestUrl, ERssImageRequestState::Loading);
				mDownloadTask = NewObject<URssDownloadImage>();

				mDownloadTask->Subsystem = this;

				mDownloadTask->OnFail.AddDynamic(this, &ARSSImageSubsystem::OnDownloadFailed);
				mDownloadTask->OnSuccess.AddDynamic(this, &ARSSImageSubsystem::OnDownloadSuccess);

				mDownloadTask->StartWithCache(Request.mRequestUrl, this);

				UE_LOG(RSSImageSubsystemLog, Log, TEXT("mDownloadTask, %s"), *Request.mRequestUrl);
				return;
			}
		}

		mRuntimeRequests.RemoveAt(0);
		OnImageRequestStateChanged.Broadcast(Request.mRequestUrl, ERssImageRequestState::Failed);
		SendRequestBack(Request);
		return;
	}

	if (mInitTimer <= 5.0f)
	{
		return;
	}

	if (!bInitDone && !bInitIsRunning && !bRequestIsRunning && bAllowToRunInitQuery)
	{
		UE_LOG(RSSImageSubsystemLog, Log, TEXT("Init: %d, %d, %d"), mInitRequests.Num(), mInitRequests.IsValidIndex(0),
		       mRuntimeRequests.Num());
		if (mInitRequests.IsValidIndex(0))
		{
			UE_LOG(RSSImageSubsystemLog, Log, TEXT("mInitRequests.IsValidIndex(0)"));
			FInitRequests Request = mInitRequests[0];
			if (IsUrlValid(Request.mUrl))
			{
				bInitIsRunning = true;

				mDownloadTask = NewObject<URssDownloadImage>();

				mDownloadTask->Subsystem = this;

				mDownloadTask->OnFail.AddDynamic(this, &ARSSImageSubsystem::OnDownloadFailed);
				mDownloadTask->OnSuccess.AddDynamic(this, &ARSSImageSubsystem::OnDownloadSuccess);

				mDownloadTask->StartWithCache(Request.mUrl, this);

				UE_LOG(RSSImageSubsystemLog, Log, TEXT("mInitRequests, %s"), *Request.mUrl);
			}
			else
			{
				mInitRequests.RemoveAt(0);
			}
		}

		bInitDone = mInitRequests.Num() == 0;
	}
}

void ARSSImageSubsystem::OnDownloadFailed(UTexture2DDynamic* Texture)
{
	UE_LOG(RSSImageSubsystemLog, Log, TEXT("OnDownloadFailed"));
	if (bInitIsRunning)
	{
		if (mInitRequests.IsValidIndex(0))
		{
			mInitRequests.RemoveAt(0);
		}
	}
	else if (bRequestIsRunning)
	{
		if (mRuntimeRequests.IsValidIndex(0))
		{
			FRssSignRequestData Request = mRuntimeRequests[0];
			mRuntimeRequests.RemoveAt(0);
			OnImageRequestStateChanged.Broadcast(Request.mRequestUrl, ERssImageRequestState::Failed);
			SendRequestBack(Request);
		}
	}

	if (mDownloadTask)
	{
		mDownloadTask->RemoveFromRoot();
		mDownloadTask = nullptr;
	}
	bInitIsRunning = false;
	bRequestIsRunning = false;
}

void ARSSImageSubsystem::OnDownloadSuccess(UTexture2DDynamic* Texture)
{
	UE_LOG(RSSImageSubsystemLog, Log, TEXT("OnDownloadSuccess"));
	if (bInitIsRunning && Texture)
	{
		if (mInitRequests.IsValidIndex(0))
		{
			FInitRequests Request = mInitRequests[0];

			FRssCustomSignUrlData UrlData;

			UrlData.mUrl = Request.mUrl;
			UrlData.mAllowedInSigns = Request.mSignSizes;

			mStoredImages.Add(Request.mUrl, UrlData);

			mInitRequests.RemoveAt(0);
		}
	}
	else if (bInitIsRunning && !Texture)
	{
		if (mInitRequests.IsValidIndex(0))
		{
			mInitRequests.RemoveAt(0);
		}
	}

	if (bRequestIsRunning && Texture)
	{
		if (mRuntimeRequests.IsValidIndex(0))
		{
			FRssSignRequestData Request = mRuntimeRequests[0];

			FRssCustomSignUrlData UrlData = GetDataFromUrl(Request.mRequestUrl);
			UrlData.mUrl = Request.mRequestUrl;

			if (Request.mBuildable)
			{
				FRssSignData SignData = IRssSignInterface::Execute_GetSignData(Request.mBuildable);
				UrlData.mAllowedInSigns.AddUnique(SignData.mSignTypeSize);
			}

			mStoredImages.Add(Request.mRequestUrl, UrlData);
			bSaveDataDirty = true;
			mSaveDirtyAccumulator = 0.f;

			mRuntimeRequests.RemoveAt(0);
			OnImageRequestStateChanged.Broadcast(Request.mRequestUrl, ERssImageRequestState::Succeeded);
			SendRequestBack(Request, Texture);
		}
	}
	else if (bRequestIsRunning && mRuntimeRequests.IsValidIndex(0))
	{
		FRssSignRequestData Request = mRuntimeRequests[0];
		mRuntimeRequests.RemoveAt(0);
		OnImageRequestStateChanged.Broadcast(Request.mRequestUrl, ERssImageRequestState::Failed);
		SendRequestBack(Request);
	}

	if (mDownloadTask)
	{
		mDownloadTask->RemoveFromRoot();
		mDownloadTask = nullptr;
	}
	bInitIsRunning = false;
	bRequestIsRunning = false;
}

void ARSSImageSubsystem::SendRequestBack(FRssSignRequestData& Request, UTexture* Texture)
{
	Request.mSuccessTexture = Texture;
	Request.mWasSuccess = Request.mSuccessTexture != nullptr;

	UE_LOG(LogTemp, Warning, TEXT("SendRequestBack, %d"), Request.mWasSuccess);
	if (!Request.mOnlyAddToUI && IsValid(Request.mBuildable))
	{
		IRssSignInterface::Execute_UpdateSignToCustomImage(Request.mBuildable, Request);
	}

	if (Request.mRequestFromWidgetObject)
	{
		OnCallToWidget(Request);
	}
}

bool ARSSImageSubsystem::IsUrlValid(FString Url)
{
	return URssDownloadImage::IsSafeRemoteImageUrl(Url);
}

void ARSSImageSubsystem::GenerateInformation()
{
	mInformation.mInformations.Empty();
	TArray<FString> Urls;
	mStoredImages.GenerateKeyArray(Urls);

	for (auto Url : Urls)
	{
		FRssCustomSignUrlData UrlData;
		UrlData.mUrl = mStoredImages[Url].mUrl;
		UrlData.mAllowedInSigns = mStoredImages[Url].mAllowedInSigns;

		mInformation.mInformations.Add(UrlData);
	}

	UE_LOG(LogTemp, Warning, TEXT("GenerateInformation %d"), mInformation.mInformations.Num());
}

TMap<FString, FRssCustomSignUrlData> ARSSImageSubsystem::GenerateMapFromInformation()
{
	TMap<FString, FRssCustomSignUrlData> Map;
	for (auto Info : mInformation.mInformations)
	{
		if (Map.Contains(Info.mUrl))
		{
			Map[Info.mUrl].mUrl = Info.mUrl;
			Map[Info.mUrl].mAllowedInSigns = Info.mAllowedInSigns;
		}
		else
		{
			Map.Add(Info.mUrl, Info);
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("GenerateMapFromInformation %d"), Map.Num());
	return Map;
}

void ARSSImageSubsystem::GenerateMapFromInformationInVar()
{
	for (auto Info : mInformation.mInformations)
	{
		if (mStoredImages.Contains(Info.mUrl))
		{
			mStoredImages[Info.mUrl].mUrl = Info.mUrl;
			mStoredImages[Info.mUrl].mAllowedInSigns = Info.mAllowedInSigns;
		}
		else
		{
			mStoredImages.Add(Info.mUrl, Info);
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("GenerateMapFromInformationInVar %d"), mStoredImages.Num());
}

bool ARSSImageSubsystem::ReadCustomDataFromFile(FString& FileContent)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	FString FilePath = GetFilePath();

	if (FileManager.FileExists(*FilePath))
	{
		return FFileHelper::LoadFileToString(FileContent, *FilePath, FFileHelper::EHashOptions::None);
	}

	return false;
}

FString ARSSImageSubsystem::GetFilePath()
{
	FString file = FPaths::ProjectConfigDir();
	if (mAddFilePath != "")
	{
		file.Append(mAddFilePath);
	}
	file.Append(mFileName);

	return file;
}

bool ARSSImageSubsystem::SaveCustomDataToFile()
{
	return SaveCustomDataToFileInternal();
}

bool ARSSImageSubsystem::SaveCustomDataToFileInternal()
{
	const FString FilePath = GetFilePath();
	GenerateInformation();

	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	const FString BackupPath = FilePath + TEXT(".bak");
	if (FileManager.FileExists(*FilePath) && !FileManager.FileExists(*BackupPath))
	{
		if (!FileManager.CopyFile(*BackupPath, *FilePath))
		{
			UE_LOG(LogTemp, Error, TEXT("RSS: failed to back up %s aborting save"), *FilePath);
			return false;
		}
	}

	FString Data = FRssJsonSerializer::CustomDataToJson(mInformation);
	return !Data.IsEmpty() && FFileHelper::SaveStringToFile(Data, *FilePath);
}

bool ARSSImageSubsystem::CheckString(FString String, TArray<FString> ExtraNameChecks)
{
	const FString CopyString = String.TrimStartAndEnd();
	TArray<FString> StringArray = UKismetStringLibrary::GetCharacterArrayFromString(CopyString);

	if (StringArray.Num() > 1)
	{
		if (StringArray[0] == "(" && StringArray.Last() == ")")
		{
			for (auto ExtraNameCheck : ExtraNameChecks)
			{
				if (!CopyString.Contains(ExtraNameCheck))
				{
					return false;
				}
			}
			return true;
		}
	}

	return false;
}

FString ARSSImageSubsystem::CustomInformationToString(UObject* WorldContext, FRssCustomDatas CustomInformation)
{
	const UScriptStruct* Struct = CustomInformation.StaticStruct();
	FString Output = TEXT("");
	if (Struct)
	{

		FRssCustomDatas DefaultData;
		Struct->ExportText(Output, &CustomInformation, &DefaultData, WorldContext,
		                   (PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited | PPF_IncludeTransient), nullptr);
	}
	return Output;
}

FRssCustomDatas ARSSImageSubsystem::CustomInformationStringToSignData(UObject* WorldContext, FString CustomInformation)
{
	FRssCustomDatas OutStruc = FRssCustomDatas();

	static UScriptStruct* Struct = OutStruc.StaticStruct();
	Struct->ImportText(*CustomInformation, &OutStruc, WorldContext, (PPF_Copy | PPF_Delimited | PPF_IncludeTransient),
	                   nullptr, "FRssCustomDatas");

	return OutStruc;
}

void ARSSImageSubsystem::onRequestImages_Implementation(FRssSignRequestData Request)
{
	UE_LOG(LogTemp, Warning, TEXT("onRequestImages_Implementation"));
	if (IsUrlValid(Request.mRequestUrl))
	{
		mRuntimeRequests.Add(Request);
		OnImageRequestStateChanged.Broadcast(Request.mRequestUrl, ERssImageRequestState::Queued);
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("onRequestImages_Implementation FAILED send back"));
	OnImageRequestStateChanged.Broadcast(Request.mRequestUrl, ERssImageRequestState::Failed);
	SendRequestBack(Request);
}

void ARSSImageSubsystem::DoRemove_Implementation(const FString& Url)
{
	if (mStoredImages.Contains(Url))
	{
		mStoredImages.Remove(Url);
		bSaveDataDirty = true;
		mSaveDirtyAccumulator = 0.f;
	}
}

void ARSSImageSubsystem::RemoveDataByUrl(FString Url)
{
	if (mStoredImages.Contains(Url))
	{
		mStoredImages.Remove(Url);
		bSaveDataDirty = true;
		mSaveDirtyAccumulator = 0.f;
	}
}

FRssCustomSignUrlData ARSSImageSubsystem::GetDataFromUrl(FString Url)
{
	FRssCustomSignUrlData ReData;
	if (const FRssCustomSignUrlData* Found = mStoredImages.Find(Url))
	{
		ReData = *Found;
		if (!ReData.mTexture && CacheManager)
		{
			UTexture2DDynamic* CachedTexture = nullptr;
			if (CacheManager->GetCachedImage(Url, CachedTexture))
			{
				ReData.mTexture = CachedTexture;
			}
		}
	}
	return ReData;
}

TArray<FRssCustomSignUrlData> ARSSImageSubsystem::GetDataFromSignSize(ESignSize Size)
{
	TArray<FRssCustomSignUrlData> ReData;
	for (const auto& Pair : mStoredImages)
	{

		if (Pair.Value.mAllowedInSigns.IsEmpty() || Pair.Value.mAllowedInSigns.Contains(Size))
		{
			FRssCustomSignUrlData UrlData = GetDataFromUrl(Pair.Key);
			if (UrlData.mTexture)
			{
				ReData.Add(MoveTemp(UrlData));
			}
		}
	}
	return ReData;
}

void ARSSImageSubsystem::ConfigureCache(const FRssImageCacheConfig& Config)
{
	if (CacheManager)
	{
		CacheManager->Initialize(Config);
	}
}

void ARSSImageSubsystem::ClearUnusedCachedImages()
{
	if (CacheManager)
	{
		CacheManager->ClearUnreferencedImages();
	}
}

void ARSSImageSubsystem::GetCacheStatistics(int32& MemoryCached, int32& DiskCached, int64& MemoryUsage,
                                            int64& DiskUsage) const
{
	if (CacheManager)
	{
		int32 PendingDownloads;
		CacheManager->GetCacheStats(MemoryCached, DiskCached, PendingDownloads);
		MemoryUsage = CacheManager->GetCurrentMemoryUsage();
		DiskUsage = CacheManager->GetCurrentDiskUsage();
	}
	else
	{
		MemoryCached = 0;
		DiskCached = 0;
		MemoryUsage = 0;
		DiskUsage = 0;
	}
}

void ARSSImageSubsystem::TriggerIconExtraction()
{
	if (!IsValid(IconExtractor))
	{
		IconExtractor = NewObject<URSSIconExtractor>(this);
		if (!IsValid(IconExtractor))
		{
			UE_LOG(RSSImageSubsystemLog, Error, TEXT("Failed to create IconExtractor"));
			return;
		}
	}

	IconExtractor->bEnabled = true;
	IconExtractor->BeginExtraction();
}
