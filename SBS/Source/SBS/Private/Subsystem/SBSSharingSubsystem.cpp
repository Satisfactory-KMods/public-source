// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Subsystem/SBSSharingSubsystem.h"

#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "FGBlueprintSubsystem.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Http/SBSHttp.h"
#include "Misc/Paths.h"
#include "Sharing/SBSCredentialStore.h"
#include "Sharing/SBSSharingProtocol.h"
#include "Structures/ApiStatics.h"
#include "Subsystem/SBSApiSubsystem.h"

namespace
{
	void CancelRequest(TSharedPtr<FSBSRequest>& State)
	{
		if (State)
		{
			State->Cancel();
			State.Reset();
		}
	}

	FSBSOperationResult HttpFailure(FHttpResponsePtr Response)
	{
		const int32 Status = Response ? Response->GetResponseCode() : 0;
		FSBSOperationResult Result = FSBSOperationResult::Make(
			TEXT("network_error"),
			NSLOCTEXT(
				"SBS", "Sharing.NetworkError",
				"Could not reach SBS. An upload may already have been published; retry with the same request ID and unchanged data."),
			false, Status);
		switch (Status)
		{
		case 401:
			Result = FSBSOperationResult::Make(
				TEXT("unauthorized"),
				NSLOCTEXT("SBS", "Sharing.Unauthorized",
						  "This SBS login has expired or was revoked. Create a new key on K-Mods.com."),
				false, Status);
			break;
		case 403:
			Result = FSBSOperationResult::Make(
				TEXT("forbidden"),
				NSLOCTEXT("SBS", "Sharing.Forbidden", "This account or key cannot publish blueprints."), false, Status);
			break;
		case 409:
			Result = FSBSOperationResult::Make(
				TEXT("request_conflict"),
				NSLOCTEXT(
					"SBS", "Sharing.RequestConflict",
					"This request ID belongs to different upload data, or is still processing. Keep the original data when retrying."),
				false, Status);
			break;
		case 413:
			Result = FSBSOperationResult::Make(
				TEXT("payload_too_large"),
				NSLOCTEXT("SBS", "Sharing.PayloadTooLarge", "The blueprint files exceed the service upload limits."),
				false, Status);
			break;
		case 400:
		case 415:
		case 422:
			Result = FSBSOperationResult::Make(
				TEXT("invalid_upload"),
				NSLOCTEXT("SBS", "Sharing.InvalidUpload", "SBS rejected the blueprint files, metadata or tags."), false,
				Status);
			break;
		case 429:
			Result = FSBSOperationResult::Make(
				TEXT("rate_limited"),
				NSLOCTEXT("SBS", "Sharing.RateLimited", "Too many SBS requests. Wait before trying again."), false,
				Status);
			break;
		case 200:
		case 201:
			Result = FSBSOperationResult::Make(
				TEXT("invalid_response"),
				NSLOCTEXT("SBS", "Sharing.InvalidResponse",
						  "SBS returned an unexpected response. For uploads, keep the request ID before retrying."),
				false, Status);
			break;
		default:
			break;
		}

		int32 Retry = 0;
		if (Response && LexTryParseString(Retry, *Response->GetHeader(TEXT("Retry-After"))))
		{
			Result.RetryAfterSeconds = FMath::Clamp(Retry, 0, 86400);
		}
		return Result;
	}
}

void USBSSharingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bInitialized = true;
}

void USBSSharingSubsystem::Deinitialize()
{
	bInitialized = false;
	ClearRuntime();
	OnSessionUpdated.Clear();
	OnUploadCompleted.Clear();
	OnUploadProgress.Clear();
	Super::Deinitialize();
}

bool USBSSharingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return Super::ShouldCreateSubsystem(Outer) && !IsRunningDedicatedServer();
}

USBSSharingSubsystem* USBSSharingSubsystem::GetSBSSharingSubsystem(const UObject* WorldContext)
{
	if (const UGameInstance* Instance = Cast<UGameInstance>(WorldContext))
	{
		return Instance->GetSubsystem<USBSSharingSubsystem>();
	}
	UWorld* World =
		GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<USBSSharingSubsystem>() : nullptr;
}

bool USBSSharingSubsystem::CanUseService() const
{
	return bInitialized && !IsRunningDedicatedServer() && (!IsRunningCommandlet() || FSBSStatics::GetLocalTestPort());
}

FString USBSSharingSubsystem::GetServiceBase() const
{
	const FString Base = FSBSStatics::MakeUrl(TEXT(""), const_cast<USBSSharingSubsystem*>(this));
	return FSBSSharingProtocol::IsCredentialUrl(Base + TEXT("auth/me"), Base) ? Base : FString();
}

bool USBSSharingSubsystem::CanRememberLogin() const
{
	return CanUseService() && !FSBSStatics::GetLocalTestPort() && !IsRunningCommandlet() &&
		GetServiceBase() == FSBSSharingProtocol::ProductionBase() && FSBSCredentialStore::IsSupported();
}

FString USBSSharingSubsystem::GetLoginUrl() const
{
	return FString(FSBSSharingProtocol::ProductionBase()) + TEXT("auth/login");
}

FSBSOperationResult USBSSharingSubsystem::OpenLoginPage()
{
	if (!CanUseService() || IsRunningCommandlet() || FSBSStatics::GetLocalTestPort())
	{
		return FSBSOperationResult::Make(
			TEXT("unavailable"),
			NSLOCTEXT("SBS", "Sharing.Unavailable", "Website login is unavailable in this context."));
	}
	FString Error;
	FPlatformProcess::LaunchURL(*GetLoginUrl(), nullptr, &Error);
	return Error.IsEmpty()
		? FSBSOperationResult::Make(TEXT("browser_opened"),
									NSLOCTEXT("SBS", "Sharing.BrowserOpened",
											  "Sign in on K-Mods.com, create an SBS key, then paste it here."),
									true)
		: FSBSOperationResult::Make(
			  TEXT("browser_error"),
			  NSLOCTEXT("SBS", "Sharing.BrowserError", "Could not open the browser. Open the login URL manually."));
}

void USBSSharingSubsystem::LoginWithKey(FString UserKey, bool bRememberLogin)
{
	BeginLogin(MoveTemp(UserKey), bRememberLogin, false);
}

void USBSSharingSubsystem::RestoreLogin()
{
	if (bChangingSession || mSession.State != ESBSLoginState::SignedOut)
	{
		return;
	}
	if (!CanRememberLogin())
	{
		NotifySession(FSBSOperationResult::Make(
			TEXT("storage_unavailable"),
			NSLOCTEXT("SBS", "Sharing.StorageUnavailable", "Saved login is unavailable in this context.")));
		return;
	}
	FString Key;
	const FSBSOperationResult Loaded = FSBSCredentialStore::Read(Key);
	if (!Loaded.bSuccess)
	{
		NotifySession(Loaded);
		return;
	}
	BeginLogin(MoveTemp(Key), true, true);
}

void USBSSharingSubsystem::BeginLogin(FString Key, bool bRemember, bool bRestore)
{
	if (bChangingSession)
	{
		FSBSSharingProtocol::ClearSecret(Key);
		return;
	}
	TGuardValue<bool> Guard(bChangingSession, true);
	const FString Base = GetServiceBase();
	if (!CanUseService() || Base.IsEmpty() || !FSBSSharingProtocol::IsUserKey(Key) ||
		(FSBSStatics::GetLocalTestPort() && Key != FSBSSharingProtocol::FixtureKey()))
	{
		FSBSSharingProtocol::ClearSecret(Key);
		NotifySession(FSBSOperationResult::Make(
			TEXT("invalid_login"),
			NSLOCTEXT("SBS", "Sharing.InvalidLogin",
					  "Use an SBS user key on the production API. Test mode accepts only its dummy key.")));
		return;
	}

	if (!bRestore && CanRememberLogin())
	{
		const FSBSOperationResult Removed = FSBSCredentialStore::Delete();
		if (!Removed.bSuccess)
		{
			FSBSSharingProtocol::ClearSecret(Key);
			NotifySession(Removed);
			return;
		}
	}
	CancelUpload();
	ClearRuntime();
	bUsesUserKeyLogin = true;
	bRememberPending = bRemember;
	bRestoring = bRestore;
	mPendingKey = MoveTemp(Key);
	mLoginBase = Base;
	mSession.State = ESBSLoginState::SigningIn;
	mLoginRequest = FSBSRequest::Create(Base + TEXT("auth/me"), TEXT("GET"), TEXT(""), this,
										FSBSSharingProtocol::MaxResponseBytes, 20.0f, false);
	mLoginRequest->mRequest->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + mPendingKey);
	mLoginRequest->mRequest->OnProcessRequestComplete().BindUObject(this, &USBSSharingSubsystem::OnLoginDone);
	NotifySession(
		FSBSOperationResult::Make(TEXT("signing_in"), NSLOCTEXT("SBS", "Sharing.SigningIn", "Checking SBS login.")));
	if (bInitialized && mLoginRequest && !mLoginRequest->Start())
	{
		OnLoginDone(mLoginRequest->mRequest, nullptr, false);
	}
}

void USBSSharingSubsystem::OnLoginDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (!bInitialized || !mLoginRequest || mLoginRequest->mRequest != Request)
	{
		return;
	}
	TGuardValue<bool> Guard(bChangingSession, true);
	const TSharedPtr<FSBSRequest> Completed = MoveTemp(mLoginRequest);
	TSharedPtr<FJsonObject> Json;
	FSBSOperationResult Result = HttpFailure(Response);
	const bool bValid = Response && Response->GetResponseCode() == 200 && GetServiceBase() == mLoginBase &&
		Completed->ReadJson(Response, bSuccess, Json) && FSBSSharingProtocol::ParseSession(Json, mSession);
	if (bValid)
	{
		mUserKey = MoveTemp(mPendingKey);
		mSessionBase = mLoginBase;
		Result = FSBSOperationResult::Make(TEXT("signed_in"),
										   NSLOCTEXT("SBS", "Sharing.SignedIn", "Signed in to K-Mods.com."), true, 200);
		if (bRestoring)
		{
			mSession.bRemembered = true;
		}
		else if (bRememberPending)
		{
			const FSBSOperationResult Saved = CanRememberLogin()
				? FSBSCredentialStore::Write(mUserKey)
				: FSBSOperationResult::Make(
					  TEXT("storage_unavailable"),
					  NSLOCTEXT("SBS", "Sharing.StorageUnavailable2", "Secure login storage is unavailable."));
			mSession.bRemembered = Saved.bSuccess;
			if (!Saved.bSuccess)
			{
				Result = FSBSOperationResult::Make(
					TEXT("signed_in_not_saved"),
					NSLOCTEXT("SBS", "Sharing.SignedInNotSaved",
							  "Signed in for this session. Windows could not save the login."),
					true, 200);
			}
		}
	}
	else
	{
		mSession = FSBSSession();

		if (bRestoring && Response && Response->GetResponseCode() == 401 && CanRememberLogin())
		{
			const FSBSOperationResult Removed = FSBSCredentialStore::Delete();
			if (!Removed.bSuccess)
			{
				Result = Removed;
			}
		}
	}
	FSBSSharingProtocol::ClearSecret(mPendingKey);
	mLoginBase.Empty();
	NotifySession(Result);
}

void USBSSharingSubsystem::ClearRuntime()
{
	CancelRequest(mLoginRequest);
	CancelRequest(mUploadRequest);
	++mUploadGeneration;
	bUploading = false;
	mUploadId.Invalidate();
	mUploadBytes = 0;
	FSBSSharingProtocol::ClearSecret(mPendingKey);
	FSBSSharingProtocol::ClearSecret(mUserKey);
	mSessionBase.Empty();
	mLoginBase.Empty();
	mSession = FSBSSession();
}

void USBSSharingSubsystem::Logout()
{
	if (bChangingSession)
	{
		return;
	}
	TGuardValue<bool> Guard(bChangingSession, true);
	CancelUpload();
	ClearRuntime();
	bUsesUserKeyLogin = true;
	FSBSOperationResult Result = FSBSOperationResult::Make(
		TEXT("signed_out"),
		NSLOCTEXT("SBS", "Sharing.SignedOut",
				  "Signed out. Revoke the key on K-Mods.com if it should stop working on other devices."),
		true);
	if (!FSBSStatics::GetLocalTestPort() && !IsRunningCommandlet() && FSBSCredentialStore::IsSupported())
	{
		const FSBSOperationResult Removed = FSBSCredentialStore::Delete();
		if (!Removed.bSuccess)
		{
			Result = Removed;
		}
	}
	NotifySession(Result);
}

void USBSSharingSubsystem::OnApiEnvironmentChanged()
{
	if (bChangingSession)
	{
		return;
	}
	TGuardValue<bool> Guard(bChangingSession, true);
	CancelUpload();
	ClearRuntime();
	OnSessionUpdated.Broadcast(
		mSession,
		FSBSOperationResult::Make(
			TEXT("environment_changed"),
			NSLOCTEXT("SBS", "Sharing.EnvironmentChanged", "API settings changed. Sign in again before publishing.")));
}

void USBSSharingSubsystem::NotifySession(const FSBSOperationResult& Result)
{
	if (UGameInstance* Instance = GetGameInstance())
	{
		if (USBSApiSubsystem* Api = Instance->GetSubsystem<USBSApiSubsystem>())
		{
			Api->OnSharingSessionChanged();
		}
	}
	if (bInitialized)
	{
		OnSessionUpdated.Broadcast(mSession, Result);
	}
}

bool USBSSharingSubsystem::ApplyAuthorization(const FString& Url, TMap<FString, FString>& Headers) const
{
	if (!CanUseService() || mSession.State != ESBSLoginState::SignedIn || mUserKey.IsEmpty() ||
		mSessionBase != GetServiceBase() || !FSBSSharingProtocol::IsCredentialUrl(Url, mSessionBase))
	{
		return false;
	}
	Headers.Add(TEXT("Authorization"), TEXT("Bearer ") + mUserKey);
	return true;
}

FString USBSSharingSubsystem::GetLocalDirectory() const
{
	AFGBlueprintSubsystem* Blueprints = GetWorld() ? AFGBlueprintSubsystem::Get(GetWorld()) : nullptr;
	FString Directory = IsValid(Blueprints) ? Blueprints->GetSessionBlueprintPath() : FString();
	if (!Directory.IsEmpty())
	{
		Directory = FPaths::ConvertRelativePathToFull(Directory);
		FPaths::NormalizeDirectoryName(Directory);
	}
	return Directory;
}

TArray<FSBSLocalBlueprint> USBSSharingSubsystem::GetLocalBlueprints(FSBSOperationResult& Result) const
{
	TArray<FSBSLocalBlueprint> Blueprints;
	const FString Directory = CanUseService() ? GetLocalDirectory() : FString();
	if (Directory.IsEmpty())
	{
		Result = FSBSOperationResult::Make(
			TEXT("no_session"),
			NSLOCTEXT("SBS", "Sharing.NoSession", "Load a save and save a blueprint on this computer first."));
		return Blueprints;
	}
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(Directory, TEXT("*.sbp")), true, false);
	Files.Sort();
	for (const FString& File : Files)
	{
		FSBSLocalBlueprint Blueprint;
		Blueprint.LocalBlueprintName = FPaths::GetBaseFilename(File);
		Blueprint.BlueprintBytes = IFileManager::Get().FileSize(*FPaths::Combine(Directory, File));
		Blueprint.ConfigBytes =
			IFileManager::Get().FileSize(*FPaths::Combine(Directory, Blueprint.LocalBlueprintName + TEXT(".sbpcfg")));
		if (FSBSStatics::IsSafeFileName(Blueprint.LocalBlueprintName) && Blueprint.BlueprintBytes >= 20 &&
			Blueprint.BlueprintBytes <= FSBSSharingProtocol::MaxBlueprintBytes && Blueprint.ConfigBytes >= 20 &&
			Blueprint.ConfigBytes <= FSBSSharingProtocol::MaxConfigBytes)
		{
			Blueprints.Add(MoveTemp(Blueprint));
		}
	}
	Result = FSBSOperationResult::Make(TEXT("local_blueprints_ready"),
									   NSLOCTEXT("SBS", "Sharing.LocalBlueprintsReady", "Local blueprint list loaded."),
									   true);
	return Blueprints;
}

FGuid USBSSharingSubsystem::UploadBlueprint(FSBSBlueprintUpload Upload)
{
	if (!Upload.RequestId.IsValid())
	{
		Upload.RequestId = FGuid::NewGuid();
	}
	const FGuid RequestId = Upload.RequestId;
	FSBSOperationResult Validation = FSBSSharingProtocol::ValidateUpload(Upload);
	const FString Directory = CanUseService() ? GetLocalDirectory() : FString();
	if (bUploading || bChangingSession)
	{
		Validation = FSBSOperationResult::Make(
			TEXT("busy"), NSLOCTEXT("SBS", "Sharing.Busy", "Wait for the current SBS operation to finish."));
	}
	else if (!CanUseService() || mSession.State != ESBSLoginState::SignedIn || mSessionBase != GetServiceBase())
	{
		Validation = FSBSOperationResult::Make(
			TEXT("login_required"),
			NSLOCTEXT("SBS", "Sharing.LoginRequired", "Sign in to K-Mods.com before uploading."));
	}
	else if (!mSession.bCanPublish)
	{
		Validation = FSBSOperationResult::Make(
			TEXT("forbidden"),
			NSLOCTEXT("SBS", "Sharing.Forbidden2", "This SBS key does not have permission to publish."));
	}
	else if (Directory.IsEmpty())
	{
		Validation = FSBSOperationResult::Make(
			TEXT("no_session"),
			NSLOCTEXT("SBS", "Sharing.NoSession2", "Load a save and select a blueprint available on this computer."));
	}
	if (!Validation.bSuccess)
	{
		FSBSBlueprintUploadResult Failed;
		Failed.RequestId = RequestId;
		Failed.Result = Validation;
		OnUploadCompleted.Broadcast(Failed);
		return RequestId;
	}
	bUploading = true;
	mUploadId = RequestId;
	const uint64 Generation = ++mUploadGeneration;
	const FString Base = mSessionBase;
	const FString Boundary = TEXT("SBS_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const TWeakObjectPtr<USBSSharingSubsystem> WeakThis(this);
	const TWeakObjectPtr<UWorld> UploadWorld(GetWorld());
	mLastProgress = 0;
	mLastProgressTime = 0;

	Async(
		EAsyncExecution::ThreadPool,
		[WeakThis, UploadWorld, Upload = MoveTemp(Upload), Directory, Base, Boundary, Generation]()
		{
			TArray<uint8> Sbp, Config, Body;
			FSBSOperationResult Prepared =
				FSBSSharingProtocol::ReadLocalFiles(Directory, Upload.LocalBlueprintName, Sbp, Config);
			if (Prepared.bSuccess)
			{
				Prepared = FSBSSharingProtocol::BuildMultipart(Upload, Sbp, Config, Boundary, Body);
			}
			AsyncTask(
				ENamedThreads::GameThread,
				[WeakThis, UploadWorld, Directory, Base, Boundary, Generation, Prepared,
				 Body = MoveTemp(Body)]() mutable
				{
					USBSSharingSubsystem* Self = WeakThis.Get();
					if (!Self || !Self->bInitialized || !Self->bUploading || Self->mUploadGeneration != Generation)
					{
						return;
					}
					if (Self->GetWorld() != UploadWorld.Get() || Directory != Self->GetLocalDirectory() ||
						Base != Self->GetServiceBase())
					{
						Prepared =
							FSBSOperationResult::Make(TEXT("session_changed"),
													  NSLOCTEXT("SBS", "Sharing.SessionChanged",
																"The save or API changed while preparing the upload."));
					}
					if (!Prepared.bSuccess)
					{
						FSBSBlueprintUploadResult Failed;
						Failed.RequestId = Self->mUploadId;
						Failed.Result = Prepared;
						Self->FinishUpload(MoveTemp(Failed));
						return;
					}
					Self->mUploadBytes = Body.Num();
					Self->mUploadRequest = FSBSRequest::Create(Base + TEXT("blueprints"), TEXT("POST"), TEXT(""), Self,
															   FSBSSharingProtocol::MaxResponseBytes, 180.0f, false);
					FHttpRequestPtr Request = Self->mUploadRequest->mRequest;
					Request->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + Self->mUserKey);
					Request->SetHeader(TEXT("Idempotency-Key"),
									   Self->mUploadId.ToString(EGuidFormats::DigitsWithHyphensLower));
					Request->SetHeader(TEXT("Content-Type"), TEXT("multipart/form-data; boundary=") + Boundary);
					Request->SetContent(MoveTemp(Body));
					Request->OnProcessRequestComplete().BindUObject(Self, &USBSSharingSubsystem::OnUploadDone);
					Request->OnRequestProgress64().BindWeakLambda(
						Self,
						[Self](FHttpRequestPtr ProgressRequest, uint64 Sent, uint64 Received)
						{
							if (!Self->mUploadRequest || Self->mUploadRequest->mRequest != ProgressRequest ||
								Self->mUploadBytes <= 0)
							{
								return;
							}
							const float Progress =
								FMath::Clamp(static_cast<float>(Sent) / Self->mUploadBytes, Self->mLastProgress, 1.0f);
							const double Now = FPlatformTime::Seconds();
							if (Progress > Self->mLastProgress &&
								(Now - Self->mLastProgressTime >= 0.1 || Progress == 1.0f))
							{
								Self->mLastProgress = Progress;
								Self->mLastProgressTime = Now;
								Self->OnUploadProgress.Broadcast(Self->mUploadId, Progress);
							}
						});
					if (!Self->mUploadRequest->Start())
					{
						Self->OnUploadDone(Request, nullptr, false);
					}
				});
		});
	OnUploadProgress.Broadcast(RequestId, 0.0f);
	return RequestId;
}

void USBSSharingSubsystem::OnUploadDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (!bInitialized || !mUploadRequest || mUploadRequest->mRequest != Request)
	{
		return;
	}
	const TSharedPtr<FSBSRequest> Completed = MoveTemp(mUploadRequest);
	FSBSBlueprintUploadResult Result;
	Result.RequestId = mUploadId;
	TSharedPtr<FJsonObject> Json;
	if (Response && (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 201) &&
		Completed->ReadJson(Response, bSuccess, Json) && FSBSSharingProtocol::ParseUpload(Json, mUploadId, Result))
	{
		Result.Result = FSBSOperationResult::Make(
			TEXT("published"), NSLOCTEXT("SBS", "Sharing.Published", "Blueprint published on K-Mods.com."), true,
			Response->GetResponseCode());
	}
	else
	{
		Result.Result = HttpFailure(Response);
	}
	if (Response && Response->GetResponseCode() == 401)
	{

		FSBSSharingProtocol::ClearSecret(mUserKey);
		mSession = FSBSSession();
		mSessionBase.Empty();
		if (CanRememberLogin())
		{
			const FSBSOperationResult Removed = FSBSCredentialStore::Delete();
			if (!Removed.bSuccess)
			{
				Result.Result = Removed;
			}
		}
		TGuardValue<bool> Guard(bChangingSession, true);
		NotifySession(Result.Result);
	}
	FinishUpload(MoveTemp(Result));
}

void USBSSharingSubsystem::FinishUpload(FSBSBlueprintUploadResult Result)
{
	CancelRequest(mUploadRequest);
	bUploading = false;
	mUploadId.Invalidate();
	mUploadBytes = 0;
	++mUploadGeneration;
	OnUploadCompleted.Broadcast(Result);
}

void USBSSharingSubsystem::CancelUpload()
{
	if (bUploading)
	{
		FSBSBlueprintUploadResult Result;
		Result.RequestId = mUploadId;
		Result.Result = FSBSOperationResult::Make(
			TEXT("cancelled"),
			NSLOCTEXT(
				"SBS", "Sharing.Cancelled",
				"Upload cancelled locally. Publication may already have completed; keep the request ID to check or retry safely."));
		FinishUpload(MoveTemp(Result));
	}
}
