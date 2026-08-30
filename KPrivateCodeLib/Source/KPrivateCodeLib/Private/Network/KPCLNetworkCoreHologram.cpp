

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

	if (!IsValid(mFaxitSubsystem))
	{
		mFaxitSubsystem = AKPCLFaxitSubsystem::Get(GetWorld());
	}

	if (!IsValid(mFaxitSubsystem))
	{
		UE_LOGFMT(LogKPCL, Error, "KPCLNetworkCoreHologram cannot find FaxitSubsystem on {0}", GetName());
		return;
	}

	UKPCLCDMaxCountReached::UpdateCount(mFaxitSubsystem->GetNetworkLimit());

	if (mFaxitSubsystem->GetNetworkCount() >= mFaxitSubsystem->GetNetworkLimit())
	{
		AddConstructDisqualifier(UKPCLCDMaxCountReached::StaticClass());
	}
}
