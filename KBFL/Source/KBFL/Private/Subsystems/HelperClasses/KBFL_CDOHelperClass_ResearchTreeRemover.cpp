#pragma once
#include "Subsystems/HelperClasses/KBFL_CDOHelperClass_ResearchTreeRemover.h"

void UKBFL_CDOHelperClass_ResearchTreeRemover::DoCDO()
{
	UE_LOG(LogTemp, Log, TEXT("CDOHelperSchematicRemover > Called %s"), *this->GetName());
	AFGRecipeManager* RecipeSubsystem = AFGRecipeManager::Get(GetWorld());
	AFGSchematicManager* SchematicSubsystem = AFGSchematicManager::Get(GetWorld());
	AFGResearchManager* ResearchManager = AFGResearchManager::Get(GetWorld());

	if (RecipeSubsystem)
	{
		for (UClass* Class : GetClasses())
		{
			if (Class->IsChildOf(UFGSchematic::StaticClass()))
			{
				RemoveResearchTree(Class, ResearchManager, RecipeSubsystem, SchematicSubsystem, {}, {},
								   GetExcludeClassesRecipes(), {}, GetExcludeClassesSchematics(), {}, RemovedClasses);
			}
		}
	}

	Super::DoCDO();
}

TArray<UClass*> UKBFL_CDOHelperClass_ResearchTreeRemover::GetClasses()
{
	TArray<UClass*> Re;

	for (auto Class : mTrees)
	{
		if (IsValidSoftClass(Class))
		{
			Re.Add(Class.LoadSynchronous());
		}
	}

	return Re;
}

TArray<TSubclassOf<UFGRecipe>> UKBFL_CDOHelperClass_ResearchTreeRemover::GetExcludeClassesRecipes()
{
	TArray<TSubclassOf<UFGRecipe>> Re = {};

	for (auto Class : mExcludeRecipes)
	{
		if (IsValidSoftClass(Class))
		{
			Re.Add(Class.LoadSynchronous());
		}
	}

	return Re;
}

TArray<TSubclassOf<UFGSchematic>> UKBFL_CDOHelperClass_ResearchTreeRemover::GetExcludeClassesSchematics()
{
	TArray<TSubclassOf<UFGSchematic>> Re = {};

	for (auto Class : mExcludeSchematics)
	{
		if (IsValidSoftClass(Class))
		{
			Re.Add(Class.LoadSynchronous());
		}
	}

	return Re;
}
