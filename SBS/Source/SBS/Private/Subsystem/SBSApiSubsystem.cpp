// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Subsystem/SBSApiSubsystem.h"

#include "BFL/KBFL_ConfigTools.h"
#include "Configuration/ConfigManager.h"
#include "Configuration/ConfigProperty.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Http/SBSHttp.h"
#include "Logging.h"
#include "Sharing/SBSSharingProtocol.h"
#include "Subsystem/SBSSharingSubsystem.h"

struct FSBSDynamicRequest
{
	FDynamicApiPostStruct mPost;
	FString mUrl;
	FString mPayload;
	bool bTags = false;
};

namespace
{
	template <typename T>
	bool ParsePage(const TSharedPtr<FJsonObject>& Json, TArray<T>& Items, int32& Total, UObject* WorldContext)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		double Count = 0;
		if (!Json->TryGetArrayField(TEXT("blueprints"), Values) || Values->Num() > 200 ||
			!Json->TryGetNumberField(TEXT("totalBlueprints"), Count) || !FMath::IsFinite(Count) || Count < 0 ||
			Count > MAX_int32 || FMath::FloorToDouble(Count) != Count)
		{
			return false;
		}
		TArray<T> Parsed;
		Parsed.Reserve(Values->Num());
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Value || !Value->TryGetObject(Object) || !Object->IsValid())
			{
				return false;
			}
			T Item;
			Item.setJsonObject(*Object);
			Item.parse(WorldContext);
			if (!FSBSStatics::IsSafeIdentifier(Item.ID))
			{
				return false;
			}
			Parsed.Add(MoveTemp(Item));
		}
		Items = MoveTemp(Parsed);
		Total = static_cast<int32>(Count);
		return true;
	}

	void CancelRequest(TSharedPtr<FSBSRequest>& State)
	{
		if (State)
		{
			State->Cancel();
			State.Reset();
		}
	}
}

void USBSApiSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UConfigManager>();
	if (!IsRunningDedicatedServer())
	{
		Collection.InitializeDependency<USBSSharingSubsystem>();
	}
	bInitialized = true;
	BindConfiguration();
}

void USBSApiSubsystem::Deinitialize()
{
	bInitialized = false;
	FTSTicker::GetCoreTicker().RemoveTicker(mDynamicPumpHandle);
	mDynamicPumpHandle.Reset();
	for (UConfigProperty* Property : mObservedProperties)
	{
		if (IsValid(Property))
		{
			Property->OnPropertyValueChanged.RemoveDynamic(this, &USBSApiSubsystem::OnAccountKeyChanged);
		}
	}
	mObservedProperties.Reset();
	CancelRequest(mBlueprintRequest);
	CancelRequest(mPackRequest);
	CancelRequest(mAuthRequest);
	CancelRequest(mDynamicRequest);
	CancelResolveDownloadId();
	mPendingDynamic.Reset();
	mDynamicQuery = FDynamicApiPostStruct();
	mUserData = FSBSUserData();
	bIsInQuery = bIsInPackQuery = bIsInAuthQuery = false;
	mOnQueryDone.Clear();
	mOnPackQueryDone.Clear();
	mOnAuthUpdated.Clear();
	mOnDynamicQueryDone.Clear();
	mOnDownloadIdResolved.Clear();
	Super::Deinitialize();
}

void USBSApiSubsystem::BindConfiguration()
{
	if (!bInitialized)
	{
		return;
	}
	for (const TCHAR* Key : {TEXT("accountkey"), TEXT("usedev"), TEXT("uselocaldev")})
	{
		UConfigProperty* Property = UKBFL_ConfigTools::GetConfigPropertyByKey(FSBSStatics::GETMODCONFIG(), Key, this);
		if (IsValid(Property) && !mObservedProperties.Contains(Property))
		{
			Property->OnPropertyValueChanged.AddUniqueDynamic(this, &USBSApiSubsystem::OnAccountKeyChanged);
			mObservedProperties.Add(Property);
		}
	}
}

void USBSApiSubsystem::QueryApi(FFilterPostStruct Post)
{
	if (!bInitialized || bChangingConfiguration || IsRunningDedicatedServer())
	{
		mOnQueryDone.Broadcast(mCurrentBlueprints, mTotalBlueprints, false);
		return;
	}
	BindConfiguration();

	CancelRequest(mBlueprintRequest);
	mBlueprintRequest = FSBSRequest::Create(Post.getUrl(this), TEXT("POST"), Post.ToString(), this);
	mBlueprintRequest->mRequest->OnProcessRequestComplete().BindUObject(this, &USBSApiSubsystem::OnBlueprintQueryDone);
	bIsInQuery = true;
	if (!mBlueprintRequest->Start())
	{
		OnBlueprintQueryDone(mBlueprintRequest->mRequest, nullptr, false);
	}
}

void USBSApiSubsystem::QueryPackApi(FPackFilterPostStruct Post)
{
	if (!bInitialized || bChangingConfiguration || IsRunningDedicatedServer())
	{
		mOnPackQueryDone.Broadcast(mCurrentBlueprintPacks, mTotalBlueprintPacks, false);
		return;
	}
	BindConfiguration();
	CancelRequest(mPackRequest);
	mPackRequest = FSBSRequest::Create(Post.getUrl(this), TEXT("POST"), Post.ToString(), this);
	mPackRequest->mRequest->OnProcessRequestComplete().BindUObject(this, &USBSApiSubsystem::OnBlueprintPackQueryDone);
	bIsInPackQuery = true;
	if (!mPackRequest->Start())
	{
		OnBlueprintPackQueryDone(mPackRequest->mRequest, nullptr, false);
	}
}

void USBSApiSubsystem::QueryTags()
{
	FDynamicApiPostStruct Post;
	Post.Indetifier = TEXT("tags");
	QueryApiDynamic(Post);
}

void USBSApiSubsystem::QueryApiDynamic(FDynamicApiPostStruct Post)
{

	const bool bTags = Post.Indetifier.Equals(TEXT("tags"), ESearchCase::IgnoreCase);
	if (!bTags)
	{
		Post.FailedText = NSLOCTEXT("SBS", "Api.UnsupportedRequest", "This SBS request is not supported.").ToString();
		mOnDynamicQueryDone.Broadcast(Post, false);
		return;
	}
	QueueDynamic(MoveTemp(Post), FSBSStatics::MakeUrl(FSBSStatics::API_TAGS, this), TEXT("{}"), true);
}

void USBSApiSubsystem::QueryRating(FRatingPostStruct Post)
{
	if (!FSBSStatics::IsSafeIdentifier(Post.BlueprintID) || Post.Rating < 1 || Post.Rating > 5 || !IsLoggedIn())
	{
		Post.FailedText =
			NSLOCTEXT("SBS", "Api.InvalidRating", "A valid blueprint, rating from 1 to 5 and SBS login are required.")
				.ToString();
		mOnDynamicQueryDone.Broadcast(Post, false);
		return;
	}

	const FString Url = Post.getUrl(this);
	const FString Payload = Post.ToString();
	QueueDynamic(Post, Url, Payload);
}

void USBSApiSubsystem::QueueDynamic(FDynamicApiPostStruct Post, FString Url, FString Payload, bool bTags)
{
	if (!bInitialized || bChangingConfiguration || IsRunningDedicatedServer() || mPendingDynamic.Num() >= 64)
	{
		Post.FailedText =
			NSLOCTEXT("SBS", "Api.QueueUnavailable", "SBS request queue is unavailable or full.").ToString();
		mOnDynamicQueryDone.Broadcast(Post, false);
		return;
	}
	BindConfiguration();
	const TSharedRef<FSBSDynamicRequest> Entry = MakeShared<FSBSDynamicRequest>();
	Entry->mPost = MoveTemp(Post);
	Entry->mUrl = MoveTemp(Url);
	Entry->mPayload = MoveTemp(Payload);
	Entry->bTags = bTags;
	mPendingDynamic.Add(Entry);
	StartNextDynamic();
}

void USBSApiSubsystem::StartNextDynamic()
{
	if (bDispatchingDynamic)
	{
		return;
	}
	TGuardValue<bool> Guard(bDispatchingDynamic, true);
	if (bInitialized && !mDynamicRequest && !mPendingDynamic.IsEmpty())
	{
		const TSharedPtr<FSBSDynamicRequest> Entry = mPendingDynamic[0];
		mPendingDynamic.RemoveAt(0);
		mDynamicQuery = Entry->mPost;
		bDynamicIsTags = Entry->bTags;
		mDynamicRequest = FSBSRequest::Create(Entry->mUrl, TEXT("POST"), Entry->mPayload, this);
		mDynamicRequest->mRequest->OnProcessRequestComplete().BindUObject(this, &USBSApiSubsystem::OnQueryDynamicDone);
		if (!mDynamicRequest->Start())
		{
			OnQueryDynamicDone(mDynamicRequest->mRequest, nullptr, false);
		}
	}
}

void USBSApiSubsystem::ScheduleNextDynamic()
{
	if (!bInitialized || mPendingDynamic.IsEmpty() || mDynamicPumpHandle.IsValid())
	{
		return;
	}
	const TWeakObjectPtr<USBSApiSubsystem> WeakThis(this);
	mDynamicPumpHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
		[WeakThis](float)
		{
			if (USBSApiSubsystem* Subsystem = WeakThis.Get())
			{
				Subsystem->mDynamicPumpHandle.Reset();
				Subsystem->StartNextDynamic();
			}
			return false;
		}));
}

bool USBSApiSubsystem::IsQuery() const { return mDynamicRequest.IsValid() || !mPendingDynamic.IsEmpty(); }
bool USBSApiSubsystem::IsPackQuery() const { return bIsInPackQuery; }
bool USBSApiSubsystem::IsBpQuery() const { return bIsInQuery; }
bool USBSApiSubsystem::IsAuthQuery() const
{
	const USBSSharingSubsystem* Sharing = USBSSharingSubsystem::GetSBSSharingSubsystem(this);
	return bIsInAuthQuery || (Sharing && Sharing->GetSession().State == ESBSLoginState::SigningIn);
}

void USBSApiSubsystem::QueryForAuth()
{
	BindConfiguration();
	CancelRequest(mAuthRequest);
	bIsInAuthQuery = false;
	mUserData = FSBSUserData();
	if (const USBSSharingSubsystem* Sharing = USBSSharingSubsystem::GetSBSSharingSubsystem(this);
		Sharing && Sharing->UsesUserKeyLogin())
	{
		TMap<FString, FString> Headers;
		const bool bAuthenticated = Sharing->ApplyAuthorization(FSBSStatics::MakeUrl(TEXT("auth/me"), this), Headers);
		if (bAuthenticated)
		{
			mUserData = Sharing->GetSession().User;
		}
		mOnAuthUpdated.Broadcast(mUserData, bAuthenticated);
		return;
	}
	if (!bInitialized || IsRunningDedicatedServer() || FSBSStatics::GetAccountKey(this).IsEmpty())
	{
		mOnAuthUpdated.Broadcast(mUserData, false);
		return;
	}
	mAuthRequest = FSBSRequest::Create(mUserData.getUrl(this), TEXT("POST"), TEXT("{}"), this);
	mAuthRequest->mRequest->OnProcessRequestComplete().BindUObject(this, &USBSApiSubsystem::OnQueryAuthDone);
	bIsInAuthQuery = true;
	if (!mAuthRequest->Start())
	{
		OnQueryAuthDone(mAuthRequest->mRequest, nullptr, false);
	}
}

USBSApiSubsystem* USBSApiSubsystem::Get(const UObject* WorldContext)
{
	UWorld* World =
		GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<USBSApiSubsystem>() : nullptr;
}

void USBSApiSubsystem::OnBlueprintQueryDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (!bInitialized || !mBlueprintRequest || mBlueprintRequest->mRequest != Request)
	{
		return;
	}
	const TSharedPtr<FSBSRequest> Completed = MoveTemp(mBlueprintRequest);
	bIsInQuery = false;
	TSharedPtr<FJsonObject> Json;
	const bool bParsed =
		Completed->ReadJson(Response, bSuccess, Json) && ParsePage(Json, mCurrentBlueprints, mTotalBlueprints, this);
	mOnQueryDone.Broadcast(mCurrentBlueprints, mTotalBlueprints, bParsed);
}

void USBSApiSubsystem::OnBlueprintPackQueryDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (!bInitialized || !mPackRequest || mPackRequest->mRequest != Request)
	{
		return;
	}
	const TSharedPtr<FSBSRequest> Completed = MoveTemp(mPackRequest);
	bIsInPackQuery = false;
	TSharedPtr<FJsonObject> Json;
	const bool bParsed = Completed->ReadJson(Response, bSuccess, Json) &&
		ParsePage(Json, mCurrentBlueprintPacks, mTotalBlueprintPacks, this);
	mOnPackQueryDone.Broadcast(mCurrentBlueprintPacks, mTotalBlueprintPacks, bParsed);
}

void USBSApiSubsystem::OnQueryDynamicDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (!bInitialized || !mDynamicRequest || mDynamicRequest->mRequest != Request)
	{
		return;
	}
	const TSharedPtr<FSBSRequest> Completed = MoveTemp(mDynamicRequest);
	FDynamicApiPostStruct Result = MoveTemp(mDynamicQuery);
	mDynamicQuery = FDynamicApiPostStruct();
	TSharedPtr<FJsonObject> Json;
	bool bParsed = Completed->ReadJson(Response, bSuccess, Json);
	if (bParsed && bDynamicIsTags)
	{
		const TArray<TSharedPtr<FJsonValue>>* Tags = nullptr;
		bParsed = Json->TryGetArrayField(TEXT("tags"), Tags) && Tags->Num() <= 512;
		if (bParsed)
		{
			FGetTagsStruct Handler;
			Handler.OnRequestDone(Request, Response, this, Json, this);
		}
	}
	if (!bParsed)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Status"), Response ? Response->GetResponseCode() : 0);
		Result.FailedText = Response
			? FText::Format(
				  NSLOCTEXT("SBS", "Api.RequestFailed", "SBS request failed (HTTP {Status} or invalid response)."),
				  Arguments)
				  .ToString()
			: NSLOCTEXT("SBS", "Api.NetworkError", "Could not reach SBS.").ToString();
	}

	mOnDynamicQueryDone.Broadcast(Result, bParsed);
	ScheduleNextDynamic();
}

void USBSApiSubsystem::OnQueryAuthDone(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	if (!bInitialized || !mAuthRequest || mAuthRequest->mRequest != Request)
	{
		return;
	}
	const TSharedPtr<FSBSRequest> Completed = MoveTemp(mAuthRequest);
	bIsInAuthQuery = false;
	mUserData = FSBSUserData();
	TSharedPtr<FJsonObject> Json;
	FSBSSession Session;
	const bool bParsed =
		Completed->ReadJson(Response, bSuccess, Json) && FSBSSharingProtocol::ParseSession(Json, Session);
	if (bParsed)
	{
		mUserData = MoveTemp(Session.User);
	}
	mOnAuthUpdated.Broadcast(mUserData, bParsed);
}

void USBSApiSubsystem::OnAccountKeyChanged()
{
	if (USBSSharingSubsystem* Sharing = USBSSharingSubsystem::GetSBSSharingSubsystem(this))
	{
		Sharing->OnApiEnvironmentChanged();
	}
	OnSharingSessionChanged();
}

void USBSApiSubsystem::OnSharingSessionChanged()
{
	if (!bInitialized || bChangingConfiguration)
	{
		return;
	}
	{
		TGuardValue<bool> Guard(bChangingConfiguration, true);
		CancelResolveDownloadId();

		TArray<FDynamicApiPostStruct> Cancelled;
		if (mDynamicRequest)
		{
			Cancelled.Add(mDynamicQuery);
		}
		for (const TSharedPtr<FSBSDynamicRequest>& Entry : mPendingDynamic)
		{
			Cancelled.Add(Entry->mPost);
		}
		const bool bHadBlueprintQuery = bIsInQuery;
		const bool bHadPackQuery = bIsInPackQuery;
		CancelRequest(mBlueprintRequest);
		CancelRequest(mPackRequest);
		CancelRequest(mAuthRequest);
		CancelRequest(mDynamicRequest);
		mPendingDynamic.Reset();
		mDynamicQuery = FDynamicApiPostStruct();
		mUserData = FSBSUserData();
		mCurrentBlueprints.Reset();
		mCurrentBlueprintPacks.Reset();
		mTags.Reset();
		mTotalBlueprints = mTotalBlueprintPacks = 0;
		bIsInQuery = bIsInPackQuery = bIsInAuthQuery = false;
		if (bHadBlueprintQuery)
		{
			mOnQueryDone.Broadcast(mCurrentBlueprints, 0, false);
		}
		if (bInitialized && bHadPackQuery)
		{
			mOnPackQueryDone.Broadcast(mCurrentBlueprintPacks, 0, false);
		}
		for (FDynamicApiPostStruct& Post : Cancelled)
		{
			if (!bInitialized)
			{
				break;
			}
			Post.FailedText = NSLOCTEXT("SBS", "Api.AccountChanged",
										"SBS request cancelled because account or API configuration changed.")
								  .ToString();
			mOnDynamicQueryDone.Broadcast(Post, false);
		}
	}
	if (bInitialized)
	{
		QueryForAuth();
	}
}
