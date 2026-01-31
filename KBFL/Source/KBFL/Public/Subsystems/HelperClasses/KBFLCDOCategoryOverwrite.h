//

#pragma once

#include "CoreMinimal.h"
#include "FGItemCategory.h"
#include "FGRecipe.h"
#include "KBFLCDOOverwriteBase.h"

#include "KBFLCDOCategoryOverwrite.generated.h"

class UFGBuildDescriptor;
class UFGBuildSubCategory;
class UFGBuildCategory;

/**
 *
 */
UCLASS()
class KBFL_API UKBFLCDOCategoryOverwrite : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:
	UKBFLCDOCategoryOverwrite();
	virtual void ApplyToInstances() override;

	/** must be set for CDO */
	UPROPERTY(EditAnywhere, Category = "CDO")
	TArray<TSoftClassPtr<UFGItemDescriptor>> mItems;

	UPROPERTY(EditAnywhere, Category = "CDO")
	TArray<TSoftClassPtr<UFGRecipe>> mRecipes;

	UPROPERTY(EditAnywhere, Category = "CDO")
	TSoftClassPtr<UFGItemCategory> mToCategory;
};
