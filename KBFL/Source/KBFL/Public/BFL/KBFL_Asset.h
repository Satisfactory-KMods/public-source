#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "KBFL_Asset.generated.h"

/**
 *
 */
UCLASS()
class KBFL_API UKBFL_Asset : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KMods|Items", CallInEditor)
	static void MarkDefaultDirty(UClass* Class);

	UFUNCTION(BlueprintCallable, Category = "KMods|Recipes")
	static bool HasProducer(TSubclassOf<UFGRecipe> Recipe, TSubclassOf<UObject> Producer);
};
