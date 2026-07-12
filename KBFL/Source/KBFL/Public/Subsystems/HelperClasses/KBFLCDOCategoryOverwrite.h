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
	/** Raises the call priority (mCallPrio = 100000) so recategorization runs late, after other CDO passes. */
	UKBFLCDOCategoryOverwrite();

	/** Moves all configured items and recipes into mToCategory, assigning each a sequential menu priority. */
	virtual void ApplyToInstances() override;

	// Lazy-load path: re-apply to a single class loaded after the initial CDO pass.
	virtual bool ShouldCallForInstance(UClass* NewClass) override;

	/** Lazy-load handler: applies mToCategory and the matching menu-priority index to a single loaded item or recipe.
	 */
	virtual void ApplyToInstance(UObject* Instance) override;

	// ===== Target Category =====
	/** Target category to assign items and recipes to */
	UPROPERTY(EditAnywhere, Category = "Target Category")
	TSoftClassPtr<UFGItemCategory> mToCategory;

	// ===== Items to Recategorize =====
	/** Item descriptors to move to the target category */
	UPROPERTY(EditAnywhere, Category = "Items to Recategorize")
	TArray<TSoftClassPtr<UFGItemDescriptor>> mItems;

	// ===== Recipes to Recategorize =====
	/** Recipes to move to the target category */
	UPROPERTY(EditAnywhere, Category = "Recipes to Recategorize")
	TArray<TSoftClassPtr<UFGRecipe>> mRecipes;
};
