// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Unlocks/FGUnlock.h"

#include "KPCLDeliveryTaskSlotUnlockBase.generated.h"

UCLASS(Abstract)
class KPRIVATECODELIB_API UKPCLDeliveryTaskSlotUnlockBase : public UFGUnlock
{
	GENERATED_BODY()

public:
	virtual bool IsRepeatPurchasesAllowed_Implementation() const override { return true; }
	virtual void Unlock(class AFGUnlockSubsystem* UnlockSubsystem) override;

protected:

	virtual int32 GetConfiguredSlotAmount() const
		PURE_VIRTUAL(UKPCLDeliveryTaskSlotUnlockBase::GetConfiguredSlotAmount, return 1;);

	virtual void ApplySlots(class AKPCLDeliveryTaskSubsystem* DeliveryTaskSubsystem, int32 Amount)
		PURE_VIRTUAL(UKPCLDeliveryTaskSlotUnlockBase::ApplySlots, );
};
