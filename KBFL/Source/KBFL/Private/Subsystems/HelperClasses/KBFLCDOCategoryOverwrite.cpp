//
#include "Subsystems/HelperClasses/KBFLCDOCategoryOverwrite.h"

#include "FGCategory.h"
#include "FGItemCategory.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

UKBFLCDOCategoryOverwrite::UKBFLCDOCategoryOverwrite() { mCallPrio = 100000.f; }

void UKBFLCDOCategoryOverwrite::ApplyToInstances()
{
	int32 TotalChanges = 0;

	TSubclassOf<UFGItemCategory> ToCategories = LoadSoftClass(mToCategory);
	TArray<TSubclassOf<UFGItemDescriptor>> Items = LoadSoftClassesArray(mItems);
	TArray<TSubclassOf<UFGRecipe>> Recipes = LoadSoftClassesArray(mRecipes);

	for (TSubclassOf<UFGItemDescriptor> Class : Items)
	{
		UFGItemDescriptor* DefaultObject = mSubsystem->GetAndStoreDefaultObject_Native<UFGItemDescriptor>(Class);
		if (IsValid(DefaultObject) && Requirements_IsMet(DefaultObject))
		{
			Requirements_NotifyOnModify(DefaultObject);
			DefaultObject->mCategory = ToCategories;
			DefaultObject->mMenuPriority = TotalChanges;
			TotalChanges++;
			Requirements_NotifyOnModified(DefaultObject);
		}
	}

	for (TSubclassOf<UFGRecipe> Class : Recipes)
	{
		UFGRecipe* DefaultObject = mSubsystem->GetAndStoreDefaultObject_Native<UFGRecipe>(Class);
		if (IsValid(DefaultObject) && Requirements_IsMet(DefaultObject))
		{
			Requirements_NotifyOnModify(DefaultObject);
			DefaultObject->mOverriddenCategory = ToCategories;
			DefaultObject->mManufacturingMenuPriority = TotalChanges;
			TotalChanges++;
			Requirements_NotifyOnModified(DefaultObject);
		}
	}
}
