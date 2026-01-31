#pragma once

#include "CoreMinimal.h"
#include "FGRecipeManager.h"
#include "FGResearchManager.h"
#include "FGSchematic.h"
#include "FGSchematicManager.h"
#include "Subsystems/HelperClasses/KBFLCDOOverwriteBase.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

#include "KBFLWorldCDOContentRemover.generated.h"

/**
 * WIP: Removes content from the world based on specified criteria
 * Can be used to remove actors, components, or other world objects
 */
UCLASS(NotBlueprintable, NotBlueprintType)
class KBFL_API UKBFLWorldCDOContentRemover : public UKBFLCDOOverwriteWorldBasedBase
{
	GENERATED_BODY()

public:
	virtual void ApplyToActorsInWorld() override;
	virtual void Clear() override;

	bool IsInPathList(TSubclassOf<UObject> Target, const TArray<FString>& PathList) const;

	void HandleSchematicsRemoval();
	void HandleSingleSchematicRemoval(TSubclassOf<UFGSchematic> Schematic, AFGSchematicManager* SchematicManager);

	void HandleRecipesRemoval();
	void HandleSingleRecipeRemoval(TSubclassOf<UFGRecipe> Recipe, AFGRecipeManager* RecipeManager);

	void HandleResearchTreesRemoval();
	void HandleSingleResearchTreeRemoval(TSubclassOf<UFGResearchTree> ResearchTree,
										 AFGResearchManager* ResearchManager);

	UFUNCTION()
	void OnSchematicUnlocked(TSubclassOf<UFGSchematic> UnlockedSchematic);

	UPROPERTY(EditAnywhere, Category = "Content Remover")
	bool bListenOnSchematicManager;

	// =============================================================================

	UPROPERTY(EditAnywhere, Category = "Content Remover|Schematics")
	TArray<TSoftClassPtr<UFGSchematic>> mSchematicsToRemove;
	TArray<TSubclassOf<UFGSchematic>> mCachedSchematicsToRemove;

	UPROPERTY(EditAnywhere, Category = "Content Remover|Schematics")
	TArray<FString> mSchematicsInPath;

	UPROPERTY(EditAnywhere, Category = "Content Remover|Schematics")
	TArray<FString> mSchematicsInPathKeep;

	UPROPERTY(EditAnywhere, Category = "Content Remover|Schematics")
	TArray<TSoftClassPtr<UFGSchematic>> mSchematicsToKeep;
	TArray<TSubclassOf<UFGSchematic>> mCachedSchematicsToKeep;

	// =============================================================================

	UPROPERTY(EditAnywhere, Category = "Content Remover|Recipes")
	TArray<TSoftClassPtr<UFGRecipe>> mRecipesToRemove;
	TArray<TSubclassOf<UFGRecipe>> mCachedRecipesToRemove;

	UPROPERTY(EditAnywhere, Category = "Content Remover|Recipes")
	TArray<FString> mRecipesInPath;

	UPROPERTY(EditAnywhere, Category = "Content Remover|Recipes")
	TArray<FString> mRecipesInPathKeep;

	UPROPERTY(EditAnywhere, Category = "Content Remover|Recipes")
	TArray<TSoftClassPtr<UFGRecipe>> mRecipesToKeep;
	TArray<TSubclassOf<UFGRecipe>> mCachedRecipesToKeep;

	// =============================================================================

	UPROPERTY(EditAnywhere, Category = "Content Remover|ResearchTrees")
	TArray<TSoftClassPtr<UFGResearchTree>> mResearchTreesToRemove;
	TArray<TSubclassOf<UFGResearchTree>> mCachedResearchTreesToRemove;

	UPROPERTY(EditAnywhere, Category = "Content Remover|ResearchTrees")
	TArray<FString> mResearchTreesInPath;

	UPROPERTY(EditAnywhere, Category = "Content Remover|ResearchTrees")
	TArray<FString> mResearchTreesInPathKeep;

	UPROPERTY(EditAnywhere, Category = "Content Remover|ResearchTrees")
	TArray<TSoftClassPtr<UFGResearchTree>> mResearchTreesToKeep;
	TArray<TSubclassOf<UFGResearchTree>> mCachedResearchTreesToKeep;
};
