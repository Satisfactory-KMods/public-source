// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGRecipe.h"
#include "FGSchematic.h"
#include "UObject/Object.h"
#include "KPCLRegistryObject.generated.h"

/**
 * Base for all descriptors in the game like resource, equipment etc.
 */
UCLASS(Blueprintable, Abstract, HideCategories=(Icon, Preview), meta=(AutoJSON=true))
class KPRIVATECODELIB_API UKPCLRegistryObject : public UObject
{
	GENERATED_BODY()

public:
	virtual TArray<TSubclassOf<UFGSchematic>> GetSchematicsToRegister();
	virtual TArray<TSubclassOf<UFGRecipe>> GetRecipesToRegister();

	static bool ShouldRegister(TSubclassOf<UKPCLRegistryObject> InClass);

protected:
	UPROPERTY(EditDefaultsOnly, Category="Dependencies")
	bool mShouldRegister = false;
};
