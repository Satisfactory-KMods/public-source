// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Subsystem/KPCLFaxitSubsystem.h"
#include "Unlocks/FGUnlock.h"

#include "KLNetworkLevelUnlock.generated.h"

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class KLIB_API UKLNetworkLevelUnlock : public UFGUnlock
{
	GENERATED_BODY()

public:
	//~ Begin UFGUnlock Interface
	virtual bool IsRepeatPurchasesAllowed_Implementation() const override;
	virtual void Unlock(AFGUnlockSubsystem* unlockSubssytem) override;
	virtual void Apply(AFGUnlockSubsystem* unlockSubssytem) override;
	//~ End UFGUnlock Interface

	void SendToSubsystem(AFGUnlockSubsystem* unlockSubssytem);
};
