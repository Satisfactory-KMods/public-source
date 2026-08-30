

#pragma once

#include "CoreMinimal.h"

#include "KPrivateCodeLibModule.h"
#include "SessionSettings/SessionSetting.h"
#include "SessionSettings/SessionSettingsManager.h"
#include "Settings/FGAdvancedGameSettings.h"

#include "KPCLAsgHelper.generated.h"

#define KPCL_ASG_INIT(Var) (Var).Init((this->GetWorld()), this)
#define KPCL_ASG_INIT_SECONDARY(Var, SecondaryFunc) (Var).Init((this->GetWorld()), this, (SecondaryFunc))
#define KPCL_ASG_DEINIT(Var) (Var).StopListenOnUpdated((this->GetWorld()))
#define KPCL_ASG_DEINIT_SECONDARY(Var) (Var).StopListenOnUpdated((this->GetWorld()))

struct FKPCLAsgHelper;

UCLASS()
class KPRIVATECODELIB_API UKPCLAsgHelperListener : public UObject
{
	GENERATED_BODY()

public:

	FKPCLAsgHelper* mHelper = nullptr;

	TWeakObjectPtr<UWorld> mWorld;

	UFUNCTION()
	void HandleOptionUpdated(FString UpdatedCVar);
};

USTRUCT(BlueprintType)
struct FKPCLAsgHelper
{
	GENERATED_USTRUCT_BODY()

public:
	virtual ~FKPCLAsgHelper() {};

	FKPCLAsgHelper() {};

	virtual void Init(UWorld* World, UObject* InObject)
	{
		if (!mSessionSetting)
		{
			UE_LOG(
				LogTemp, Warning,
				TEXT(
					"FKPCLAsgHelper::Init — mSessionSetting is null on object '%s'. Session setting feature disabled."),
				InObject ? *InObject->GetName() : TEXT("<null>"));
			return;
		}
		mBoundObject = InObject;
		if (!mListener)
		{
			mListener = NewObject<UKPCLAsgHelperListener>(InObject);
		}
		mListener->mHelper = this;
		mListener->mWorld = World;
		mOnOptionsUpdatedDelegate.BindUFunction(mListener, TEXT("HandleOptionUpdated"));
		Init(World, mOnOptionsUpdatedDelegate);
	}

	virtual void Init(UWorld* World, UObject* InObject, const FName& InSecondaryFunctionName)
	{
		mSecondaryFunctionName = InSecondaryFunctionName;
		Init(World, InObject);
	}

	virtual void Init(UWorld* World, FOptionUpdated& Delegate)
	{
		ListenOnUpdated(World, Delegate);
		Init(World);
	}

	virtual void Init(UWorld* World) { OnUpdate(World); }

	virtual void OnUpdate(UWorld* World, FString UpdatedCVar = FString()) { FireSecondary(UpdatedCVar); }

	void FireSecondary(const FString& UpdatedCVar)
	{
		if (mSecondaryFunctionName == NAME_None || !mBoundObject.IsValid())
		{
			return;
		}
		UObject* Object = mBoundObject.Get();
		if (UFunction* Function = Object->FindFunction(mSecondaryFunctionName))
		{
			struct FSecondaryParams
			{
				FString UpdatedCVar;
			} Params{UpdatedCVar};
			Object->ProcessEvent(Function, &Params);
		}
	}

	virtual void ListenOnUpdated(UWorld* World, FOptionUpdated& Delegate)
	{
		const FString Key = GetKey();
		if (!Key.IsEmpty() && IsValid(World))
		{
			if (USessionSettingsManager* SessionSettings = World->GetSubsystem<USessionSettingsManager>())
			{
				SessionSettings->SubscribeToDynamicOptionUpdate(Key, Delegate);
			}
		}
		mOnOptionsUpdatedDelegate = Delegate;
	}

	virtual void StopListenOnUpdated(UWorld* World)
	{
		if (mOnOptionsUpdatedDelegate.IsBound())
		{
			const FString Key = GetKey();
			if (!Key.IsEmpty() && IsValid(World))
			{
				if (USessionSettingsManager* SessionSettings = World->GetSubsystem<USessionSettingsManager>())
				{
					SessionSettings->UnsubscribeToDynamicOptionUpdate(Key, mOnOptionsUpdatedDelegate);
				}
			}
		}
		mSecondaryFunctionName = NAME_None;
		mBoundObject = nullptr;
		if (IsValid(mListener))
		{
			mListener->mHelper = nullptr;
			mListener->mWorld = nullptr;
			mListener->MarkAsGarbage();
			mListener = nullptr;
		}
	}

	FString GetKey() const
	{
		if (!IsValid(mSessionSetting.Get()))
		{
			return FString();
		}
		return mSessionSetting->StrId;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<USMLSessionSetting> mSessionSetting;

	FOptionUpdated mOnOptionsUpdatedDelegate;

	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> mBoundObject;
	FName mSecondaryFunctionName = NAME_None;

	UPROPERTY(Transient)
	TObjectPtr<UKPCLAsgHelperListener> mListener;
};

USTRUCT(BlueprintType)
struct FKPCLAsgHelperBool : public FKPCLAsgHelper
{
	GENERATED_USTRUCT_BODY()

public:
	bool GetValue() const { return mCachedValue; }

	bool GetValue(UWorld* World) const
	{
		const FString Key = GetKey();
		if (Key.IsEmpty() || !IsValid(World))
		{
			return mCachedValue;
		}
		if (USessionSettingsManager* SessionSettings = World->GetSubsystem<USessionSettingsManager>())
		{
			return SessionSettings->GetBoolOptionValue(Key);
		}
		return mCachedValue;
	}

	virtual void OnUpdate(UWorld* World, FString UpdatedCVar = FString()) override
	{
		if (!UpdatedCVar.IsEmpty() && UpdatedCVar != GetKey())
		{
			return;
		}
		mCachedValue = GetValue(World);
		FireSecondary(UpdatedCVar);
	}

	UPROPERTY(BlueprintReadOnly)
	bool mCachedValue = false;
};

USTRUCT(BlueprintType)
struct FKPCLAsgHelperInt32 : public FKPCLAsgHelper
{
	GENERATED_USTRUCT_BODY()

public:
	int32 GetValue() const { return mCachedValue; }

	int32 GetValue(UWorld* World) const
	{
		const FString Key = GetKey();
		if (Key.IsEmpty() || !IsValid(World))
		{
			return mCachedValue;
		}
		if (USessionSettingsManager* SessionSettings = World->GetSubsystem<USessionSettingsManager>())
		{
			return SessionSettings->GetIntOptionValue(Key);
		}
		return mCachedValue;
	}

	virtual void OnUpdate(UWorld* World, FString UpdatedCVar = FString()) override
	{
		if (!UpdatedCVar.IsEmpty() && UpdatedCVar != GetKey())
		{
			return;
		}
		mCachedValue = GetValue(World);
		FireSecondary(UpdatedCVar);
	}

	UPROPERTY(BlueprintReadOnly)
	int32 mCachedValue = 0;
};

USTRUCT(BlueprintType)
struct FKPCLAsgHelperFloat : public FKPCLAsgHelper
{
	GENERATED_USTRUCT_BODY()

public:
	float GetValue() const { return mCachedValue; }

	float GetValue(UWorld* World) const
	{
		const FString Key = GetKey();
		if (Key.IsEmpty() || !IsValid(World))
		{
			return mCachedValue;
		}
		if (USessionSettingsManager* SessionSettings = World->GetSubsystem<USessionSettingsManager>())
		{
			return SessionSettings->GetFloatOptionValue(Key);
		}
		return mCachedValue;
	}

	virtual void OnUpdate(UWorld* World, FString UpdatedCVar = FString()) override
	{
		if (!UpdatedCVar.IsEmpty() && UpdatedCVar != GetKey())
		{
			return;
		}
		mCachedValue = GetValue(World);
		FireSecondary(UpdatedCVar);
	}

	UPROPERTY(BlueprintReadOnly)
	float mCachedValue = 0.f;
};
