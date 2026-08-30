#pragma once

#include "CoreMinimal.h"
#include "Unlocks/KPCLDeliveryTaskSlotUnlockBase.h"

#include "KPCLDeliveryTaskQueueSlotUnlock.generated.h"

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class KPRIVATECODELIB_API UKPCLDeliveryTaskQueueSlotUnlock : public UKPCLDeliveryTaskSlotUnlockBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	int32 GetNumQueueSlotsToUnlock() const { return mAmount; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 mAmount = 1;

protected:
	virtual int32 GetConfiguredSlotAmount() const override { return mAmount; }
	virtual void ApplySlots(class AKPCLDeliveryTaskSubsystem* DeliveryTaskSubsystem, int32 Amount) override;
};
