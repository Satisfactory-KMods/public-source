#include "DataAssets/KAPIModularMinerDescription.h"

void FKAPIModuleItems::TryToSetDynamic(UObject* WorldContext, TSubclassOf<UFGResourceDescriptor> Resource)
{
	if (mTryProductionDynamic)
	{
		AFGRecipeManager* Manager = AFGRecipeManager::Get(WorldContext);
		if (Manager)
		{
			TArray<TSubclassOf<UFGRecipe>> Recipes = Manager->FindRecipesByIngredient(Resource);
			for (TSubclassOf<UFGRecipe> Recipe : Recipes)
			{
				if (UFGRecipe::GetIngredients(Recipe).Num() == 1 && UFGRecipe::GetProducts(Recipe).Num() == 1)
				{
					mProductionItem = UFGRecipe::GetProducts(Recipe)[0].ItemClass;
				}
			}
		}
	}
}

bool FKAPIModuleItems::IsValid() const
{
	return mTrashItem && mProductionItem;
}

bool UKAPIModularMinerDescription::IsModuleAllowed(
	TSubclassOf<AFGBuildable> Module) const
{
	return !mPreventModules.Contains(Module);
}

FKAPIModuleItems UKAPIModularMinerDescription::GetItemsForModule(
	TSubclassOf<UKAPIWasteProducerType> Module) const
{
	if (!IsValid(Module) || !mModuleInformation.Contains(Module))
	{
		return FKAPIModuleItems();
	}

	return mModuleInformation.FindRef(Module);
}
