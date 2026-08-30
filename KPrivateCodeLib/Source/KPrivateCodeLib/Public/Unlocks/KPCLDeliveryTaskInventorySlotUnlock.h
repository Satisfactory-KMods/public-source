#pragma once

#include "CoreMinimal.h"
#include "Unlocks/KPCLDeliveryTaskSlotUnlockBase.h"

#include "KPCLDeliveryTaskInventorySlotUnlock.generated.h"

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class KPRIVATECODELIB_API UKPCLDeliveryTaskInventorySlotUnlock : public UKPCLDeliveryTaskSlotUnlockBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	int32 GetNumInventorySlotsToUnlock() const { return mNumInventorySlotsToUnlock; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 mNumInventorySlotsToUnlock = 1;

protected:
	virtual int32 GetConfiguredSlotAmount() const override { return mNumInventorySlotsToUnlock; }
	virtual void ApplySlots(class AKPCLDeliveryTaskSubsystem* DeliveryTaskSubsystem, int32 Amount) override;
};
