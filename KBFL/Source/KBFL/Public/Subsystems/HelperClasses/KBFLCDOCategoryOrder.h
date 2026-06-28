//

#pragma once

#include "CoreMinimal.h"
#include "KBFLCDOOverwriteBase.h"

#include "KBFLCDOCategoryOrder.generated.h"

class UFGBuildDescriptor;
class UFGBuildSubCategory;
class UFGBuildCategory;

/**
 *
 */
UCLASS()
class KBFL_API UKBFLCDOCategoryOrder : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:
	UKBFLCDOCategoryOrder();
	virtual void ApplyToInstances() override;

	// Lazy-load path: re-apply to a single class loaded after the initial CDO pass.
	virtual bool ShouldCallForInstance(UClass* NewClass) override;
	virtual void ApplyToInstance(UObject* Instance) override;

	// ===== Category Settings =====
	/** Target build category to reorder */
	UPROPERTY(EditAnywhere, Category = "Category Settings")
	TSoftClassPtr<UFGBuildCategory> mTargetCategory;

	/** Priority for the target category (lower values = higher priority) */
	UPROPERTY(EditAnywhere, Category = "Category Settings")
	float mTargetCategoryPrio = -1000.f;

	// ===== SubCategory Settings =====
	/** Target sub-category to reorder */
	UPROPERTY(EditAnywhere, Category = "SubCategory Settings")
	TSoftClassPtr<UFGBuildSubCategory> mSubTargetCategory;

	/** Priority for the target sub-category (lower values = higher priority) */
	UPROPERTY(EditAnywhere, Category = "SubCategory Settings")
	float mSubTargetCategoryPrio = -1000.f;

	// ===== Descriptor Order =====
	/** Desired order of build descriptors within the category */
	UPROPERTY(EditAnywhere, Category = "Descriptor Order")
	TArray<TSoftClassPtr<UFGBuildDescriptor>> mTargetOrder;
};
