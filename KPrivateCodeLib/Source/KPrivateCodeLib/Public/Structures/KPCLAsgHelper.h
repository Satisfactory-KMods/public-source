#pragma once
#include "SessionSettings/SessionSetting.h"
#include "SessionSettings/SessionSettingsManager.h"
#include "Settings/FGAdvancedGameSettings.h"
#include "KPCLAsgHelper.generated.h"

USTRUCT(BlueprintType)
struct FKPCLAsgHelper
{
	GENERATED_USTRUCT_BODY()

public:
	virtual ~FKPCLAsgHelper()
	{
	};

	FKPCLAsgHelper()
	{
	};

	// Init with delegate for listening to option updates
	virtual void Init(UWorld* World, UObject* InObject, const FName& InFunctionName = TEXT("OnOptionsUpdated"))
	{
		mOnOptionsUpdatedDelegate.BindUFunction(InObject, InFunctionName);
		Init(World, mOnOptionsUpdatedDelegate);
	}

	// Init with delegate for listening to option updates
	virtual void Init(UWorld* World, FOptionUpdated& Delegate)
	{
		ListenOnUpdated(World, Delegate);
		Init(World);
	}

	// Init without delegate
	virtual void Init(UWorld* World)
	{
		OnUpdate(World);
	}

	// Should be called when the option is updated by the owning object
	virtual void OnUpdate(UWorld* World, FString UpdatedCVar = FString())
	{
	}

	// init listening to option updates
	virtual void ListenOnUpdated(UWorld* World, FOptionUpdated& Delegate)
	{
		fgcheckf(IsValid(World), TEXT("World is null in FKPCLAsgHelper::ListenOnUpdated -> %s"), *GetKey());
		USessionSettingsManager* SessionSettings = World->GetSubsystem<USessionSettingsManager>();
		SessionSettings->SubscribeToDynamicOptionUpdate(GetKey(), Delegate);
		mOnOptionsUpdatedDelegate = Delegate;
	}

	// Stop listening to option updates
	virtual void StopListenOnUpdated(UWorld* World)
	{
		if (!mOnOptionsUpdatedDelegate.IsBound())
		{
			return;
		}
		fgcheckf(IsValid(World), TEXT("World is null in FKPCLAsgHelper::ListenOnUpdated -> %s"), *GetKey());
		USessionSettingsManager* SessionSettings = World->GetSubsystem<USessionSettingsManager>();
		SessionSettings->UnsubscribeToDynamicOptionUpdate(GetKey(), mOnOptionsUpdatedDelegate);
	}

	FString GetKey() const
	{
		fgcheckf(mSessionSetting, TEXT("SessionSetting is null in FKPCLAsgHelper::GetKey"));
		return mSessionSetting->StrId;
	}

	// Or use mKey to get the value
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USMLSessionSetting* mSessionSetting;

	// Delegate for listening to option updates
	FOptionUpdated mOnOptionsUpdatedDelegate;
};


USTRUCT(BlueprintType)
struct FKPCLAsgHelperBool : public FKPCLAsgHelper
{
	GENERATED_USTRUCT_BODY()

public:
	bool GetValue() const
	{
		return mCachedValue;
	}

	bool GetValue(UWorld* World) const
	{
		fgcheckf(IsValid(World), TEXT("World is null in FKPCLAsgHelperBool::GetValue -> %s"), *GetKey());
		USessionSettingsManager* SessionSettings = World->GetSubsystem<USessionSettingsManager>();
		return SessionSettings->GetBoolOptionValue(GetKey());
	}

	virtual void OnUpdate(UWorld* World, FString UpdatedCVar = FString()) override
	{
		if (!UpdatedCVar.IsEmpty() && UpdatedCVar != GetKey())
		{
			return;
		}
		mCachedValue = GetValue(World);
	}

	UPROPERTY(BlueprintReadOnly)
	bool mCachedValue = false;
};


USTRUCT(BlueprintType)
struct FKPCLAsgHelperInt32 : public FKPCLAsgHelper
{
	GENERATED_USTRUCT_BODY()

public:
	int32 GetValue() const
	{
		return mCachedValue;
	}

	int32 GetValue(UWorld* World) const
	{
		fgcheckf(IsValid(World), TEXT("World is null in FKPCLAsgHelperBool::GetValue -> %s"), *GetKey());
		USessionSettingsManager* SessionSettings = World->GetSubsystem<USessionSettingsManager>();
		return SessionSettings->GetIntOptionValue(GetKey());
	}

	virtual void OnUpdate(UWorld* World, FString UpdatedCVar = FString()) override
	{
		if (!UpdatedCVar.IsEmpty() && UpdatedCVar != GetKey())
		{
			return;
		}
		mCachedValue = GetValue(World);
	}

	UPROPERTY(BlueprintReadOnly)
	int32 mCachedValue = 0;
};


USTRUCT(BlueprintType)
struct FKPCLAsgHelperFloat : public FKPCLAsgHelper
{
	GENERATED_USTRUCT_BODY()

public:
	float GetValue() const
	{
		return mCachedValue;
	}

	float GetValue(UWorld* World) const
	{
		fgcheckf(IsValid(World), TEXT("World is null in FKPCLAsgHelperBool::GetValue -> %s"), *GetKey());
		USessionSettingsManager* SessionSettings = World->GetSubsystem<USessionSettingsManager>();
		return SessionSettings->GetFloatOptionValue(GetKey());
	}

	virtual void OnUpdate(UWorld* World, FString UpdatedCVar = FString()) override
	{
		if (!UpdatedCVar.IsEmpty() && UpdatedCVar != GetKey())
		{
			return;
		}
		mCachedValue = GetValue(World);
	}

	UPROPERTY(BlueprintReadOnly)
	float mCachedValue = 0.f;
};
