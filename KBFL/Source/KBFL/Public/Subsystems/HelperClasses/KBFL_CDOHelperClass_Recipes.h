#pragma once

#include "CoreMinimal.h"
#include "FGRecipe.h"
#include "KBFL_CDOHelperClass_Base.h"
#include "KBFL_CDOHelperClass_Recipes.generated.h"

/**
 *@deprecated: use the new UKBFLCDOOverwrite system instead
 */
UCLASS()
class KBFL_API UKBFL_CDOHelperClass_Recipes : public UKBFL_CDOHelperClass_Base
{
	GENERATED_BODY()

public:
	virtual void DoCDO() override;
	virtual TArray<UClass*> GetClasses() override;

	/** must be set for CDO */
	UPROPERTY(EditDefaultsOnly, Category = "CDO Helper")
	TArray<TSoftClassPtr<UFGRecipe>> mRecipes;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mDisplayNameOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mDisplayNameOverride),
			  Category = "FG Recipe")
	FText mDisplayName = FText::GetEmpty();

	UPROPERTY(meta = (NoAutoJson = true))
	bool mIngredientsOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mIngredientsOverride),
			  Category = "FG Recipe")
	TArray<FItemAmount> mIngredients = {};

	UPROPERTY(meta = (NoAutoJson = true))
	bool mProductOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mProductOverride), Category = "FG Recipe")
	TArray<FItemAmount> mProduct = {};

	UPROPERTY(meta = (NoAutoJson = true))
	bool mOverriddenCategoryOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mOverriddenCategoryOverride),
			  Category = "FG Recipe")
	TSubclassOf<class UFGItemCategory> mOverriddenCategory = nullptr;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mManufacturingMenuPriorityOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mManufacturingMenuPriorityOverride),
			  Category = "FG Recipe")
	float mManufacturingMenuPriority = 0.0f;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mManufactoringDurationOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mManufactoringDurationOverride),
			  Category = "FG Recipe")
	float mManufactoringDuration = 0.0f;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mManualManufacturingMultiplierOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mManualManufacturingMultiplierOverride),
			  Category = "FG Recipe")
	float mManualManufacturingMultiplier = 0.0f;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mProducedInOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mProducedInOverride),
			  Category = "FG Recipe")
	TArray<TSoftClassPtr<UObject>> mProducedIn = {};

	UPROPERTY(meta = (NoAutoJson = true))
	bool mMaterialCustomizationRecipeOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mMaterialCustomizationRecipeOverride),
			  Category = "FG Recipe")
	TSubclassOf<class UFGCustomizationRecipe> mMaterialCustomizationRecipe = nullptr;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mRelevantEventsOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mRelevantEventsOverride),
			  Category = "FG Recipe")
	TArray<EEvents> mRelevantEvents = {};

	UPROPERTY(meta = (NoAutoJson = true))
	bool mVariablePowerConsumptionConstantOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mVariablePowerConsumptionConstantOverride),
			  Category = "FG Recipe")
	float mVariablePowerConsumptionConstant = 0.0f;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mVariablePowerConsumptionFactorOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mVariablePowerConsumptionFactorOverride),
			  Category = "FG Recipe")
	float mVariablePowerConsumptionFactor = 0.0f;
};
