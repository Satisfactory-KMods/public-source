#include "Unlocks/KPCLDeliveryTaskQueueSlotUnlock.h"

#include "Subsystem/KPCLDeliveryTaskSubsystem.h"

void UKPCLDeliveryTaskQueueSlotUnlock::ApplySlots(AKPCLDeliveryTaskSubsystem* DeliveryTaskSubsystem, int32 Amount)
{
	DeliveryTaskSubsystem->AddQueueSlots(Amount);
}
