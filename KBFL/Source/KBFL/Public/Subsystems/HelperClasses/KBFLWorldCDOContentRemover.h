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
