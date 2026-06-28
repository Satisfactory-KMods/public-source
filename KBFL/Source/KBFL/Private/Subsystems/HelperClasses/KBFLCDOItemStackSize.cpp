//
#include "Subsystems/HelperClasses/KBFLCDOItemStackSize.h"

#include "Resources/FGItemDescriptor.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

void UKBFLCDOItemStackSize::ApplyToInstances()
{
	Super::ApplyToInstances();

	const FString AssetPath = GetPathName();

	for (const TPair<EStackSize, FKBFLItemArray>& Pair : mItemStackSizeCDO)
	{
		for (const TSubclassOf<UFGItemDescriptor>& Item : Pair.Value.mItems)
		{
			if (!IsValid(Item))
			{
				continue;
			}

			UFGItemDescriptor* DefaultObject = mSubsystem->GetAndStoreDefaultObject_Native<UFGItemDescriptor>(Item);
			if (!IsValid(DefaultObject) || !Requirements_IsMet(DefaultObject))
			{
				continue;
			}

			Requirements_NotifyOnModify(DefaultObject);
			DefaultObject->mStackSize = Pair.Key;
			DefaultObject->mCachedStackSize = UFGItemDescriptor::GetStackSize(Item);
			Requirements_NotifyOnModified(DefaultObject);

			UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
					  "KBFLCDOItemStackSize: Set StackSize {0} to Item {1} | Asset: {AssetPath}",
					  static_cast<int32>(Pair.Key), *Item->GetName(), AssetPath);
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
	if (!IsValid(DefaultObject))
	{
		return;
	}

	UClass* InstanceClass = DefaultObject->GetClass();

	for (const TPair<EStackSize, FKBFLItemArray>& Pair : mItemStackSizeCDO)
	{
		if (Pair.Value.mItems.Contains(InstanceClass))
		{
			DefaultObject->mStackSize = Pair.Key;
			DefaultObject->mCachedStackSize = UFGItemDescriptor::GetStackSize(InstanceClass);
			return;
		}
	}
}
