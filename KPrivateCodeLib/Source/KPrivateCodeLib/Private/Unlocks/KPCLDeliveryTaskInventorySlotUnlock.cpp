// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Unlocks/KPCLDeliveryTaskInventorySlotUnlock.h"

#include "Subsystem/KPCLDeliveryTaskSubsystem.h"

void UKPCLDeliveryTaskInventorySlotUnlock::ApplySlots(AKPCLDeliveryTaskSubsystem* DeliveryTaskSubsystem, int32 Amount)
{
	DeliveryTaskSubsystem->AddRewardInventorySlots(Amount);
}
