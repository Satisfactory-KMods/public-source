#include "Subsystems/HelperClasses/KBFLWorldCDOContentRemover.h"

#include "FGCustomizationRecipe.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY_STATIC(LogKBFLContentRemover, VeryVerbose, All);


void UKBFLWorldCDOContentRemover::ApplyToActorsInWorld()
{
	Super::ApplyToActorsInWorld();

	mCachedRecipesToKeep = LoadSoftClassesArray(mRecipesToKeep);
	mCachedRecipesToRemove = LoadSoftClassesArray(mRecipesToRemove);
	mCachedSchematicsToKeep = LoadSoftClassesArray(mSchematicsToKeep);
	mCachedSchematicsToRemove = LoadSoftClassesArray(mSchematicsToRemove);
	mCachedResearchTreesToKeep = LoadSoftClassesArray(mResearchTreesToKeep);
	mCachedResearchTreesToRemove = LoadSoftClassesArray(mResearchTreesToRemove);

	UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());


	if (!mSchematicsInPath.IsEmpty() || !mSchematicsInPathKeep.IsEmpty())
	{
		for (const TSubclassOf<UFGSchematic>& Schematic : AssetDataSubsystem->GetAllSchematics())
		{
			bool Keep = IsInPathList(Schematic, mSchematicsInPathKeep);
			bool Remove = IsInPathList(Schematic, mSchematicsInPath);

			if (!Keep && Remove)
			{
				mCachedSchematicsToRemove.Add(Schematic);
			}
			else if (Keep)
			{
				mCachedSchematicsToKeep.Add(Schematic);
			}
		}
	}

	if (!mRecipesInPath.IsEmpty() || !mRecipesInPathKeep.IsEmpty())
	{
		for (const TSubclassOf<UFGRecipe>& Recipe : AssetDataSubsystem->GetAllRecipes())
		{
			bool Keep = IsInPathList(Recipe, mRecipesInPathKeep);
			bool Remove = IsInPathList(Recipe, mRecipesInPath);

			if (!Keep && Remove)
			{
				mCachedRecipesToRemove.Add(Recipe);
			}
			else if (Keep)
			{
				mCachedRecipesToKeep.Add(Recipe);
			}
		}
	}

	if (!mRecipesInPath.IsEmpty() || !mRecipesInPathKeep.IsEmpty())
	{
		for (const TSubclassOf<UFGResearchTree>& ResearchTree : AssetDataSubsystem->GetAllResearchTrees())
		{
			bool Keep = IsInPathList(ResearchTree, mRecipesInPathKeep);
			bool Remove = IsInPathList(ResearchTree, mRecipesInPath);

			if (!Keep && Remove)
			{
				mCachedResearchTreesToRemove.Add(ResearchTree);
			}
			else if (Keep)
			{
				mCachedResearchTreesToKeep.Add(ResearchTree);
			}
		}
	}

	HandleSchematicsRemoval();
	HandleRecipesRemoval();
	HandleResearchTreesRemoval();
}

void UKBFLWorldCDOContentRemover::Clear()
{
	Super::Clear();

	mCachedRecipesToKeep.Empty();
	mCachedRecipesToRemove.Empty();
	mCachedSchematicsToKeep.Empty();
	mCachedSchematicsToRemove.Empty();
	mCachedResearchTreesToKeep.Empty();
	mCachedResearchTreesToRemove.Empty();
}
bool UKBFLWorldCDOContentRemover::IsInPathList(TSubclassOf<UObject> Target, const TArray<FString>& PathList) const
{
	const FString& TargetPath = Target->GetPathName();
	for (const FString& Path : PathList)
	{
		if (TargetPath.Contains(Path))
		{
			return true;
		}
	}
	return false;
}

void UKBFLWorldCDOContentRemover::HandleSchematicsRemoval()
{
	AFGSchematicManager* Manager = AFGSchematicManager::Get(GetWorld());
	if (!IsValid(Manager))
	{
		return;
	}

	for (TSubclassOf<UFGSchematic> Schematic : mCachedSchematicsToRemove)
	{
		HandleSingleSchematicRemoval(Schematic, Manager);
	}

	if (bListenOnSchematicManager)
	{
		Manager->PurchasedSchematicDelegate.AddUniqueDynamic(this, &UKBFLWorldCDOContentRemover::OnSchematicUnlocked);
	}
}

void UKBFLWorldCDOContentRemover::HandleSingleSchematicRemoval(TSubclassOf<UFGSchematic> Schematic,
															   AFGSchematicManager* SchematicManager)
{
	if (mCachedSchematicsToKeep.Contains(Schematic) || !IsValid(Schematic))
	{
		return;
	}

	UFGSchematic* CDO = Schematic->GetDefaultObject<UFGSchematic>();
	if (Requirements_IsMet(CDO))
	{
		Requirements_NotifyOnModify(CDO);

		UE_LOGFMT(LogKBFLContentRemover, Log,
				  "Removing Schematic: {SchematicName} | Path: {SchematicPath} | Mod: {ModRef}", Schematic->GetName(),
				  Schematic->GetPathName(), GetName());

		SchematicManager->ResetPurchasedSchematics({Schematic});
		SchematicManager->ReceiveSchematicsReset({Schematic});

		SchematicManager->mAllSchematics.Remove(Schematic);
		SchematicManager->mPurchasedSchematics.Remove(Schematic);

		if (SchematicManager->mActiveSchematic == Schematic)
		{
			SchematicManager->mActiveSchematic = nullptr;
			UE_LOGFMT(LogKBFLContentRemover, Verbose, "Cleared active schematic: {SchematicName}",
					  Schematic->GetName());
		}
		if (SchematicManager->mLastActiveSchematic == Schematic)
		{
			SchematicManager->mLastActiveSchematic = nullptr;
			UE_LOGFMT(LogKBFLContentRemover, Verbose, "Cleared last active schematic: {SchematicName}",
					  Schematic->GetName());
		}

		SchematicManager->PopulateSchematicsLists();

		Requirements_NotifyOnModified(CDO);
	}
}

void UKBFLWorldCDOContentRemover::HandleRecipesRemoval()
{
	AFGRecipeManager* Manager = AFGRecipeManager::Get(GetWorld());
	if (!IsValid(Manager))
	{
		return;
	}

	if (mCachedRecipesToRemove.Num() > 0)
	{
		UE_LOGFMT(LogKBFLContentRemover, Log, "Starting Recipe removal - Count: {Count} | Mod: {ModRef}",
				  mCachedRecipesToRemove.Num(), GetName());
	}

	for (TSubclassOf<UFGRecipe> Recipe : mCachedRecipesToRemove)
	{
		HandleSingleRecipeRemoval(Recipe, Manager);
	}
}

void UKBFLWorldCDOContentRemover::HandleSingleRecipeRemoval(TSubclassOf<UFGRecipe> Recipe,
															AFGRecipeManager* RecipeManager)
{
	if (mCachedRecipesToKeep.Contains(Recipe) || !IsValid(Recipe))
	{
		return;
	}

	UFGRecipe* CDO = Recipe->GetDefaultObject<UFGRecipe>();
	if (Requirements_IsMet(CDO))
	{
		Requirements_NotifyOnModify(CDO);

		UE_LOGFMT(LogKBFLContentRemover, Log, "Removing Recipe: {RecipeName} | Path: {RecipePath} | Mod: {ModRef}",
				  Recipe->GetName(), Recipe->GetPathName(), GetName());

		RecipeManager->RemoveAvailableRecipes({Recipe});

		RecipeManager->mAvailableRecipes.Remove(Recipe);
		RecipeManager->mAllRecipes.Remove(Recipe);

		if (TSubclassOf<UFGCustomizationRecipe> Customization{Recipe})
		{
			RecipeManager->mAvailableCustomizationRecipes.Remove(Customization);
			UE_LOGFMT(LogKBFLContentRemover, Verbose, "Also removed as customization recipe: {RecipeName}",
					  Recipe->GetName());
		}

		RecipeManager->RebuildAvailableBuildings();
		RecipeManager->PopulateAllRecipesList();

		Requirements_NotifyOnModified(CDO);
	}
}

void UKBFLWorldCDOContentRemover::HandleResearchTreesRemoval()
{
	AFGResearchManager* Manager = AFGResearchManager::Get(GetWorld());
	if (!IsValid(Manager))
	{
		return;
	}

	if (mCachedResearchTreesToRemove.Num() > 0)
	{
		UE_LOGFMT(LogKBFLContentRemover, Log, "Starting ResearchTree removal - Count: {Count} | Mod: {ModRef}",
				  mCachedResearchTreesToRemove.Num(), GetName());
	}

	for (TSubclassOf<UFGResearchTree> Recipe : mCachedResearchTreesToRemove)
	{
		HandleSingleResearchTreeRemoval(Recipe, Manager);
	}
}

void UKBFLWorldCDOContentRemover::HandleSingleResearchTreeRemoval(TSubclassOf<UFGResearchTree> ResearchTree,
																  AFGResearchManager* ResearchManager)
{
	if (mCachedResearchTreesToKeep.Contains(ResearchTree) || !IsValid(ResearchTree))
	{
		return;
	}

	UFGResearchTree* CDO = ResearchTree->GetDefaultObject<UFGResearchTree>();
	if (Requirements_IsMet(CDO))
	{
		Requirements_NotifyOnModify(CDO);

		UE_LOGFMT(LogKBFLContentRemover, Log, "Removing ResearchTree: {TreeName} | Path: {TreePath} | Mod: {ModRef}",
				  ResearchTree->GetName(), ResearchTree->GetPathName(), GetName());

		ResearchManager->mAvailableResearchTrees.Remove(ResearchTree);
		ResearchManager->mUnlockedResearchTrees.Remove(ResearchTree);

		Requirements_NotifyOnModified(CDO);
	}
}
void UKBFLWorldCDOContentRemover::OnSchematicUnlocked(TSubclassOf<UFGSchematic> UnlockedSchematic)
{
	if (mCachedSchematicsToKeep.Contains(UnlockedSchematic))
	{
		UE_LOGFMT(LogKBFLContentRemover, Verbose,
				  "Schematic unlocked but kept: {SchematicName} | Path: {SchematicPath}", UnlockedSchematic->GetName(),
				  UnlockedSchematic->GetPathName());
		return;
	}
	if (mCachedSchematicsToRemove.Contains(UnlockedSchematic))
	{
		UE_LOGFMT(LogKBFLContentRemover, Log,
				  "Schematic unlocked and marked for removal: {SchematicName} | Path: {SchematicPath}",
				  UnlockedSchematic->GetName(), UnlockedSchematic->GetPathName());

		AFGSchematicManager* Manager = AFGSchematicManager::Get(GetWorld());
		if (!IsValid(Manager))
		{
			return;
		}

		HandleSingleSchematicRemoval(UnlockedSchematic, Manager);
	}
}
