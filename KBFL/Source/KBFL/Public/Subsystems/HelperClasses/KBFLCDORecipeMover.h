//

#pragma once

#include "CoreMinimal.h"
#include "FGRecipe.h"
#include "KBFLCDOOverwriteBase.h"
#include "KBFLCDORecipeMover.generated.h"

/**
 *
 */
UCLASS()
class KBFL_API UKBFLCDORecipeMover : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:
	/**
	 * Moves every recipe produced in mSourceTarget over to the mProducedIn producers (replacing or merging the existing
	 * producer set per bReplace), skipping any recipes listed in mRecipesToIgnore.
	 */
	virtual void ApplyToInstances() override;

	// Lazy-load path: re-apply to a single recipe class loaded after the initial CDO pass.
	virtual bool ShouldCallForInstance(UClass* NewClass) override;

	/** Lazy-load handler: re-targets a single loaded recipe's producers from mSourceTarget to mProducedIn. */
	virtual void ApplyToInstance(UObject* Instance) override;

	// ===== Source Settings =====
	/** Source producer to move recipes from (must implement FGRecipeProducerInterface) */
	UPROPERTY(EditAnywhere, Meta = (MustImplement = "FGRecipeProducerInterface"), Category = "Source Settings")
	TSoftClassPtr<UObject> mSourceTarget;

	// ===== Target Settings =====
	/** Target producers to move recipes to (must implement FGRecipeProducerInterface) */
	UPROPERTY(EditAnywhere, Meta = (MustImplement = "FGRecipeProducerInterface"), Category = "Target Settings")
	TArray<TSoftClassPtr<UObject>> mProducedIn;

	/** If true, replace existing recipes in target producers. If false, merge with existing recipes */
	UPROPERTY(EditAnywhere, Category = "Target Settings")
	bool bReplace = true;

	// ===== Exclusions =====
	/** Recipes to ignore when moving (won't be moved) */
	UPROPERTY(EditAnywhere, Category = "Exclusions")
	TArray<TSoftClassPtr<UFGRecipe>> mRecipesToIgnore;
};
