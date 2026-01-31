#pragma once

#include "CoreMinimal.h"
#include "FGRecipeManager.h"
#include "FGResearchManager.h"
#include "FGResearchTree.h"
#include "FGSchematic.h"
#include "FGSchematicManager.h"
#include "KBFL_CDOHelperClass_Base.h"
#include "KBFL_CDOHelperClass_RemoverBase.generated.h"

/**
 *@deprecated: use the new UKBFLCDOOverwrite system instead
 */
UCLASS()
class KBFL_API UKBFL_CDOHelperClass_RemoverBase : public UKBFL_CDOHelperClass_Base
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static bool RemoveSchematic(TSubclassOf<UFGSchematic> Subclass, AFGRecipeManager* RecipeSubsystem,
								AFGSchematicManager* SchematicSubsystem, TArray<TSubclassOf<UFGSchematic>> Exclude,
								TArray<FString> ExcludeStrings, TArray<TSubclassOf<UFGRecipe>> ExcludeRecipes,
								TArray<FString> ExcludeRecipesStrings, TArray<UClass*>& RemovedClasses);

	UFUNCTION(BlueprintCallable)
	static bool RemoveRecipe(TSubclassOf<UFGRecipe> Subclass, AFGRecipeManager* RecipeSubsystem,
							 TArray<TSubclassOf<UFGRecipe>> Exclude, TArray<FString> ExcludeStrings,
							 TArray<UClass*>& RemovedClasses);

	UFUNCTION(BlueprintCallable)
	static bool RemoveResearchTree(TSubclassOf<UFGResearchTree> Subclass, AFGResearchManager* ResearchManager,
								   AFGRecipeManager* RecipeSubsystem, AFGSchematicManager* SchematicSubsystem,
								   TArray<TSubclassOf<UFGResearchTree>> Exclude, TArray<FString> ExcludeStrings,
								   TArray<TSubclassOf<UFGRecipe>> ExcludeRecipes, TArray<FString> ExcludeRecipesStrings,
								   TArray<TSubclassOf<UFGSchematic>> ExcludeSchematics,
								   TArray<FString> ExcludeSchematicsStrings, TArray<UClass*>& RemovedClasses);

	UFUNCTION(BlueprintCallable)
	static void ExtractSchematicsFromResearchTree(TSubclassOf<UFGResearchTree> ResearchTree,
												  TArray<TSubclassOf<UFGSchematic>>& OutSchematics);

	UFUNCTION(BlueprintPure)
	static bool CheckClassString(UClass* Class, TArray<FString> Strings);

	UPROPERTY(Transient)
	TArray<UClass*> RemovedClasses;
};
