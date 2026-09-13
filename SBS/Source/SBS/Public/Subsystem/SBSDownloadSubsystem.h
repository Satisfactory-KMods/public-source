// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FGBlueprintSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Structures/ApiJsonStruct.h"
#include "Structures/SBSDownloadIdTypes.h"
#include "Subsystem/KPCLModSubsystem.h"
#include "SBSDownloadSubsystem.generated.h"

class USBSApiSubsystem;
class UFGLocalPlayer;
struct FSBSRequest;
class FSBSBlueprintFileTransaction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDownloadComplete, FBlueprintJsonStructure, Blueprint, bool, Success);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDownloadProgress, FBlueprintJsonStructure, Blueprint, float, Progress,
											   FString, FileName);

UCLASS()
class SBS_API ASBSDownloadSubsystem : public AKPCLModSubsystem
{
	GENERATED_BODY()

public:

	ASBSDownloadSubsystem();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void SubsytemTick(float dt) override;

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "KMods|Subsystem|SBS",
			  DisplayName = "GetSBSDownloadSubsystem")
	static ASBSDownloadSubsystem* Get(UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "KMods|Json")
	bool DownloadBlueprint(FBlueprintJsonStructure Blueprint);

	UFUNCTION(BlueprintCallable, Category = "KMods|Json")
	bool DownloadBlueprintPack(FBlueprintPackJsonStructure BlueprintPack);

	UFUNCTION(BlueprintCallable, Category = "SBS|Download ID")
	FSBSOperationResult DownloadResolvedId(const FSBSResolvedDownload& Download);

	UFUNCTION(BlueprintCallable, Category = "KMods|Json")
	int32 GetDownloadStateForBlueprint(FBlueprintJsonStructure Blueprint);

	UFUNCTION(BlueprintCallable, Category = "KMods|Json")
	bool GetDownloadStateForBlueprintPack(FBlueprintPackJsonStructure BlueprintPack);

	UFUNCTION(BlueprintCallable, Category = "KMods|Json")
	FString GetCurrentBlueprintPath();

	UFUNCTION(BlueprintCallable, Category = "KMods|Json")
	bool IsBlueprintInstalled(FBlueprintJsonStructure Blueprint);

	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Json")
	bool OnBlueprintCreated(UFGBlueprintDescriptor* BlueprintDescriptor, FBlueprintJsonStructure Blueprint);

	UFUNCTION(BlueprintPure, Category = "KMods|Json")
	static UFGLocalPlayer* SBS_GetLocalPlayer(UObject* WorldContext);

	UPROPERTY(BlueprintReadOnly)
	FString mSaveSessionName = FString();

private:
	UPROPERTY()
	TObjectPtr<USBSApiSubsystem> mApiSubsystem = nullptr;

	bool bDownloadFile1Completed = false;
	bool bDownloadFile2Completed = false;

	bool bDownloadFailed = false;

	void OnOneDownloadComplete();

	void OnDownloadCompleteFile1(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

	void OnDownloadCompleteFile2(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

	void OnDownloadProgressFile1(FHttpRequestPtr HttpRequest, uint64 BytesSend, uint64 InBytesReceived);

	void OnDownloadProgressFile2(FHttpRequestPtr HttpRequest, uint64 BytesSend, uint64 InBytesReceived);

	void StartNextDownload();
	void FinishDownload(bool bSuccess);
	void ReportProgress(FHttpRequestPtr Request, uint64 BytesReceived, int32 FileIndex);
	void OnFilesStaged(TSharedPtr<FSBSBlueprintFileTransaction, ESPMode::ThreadSafe> Transaction, bool bSuccess);

	TSharedPtr<FSBSRequest> mSbpRequest;
	TSharedPtr<FSBSRequest> mConfigRequest;
	TSet<FString> mQueuedIds;
	FString mDownloadDirectory;
	FTimerHandle mNextDownloadTimer;
	bool bStarted = false;
	bool bEndingPlay = false;
	bool bInstalling = false;
	double mLastProgressTime[2] = {};
	float mLastProgress[2] = {};

	UPROPERTY()
	FBlueprintJsonStructure mCurrentDownload;

	UPROPERTY(Transient)
	TArray<FBlueprintJsonStructure> mDownloadQueueChecker;

	UPROPERTY(BlueprintAssignable)
	FOnDownloadComplete mFOnDownloadComplete;

	UPROPERTY(BlueprintAssignable)
	FOnDownloadProgress mOnDownloadProgress;

	UPROPERTY()
	TObjectPtr<AFGBlueprintSubsystem> mBlueprintSubsystem = nullptr;
};
