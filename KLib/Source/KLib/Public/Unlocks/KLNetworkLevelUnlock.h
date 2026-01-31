// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystem/KPCLFaxitSubsystem.h"
#include "Unlocks/FGUnlock.h"

#include "KLNetworkLevelUnlock.generated.h"

UCLASS(Blueprintable, EditInlineNew, abstract, DefaultToInstanced)
class KLIB_API UKLNetworkLevelUnlock : public UFGUnlock
{
	GENERATED_BODY()

	virtual bool IsRepeatPurchasesAllowed_Implementation() const override;

	virtual void Unlock(AFGUnlockSubsystem* unlockSubssytem) override;
	virtual void Apply(AFGUnlockSubsystem* unlockSubssytem) override;
	void SendToSubsystem(AFGUnlockSubsystem* unlockSubssytem);
};
