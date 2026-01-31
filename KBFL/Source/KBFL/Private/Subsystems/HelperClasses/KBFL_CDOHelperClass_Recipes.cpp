#pragma once
#include "Subsystems/HelperClasses/KBFL_CDOHelperClass_Recipes.h"

#include "FGItemCategory.h"


DECLARE_LOG_CATEGORY_EXTERN(CDOHelperRecipesLog, Log, All)

DEFINE_LOG_CATEGORY(CDOHelperRecipesLog)

void UKBFL_CDOHelperClass_Recipes::DoCDO()
{
	UE_LOG(CDOHelperRecipesLog, Log, TEXT("CDOHelperRecipes > Called %s"), *this->GetName());
	for (TSubclassOf<UFGRecipe> Class : GetClasses())
	{
		if (UFGRecipe* DefaultObject = Class.GetDefaultObject())
		{
			// DefaultObject->AddToRoot();
			UE_LOG(CDOHelperRecipesLog, Log, TEXT("CDOHelperRecipes > DoSchematicCDO > %s"), *DefaultObject->GetName());
			if (mDisplayNameOverride)
			{
				DefaultObject->mDisplayName = mDisplayName;
				DefaultObject->mDisplayNameOverride = true;
			}

			if (mIngredientsOverride)
			{
				for (FItemAmount Ingredient : mIngredients)
				{
					if (IsValid(Ingredient.ItemClass))
					{
						// Ingredient.ItemClass->AddToRoot();
					}
				}

				DefaultObject->mIngredients = mIngredients;
			}

			if (mProductOverride)
			{
				for (FItemAmount Ingredient : mProduct)
				{
					if (IsValid(Ingredient.ItemClass))
					{
						// Ingredient.ItemClass->AddToRoot();
					}
				}

				DefaultObject->mProduct = mProduct;
			}

			if (mOverriddenCategoryOverride)
			{
				if (IsValid(mOverriddenCategory))
				{
					// mOverriddenCategory->AddToRoot();
				}

				DefaultObject->mOverriddenCategory = mOverriddenCategory;
			}

			if (mManufacturingMenuPriorityOverride)
			{
				DefaultObject->mManufacturingMenuPriority = mManufacturingMenuPriority;
			}

			if (mManufactoringDurationOverride)
			{
				DefaultObject->mManufactoringDuration = mManufactoringDuration;
			}

			if (mManualManufacturingMultiplierOverride)
			{
				DefaultObject->mManualManufacturingMultiplier = mManualManufacturingMultiplier;
			}

			if (mProducedInOverride)
			{
				DefaultObject->mProducedIn = mProducedIn;
			}

			if (mMaterialCustomizationRecipeOverride)
			{
				DefaultObject->mMaterialCustomizationRecipe = mMaterialCustomizationRecipe;
			}

			if (mRelevantEventsOverride)
			{
				DefaultObject->mRelevantEvents = mRelevantEvents;
			}

			if (mVariablePowerConsumptionConstantOverride)
			{
				DefaultObject->mVariablePowerConsumptionConstant = mVariablePowerConsumptionConstant;
			}

			if (mVariablePowerConsumptionFactorOverride)
			{
				DefaultObject->mVariablePowerConsumptionFactor = mVariablePowerConsumptionFactor;
			}
		}
	}

	Super::DoCDO();
}

TArray<UClass*> UKBFL_CDOHelperClass_Recipes::GetClasses()
{
	TArray<UClass*> Re;

	for (TSoftClassPtr<UFGRecipe> Class : mRecipes)
	{
		if (IsValidSoftClass(Class))
		{
			Re.Add(Class.LoadSynchronous());
		}
	}

	return Re;
}
