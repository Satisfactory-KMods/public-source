// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Network/KPCLNetworkCoreHologram.h"

#include "KPrivateCodeLibModule.h"
#include "Logging/StructuredLog.h"
#include "Subsystem/KPCLUnlockSubsystem.h"

void AKPCLNetworkCoreHologram::BeginPlay()
{
	Super::BeginPlay();

	mFaxitSubsystem = AKPCLFaxitSubsystem::Get(GetWorld());
}

void AKPCLNetworkCoreHologram::CheckValidPlacement()
{
	Super::CheckValidPlacement();

	// Lazy-fetch the subsystem if it wasn't available during BeginPlay.
	if (!IsValid(mFaxitSubsystem))
	{
		mFaxitSubsystem = AKPCLFaxitSubsystem::Get(GetWorld());
	}

	if (!IsValid(mFaxitSubsystem))
	{
		UE_LOGFMT(LogKPCL, Error, "KPCLNetworkCoreHologram cannot find FaxitSubsystem on {0}", GetName());
		return;
	}

	// Update the disqualifier text to show the current limit.
	UKPCLCDMaxCountReached::UpdateCount(mFaxitSubsystem->GetNetworkLimit());

	// Fixed: the old else-branch (subsystem refetch path) unconditionally disqualified
	// without checking the count, so the first placement after a late subsystem fetch
	// was always blocked even when under the limit. Now the check is unified.
	if (mFaxitSubsystem->GetNetworkCount() >= mFaxitSubsystem->GetNetworkLimit())
	{
		AddConstructDisqualifier(UKPCLCDMaxCountReached::StaticClass());
	}
}
