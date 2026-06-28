// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "BFL/KBFL_ConfigTools.h"
#include "Components/AudioComponent.h"
#include "Configuration/ModConfiguration.h"

#include "KPCLFunctionalStructure.generated.h"

UENUM(BlueprintType)
enum class EFound : uint8
{
	Yes UMETA(DisplayName = "Found"),
	No UMETA(DisplayName = "Not Found"),
};

USTRUCT(BlueprintType)
struct KPRIVATECODELIB_API FPowerOptions
{
	GENERATED_BODY()

	FPowerOptions() {}

	~FPowerOptions() {}

	FPowerOptions(float Consume) { mNormalPowerConsume = Consume; }

	FPowerOptions(UCurveFloat* Curve, float VariablePower, float VariablePowerTime)
	{
		mPowerCurve = Curve;
		mMaxVariablePowerValue = VariablePower;
		mVariablePowerTime = VariablePowerTime;
	}

	FPowerOptions(float Consume, UCurveFloat* Curve, float VariablePower, float VariablePowerTime)
	{
		mNormalPowerConsume = Consume;
		mPowerCurve = Curve;
		mMaxVariablePowerValue = VariablePower;
		mVariablePowerTime = VariablePowerTime;
	}

	UPROPERTY(EditAnywhere, NotReplicated)
	bool bUseExponent = true;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1.0", ClampMax = 2))
	float mPowerConsumptionExponent = 1.321929;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	float mCurrentPotential = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool mIsProducer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = mIsProducer, EditConditionHides))
	bool mIsDynamicProducer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = mIsProducer, EditConditionHides))
	float mForcePowerConsume = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float mNormalPowerConsume = 0.0f;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	float mOtherPowerConsume = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, NotReplicated)
	TObjectPtr<UCurveFloat> mPowerCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float mVariablePowerTime = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, NotReplicated)
	float mMaxVariablePowerValue = 0.0f;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadOnly)
	float mPowerMultiplier = 1.0f;

	UPROPERTY(SaveGame, NotReplicated)
	float mCurrentPowerCurvePercent = 0.0f;

	UPROPERTY(SaveGame, NotReplicated)
	float mVariableTimer = 0.0f;

	bool bWasInit = false;
	bool bIsRunning = false;

	UPROPERTY(BlueprintReadOnly)
	bool bHasPower = false;

	bool IsValid() const;

	void Init();

	void MergePowerOptions(FPowerOptions OtherOption);

	void OverWritePowerOptions(FPowerOptions OtherOption);

	void StructureTick(float dt, bool IsConsuming = true);

	float GetMaxPowerConsume() const;

	float GetPowerConsume() const;

	float GetCurrentVariablePower() const;

	bool IsPowerVariable() const;
};

USTRUCT(BlueprintType)
struct KPRIVATECODELIB_API FFullProductionHandle
{
	GENERATED_BODY()

	FFullProductionHandle() {}

	FFullProductionHandle(float ProductionTime) { mProductionTime = ProductionTime; }

	UPROPERTY(SaveGame, BlueprintReadWrite)
	float mCurrentTime = 0.f;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	float mProductionTime = 5.f;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	float mProductivity = 0.f;

	UPROPERTY(SaveGame)
	float mCurrentPotential = 1.0f;

	UPROPERTY(SaveGame)
	float mPendingPotential = 1.0f;

	UPROPERTY(BlueprintReadWrite, NotReplicated)
	float mProductivityTime = 0.f;

	UPROPERTY(BlueprintReadWrite)
	float mCurrentProductionTime = 0.f;

	/** Return True if Production FINIAL (Timer end) */
	void TickHandle(float dt, bool IsProducing, TFunction<void()> Callback);

	/** Productivity in the last 5 minutes
	 * \n\n
	 * Go up if IsProducing\n
	 * Go down if !IsProducing
	 */
	void TickProductivity(float dt, bool IsProducing);

	void SetNewTime(float NewTime, bool ShouldReset = true);

	bool ShouldDo() const;

	float GetProductionTime() const;

	float GetPendingProductionTime() const;

	void Reset();
};

USTRUCT(BlueprintType)
struct KPRIVATECODELIB_API FSmartTimer
{
	GENERATED_BODY()

	FSmartTimer();

	FSmartTimer(float Time);

	FSmartTimer(float Time, bool Active);

	UPROPERTY(EditAnywhere)
	float mTime = 5.0f;

	UPROPERTY(EditAnywhere)
	bool mIsActive = true;

	UPROPERTY(SaveGame)
	float mTimer = 0;

	bool Tick(float dt);

	void Reset();
};

USTRUCT(BlueprintType)
struct KPRIVATECODELIB_API FKPCLModConfigHelper
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UModConfiguration> mModConfig = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString mModConfigSection = FString();

	bool IsValid() const;

	UConfigProperty* GetProperty(UObject* Context);

private:
	UPROPERTY()
	TObjectPtr<UConfigProperty> mCachedProperty = nullptr;

protected:
	template <class T>
	T* GetConfigProperty(UObject* Context);
};

template <class T>
T* FKPCLModConfigHelper::GetConfigProperty(UObject* Context)
{
	if (mCachedProperty)
	{
		return Cast<T>(mCachedProperty);
	}

	if (IsValid() && mCachedProperty == nullptr)
	{
		mCachedProperty = UKBFL_ConfigTools::GetConfigPropertyByKey(mModConfig, mModConfigSection, Context);
	}

	return Cast<T>(mCachedProperty);
}

USTRUCT(BlueprintType)
struct KPRIVATECODELIB_API FKPCLModConfigHelper_Float : public FKPCLModConfigHelper
{
	GENERATED_BODY()

	float GetValue(UObject* Context);

	UConfigPropertyFloat* GetPropertyAsType(UObject* Context);

private:
	UPROPERTY(EditAnywhere)
	float mDefaultValue = 1.0f;

	UPROPERTY(EditAnywhere)
	bool mUseAsPercent = false;
};

USTRUCT(BlueprintType)
struct KPRIVATECODELIB_API FKPCLModConfigHelper_Bool : public FKPCLModConfigHelper
{
	GENERATED_BODY()

	bool GetValue(UObject* Context);

	UConfigPropertyBool* GetPropertyAsType(UObject* Context);

private:
	UPROPERTY(EditAnywhere)
	bool mDefaultValue = 1.0f;
};

USTRUCT(BlueprintType)
struct KPRIVATECODELIB_API FKPCLAudioComponent
{
	GENERATED_BODY()

	FKPCLAudioComponent() {};

	FKPCLAudioComponent(UAudioComponent* Component)
	{
		mComponent = Component;
		if (mComponent)
		{
			mCachedVolume = mComponent->VolumeMultiplier;
		}
	}

	bool IsValid() const;

	void SetVolumePercent(float Percent) const;

private:
	UPROPERTY()
	TObjectPtr<UAudioComponent> mComponent = nullptr;
	float mCachedVolume = 1.0f;
};
