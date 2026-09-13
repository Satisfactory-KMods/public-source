// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Unlocks/KPCLDeliveryTaskSlotUnlockBase.h"

#include "FGUnlockSubsystem.h"
#include "Subsystem/KPCLDeliveryTaskSubsystem.h"

void UKPCLDeliveryTaskSlotUnlockBase::Unlock(AFGUnlockSubsystem* UnlockSubsystem)
{
	Super::Unlock(UnlockSubsystem);
	if (!IsValid(UnlockSubsystem))
	{
		return;
	}

	if (AKPCLDeliveryTaskSubsystem* DeliveryTaskSubsystem = AKPCLDeliveryTaskSubsystem::Get(UnlockSubsystem))
	{
		ApplySlots(DeliveryTaskSubsystem, FMath::Max(1, GetConfiguredSlotAmount()));
	}
}
