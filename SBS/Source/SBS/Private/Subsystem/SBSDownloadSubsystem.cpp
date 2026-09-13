// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Subsystem/SBSDownloadSubsystem.h"

#include "Async/Async.h"
#include "Download/SBSBlueprintFileTransaction.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "FGLocalPlayer.h"
#include "HAL/PlatformTime.h"
#include "Http/SBSHttp.h"
#include "Logging.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Structures/ApiStatics.h"
#include "Subsystem/SBSApiSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "TimerManager.h"

namespace
{
	constexpr int32 MaxQueuedDownloads = 512;
	constexpr int32 MaxBlueprintBytes = 64 * 1024 * 1024;
	constexpr int32 MaxConfigBytes = 1024 * 1024;

	bool IsValidDownload(const FBlueprintJsonStructure& Blueprint)
	{
		return FSBSStatics::IsSafeIdentifier(Blueprint.ID) && FSBSStatics::IsSafeFileName(Blueprint.OriginalName);
	}

	bool ValidateBlueprintFiles(const TArray<uint8>& Sbp, const TArray<uint8>& Config)
	{
		if (Sbp.Num() < 20 || Config.Num() < 20)
		{
			return false;
		}
		FMemoryReader HeaderReader(Sbp);
		FMemoryReader ConfigReader(Config);
		HeaderReader.ArMaxSerializeSize = MaxBlueprintBytes;
		ConfigReader.ArMaxSerializeSize = MaxConfigBytes;
		FBlueprintHeader Header;
		FBlueprintRecord Record;
		return AFGBlueprintSubsystem::SerializeBlueprintHeader(HeaderReader, Header) && !HeaderReader.IsError() &&
			AFGBlueprintSubsystem::SerializeBlueprintConfig(ConfigReader, Record) && !ConfigReader.IsError();
	}

	bool DownloadSucceeded(const TSharedPtr<FSBSRequest>& State, FHttpResponsePtr Response, bool bSuccess)
	{
		return bSuccess && Response && Response->GetResponseCode() == EHttpResponseCodes::Ok &&
			!State->mBody->IsError() && !State->mBody->mData.IsEmpty();
	}
}

ASBSDownloadSubsystem::ASBSDownloadSubsystem()
{
	bUseSubsystemTick = false;
	PrimaryActorTick.bCanEverTick = false;
	mShouldSave = false;
}

void ASBSDownloadSubsystem::BeginPlay()
{

	bUseSubsystemTick = false;
	Super::BeginPlay();
	SetActorTickEnabled(false);
	mApiSubsystem = USBSApiSubsystem::Get(this);
	mBlueprintSubsystem = AFGBlueprintSubsystem::Get(GetWorld());
	bStarted = true;
	mSaveSessionName = FPaths::GetCleanFilename(GetCurrentBlueprintPath());
	UE_LOG(LogSBS, Display, TEXT("SBS download subsystem ready (authority=%d)"), HasAuthority());
	StartNextDownload();
}

void ASBSDownloadSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	GetWorldTimerManager().ClearTimer(mNextDownloadTimer);
	if (mSbpRequest)
	{
		mSbpRequest->Cancel();
	}
	if (mConfigRequest)
	{
		mConfigRequest->Cancel();
	}
	mSbpRequest.Reset();
	mConfigRequest.Reset();
	mDownloadQueueChecker.Reset();
	mQueuedIds.Reset();
	mFOnDownloadComplete.Clear();
	mOnDownloadProgress.Clear();
	mBlueprintSubsystem = nullptr;
	mApiSubsystem = nullptr;
	Super::EndPlay(EndPlayReason);
}

void ASBSDownloadSubsystem::SubsytemTick(float dt)
{

}

ASBSDownloadSubsystem* ASBSDownloadSubsystem::Get(UObject* WorldContext)
{
	UWorld* World =
		GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	USubsystemActorManager* Manager = World ? World->GetSubsystem<USubsystemActorManager>() : nullptr;
	if (!Manager)
	{
		return nullptr;
	}
	static TWeakObjectPtr<UClass> DownloadClass;
	if (!DownloadClass.IsValid())
	{
		DownloadClass = LoadClass<ASBSDownloadSubsystem>(
			nullptr, TEXT("/SBS/Subsystem/BP_SBSDownloadSubsystem.BP_SBSDownloadSubsystem_C"));
	}
	return DownloadClass.IsValid() ? Cast<ASBSDownloadSubsystem>(Manager->K2_GetSubsystemActor(DownloadClass.Get()))
								   : nullptr;
}

UFGLocalPlayer* ASBSDownloadSubsystem::SBS_GetLocalPlayer(UObject* WorldContext)
{
	UWorld* World =
		GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? Cast<UFGLocalPlayer>(GameInstance->GetFirstGamePlayer()) : nullptr;
}

bool ASBSDownloadSubsystem::DownloadBlueprint(FBlueprintJsonStructure Blueprint)
{
	check(IsInGameThread());
	if (bEndingPlay || !HasAuthority() || !IsValidDownload(Blueprint) || mQueuedIds.Contains(Blueprint.ID) ||
		mCurrentDownload.ID == Blueprint.ID || mDownloadQueueChecker.Num() >= MaxQueuedDownloads)
	{
		return false;
	}
	mQueuedIds.Add(Blueprint.ID);
	mDownloadQueueChecker.Add(MoveTemp(Blueprint));

	if (bStarted && mCurrentDownload.ID.IsEmpty() && !GetWorldTimerManager().IsTimerActive(mNextDownloadTimer))
	{
		mNextDownloadTimer =
			GetWorldTimerManager().SetTimerForNextTick(this, &ASBSDownloadSubsystem::StartNextDownload);
	}
	return true;
}

bool ASBSDownloadSubsystem::DownloadBlueprintPack(FBlueprintPackJsonStructure BlueprintPack)
{
	if (bEndingPlay || !HasAuthority() || BlueprintPack.Blueprints.IsEmpty() ||
		BlueprintPack.Blueprints.Num() > MaxQueuedDownloads)
	{
		return false;
	}
	TArray<FBlueprintJsonStructure> Pending;
	TSet<FString> PackIds;
	for (const FBlueprintInPackJsonStructure& Blueprint : BlueprintPack.Blueprints)
	{
		FBlueprintJsonStructure Item;
		Item.ID = Blueprint.ID;
		Item.Name = Blueprint.Name;
		Item.OriginalName = Blueprint.OriginalName;
		Item.IconData = Blueprint.IconData;
		if (!IsValidDownload(Item))
		{
			return false;
		}
		if (!mQueuedIds.Contains(Item.ID) && Item.ID != mCurrentDownload.ID && !PackIds.Contains(Item.ID))
		{
			PackIds.Add(Item.ID);
			Pending.Add(MoveTemp(Item));
		}
	}
	if (Pending.Num() + mDownloadQueueChecker.Num() > MaxQueuedDownloads)
	{
		return false;
	}
	for (FBlueprintJsonStructure& Item : Pending)
	{
		DownloadBlueprint(MoveTemp(Item));
	}
	return true;
}

FSBSOperationResult ASBSDownloadSubsystem::DownloadResolvedId(const FSBSResolvedDownload& Download)
{
	check(IsInGameThread());
	if (bEndingPlay || !bStarted)
	{
		return FSBSOperationResult::Make(TEXT("download_unavailable"),
										 NSLOCTEXT("SBS", "DownloadId.DownloadUnavailable",
												   "Blueprint downloads are not ready in this game session."));
	}
	if (!HasAuthority())
	{
		return FSBSOperationResult::Make(
			TEXT("host_required"),
			NSLOCTEXT("SBS", "DownloadId.HostRequired", "The host must start this blueprint download."));
	}
	const bool bBlueprint = Download.Reference.Kind == ESBSDownloadKind::Blueprint;
	const bool bPack = Download.Reference.Kind == ESBSDownloadKind::Pack;
	if (!Download.Result.bSuccess || (!bBlueprint && !bPack) || !FSBSStatics::IsSafeIdentifier(Download.Reference.ID) ||
		Download.Reference.ID != (bBlueprint ? Download.Blueprint.ID : Download.Pack.ID))
	{
		return FSBSOperationResult::Make(
			TEXT("unresolved_download_id"),
			NSLOCTEXT("SBS", "DownloadId.NotResolved", "Resolve a valid download ID first."));
	}
	if ((bBlueprint && GetDownloadStateForBlueprint(Download.Blueprint) != 0) ||
		(bPack && GetDownloadStateForBlueprintPack(Download.Pack)))
	{
		return FSBSOperationResult::Make(TEXT("download_already_queued"),
										 NSLOCTEXT("SBS", "DownloadId.AlreadyQueued",
												   "This blueprint or part of this pack is already downloading."));
	}
	const bool bQueued = bBlueprint ? DownloadBlueprint(Download.Blueprint) : DownloadBlueprintPack(Download.Pack);
	return bQueued
		? FSBSOperationResult::Make(TEXT("download_queued"),
									NSLOCTEXT("SBS", "DownloadId.Queued", "Download added to the queue."), true)
		: FSBSOperationResult::Make(
			  TEXT("download_queue_rejected"),
			  NSLOCTEXT("SBS", "DownloadId.QueueRejected",
						"Could not queue this download. The queue may be full or its files invalid."));
}

void ASBSDownloadSubsystem::StartNextDownload()
{
	if (!bStarted || bEndingPlay || !mCurrentDownload.ID.IsEmpty() || mDownloadQueueChecker.IsEmpty())
	{
		return;
	}
	mCurrentDownload = MoveTemp(mDownloadQueueChecker[0]);
	mDownloadQueueChecker.RemoveAt(0);
	mQueuedIds.Remove(mCurrentDownload.ID);
	mDownloadDirectory = GetCurrentBlueprintPath();
	if (mDownloadDirectory.IsEmpty() || !IsValid(mBlueprintSubsystem))
	{
		FinishDownload(false);
		return;
	}
	bDownloadFailed = bDownloadFile1Completed = bDownloadFile2Completed = false;
	bInstalling = false;
	mLastProgressTime[0] = mLastProgressTime[1] = 0;
	mLastProgress[0] = mLastProgress[1] = -1;
	FDownloadSbpStruct Sbp;
	FDownloadSbpcfgStruct Config;
	Sbp.ID = Config.ID = mCurrentDownload.ID;
	mSbpRequest = FSBSRequest::Create(Sbp.getUrl(this), TEXT("GET"), FString(), this, MaxBlueprintBytes, 120.0f);
	mConfigRequest = FSBSRequest::Create(Config.getUrl(this), TEXT("GET"), FString(), this, MaxConfigBytes, 60.0f);
	mSbpRequest->mRequest->OnProcessRequestComplete().BindUObject(this,
																  &ASBSDownloadSubsystem::OnDownloadCompleteFile1);
	mSbpRequest->mRequest->OnRequestProgress64().BindUObject(this, &ASBSDownloadSubsystem::OnDownloadProgressFile1);
	mConfigRequest->mRequest->OnProcessRequestComplete().BindUObject(this,
																	 &ASBSDownloadSubsystem::OnDownloadCompleteFile2);
	mConfigRequest->mRequest->OnRequestProgress64().BindUObject(this, &ASBSDownloadSubsystem::OnDownloadProgressFile2);
	UE_LOG(LogSBS, Display, TEXT("Downloading SBS blueprint %s"), *mCurrentDownload.ID);
	if (!mSbpRequest->Start())
	{
		OnDownloadCompleteFile1(mSbpRequest->mRequest, nullptr, false);
	}

	if (mConfigRequest && !mConfigRequest->Start())
	{
		OnDownloadCompleteFile2(mConfigRequest->mRequest, nullptr, false);
	}
}

void ASBSDownloadSubsystem::OnDownloadCompleteFile1(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (bEndingPlay || !mSbpRequest || mSbpRequest->mRequest != Request || bDownloadFile1Completed)
	{
		return;
	}
	bDownloadFile1Completed = true;
	bDownloadFailed |= !DownloadSucceeded(mSbpRequest, Response, bSuccess);
	OnOneDownloadComplete();
}

void ASBSDownloadSubsystem::OnDownloadCompleteFile2(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (bEndingPlay || !mConfigRequest || mConfigRequest->mRequest != Request || bDownloadFile2Completed)
	{
		return;
	}
	bDownloadFile2Completed = true;
	bDownloadFailed |= !DownloadSucceeded(mConfigRequest, Response, bSuccess);
	OnOneDownloadComplete();
}

void ASBSDownloadSubsystem::OnOneDownloadComplete()
{
	if (bDownloadFailed)
	{
		FinishDownload(false);
		return;
	}
	if (!bDownloadFile1Completed || !bDownloadFile2Completed || bInstalling)
	{
		return;
	}
	if (!ValidateBlueprintFiles(mSbpRequest->mBody->mData, mConfigRequest->mBody->mData))
	{
		FinishDownload(false);
		return;
	}
	bInstalling = true;
	const auto Transaction = MakeShared<FSBSBlueprintFileTransaction, ESPMode::ThreadSafe>(
		mDownloadDirectory, mCurrentDownload.OriginalName, MoveTemp(mSbpRequest->mBody->mData),
		MoveTemp(mConfigRequest->mBody->mData));
	const TWeakObjectPtr<ASBSDownloadSubsystem> WeakThis(this);
	Async(EAsyncExecution::ThreadPool,
		  [Transaction, WeakThis]()
		  {
			  const bool bStaged = Transaction->Stage();
			  AsyncTask(ENamedThreads::GameThread,
						[Transaction, WeakThis, bStaged]()
						{
							if (ASBSDownloadSubsystem* Subsystem = WeakThis.Get(); Subsystem && !Subsystem->bEndingPlay)
							{
								Subsystem->OnFilesStaged(Transaction, bStaged);
							}
						});
		  });
}

void ASBSDownloadSubsystem::OnFilesStaged(TSharedPtr<FSBSBlueprintFileTransaction, ESPMode::ThreadSafe> Transaction,
										  bool bSuccess)
{
	if (!bSuccess || !IsValid(mBlueprintSubsystem) || GetCurrentBlueprintPath() != Transaction->GetDirectory() ||
		!Transaction->Commit())
	{
		FinishDownload(false);
		return;
	}
	if (!mBlueprintSubsystem->ReadBlueprintFromDisc(mCurrentDownload.OriginalName))
	{
		Transaction->Rollback();
		mBlueprintSubsystem->RefreshBlueprintsAndDescriptors();
		mBlueprintSubsystem->MarkRecordDataDirty();
		mBlueprintSubsystem->GenerateManifest();
		FinishDownload(false);
		return;
	}

	mBlueprintSubsystem->RefreshBlueprintsAndDescriptors();
	mBlueprintSubsystem->MarkRecordDataDirty();
	mBlueprintSubsystem->GenerateManifest();
	UFGBlueprintDescriptor* Descriptor =
		mBlueprintSubsystem->GetBlueprintDescriptorByNameString(mCurrentDownload.OriginalName);
	if (!IsValid(Descriptor))
	{
		Transaction->Rollback();
		mBlueprintSubsystem->RefreshBlueprintsAndDescriptors();
		mBlueprintSubsystem->MarkRecordDataDirty();
		mBlueprintSubsystem->GenerateManifest();
		FinishDownload(false);
		return;
	}
	Transaction->Accept();

	OnBlueprintCreated(Descriptor, mCurrentDownload);
	FinishDownload(true);
}

void ASBSDownloadSubsystem::FinishDownload(bool bSuccess)
{
	if (mSbpRequest)
	{
		mSbpRequest->Cancel();
	}
	if (mConfigRequest)
	{
		mConfigRequest->Cancel();
	}
	mSbpRequest.Reset();
	mConfigRequest.Reset();
	bInstalling = false;
	FBlueprintJsonStructure Completed = MoveTemp(mCurrentDownload);
	mCurrentDownload = FBlueprintJsonStructure();
	UE_LOG(LogSBS, Display, TEXT("SBS blueprint %s: %s"), *Completed.ID, bSuccess ? TEXT("imported") : TEXT("failed"));
	if (!bEndingPlay)
	{
		mFOnDownloadComplete.Broadcast(Completed, bSuccess);
		if (!bEndingPlay && !mDownloadQueueChecker.IsEmpty())
		{
			mNextDownloadTimer =
				GetWorldTimerManager().SetTimerForNextTick(this, &ASBSDownloadSubsystem::StartNextDownload);
		}
	}
}

void ASBSDownloadSubsystem::ReportProgress(FHttpRequestPtr Request, uint64 BytesReceived, int32 FileIndex)
{
	const TSharedPtr<FSBSRequest>& State = FileIndex == 0 ? mSbpRequest : mConfigRequest;
	if (bEndingPlay || !State || State->mRequest != Request)
	{
		return;
	}
	const FHttpResponsePtr Response = Request->GetResponse();
	const int32 Total = Response ? Response->GetContentLength() : 0;
	const float Percent =
		Total > 0 ? static_cast<float>(FMath::Clamp(100.0 * BytesReceived / Total, 0.0, 100.0)) : 0.0f;
	const double Now = FPlatformTime::Seconds();
	if (Percent != mLastProgress[FileIndex] && (Now - mLastProgressTime[FileIndex] >= 0.1 || Percent == 100.0f))
	{
		mLastProgressTime[FileIndex] = Now;
		mLastProgress[FileIndex] = Percent;
		mOnDownloadProgress.Broadcast(mCurrentDownload, Percent, FileIndex == 0 ? TEXT(".sbp") : TEXT(".sbpcfg"));
	}
}

void ASBSDownloadSubsystem::OnDownloadProgressFile1(FHttpRequestPtr Request, uint64 BytesSend, uint64 BytesReceived)
{
	ReportProgress(Request, BytesReceived, 0);
}

void ASBSDownloadSubsystem::OnDownloadProgressFile2(FHttpRequestPtr Request, uint64 BytesSend, uint64 BytesReceived)
{
	ReportProgress(Request, BytesReceived, 1);
}

int32 ASBSDownloadSubsystem::GetDownloadStateForBlueprint(FBlueprintJsonStructure Blueprint)
{
	if (Blueprint.ID.IsEmpty())
	{
		return 0;
	}
	return mCurrentDownload.ID == Blueprint.ID ? 2 : (mQueuedIds.Contains(Blueprint.ID) ? 1 : 0);
}

bool ASBSDownloadSubsystem::GetDownloadStateForBlueprintPack(FBlueprintPackJsonStructure BlueprintPack)
{
	for (const FBlueprintInPackJsonStructure& Blueprint : BlueprintPack.Blueprints)
	{
		if (!Blueprint.ID.IsEmpty() && (Blueprint.ID == mCurrentDownload.ID || mQueuedIds.Contains(Blueprint.ID)))
		{
			return true;
		}
	}
	return false;
}

FString ASBSDownloadSubsystem::GetCurrentBlueprintPath()
{
	if (!IsValid(mBlueprintSubsystem))
	{
		mBlueprintSubsystem = GetWorld() ? AFGBlueprintSubsystem::Get(GetWorld()) : nullptr;
	}
	FString Path = IsValid(mBlueprintSubsystem) ? mBlueprintSubsystem->GetSessionBlueprintPath() : FString();
	if (!Path.IsEmpty())
	{
		Path = FPaths::ConvertRelativePathToFull(Path);
		FPaths::NormalizeDirectoryName(Path);
	}
	return Path;
}

bool ASBSDownloadSubsystem::IsBlueprintInstalled(FBlueprintJsonStructure Blueprint)
{
	const FString Directory = GetCurrentBlueprintPath();
	return !Directory.IsEmpty() && FSBSStatics::IsSafeFileName(Blueprint.OriginalName) &&
		FPaths::FileExists(FPaths::Combine(Directory, Blueprint.OriginalName + TEXT(".sbp"))) &&
		FPaths::FileExists(FPaths::Combine(Directory, Blueprint.OriginalName + TEXT(".sbpcfg")));
}

bool ASBSDownloadSubsystem::OnBlueprintCreated_Implementation(UFGBlueprintDescriptor* BlueprintDescriptor,
															  FBlueprintJsonStructure Blueprint)
{
	return false;
}
