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

	/** must be set for CDO */
	UPROPERTY(EditAnywhere, Category = "CDO")
	TSoftClassPtr<UFGBuildCategory> mTargetCategory;

	UPROPERTY(EditAnywhere, Category = "CDO")
	float mTargetCategoryPrio = -1000.f;

	/** must be set for CDO */
	UPROPERTY(EditAnywhere, Category = "CDO")
	TSoftClassPtr<UFGBuildSubCategory> mSubTargetCategory;

	UPROPERTY(EditAnywhere, Category = "CDO")
	float mSubTargetCategoryPrio = -1000.f;

	/** must be set for CDO */
	UPROPERTY(EditAnywhere, Category = "CDO")
	TArray<TSoftClassPtr<UFGBuildDescriptor>> mTargetOrder;
};
