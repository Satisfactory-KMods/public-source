// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "Containers/Ticker.h"
#include "CoreMinimal.h"

struct FSBSRequest;
struct FSBSDynamicRequest;
class UConfigProperty;

#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Structures/ApiJsonStruct.h"
#include "Structures/ApiPostStruct.h"
#include "Structures/SBSDownloadIdTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SBSApiSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBlueprintQueryDone, const TArray<FBlueprintJsonStructure>&,
											   Blueprints, int32, Max, bool, Success);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBlueprintPackQueryDone, const TArray<FBlueprintPackJsonStructure>&,
											   BlueprintPacks, int32, Max, bool, Success);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDynamicQueryDone, FDynamicApiPostStruct, PostResult, bool, Success);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAuthUpdated, FSBSUserData, UserData, bool, Success);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDownloadIdResolved, FSBSResolvedDownload, Download);

UCLASS()
class SBS_API USBSApiSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

public:
	UFUNCTION(BlueprintCallable, Category = "KMods|Json")
	void QueryTags();

	UFUNCTION(BlueprintCallable, Category = "KMods|Json")
	void QueryApi(FFilterPostStruct Post);

	UFUNCTION(BlueprintCallable, Category = "KMods|Json")
	void QueryPackApi(FPackFilterPostStruct Post);

	UFUNCTION(BlueprintCallable, Category = "KMods|Json")
	void QueryApiDynamic(FDynamicApiPostStruct Post);

	UFUNCTION(BlueprintCallable, Category = "KMods|Json")
	void QueryRating(FRatingPostStruct Post);

	UFUNCTION(BlueprintPure, Category = "KMods|Json")
	bool IsQuery() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Json")
	bool IsPackQuery() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Json")
	bool IsBpQuery() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Json")
	bool IsAuthQuery() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Json")
	void QueryForAuth();

	UFUNCTION(BlueprintCallable, Category = "SBS|Download ID")
	FGuid ResolveDownloadId(FString Input);

	UFUNCTION(BlueprintCallable, Category = "SBS|Download ID")
	void CancelResolveDownloadId();

	UFUNCTION(BlueprintPure, Category = "SBS|Download ID")
	bool IsResolvingDownloadId() const { return mResolveRequest.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "KMods|Json")
	FORCEINLINE TArray<FBlueprintJsonStructure> GetBlueprints() const { return mCurrentBlueprints; }

	UFUNCTION(BlueprintPure, Category = "KMods|Json")
	FORCEINLINE TArray<FBlueprintPackJsonStructure> GetBlueprintPacks() const { return mCurrentBlueprintPacks; }

	UFUNCTION(BlueprintPure, Category = "KMods|Json")
	FORCEINLINE int32 GetTotalBlueprints() const { return mTotalBlueprints; }

	UFUNCTION(BlueprintPure, Category = "KMods|Json")
	FORCEINLINE int32 GetTotalBlueprintPacks() const { return mTotalBlueprintPacks; }

	UFUNCTION(BlueprintPure, Category = "KMods|Json")
	FORCEINLINE FSBSUserData GetUserData() const { return mUserData; }

	UFUNCTION(BlueprintPure, Category = "KMods|Json")
	FORCEINLINE bool IsLoggedIn() const { return FSBSStatics::IsSafeIdentifier(mUserData.ID); }

	UFUNCTION(BlueprintPure, Category = "KMods|Json")
	FORCEINLINE bool GetIsInQuery() const { return bIsInQuery; }

	UFUNCTION(BlueprintPure, Category = "KMods|Json")
	FORCEINLINE TArray<FBlueprintJsonTagStructure> GetTags() const { return mTags; }

	static USBSApiSubsystem* Get(const UObject* WorldContext);
	void OnSharingSessionChanged();

	UPROPERTY(BlueprintAssignable)
	FOnBlueprintQueryDone mOnQueryDone;

	UPROPERTY(BlueprintAssignable)
	FOnBlueprintPackQueryDone mOnPackQueryDone;

	UPROPERTY(BlueprintAssignable)
	FOnDynamicQueryDone mOnDynamicQueryDone;

	UPROPERTY(BlueprintAssignable)
	FOnAuthUpdated mOnAuthUpdated;

	UPROPERTY(BlueprintAssignable, Category = "SBS|Download ID")
	FOnDownloadIdResolved mOnDownloadIdResolved;

	UPROPERTY(BlueprintReadOnly, Category = "BlueprintJsonStructure")
	TArray<FBlueprintJsonTagStructure> mTags;

protected:
	void OnBlueprintQueryDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

	void OnBlueprintPackQueryDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

	void OnQueryDynamicDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

	void OnQueryAuthDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

	UFUNCTION()
	void OnAccountKeyChanged();

private:

	friend class USBSBlueprintMigrationLibrary;

	UPROPERTY()
	TArray<FBlueprintJsonStructure> mCurrentBlueprints;

	UPROPERTY()
	TArray<FBlueprintPackJsonStructure> mCurrentBlueprintPacks;

	UPROPERTY()
	int32 mTotalBlueprints = 0;

	UPROPERTY()
	int32 mTotalBlueprintPacks = 0;

	UPROPERTY()
	FSBSUserData mUserData;

	UPROPERTY()
	FDynamicApiPostStruct mDynamicQuery;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UConfigProperty>> mObservedProperties;

	TSharedPtr<FSBSRequest> mBlueprintRequest;
	TSharedPtr<FSBSRequest> mPackRequest;
	TSharedPtr<FSBSRequest> mAuthRequest;
	TSharedPtr<FSBSRequest> mDynamicRequest;
	TSharedPtr<FSBSRequest> mResolveRequest;
	FGuid mResolveId;
	FSBSDownloadReference mResolveReference;
	FString mResolveBase;
	TWeakObjectPtr<UWorld> mResolveWorld;
	TArray<TSharedPtr<FSBSDynamicRequest>> mPendingDynamic;
	bool bDynamicIsTags = false;
	bool bInitialized = false;
	bool bDispatchingDynamic = false;
	bool bChangingConfiguration = false;
	FTSTicker::FDelegateHandle mDynamicPumpHandle;

	void BindConfiguration();
	void QueueDynamic(FDynamicApiPostStruct Post, FString Url, FString Payload, bool bTags = false);
	void StartNextDynamic();
	void ScheduleNextDynamic();
	void OnResolveDownloadIdDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void FinishResolveDownloadId(FSBSResolvedDownload Download);

	UPROPERTY()
	bool bIsInQuery = false;

	UPROPERTY()
	bool bIsInPackQuery = false;

	UPROPERTY()
	bool bIsInAuthQuery = false;
};
