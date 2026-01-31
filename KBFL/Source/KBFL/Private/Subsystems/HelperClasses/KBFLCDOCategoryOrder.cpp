//
#include "Subsystems/HelperClasses/KBFLCDOCategoryOrder.h"

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
