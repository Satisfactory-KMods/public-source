#include "Unlocks/KPCLFaxitSpeedUnlock.h"

#include "Subsystem/KPCLFaxitSubsystem.h"

void UKPCLFaxitSpeedUnlock::ApplyToFaxit(AKPCLFaxitSubsystem* Subsystem) const
{
	if (!IsValid(Subsystem))
	{
		return;
	}

	if (bIsFluid)
	{
		Subsystem->SetFluidPerMinute(Subsystem->mFluidPerMinute + mAmount);
	}
	else
	{
		Subsystem->SetItemsPerMinute(Subsystem->mItemsPerMinute + mAmount);
	}
}
