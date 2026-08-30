#include "Unlocks/KPCLFaxitUnlockBase.h"

#include "FGUnlockSubsystem.h"
#include "Subsystem/KPCLFaxitSubsystem.h"

bool UKPCLFaxitUnlockBase::IsRepeatPurchasesAllowed_Implementation() const { return false; }

void UKPCLFaxitUnlockBase::Unlock(AFGUnlockSubsystem* UnlockSubsystem)
{
	Super::Unlock(UnlockSubsystem);
	RecomputeFaxitValues(UnlockSubsystem);
}

void UKPCLFaxitUnlockBase::Apply(AFGUnlockSubsystem* UnlockSubsystem)
{
	Super::Apply(UnlockSubsystem);
	RecomputeFaxitValues(UnlockSubsystem);
}

void UKPCLFaxitUnlockBase::RecomputeFaxitValues(AFGUnlockSubsystem* UnlockSubsystem) const
{
	if (!IsValid(UnlockSubsystem) || !UnlockSubsystem->HasAuthority())
	{
		return;
	}

	if (AKPCLFaxitSubsystem* FaxitSubsystem = AKPCLFaxitSubsystem::Get(UnlockSubsystem))
	{
		FaxitSubsystem->RequestUnlockNetworkFeature();
	}
}
