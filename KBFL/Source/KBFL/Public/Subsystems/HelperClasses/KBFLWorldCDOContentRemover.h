#pragma once

#include "CoreMinimal.h"
#include "FGRecipeManager.h"
#include "FGResearchManager.h"
#include "FGSchematic.h"
#include "FGSchematicManager.h"
#include "Subsystems/HelperClasses/KBFLCDOOverwriteBase.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

#include "KBFLWorldCDOContentRemover.generated.h"

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

	UPROPERTY(EditAnywhere, Category = "General Settings")
	bool bListenOnSchematicManager;

	UPROPERTY(EditAnywhere, Category = "Schematics|Remove")
	TArray<TSoftClassPtr<UFGSchematic>> mSchematicsToRemove;
	TArray<TSubclassOf<UFGSchematic>> mCachedSchematicsToRemove;

	UPROPERTY(EditAnywhere, Category = "Schematics|Remove")
	TArray<FString> mSchematicsInPath;

	UPROPERTY(EditAnywhere, Category = "Schematics|Keep")
	TArray<FString> mSchematicsInPathKeep;

	UPROPERTY(EditAnywhere, Category = "Schematics|Keep")
	TArray<TSoftClassPtr<UFGSchematic>> mSchematicsToKeep;
	TArray<TSubclassOf<UFGSchematic>> mCachedSchematicsToKeep;

	UPROPERTY(EditAnywhere, Category = "Recipes|Remove")
	TArray<TSoftClassPtr<UFGRecipe>> mRecipesToRemove;
	TArray<TSubclassOf<UFGRecipe>> mCachedRecipesToRemove;

	UPROPERTY(EditAnywhere, Category = "Recipes|Remove")
	TArray<FString> mRecipesInPath;

	UPROPERTY(EditAnywhere, Category = "Recipes|Keep")
	TArray<FString> mRecipesInPathKeep;

	UPROPERTY(EditAnywhere, Category = "Recipes|Keep")
	TArray<TSoftClassPtr<UFGRecipe>> mRecipesToKeep;
	TArray<TSubclassOf<UFGRecipe>> mCachedRecipesToKeep;

	UPROPERTY(EditAnywhere, Category = "Research Trees|Remove")
	TArray<TSoftClassPtr<UFGResearchTree>> mResearchTreesToRemove;
	TArray<TSubclassOf<UFGResearchTree>> mCachedResearchTreesToRemove;

	UPROPERTY(EditAnywhere, Category = "Research Trees|Remove")
	TArray<FString> mResearchTreesInPath;

	UPROPERTY(EditAnywhere, Category = "Research Trees|Keep")
	TArray<FString> mResearchTreesInPathKeep;

	UPROPERTY(EditAnywhere, Category = "Research Trees|Keep")
	TArray<TSoftClassPtr<UFGResearchTree>> mResearchTreesToKeep;
	TArray<TSubclassOf<UFGResearchTree>> mCachedResearchTreesToKeep;
};
