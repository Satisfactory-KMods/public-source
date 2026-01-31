// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Unlocks/KLNetworkLevelUnlock.h"

#include "FGUnlockSubsystem.h"

bool UKLNetworkLevelUnlock::IsRepeatPurchasesAllowed_Implementation() const
{
	return false;
}

void UKLNetworkLevelUnlock::Unlock(AFGUnlockSubsystem* unlockSubssytem)
{
	Super::Unlock(unlockSubssytem);
}

void UKLNetworkLevelUnlock::Apply(AFGUnlockSubsystem* unlockSubssytem)
{
	Super::Apply(unlockSubssytem);
	SendToSubsystem(unlockSubssytem);
}

void UKLNetworkLevelUnlock::SendToSubsystem(AFGUnlockSubsystem* unlockSubssytem)
{
	AKPCLFaxitSubsystem* FaxitSubsystem = AKPCLFaxitSubsystem::Get(unlockSubssytem->GetWorld());
	if (IsValid(FaxitSubsystem))
	{
		FaxitSubsystem->UnlockNetworkFeature();
	}
}
