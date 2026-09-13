// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Unlocks/KPCLFaxitCapacityUnlock.h"

#include "Subsystem/KPCLFaxitSubsystem.h"

#if WITH_EDITOR
void UKPCLFaxitCapacityUnlock::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UKPCLFaxitCapacityUnlock, mCapacityType) &&
		mCapacityType == EKPCLFaxitCapacityType::BufferInventory)
	{
		bUseCapacityTypeDefaultAmount = mAmount == 1;
		if (bUseCapacityTypeDefaultAmount)
		{
			mAmount = 3;
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UKPCLFaxitCapacityUnlock, mCapacityType) &&
		bUseCapacityTypeDefaultAmount && mAmount == 3)
	{
		mAmount = 1;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UKPCLFaxitCapacityUnlock, mAmount))
	{
		bUseCapacityTypeDefaultAmount = false;
	}
}
#endif

int32 UKPCLFaxitCapacityUnlock::GetAmount() const
{
	const int32 ClampedAmount = FMath::Max(1, mAmount);
	if (mCapacityType == EKPCLFaxitCapacityType::BufferInventory && bUseCapacityTypeDefaultAmount && ClampedAmount == 1)
	{
		return 3;
	}

	return ClampedAmount;
}

void UKPCLFaxitCapacityUnlock::ApplyToFaxit(AKPCLFaxitSubsystem* Subsystem) const
{
	if (!IsValid(Subsystem))
	{
		return;
	}

	const int32 Amount = GetAmount();
	switch (mCapacityType)
	{
	case EKPCLFaxitCapacityType::MachineNetwork:
		Subsystem->SetMachineNetworkLimit(Subsystem->mMachineNetworkLimit + Amount);
		break;
	case EKPCLFaxitCapacityType::Network:
		Subsystem->SetNetworkLimit(Subsystem->mNetworkLimit + Amount);
		break;
	case EKPCLFaxitCapacityType::Drive:
		Subsystem->SetDriveLimit(Subsystem->mDriveLimit + Amount);
		break;
	case EKPCLFaxitCapacityType::BufferInventory:
		Subsystem->SetNetworkBuildingBufferInventorySize(Subsystem->mNetworkBuildingBufferInventorySize + Amount);
		break;
	}
}
