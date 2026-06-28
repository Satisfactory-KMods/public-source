// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "KPrivateCodeLibModule.h"
#include "SessionSettings/SessionSetting.h"
#include "SessionSettings/SessionSettingsManager.h"
#include "Settings/FGAdvancedGameSettings.h"

#include "KPCLAsgHelper.generated.h"

/**
 * Convenience macros for wiring an FKPCLAsgHelper member to its owner.
 *
 * Use the *_INIT macros in BeginPlay / Init and the *_DEINIT macros in EndPlay.
 * Binding is fully automatic: the helper caches the new value on every update by
 * itself, so the owner does NOT need an OnOptionsUpdated UFUNCTION or any manual
 * Var.OnUpdate(...) dispatch.
 *
 * The *_SECONDARY variant additionally invokes a UFUNCTION on the owner (same
 * void(FString) signature, e.g. "OnOptionsUpdated") every time the helper
 * updates. That secondary UFUNCTION must NOT call Var.OnUpdate itself, or it
 * would recurse.
 */
#define KPCL_ASG_INIT(Var) (Var).Init((this->GetWorld()), this)
#define KPCL_ASG_INIT_SECONDARY(Var, SecondaryFunc) (Var).Init((this->GetWorld()), this, (SecondaryFunc))
#define KPCL_ASG_DEINIT(Var) (Var).StopListenOnUpdated((this->GetWorld()))
#define KPCL_ASG_DEINIT_SECONDARY(Var) (Var).StopListenOnUpdated((this->GetWorld()))

struct FKPCLAsgHelper;

/**
 * Internal UObject proxy that receives the dynamic FOptionUpdated callback and
 * forwards it to its owning FKPCLAsgHelper. Required because a USTRUCT cannot
 * host the UFUNCTION that SubscribeToDynamicOptionUpdate needs.
 */
UCLASS()
class KPRIVATECODELIB_API UKPCLAsgHelperListener : public UObject
{
	GENERATED_BODY()

public:
	/** Non-owning back-pointer to the helper. Helper must live in a stable location (not a relocating container). */
	FKPCLAsgHelper* mHelper = nullptr;

	/** World captured at Init, used to read the updated option value. */
	TWeakObjectPtr<UWorld> mWorld;

	/** Bound to the dynamic option-update delegate; forwards to the helper. */
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

	/** Init + auto-bind: caches the value on every option update by itself, no owner UFUNCTION required. */
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

	/** Same as Init(World, InObject) but also calls InSecondaryFunctionName (void(FString)) on InObject per update. */
	virtual void Init(UWorld* World, UObject* InObject, const FName& InSecondaryFunctionName)
	{
		mSecondaryFunctionName = InSecondaryFunctionName;
		Init(World, InObject);
	}

	/** Init with delegate for listening to option updates. */
	virtual void Init(UWorld* World, FOptionUpdated& Delegate)
	{
		ListenOnUpdated(World, Delegate);
		Init(World);
	}

	/** Init without delegate. */
	virtual void Init(UWorld* World) { OnUpdate(World); }

	/** Called when the option is updated by the owning object. */
	virtual void OnUpdate(UWorld* World, FString UpdatedCVar = FString()) { FireSecondary(UpdatedCVar); }

	/** Invoke the optional secondary UFUNCTION on the bound object. */
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

	/** Init listening to option updates. */
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

	/** Stop listening to option updates. */
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

	/** Returns the setting's StrId key, or an empty string if mSessionSetting is not set. */
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

	/** Owner + UFUNCTION name fired in addition to the auto value-cache on every update (optional). */
	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> mBoundObject;
	FName mSecondaryFunctionName = NAME_None;

	/** Proxy that receives the dynamic delegate; kept alive as a UPROPERTY so it is not GC'd. */
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
