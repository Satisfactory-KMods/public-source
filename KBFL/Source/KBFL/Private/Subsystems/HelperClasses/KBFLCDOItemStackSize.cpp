//
#include "Subsystems/HelperClasses/KBFLCDOItemStackSize.h"

#include "FGResourceSettings.h"
#include "Resources/FGItemDescriptor.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

void UKBFLCDOItemStackSize::ApplyToInstances()
{
	Super::ApplyToInstances();

	for (const TPair<EStackSize, FKBFLItemArray>& Pair : mItemStackSizeCDO)
	{
		for (const TSubclassOf<UFGItemDescriptor>& Item : Pair.Value.mItems)
		{
			if (!IsValid(Item))
			{
				continue;
			}

			// Resolve + store the CDO (GC retention) before delegating the actual apply.
			UFGItemDescriptor* DefaultObject = mSubsystem->GetAndStoreDefaultObject_Native<UFGItemDescriptor>(Item);
			ApplyToInstance(DefaultObject);
		}
	}
}

bool UKBFLCDOItemStackSize::ShouldCallForInstance(UClass* NewClass)
{
	if (!IsValid(NewClass) || !NewClass->IsChildOf(UFGItemDescriptor::StaticClass()))
	{
		return false;
	}

	for (const TPair<EStackSize, FKBFLItemArray>& Pair : mItemStackSizeCDO)
	{
		if (Pair.Value.mItems.Contains(NewClass))
		{
			UObject* CDO = NewClass->GetDefaultObject();
			return IsValid(CDO) && Requirements_IsMet(CDO);
		}
	}

	return false;
}

void UKBFLCDOItemStackSize::ApplyToInstance(UObject* Instance)
{
	UFGItemDescriptor* DefaultObject = Cast<UFGItemDescriptor>(Instance);
	if (!IsValid(DefaultObject) || !Requirements_IsMet(DefaultObject))
	{
		return;
	}

	UClass* InstanceClass = DefaultObject->GetClass();

	for (const TPair<EStackSize, FKBFLItemArray>& Pair : mItemStackSizeCDO)
	{
		if (Pair.Value.mItems.Contains(InstanceClass))
		{
			Requirements_NotifyOnModify(DefaultObject);
			DefaultObject->mStackSize = Pair.Key;
			if (const int32* FoundSize = UFGResourceSettings::Get()->mStackSizes.FindKey(DefaultObject->mStackSize))
			{
				DefaultObject->mCachedStackSize = *FoundSize;
			}
			Requirements_NotifyOnModified(DefaultObject);

			UE_LOGFMT(LogKBFLCDOOverwrite, Warning, "KBFLCDOItemStackSize: Set StackSize {0} to Item {1} | Asset: {2}",
					  static_cast<int32>(Pair.Key), *InstanceClass->GetName(), GetPathName());
			return;
		}
	}
}
