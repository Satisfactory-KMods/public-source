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
	/**
	 * Entry point: resolves the soft-class remove/keep lists, expands the path-based lists against all discovered
	 * schematics/recipes/research trees (keep overrides remove), then removes schematics, recipes, and research trees.
	 */
	virtual void ApplyToActorsInWorld() override;

	/** Empties all cached remove/keep lists. */
	virtual void Clear() override;

	/** Returns true if Target's path name contains any of the substrings in PathList. */
	bool IsInPathList(TSubclassOf<UObject> Target, const TArray<FString>& PathList) const;

	/**
	 * Removes all cached schematics from the schematic manager. When bListenOnSchematicManager is set, also subscribes
	 * to PurchasedSchematicDelegate so schematics are removed again if unlocked later.
	 */
	void HandleSchematicsRemoval();

	/**
	 * Removes a single schematic from the manager (reset purchased, drop from all/purchased lists, clear active/last
	 * active, repopulate). Skipped if the schematic is in the keep list or fails the requirements check.
	 */
	void HandleSingleSchematicRemoval(TSubclassOf<UFGSchematic> Schematic, AFGSchematicManager* SchematicManager);

	/** Removes all cached recipes from the recipe manager. */
	void HandleRecipesRemoval();

	/**
	 * Removes a single recipe from the manager (available/all lists, plus the customization list if it is a
	 * customization recipe) and rebuilds derived recipe data. Skipped if kept or requirements not met.
	 */
	void HandleSingleRecipeRemoval(TSubclassOf<UFGRecipe> Recipe, AFGRecipeManager* RecipeManager);

	/** Removes all cached research trees from the research manager. */
	void HandleResearchTreesRemoval();

	/**
	 * Removes a single research tree from the manager's available and unlocked lists. Skipped if the tree is in the
	 * keep list or fails the requirements check.
	 */
	void HandleSingleResearchTreeRemoval(TSubclassOf<UFGResearchTree> ResearchTree,
										 AFGResearchManager* ResearchManager);

	/**
	 * PurchasedSchematicDelegate callback (active when bListenOnSchematicManager): re-removes a just-unlocked schematic
	 * if it is marked for removal, or leaves it in place if it is in the keep list.
	 */
	UFUNCTION()
	void OnSchematicUnlocked(TSubclassOf<UFGSchematic> UnlockedSchematic);

	// ===== General Settings =====
	/** Listen for schematic unlocks and remove schematics dynamically */
	UPROPERTY(EditAnywhere, Category = "General Settings")
	bool bListenOnSchematicManager;

	// ===== Schematics Removal =====
	/** Specific schematics to remove */
	UPROPERTY(EditAnywhere, Category = "Schematics|Remove")
	TArray<TSoftClassPtr<UFGSchematic>> mSchematicsToRemove;
	TArray<TSubclassOf<UFGSchematic>> mCachedSchematicsToRemove;

	/** Remove all schematics in these paths */
	UPROPERTY(EditAnywhere, Category = "Schematics|Remove")
	TArray<FString> mSchematicsInPath;

	/** Keep schematics in these paths (overrides removal) */
	UPROPERTY(EditAnywhere, Category = "Schematics|Keep")
	TArray<FString> mSchematicsInPathKeep;

	/** Specific schematics to keep (overrides removal) */
	UPROPERTY(EditAnywhere, Category = "Schematics|Keep")
	TArray<TSoftClassPtr<UFGSchematic>> mSchematicsToKeep;
	TArray<TSubclassOf<UFGSchematic>> mCachedSchematicsToKeep;

	// ===== Recipes Removal =====
	/** Specific recipes to remove */
	UPROPERTY(EditAnywhere, Category = "Recipes|Remove")
	TArray<TSoftClassPtr<UFGRecipe>> mRecipesToRemove;
	TArray<TSubclassOf<UFGRecipe>> mCachedRecipesToRemove;

	/** Remove all recipes in these paths */
	UPROPERTY(EditAnywhere, Category = "Recipes|Remove")
	TArray<FString> mRecipesInPath;

	/** Keep recipes in these paths (overrides removal) */
	UPROPERTY(EditAnywhere, Category = "Recipes|Keep")
	TArray<FString> mRecipesInPathKeep;

	/** Specific recipes to keep (overrides removal) */
	UPROPERTY(EditAnywhere, Category = "Recipes|Keep")
	TArray<TSoftClassPtr<UFGRecipe>> mRecipesToKeep;
	TArray<TSubclassOf<UFGRecipe>> mCachedRecipesToKeep;

	// ===== Research Trees Removal =====
	/** Specific research trees to remove */
	UPROPERTY(EditAnywhere, Category = "Research Trees|Remove")
	TArray<TSoftClassPtr<UFGResearchTree>> mResearchTreesToRemove;
	TArray<TSubclassOf<UFGResearchTree>> mCachedResearchTreesToRemove;

	/** Remove all research trees in these paths */
	UPROPERTY(EditAnywhere, Category = "Research Trees|Remove")
	TArray<FString> mResearchTreesInPath;

	/** Keep research trees in these paths (overrides removal) */
	UPROPERTY(EditAnywhere, Category = "Research Trees|Keep")
	TArray<FString> mResearchTreesInPathKeep;

	/** Specific research trees to keep (overrides removal) */
	UPROPERTY(EditAnywhere, Category = "Research Trees|Keep")
	TArray<TSoftClassPtr<UFGResearchTree>> mResearchTreesToKeep;
	TArray<TSubclassOf<UFGResearchTree>> mCachedResearchTreesToKeep;
};
