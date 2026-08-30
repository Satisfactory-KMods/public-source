

#pragma once

#include "CoreMinimal.h"
#include "FGItemCategory.h"
#include "FGRecipe.h"
#include "KBFLCDOOverwriteBase.h"

#include "KBFLCDOCategoryOverwrite.generated.h"

class UFGBuildDescriptor;
class UFGBuildSubCategory;
class UFGBuildCategory;

UCLASS()
class KBFL_API UKBFLCDOCategoryOverwrite : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:

	UKBFLCDOCategoryOverwrite();

	virtual void ApplyToInstances() override;

	virtual bool ShouldCallForInstance(UClass* NewClass) override;

	virtual void ApplyToInstance(UObject* Instance) override;

	UPROPERTY(EditAnywhere, Category = "Target Category")
	TSoftClassPtr<UFGItemCategory> mToCategory;

	UPROPERTY(EditAnywhere, Category = "Items to Recategorize")
	TArray<TSoftClassPtr<UFGItemDescriptor>> mItems;

	UPROPERTY(EditAnywhere, Category = "Recipes to Recategorize")
	TArray<TSoftClassPtr<UFGRecipe>> mRecipes;
};
