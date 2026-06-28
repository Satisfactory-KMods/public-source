// ILikeBanas

#include "Buildable/KPCLPressureRegulatorValveHologram.h"

bool AKPCLPressureRegulatorValveHologram::TrySnapToActor(const FHitResult& HitResult)
{
	bLastSnapRejectedDueToVertical = false;

	if (!Super::TrySnapToActor(HitResult))
	{
		return false;
	}

	// Reject the snap if the matched pipe connection is too vertical.
	// mSnappedConnectionComponentForwardVector is set by the parent's TrySnapToActor to the
	// world-space forward vector of the pipe connection that was selected for snapping.
	// A horizontal connection has |Z| near 0; a vertical one has |Z| near 1.
	if (FMath::Abs(mSnappedConnectionComponentForwardVector.Z) > mMaxVerticalComponent)
	{
		bLastSnapRejectedDueToVertical = true;

		// Clear all snap state so SetHologramLocationAndRotation treats this as unsnapped.
		mSnappedPipe = nullptr;
		mSnappedConnectionComponent = nullptr;
		mSnappedConnectionComponentForwardVector = FVector::ZeroVector;
		mSnapConnectionIndex = 0;
		mSnappedPipeOffset = 0.f;
		return false;
	}

	return true;
}

void AKPCLPressureRegulatorValveHologram::CheckValidPlacement()
{
	Super::CheckValidPlacement();

	if (bLastSnapRejectedDueToVertical)
	{
		AddConstructDisqualifier(UKPCLCDVerticalPipeConnection::StaticClass());
	}
}
