#pragma once

#include "CoreMinimal.h"
#include "KBFL_CDOHelperClass_Base.h"
#include "Resources/FGItemDescriptor.h"
#include "Resources/FGPowerShardDescriptor.h"

#include "KBFL_CDOHelperClass_Items.generated.h"

/**
 *@deprecated: use the new UKBFLCDOOverwrite system instead
 */
UCLASS()
class KBFL_API UKBFL_CDOHelperClass_Items : public UKBFL_CDOHelperClass_Base
{
	GENERATED_BODY()

public:
	virtual void DoCDO() override;
	virtual TArray<UClass*> GetClasses() override;

	/** must be set for CDO */
	UPROPERTY(EditAnywhere, Category = "CDO Helper")
	TArray<TSoftClassPtr<UFGItemDescriptor>> mItems;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mDisplayNameOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mDisplayNameOverride),
			  Category = "FG Item Descriptor")
	FText mDisplayName = FText::GetEmpty();

	UPROPERTY(meta = (NoAutoJson = true))
	bool mAbbreviatedDisplayNameOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mAbbreviatedDisplayNameOverride),
			  Category = "FG Item Descriptor")
	FText mAbbreviatedDisplayName = FText::GetEmpty();

	UPROPERTY(meta = (NoAutoJson = true))
	bool mDescriptionOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mDescriptionOverride, MultiLine = true),
			  Category = "FG Item Descriptor")
	FText mDescription = FText::GetEmpty();

	UPROPERTY(meta = (NoAutoJson = true))
	bool mStackSizeOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mStackSizeOverride),
			  Category = "FG Item Descriptor")
	EStackSize mStackSize = EStackSize::SS_MEDIUM;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mRememberPickUpOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mRememberPickUpOverride),
			  Category = "FG Item Descriptor")
	bool mRememberPickUp = false;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mCanBeDiscardedOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mCanBeDiscardedOverride),
			  Category = "FG Item Descriptor")
	bool mCanBeDiscarded = false;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mRadioactiveDecayOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
			  meta = (EditCondition = mRadioactiveDecayOverride, ClampMin = 0, UIMin = 0, UIMax = 1),
			  Category = "FG Item Descriptor")
	float mRadioactiveDecay = 0.0f;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mEnergyValueOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mEnergyValueOverride),
			  Category = "FG Item Descriptor")
	float mEnergyValue = 0.0f;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mFormOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mFormOverride), Category = "FG Item Descriptor")
	EResourceForm mForm = EResourceForm::RF_SOLID;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mConveyorMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mConveyorMeshOverride),
			  Category = "FG Item Descriptor")
	UStaticMesh* mConveyorMesh = nullptr;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mSmallIconOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mSmallIconOverride),
			  Category = "FG Item Descriptor")
	UTexture2D* mSmallIcon = nullptr;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mPersistentBigIconOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mPersistentBigIconOverride),
			  Category = "FG Item Descriptor")
	UTexture2D* mPersistentBigIcon = nullptr;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mCategoryOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mCategoryOverride),
			  Category = "FG Item Descriptor")
	TSubclassOf<UFGCategory> mCategory = nullptr;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mSubCategoriesOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mSubCategoriesOverride),
			  Category = "FG Item Descriptor")
	TArray<TSubclassOf<UFGCategory>> mSubCategories = {};

	UPROPERTY(meta = (NoAutoJson = true))
	bool mQuickSwitchGroupOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mQuickSwitchGroupOverride),
			  Category = "FG Item Descriptor")
	TSubclassOf<UFGQuickSwitchGroup> mQuickSwitchGroup = nullptr;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mFluidColorOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mFluidColorOverride),
			  Category = "FG Item Descriptor")
	FColor mFluidColor = FColor(0);

	UPROPERTY(meta = (NoAutoJson = true))
	bool mGasColorOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mGasColorOverride),
			  Category = "FG Item Descriptor")
	FColor mGasColor = FColor(0);

	UPROPERTY(meta = (NoAutoJson = true))
	bool mDepositMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mDepositMeshOverride),
			  Category = "FG Resource Descriptor")
	UStaticMesh* mDepositMesh = nullptr;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mDepositMaterialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mDepositMaterialOverride),
			  Category = "FG Resource Descriptor")
	UMaterialInstance* mDepositMaterial = nullptr;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mDecalSizeOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mDecalSizeOverride),
			  Category = "FG Resource Descriptor")
	float mDecalSize = 0.0f;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mCompassTextureOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mCompassTextureOverride),
			  Category = "FG Resource Descriptor")
	UTexture2D* mCompassTexture = nullptr;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mCollectSpeedMultiplierOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mCollectSpeedMultiplierOverride),
			  Category = "FG Resource Descriptor")
	float mCollectSpeedMultiplier = 0.0f;


	UPROPERTY(meta = (NoAutoJson = true))
	bool mSpentFuelClassOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mSpentFuelClassOverride),
			  Category = "FG Nuclear Fuel Descriptor")
	TSubclassOf<UFGItemDescriptor> mSpentFuelClass = nullptr;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mAmountOfWasteOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mAmountOfWasteOverride),
			  Category = "FG Nuclear Fuel Descriptor")
	int32 mAmountOfWaste = 1;

	// Power Shard
	UPROPERTY(meta = (NoAutoJson = true))
	bool mPowerShardTypeOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mPowerShardTypeOverride),
			  Category = "FG Power Shard Descriptor")
	EPowerShardType mPowerShardType;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mExtraPotentialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mExtraPotentialOverride),
			  Category = "FG Power Shard Descriptor")
	float mExtraPotential = 0.2f;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mExtraProductionBoostOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mExtraProductionBoostOverride),
			  Category = "FG Power Shard Descriptor")
	float mExtraProductionBoost = 0.2f;
};
