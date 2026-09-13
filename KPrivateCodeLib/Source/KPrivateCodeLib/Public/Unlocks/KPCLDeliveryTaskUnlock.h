// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Unlocks/FGUnlock.h"

#include "KPCLDeliveryTaskUnlock.generated.h"

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class KPRIVATECODELIB_API UKPCLDeliveryTaskUnlock : public UFGUnlock
{
	GENERATED_BODY()

public:
	virtual bool IsRepeatPurchasesAllowed_Implementation() const override;
	virtual void Unlock(AFGUnlockSubsystem* UnlockSubsystem) override;
	virtual void Apply(AFGUnlockSubsystem* UnlockSubsystem) override;

private:
	void ApplyToKPCLSubsystem(AFGUnlockSubsystem* UnlockSubsystem) const;
};
