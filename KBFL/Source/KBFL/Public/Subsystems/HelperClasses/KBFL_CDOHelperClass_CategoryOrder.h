#pragma once

#include "CoreMinimal.h"
#include "FGBuildCategory.h"
#include "FGBuildSubCategory.h"
#include "KBFL_CDOHelperClass_Base.h"
#include "Resources/FGBuildDescriptor.h"
#include "Resources/FGBuildingDescriptor.h"

#include "KBFL_CDOHelperClass_CategoryOrder.generated.h"

/**
 *@deprecated: use the new UKBFLCDOOverwrite system instead
 * This CDO should call in the Main Menu!
 */
UCLASS()
class KBFL_API UKBFL_CDOHelperClass_CategoryOrder : public UKBFL_CDOHelperClass_Base
{
	GENERATED_BODY()

public:
	virtual void DoCDO() override;
	virtual TArray<UClass*> GetClasses() override;
	/** must be set for CDO */
	UPROPERTY(EditAnywhere, Category = "CDO Helper")
	TSoftClassPtr<UFGBuildCategory> mTargetCategory;

	UPROPERTY(EditAnywhere, Category = "CDO Helper")
	float mTargetCategoryPrio = -1000.f;

	/** must be set for CDO */
	UPROPERTY(EditAnywhere, Category = "CDO Helper")
	TSoftClassPtr<UFGBuildSubCategory> mSubTargetCategory;

	UPROPERTY(EditAnywhere, Category = "CDO Helper")
	float mSubTargetCategoryPrio = -1000.f;

	/** must be set for CDO */
	UPROPERTY(EditAnywhere, Category = "CDO Helper")
	TArray<TSoftClassPtr<UFGBuildDescriptor>> mTargetOrder;
};
