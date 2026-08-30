#include "Unlocks/KPCLDeliveryTaskUnlock.h"

#include "FGUnlockSubsystem.h"
#include "Subsystem/KPCLUnlockSubsystem.h"

bool UKPCLDeliveryTaskUnlock::IsRepeatPurchasesAllowed_Implementation() const { return false; }

void UKPCLDeliveryTaskUnlock::Unlock(AFGUnlockSubsystem* UnlockSubsystem)
{
	Super::Unlock(UnlockSubsystem);
	ApplyToKPCLSubsystem(UnlockSubsystem);
}

void UKPCLDeliveryTaskUnlock::Apply(AFGUnlockSubsystem* UnlockSubsystem)
{
	Super::Apply(UnlockSubsystem);
	ApplyToKPCLSubsystem(UnlockSubsystem);
}

void UKPCLDeliveryTaskUnlock::ApplyToKPCLSubsystem(AFGUnlockSubsystem* UnlockSubsystem) const
{
	if (!IsValid(UnlockSubsystem) || !UnlockSubsystem->HasAuthority())
	{
		return;
	}

	AKPCLUnlockSubsystem* KPCLUnlockSubsystem = AKPCLUnlockSubsystem::Get(UnlockSubsystem);
	fgcheckf(IsValid(KPCLUnlockSubsystem), TEXT("KPCLUnlockSubsystem is Invalid!"))

	KPCLUnlockSubsystem->UnlockDeliveryTaskSystem();
}
