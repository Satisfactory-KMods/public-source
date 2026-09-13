// Copyright Kyri123 / KMods 2026. All Rights Reserved.



#pragma once

#include "CoreMinimal.h"
#include "KBFLCDOOverwriteBase.h"

#include "KBFLCDOCategoryOrder.generated.h"

class UFGBuildDescriptor;
class UFGBuildSubCategory;
class UFGBuildCategory;

UCLASS()
class KBFL_API UKBFLCDOCategoryOrder : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:

	UKBFLCDOCategoryOrder();

	virtual void ApplyToInstances() override;

	virtual bool ShouldCallForInstance(UClass* NewClass) override;

	virtual void ApplyToInstance(UObject* Instance) override;

	UPROPERTY(EditAnywhere, Category = "Category Settings")
	TSoftClassPtr<UFGBuildCategory> mTargetCategory;

	UPROPERTY(EditAnywhere, Category = "Category Settings")
	float mTargetCategoryPrio = -1000.f;

	UPROPERTY(EditAnywhere, Category = "SubCategory Settings")
	TSoftClassPtr<UFGBuildSubCategory> mSubTargetCategory;

	UPROPERTY(EditAnywhere, Category = "SubCategory Settings")
	float mSubTargetCategoryPrio = -1000.f;

	UPROPERTY(EditAnywhere, Category = "Descriptor Order")
	TArray<TSoftClassPtr<UFGBuildDescriptor>> mTargetOrder;
};
