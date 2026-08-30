#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class UFGRecipe;
class UWorld;

class KDATAFORGE_API FKDFRecipeRemoval
{
public:

	static bool HasAnyRemovalRules(const UObject* WorldContext);

	static bool IsRecipeRemoved(const UObject* WorldContext, TSubclassOf<UFGRecipe> Recipe);

	static void SweepSaveRestoredRecipes(UWorld& World);
};
