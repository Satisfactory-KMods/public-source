#pragma once
#include "Subsystems/HelperClasses/KBFL_CDOHelperClass_SchematicRemover.h"

void UKBFL_CDOHelperClass_SchematicRemover::DoCDO()
{
	UE_LOG(LogTemp, Log, TEXT("CDOHelperSchematicRemover > Called %s"), *this->GetName());
	AFGRecipeManager* RecipeSubsystem = AFGRecipeManager::Get(GetWorld());
	AFGSchematicManager* SchematicSubsystem = AFGSchematicManager::Get(GetWorld());

	if (RecipeSubsystem)
	{
		for (UClass* Class : GetClasses())
		{
			if (Class->IsChildOf(UFGSchematic::StaticClass()))
			{
				RemoveSchematic(Class, RecipeSubsystem, SchematicSubsystem, {}, {}, GetExcludeClasses(), {},
								RemovedClasses);
			}
		}
	}

	Super::DoCDO();
}

TArray<UClass*> UKBFL_CDOHelperClass_SchematicRemover::GetClasses()
{
	TArray<UClass*> Re;

	for (auto Class : mSchematics)
	{
		if (IsValidSoftClass(Class))
		{
			Re.Add(Class.LoadSynchronous());
		}
	}

	return Re;
}

TArray<TSubclassOf<UFGRecipe>> UKBFL_CDOHelperClass_SchematicRemover::GetExcludeClasses()
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
