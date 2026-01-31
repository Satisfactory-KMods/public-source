// Fill out your copyright notice in the Description page of Project Settings.
#include "Subsystem/KPCLPatreonSubsystem.h"

#include "HttpModule.h"
#include "BFL/KBFL_ConfigTools.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

void UKPCLPatreonSubsystem::Tick(float DeltaTime)
{
	if (bOldState != bPatreonIsActive)
	{
		OnPatreonBenefitHasChanged.Broadcast(bPatreonIsActive);
		bOldState = bPatreonIsActive;
	}

	if (!bValid)
	{
		Query();
		if (!mConfig) UE_LOG(LogTemp, Error, TEXT("mConfig is INVALID!"))

		if (mConfig && !bValid)
		{
			UConfigPropertyBool* PropertyBool = UKBFL_ConfigTools::GetPropertyByKey<UConfigPropertyBool>(
				mConfig, "PatreonList", GetWorld());
			if (IsValid(PropertyBool))
			{
				UE_LOG(LogTemp, Warning, TEXT("Bind: PatreonList"))
				PropertyBool->OnPropertyValueChanged.AddUniqueDynamic(this, &UKPCLPatreonSubsystem::OnSettingChanged);
				bActivePatreonList = PropertyBool->Value;
			}

			PropertyBool = UKBFL_ConfigTools::GetPropertyByKey<UConfigPropertyBool>(
				mConfig, "PatreonButton", GetWorld());
			if (IsValid(PropertyBool))
			{
				UE_LOG(LogTemp, Warning, TEXT("Bind: PatreonButton"))
				PropertyBool->OnPropertyValueChanged.AddUniqueDynamic(this, &UKPCLPatreonSubsystem::OnSettingChanged);
				bActivePatreonButton = PropertyBool->Value;
			}

			PropertyBool = UKBFL_ConfigTools::GetPropertyByKey<UConfigPropertyBool>(mConfig, "NewsFeed", GetWorld());
			if (IsValid(PropertyBool))
			{
				UE_LOG(LogTemp, Warning, TEXT("Bind: NewsFeed"))
				PropertyBool->OnPropertyValueChanged.AddUniqueDynamic(this, &UKPCLPatreonSubsystem::OnSettingChanged);
				bActiveNewsFeed = PropertyBool->Value;
			}

			bValid = true;

			if (OnSettingsUpdated.IsBound())
			{
				OnSettingsUpdated.Broadcast();
			}
		}
	}
}

void UKPCLPatreonSubsystem::OnSettingChanged()
{
	if (mConfig)
	{
		UConfigPropertyBool* PropertyBool = UKBFL_ConfigTools::GetPropertyByKey<UConfigPropertyBool>(
			mConfig, "PatreonList", GetWorld());
		if (IsValid(PropertyBool))
		{
			bActivePatreonList = PropertyBool->Value;
		}

		PropertyBool = UKBFL_ConfigTools::GetPropertyByKey<UConfigPropertyBool>(mConfig, "PatreonButton", GetWorld());
		if (IsValid(PropertyBool))
		{
			bActivePatreonButton = PropertyBool->Value;
		}

		PropertyBool = UKBFL_ConfigTools::GetPropertyByKey<UConfigPropertyBool>(mConfig, "PatreonList", GetWorld());
		if (IsValid(PropertyBool))
		{
			bActiveNewsFeed = PropertyBool->Value;
		}

		if (OnSettingsUpdated.IsBound())
		{
			OnSettingsUpdated.Broadcast();
		}
	}
}

bool UKPCLPatreonSubsystem::IsTickable() const
{
	return !HasAnyFlags(RF_ClassDefaultObject) && IsValid(this);
}

UWorld* UKPCLPatreonSubsystem::GetTickableGameObjectWorld() const
{
	return nullptr;
}

TStatId UKPCLPatreonSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKssSaveManager, STATGROUP_Tickables);
}

UWorld* UKPCLPatreonSubsystem::GetWorld() const
{
	check(GetGameInstance());

	// If we are a CDO, we must return nullptr instead to fool UObject::ImplementsGetWorld.
	if (HasAllFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	return GetGameInstance()->GetWorld();
}

UKPCLPatreonSubsystem::UKPCLPatreonSubsystem()
{
	mConfig = LoadClass<UModConfiguration>(nullptr, TEXT("/KPrivateCodeLib/KPCL_Config.KPCL_Config_C"));
}

UKPCLPatreonSubsystem* UKPCLPatreonSubsystem::Get(UObject* WorldContext)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		return UGameInstance::GetSubsystem<UKPCLPatreonSubsystem>(World->GetGameInstance());
	}
	return nullptr;
}

void UKPCLPatreonSubsystem::Query()
{
	if (!mConfig)
	{
		mConfig = LoadClass<UModConfiguration>(nullptr, TEXT("/KPrivateCodeLib/KPCL_Config.KPCL_Config_C"));
	}

	if (!bActivePatreonList)
	{
		return;
	}

	if (mConfig)
	{
		mCode = UKBFL_ConfigTools::GetStringFromConfig(mConfig, "UserID", GetWorld());
		mDiscordID = UKBFL_ConfigTools::GetStringFromConfig(mConfig, "PatreonCode", GetWorld());

		if (!mCode.IsEmpty() && !mDiscordID.IsEmpty())
		{
			FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
			Request->OnProcessRequestComplete().BindLambda(
				[&](FHttpRequestPtr InRequest, FHttpResponsePtr Response, bool bSuccess)
				{
					UE_LOG(LogTemp, Error, TEXT("OnProcessRequestComplete"));
					if (bSuccess)
					{
						if (ParseApiQuery(Response, mJsonObject))
						{
							if (!mJsonObject->TryGetBoolField("HasBenefit", bPatreonIsActive))
							{
								bPatreonIsActive = false;
							}
							UE_LOG(LogTemp, Error, TEXT("%d"), bPatreonIsActive);
						}
					}
				});

			QueryApi(Request, "HasBenefits", {mCode, mDiscordID});
		}
		UE_LOG(LogTemp, Error, TEXT("%s / %s"), *mCode, *mDiscordID);
	}
}

void UKPCLPatreonSubsystem::QueryApi(FHttpRequestRef& Request, FString QueryName, TArray<FString> Parameter,
                                     bool Execute, bool IsPost)
{
	FString Url = TEXT("https://kmods.de/");

	Url.Append(QueryName);
	if (!Url.EndsWith("/"))
	{
		Url.Append("/");
	}

	for (FString Element : Parameter)
	{
		Url.Append(Element).Append("/");
		if (!Url.EndsWith("/"))
		{
			Url.Append("/");
		}
	}

	UE_LOG(LogTemp, Error, TEXT("QueryApi %s"), *Url);

	Request->SetURL(Url);
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Accepts"), TEXT("application/json"));

	Request->SetVerb(IsPost ? "POST" : "GET");

	if (Execute)
	{
		Request->ProcessRequest();
	}
}

bool UKPCLPatreonSubsystem::ParseApiQuery(FHttpResponsePtr Response, TSharedPtr<FJsonObject>& Json)
{
	bool Success = false;
	if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response->GetContentAsString()), Json))
	{
		Json->TryGetBoolField("state", Success);
	}
	return Success;
}

void UKPCLPatreonSubsystem::StartReQueryPatreon(UObject* WorldContext)
{
	if (UKPCLPatreonSubsystem* Subsystem = Get(WorldContext))
	{
		Subsystem->Query();
	}
}
