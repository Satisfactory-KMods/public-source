//
#include "Subsystems/HelperClasses/KBFLCDOCategoryOrder.h"

#include "FGBuildSubCategory.h"
#include "FGCategory.h"
#include "FGItemCategory.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

UKBFLCDOCategoryOrder::UKBFLCDOCategoryOrder() { mCallPrio = 100000.f; }

void UKBFLCDOCategoryOrder::ApplyToInstances()
{
	const FString AssetPath = GetPathName();

	TSubclassOf<UFGBuildCategory> TargetCategory = LoadSoftClass(mTargetCategory);
	TSubclassOf<UFGBuildSubCategory> TargetSubCategory = LoadSoftClass(mSubTargetCategory);

	if (!IsValid(TargetCategory) || !IsValid(TargetSubCategory))
	{
		return;
	}

	TArray<TSubclassOf<UFGBuildDescriptor>> Classes = LoadSoftClassesArray(mTargetOrder);

	UFGBuildCategory* DefaultCategoryObject =
		mSubsystem->GetAndStoreDefaultObject_Native<UFGBuildCategory>(TargetCategory);
	UFGBuildSubCategory* DefaultSubCategoryObject =
		mSubsystem->GetAndStoreDefaultObject_Native<UFGBuildSubCategory>(TargetSubCategory);
	if (IsValid(DefaultCategoryObject) && mTargetCategoryPrio > -1000.f && Requirements_IsMet(DefaultCategoryObject))
	{
		Requirements_NotifyOnModify(DefaultCategoryObject);
		DefaultCategoryObject->mMenuPriority = mTargetCategoryPrio;
		Requirements_NotifyOnModified(DefaultCategoryObject);
	}

	if (IsValid(DefaultSubCategoryObject) && mSubTargetCategoryPrio > -1000.f &&
		Requirements_IsMet(DefaultSubCategoryObject))
	{
		Requirements_NotifyOnModify(DefaultSubCategoryObject);
		DefaultSubCategoryObject->mMenuPriority = mSubTargetCategoryPrio;
		Requirements_NotifyOnModified(DefaultSubCategoryObject);
	}

	for (int32 Index = 0; Index < Classes.Num(); Index++)
	{
		if (TSubclassOf<UFGBuildDescriptor> Class = Classes[Index])
		{
			UFGBuildDescriptor* DefaultObject = mSubsystem->GetAndStoreDefaultObject_Native<UFGBuildDescriptor>(Class);

			if (!IsValid(DefaultObject) || !Requirements_IsMet(DefaultObject))
			{
				continue;
			}

			Requirements_NotifyOnModify(DefaultObject);
			DefaultObject->mCategory = TargetCategory;
			DefaultObject->mSubCategories.Empty();
			DefaultObject->mSubCategories.Add(TargetSubCategory);
			DefaultObject->mMenuPriority = Index;
			Requirements_NotifyOnModified(DefaultObject);

			UE_LOGFMT(LogKBFLCDOOverwrite, Warning,
					  "KBFLCDOCategoryOrder: Set Category {0} and SubCategory {1} with Prio {2} to Item {3} | Asset: "
					  "{AssetPath}",
					  *TargetCategory->GetName(), *TargetSubCategory->GetName(), Index, *Class->GetName(), AssetPath);
		}
	}
}

bool UKBFLCDOCategoryOrder::ShouldCallForInstance(UClass* NewClass)
{
	if (!IsValid(NewClass))
	{
		return false;
	}

	// Category / subcategory priority targets (only when a priority is actually set).
	if (mTargetCategoryPrio > -1000.f && mTargetCategory.Get() == NewClass)
	{
		UObject* CDO = NewClass->GetDefaultObject();
		return IsValid(CDO) && Requirements_IsMet(CDO);
	}
	if (mSubTargetCategoryPrio > -1000.f && mSubTargetCategory.Get() == NewClass)
	{
		UObject* CDO = NewClass->GetDefaultObject();
		return IsValid(CDO) && Requirements_IsMet(CDO);
	}

	// Build descriptor ordering — only valid when both category + subcategory resolve.
	bool bInOrder = false;
	for (const TSoftClassPtr<UFGBuildDescriptor>& Cls : mTargetOrder)
	{
		if (Cls.Get() == NewClass)
		{
			bInOrder = true;
			break;
		}
	}
	if (!bInOrder)
	{
		return false;
	}

	if (!IsValid(LoadSoftClass(mTargetCategory)) || !IsValid(LoadSoftClass(mSubTargetCategory)))
	{
		return false;
	}

	UObject* CDO = NewClass->GetDefaultObject();
	return IsValid(CDO) && Requirements_IsMet(CDO);
}

void UKBFLCDOCategoryOrder::ApplyToInstance(UObject* Instance)
{
	if (!IsValid(Instance))
	{
		return;
	}

	UClass* InstanceClass = Instance->GetClass();

	// Category priority
	if (InstanceClass == mTargetCategory.Get())
	{
		if (UFGBuildCategory* Cat = Cast<UFGBuildCategory>(Instance))
		{
			Cat->mMenuPriority = mTargetCategoryPrio;
		}
		return;
	}

	// SubCategory priority
	if (InstanceClass == mSubTargetCategory.Get())
	{
		if (UFGBuildSubCategory* Sub = Cast<UFGBuildSubCategory>(Instance))
		{
			Sub->mMenuPriority = mSubTargetCategoryPrio;
		}
		return;
	}

	// Build descriptor ordering
	if (UFGBuildDescriptor* Desc = Cast<UFGBuildDescriptor>(Instance))
	{
		TSubclassOf<UFGBuildCategory> TargetCategory = LoadSoftClass(mTargetCategory);
		TSubclassOf<UFGBuildSubCategory> TargetSubCategory = LoadSoftClass(mSubTargetCategory);
		if (!IsValid(TargetCategory) || !IsValid(TargetSubCategory))
		{
			return;
		}

		int32 Index = 0;
		for (int32 i = 0; i < mTargetOrder.Num(); i++)
		{
			if (mTargetOrder[i].Get() == InstanceClass)
			{
				Index = i;
				break;
			}
		}

		Desc->mCategory = TargetCategory;
		Desc->mSubCategories.Empty();
		Desc->mSubCategories.Add(TargetSubCategory);
		Desc->mMenuPriority = Index;
	}
}
