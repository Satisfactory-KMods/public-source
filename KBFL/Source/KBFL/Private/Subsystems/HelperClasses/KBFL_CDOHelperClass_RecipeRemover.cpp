#pragma once
#include "Subsystems/HelperClasses/KBFL_CDOHelperClass_RecipeRemover.h"

void UKBFL_CDOHelperClass_RecipeRemover::DoCDO()
{
	UE_LOG(LogTemp, Log, TEXT("CDOHelperRecipeRemover > Called %s"), *this->GetName());

	if (AFGRecipeManager* RecipeSubsystem = AFGRecipeManager::Get(GetWorld()))
	{
		for (UClass* Class : GetClasses())
		{
			if (Class->IsChildOf(UFGRecipe::StaticClass()))
			{
				RemoveRecipe(Class, RecipeSubsystem, {}, {}, RemovedClasses);
			}
		}
	}

	Super::DoCDO();
}

TArray<UClass*> UKBFL_CDOHelperClass_RecipeRemover::GetClasses()
{
	TArray<UClass*> Re;

	for (auto Class : mRecipes)
	{
		if (IsValidSoftClass(Class))
		{
			Re.Add(Class.LoadSynchronous());
		}
	}

	return Re;
}
