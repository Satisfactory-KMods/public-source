#include "Components/KPCLBetterIndicator.h"

UKPCLBetterIndicator::UKPCLBetterIndicator()
{
	mCustomExtraData = {3.0f, .0f, 0.473958f, 0.026989f};
}

float UKPCLBetterIndicator::GetEmissive() const
{
	if (mEmissiveIntensityOverwrite < 0.0f)
	{
		return mEmissiveIntensity;
	}
	return mEmissiveIntensityOverwrite;
}

void UKPCLBetterIndicator::SetState(ENewProductionState NewState, bool MarkStateDirty)
{
	if (mCurrentState != NewState)
	{
		mCurrentState = NewState;
		if (IsInGameThread())
		{
			OnIndicatorStateChanged.Broadcast(mCurrentState);
		}
		else
		{
			AsyncTask(ENamedThreads::GameThread, [&]()
			{
				OnIndicatorStateChanged.Broadcast(mCurrentState);
			});
		}
	}
	ApplyNewColorData(FKPCLColorData(0, GetEmissive()), MarkStateDirty);
	ApplyNewLinearColorData(FKPCLLinearColorData(1, GetCurrentColor()), MarkStateDirty);
	ApplyNewColorData(FKPCLColorData(4, mPulsStates.Contains(NewState)), MarkStateDirty);
}

void UKPCLBetterIndicator::SetEmissiveOverwrite(float NewIntensity, bool MarkStateDirty)
{
	if (mEmissiveIntensityOverwrite != NewIntensity)
	{
		mEmissiveIntensityOverwrite = NewIntensity;
		if (MarkStateDirty)
		{
			ApplyNewData();
		}
	}
}

FLinearColor UKPCLBetterIndicator::GetColorByState(ENewProductionState State) const
{
	return mStateColors[static_cast<uint8>(State)];
}

FLinearColor UKPCLBetterIndicator::GetCurrentColor() const
{
	return GetColorByState(mCurrentState);
}

ENewProductionState UKPCLBetterIndicator::GetState() const
{
	return mCurrentState;
}
