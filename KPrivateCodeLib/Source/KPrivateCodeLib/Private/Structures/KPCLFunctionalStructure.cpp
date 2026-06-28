#include "Structures/KPCLFunctionalStructure.h"

#include "Configuration/Properties/ConfigPropertyBool.h"
#include "Configuration/Properties/ConfigPropertyFloat.h"

bool FPowerOptions::IsValid() const
{
	return !IsPowerVariable() && (mNormalPowerConsume <= 0.0f || mOtherPowerConsume <= 0.0f);
}

void FPowerOptions::Init() { bWasInit = true; }

void FPowerOptions::MergePowerOptions(FPowerOptions OtherOption)
{
	// Apply curve if other Options is variable
	if (OtherOption.IsPowerVariable() && !IsPowerVariable())
	{
		mPowerCurve = OtherOption.mPowerCurve;
	}

	mNormalPowerConsume += OtherOption.mNormalPowerConsume;
	mOtherPowerConsume += OtherOption.mOtherPowerConsume;
	mMaxVariablePowerValue += OtherOption.mMaxVariablePowerValue;
}

void FPowerOptions::OverWritePowerOptions(FPowerOptions OtherOption)
{
	mPowerCurve = OtherOption.mPowerCurve;
	mNormalPowerConsume = OtherOption.mNormalPowerConsume;
	mOtherPowerConsume = OtherOption.mOtherPowerConsume;
	mMaxVariablePowerValue = OtherOption.mMaxVariablePowerValue;
	mVariablePowerTime = OtherOption.mVariablePowerTime;
	mVariableTimer = 0.0f;
	mCurrentPowerCurvePercent = 0.0f;
}

void FPowerOptions::StructureTick(float dt, bool IsConsuming)
{
	bIsRunning = IsConsuming;
	if (bIsRunning)
	{
		mVariableTimer += dt;
		if (mVariableTimer >= mVariablePowerTime)
		{
			mVariableTimer -= mVariablePowerTime;
		}
		mCurrentPowerCurvePercent = mVariableTimer / mVariablePowerTime;
	}
}

float FPowerOptions::GetMaxPowerConsume() const
{
	float ReturnPowerConsume = mNormalPowerConsume + mOtherPowerConsume;
	if (IsPowerVariable())
	{
		ReturnPowerConsume += mMaxVariablePowerValue;
	}

	if (!bUseExponent)
	{
		return ReturnPowerConsume * mPowerMultiplier;
	}

	return ReturnPowerConsume * mPowerMultiplier * FMath::Pow(mCurrentPotential, mPowerConsumptionExponent);
}

float FPowerOptions::GetPowerConsume() const
{
	if (!bIsRunning)
	{
		return .1f;
	}

	float ReturnPowerConsume = mNormalPowerConsume + mOtherPowerConsume;
	if (IsPowerVariable())
	{
		ReturnPowerConsume += GetCurrentVariablePower();
	}

	if (!bUseExponent)
	{
		return ReturnPowerConsume * mPowerMultiplier;
	}

	return ReturnPowerConsume * mPowerMultiplier * FMath::Pow(mCurrentPotential, mPowerConsumptionExponent);
}

float FPowerOptions::GetCurrentVariablePower() const
{
	if (IsPowerVariable())
	{
		return mMaxVariablePowerValue * mPowerCurve->GetFloatValue(mCurrentPowerCurvePercent);
	}
	return 0.0f;
}

bool FPowerOptions::IsPowerVariable() const { return mPowerCurve != nullptr && mMaxVariablePowerValue > 0.0f; }

void FFullProductionHandle::TickHandle(float dt, bool IsProducing, TFunction<void()> Callback)
{
	TickProductivity(dt, IsProducing);
	mCurrentProductionTime = GetProductionTime();
	if (ShouldDo() && IsProducing)
	{
		mCurrentTime += dt;

		int32 ProductionCycles = FMath::FloorToInt(mCurrentTime / mCurrentProductionTime);

		for (int32 i = 0; i < ProductionCycles; i++)
		{
			Callback();
			mCurrentTime -= mCurrentProductionTime;
			mCurrentPotential = mPendingPotential;
		}
	}
}

void FFullProductionHandle::TickProductivity(float dt, bool IsProducing)
{
	mProductivityTime += dt;
	if (mProductivityTime >= .1f)
	{
		mProductivityTime -= .1f;
		if (IsProducing)
		{
			mProductivity = FMath::Clamp(mProductivity + 0.333f, 0.f, 100.f);
		}
		else
		{
			mProductivity = FMath::Clamp(mProductivity - 0.333f, 0.f, 100.f);
		}
	}
}

void FFullProductionHandle::SetNewTime(float NewTime, bool ShouldReset)
{
	mProductionTime = NewTime;
	mCurrentProductionTime = GetProductionTime();
	if (ShouldReset)
	{
		Reset();
	}
}

bool FFullProductionHandle::ShouldDo() const { return mProductionTime > 0; }

float FFullProductionHandle::GetProductionTime() const
{
	// Guard against division by zero: mCurrentPotential can be 0 on a freshly placed building
	// before the SaveGame float is initialised. Fall back to 1x speed so the handle always advances.
	return mCurrentPotential > 0.f ? mProductionTime / mCurrentPotential : mProductionTime;
}

float FFullProductionHandle::GetPendingProductionTime() const
{
	return mPendingPotential > 0.f ? mProductionTime / mPendingPotential : mProductionTime;
}

void FFullProductionHandle::Reset()
{
	mCurrentTime = 0.0f;
	mProductivity = 0.0f;
	mCurrentProductionTime = GetProductionTime();
}

FSmartTimer::FSmartTimer() {}

FSmartTimer::FSmartTimer(float Time)
{
	mTime = Time;
	mIsActive = Time > 0.0f;
}

FSmartTimer::FSmartTimer(float Time, bool Active)
{
	mTime = Time;
	mIsActive = Active && Time > 0.0f;
}

bool FSmartTimer::Tick(float dt)
{
	if (mIsActive)
	{
		mTimer += dt;
		if (mTimer >= mTime)
		{
			mTimer -= mTime;
			return true;
		}
	}
	return false;
}

void FSmartTimer::Reset() { mTimer = 0.0f; }

bool FKPCLModConfigHelper::IsValid() const { return mModConfig != nullptr && !mModConfigSection.IsEmpty(); }

UConfigProperty* FKPCLModConfigHelper::GetProperty(UObject* Context)
{
	return GetConfigProperty<UConfigProperty>(Context);
}

float FKPCLModConfigHelper_Float::GetValue(UObject* Context)
{
	if (const UConfigPropertyFloat* Property = GetConfigProperty<UConfigPropertyFloat>(Context))
	{
		if (mUseAsPercent)
		{
			return Property->Value / 100;
		}
		return Property->Value;
	}

	if (mUseAsPercent)
	{
		return mDefaultValue / 100;
	}
	return mDefaultValue;
}

UConfigPropertyFloat* FKPCLModConfigHelper_Float::GetPropertyAsType(UObject* Context)
{
	return GetConfigProperty<UConfigPropertyFloat>(Context);
}

bool FKPCLModConfigHelper_Bool::GetValue(UObject* Context)
{
	if (const UConfigPropertyBool* Property = GetConfigProperty<UConfigPropertyBool>(Context))
	{
		return Property->Value;
	}
	return mDefaultValue;
}

UConfigPropertyBool* FKPCLModConfigHelper_Bool::GetPropertyAsType(UObject* Context)
{
	return GetConfigProperty<UConfigPropertyBool>(Context);
}

bool FKPCLAudioComponent::IsValid() const { return mComponent != nullptr; }

void FKPCLAudioComponent::SetVolumePercent(float Percent) const
{
	if (IsValid())
	{
		mComponent->SetVolumeMultiplier(mCachedVolume * Percent);
	}
}
