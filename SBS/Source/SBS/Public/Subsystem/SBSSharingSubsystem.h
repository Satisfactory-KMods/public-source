// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Structures/SBSSharingTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SBSSharingSubsystem.generated.h"

struct FSBSRequest;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSBSSessionUpdated, FSBSSession, Session, FSBSOperationResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSBSUploadCompleted, FSBSBlueprintUploadResult, Upload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSBSUploadProgress, FGuid, RequestId, float, Progress);

UCLASS()
class SBS_API USBSSharingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	UFUNCTION(BlueprintPure, Category = "SBS|Sharing", meta = (WorldContext = "WorldContext"))
	static USBSSharingSubsystem* GetSBSSharingSubsystem(const UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "SBS|Account")
	FSBSOperationResult OpenLoginPage();
	UFUNCTION(BlueprintPure, Category = "SBS|Account")
	FString GetLoginUrl() const;

	UFUNCTION(BlueprintCallable, Category = "SBS|Account")
	void LoginWithKey(FString UserKey, bool bRememberLogin = false);

	UFUNCTION(BlueprintCallable, Category = "SBS|Account")
	void RestoreLogin();

	UFUNCTION(BlueprintCallable, Category = "SBS|Account")
	void Logout();
	UFUNCTION(BlueprintPure, Category = "SBS|Account")
	FSBSSession GetSession() const { return mSession; }
	UFUNCTION(BlueprintPure, Category = "SBS|Account")
	bool CanRememberLogin() const;

	UFUNCTION(BlueprintCallable, Category = "SBS|Upload")
	TArray<FSBSLocalBlueprint> GetLocalBlueprints(FSBSOperationResult& Result) const;

	UFUNCTION(BlueprintCallable, Category = "SBS|Upload")
	FGuid UploadBlueprint(FSBSBlueprintUpload Upload);

	UFUNCTION(BlueprintCallable, Category = "SBS|Upload")
	void CancelUpload();
	UFUNCTION(BlueprintPure, Category = "SBS|Upload")
	bool IsUploading() const { return bUploading; }

	UPROPERTY(BlueprintAssignable, Category = "SBS|Account")
	FSBSSessionUpdated OnSessionUpdated;
	UPROPERTY(BlueprintAssignable, Category = "SBS|Upload")
	FSBSUploadCompleted OnUploadCompleted;
	UPROPERTY(BlueprintAssignable, Category = "SBS|Upload")
	FSBSUploadProgress OnUploadProgress;

	bool UsesUserKeyLogin() const { return bUsesUserKeyLogin; }
	bool ApplyAuthorization(const FString& Url, TMap<FString, FString>& Headers) const;
	void OnApiEnvironmentChanged();

private:
	UPROPERTY(Transient)
	FSBSSession mSession;
	FString mUserKey;
	FString mPendingKey;
	FString mSessionBase;
	FString mLoginBase;
	TSharedPtr<FSBSRequest> mLoginRequest;
	TSharedPtr<FSBSRequest> mUploadRequest;
	FGuid mUploadId;
	uint64 mUploadGeneration = 0;
	bool bInitialized = false;
	bool bUsesUserKeyLogin = false;
	bool bChangingSession = false;
	bool bRememberPending = false;
	bool bRestoring = false;
	bool bUploading = false;
	int64 mUploadBytes = 0;
	double mLastProgressTime = 0;
	float mLastProgress = 0;

	bool CanUseService() const;
	FString GetServiceBase() const;
	FString GetLocalDirectory() const;
	void BeginLogin(FString Key, bool bRemember, bool bRestore);
	void ClearRuntime();
	void NotifySession(const FSBSOperationResult& Result);
	void OnLoginDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void OnUploadDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void FinishUpload(FSBSBlueprintUploadResult Result);
};
