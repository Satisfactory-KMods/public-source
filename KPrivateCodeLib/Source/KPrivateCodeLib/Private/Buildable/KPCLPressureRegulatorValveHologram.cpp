// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Buildable/KPCLPressureRegulatorValveHologram.h"

bool AKPCLPressureRegulatorValveHologram::TrySnapToActor(const FHitResult& HitResult)
{
	bLastSnapRejectedDueToVertical = false;

	if (!Super::TrySnapToActor(HitResult))
	{
		return false;
	}

	if (FMath::Abs(mSnappedConnectionComponentForwardVector.Z) > mMaxVerticalComponent)
	{
		bLastSnapRejectedDueToVertical = true;

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
