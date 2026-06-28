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

bool UKBFLCDOCategoryOverwrite::ShouldCallForInstance(UClass* NewClass)
{
	if (!IsValid(NewClass))
	{
		return false;
	}

	// .Get() only — the just-loaded class is already in memory, so a matching soft ptr resolves.
	bool bMatch = false;
	for (const TSoftClassPtr<UFGItemDescriptor>& Item : mItems)
	{
		if (Item.Get() == NewClass)
		{
			bMatch = true;
			break;
		}
	}
	if (!bMatch)
	{
		for (const TSoftClassPtr<UFGRecipe>& Recipe : mRecipes)
		{
			if (Recipe.Get() == NewClass)
			{
				bMatch = true;
				break;
			}
		}
	}
	if (!bMatch)
	{
		return false;
	}

	UObject* CDO = NewClass->GetDefaultObject();
	return IsValid(CDO) && Requirements_IsMet(CDO);
}

void UKBFLCDOCategoryOverwrite::ApplyToInstance(UObject* Instance)
{
	if (!IsValid(Instance))
	{
		return;
	}

	TSubclassOf<UFGItemCategory> ToCategory = LoadSoftClass(mToCategory);
	UClass* InstanceClass = Instance->GetClass();

	if (UFGItemDescriptor* Item = Cast<UFGItemDescriptor>(Instance))
	{
		// Menu priority mirrors the combined ApplyToInstances counter (items first).
		int32 Index = 0;
		for (int32 i = 0; i < mItems.Num(); i++)
		{
			if (mItems[i].Get() == InstanceClass)
			{
				Index = i;
				break;
			}
		}
		Item->mCategory = ToCategory;
		Item->mMenuPriority = Index;
	}
	else if (UFGRecipe* Recipe = Cast<UFGRecipe>(Instance))
	{
		int32 Index = mItems.Num();
		for (int32 i = 0; i < mRecipes.Num(); i++)
		{
			if (mRecipes[i].Get() == InstanceClass)
			{
				Index = mItems.Num() + i;
				break;
			}
		}
		Recipe->mOverriddenCategory = ToCategory;
		Recipe->mManufacturingMenuPriority = Index;
	}
}
