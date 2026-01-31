#pragma once

#include "CoreMinimal.h"
#include "KBFL_CDOHelperClass_Buildable.h"
#include "KBFL_CDOHelperClass_BuildableFactory.generated.h"

/**
 *@deprecated: use the new UKBFLCDOOverwrite system instead
 * This CDO should call in the Main Menu!
 */
UCLASS()
class KBFL_API UKBFL_CDOHelperClass_BuildableFactory : public UKBFL_CDOHelperClass_Buildable
{
	GENERATED_BODY()

public:
	virtual void DoCDO() override;

	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mPowerConsumptionOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mPowerConsumptionOverride),
			  Category = "FGBuildableFactory")
	float mPowerConsumption;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mPowerConsumptionExponentOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mPowerConsumptionOverride),
			  Category = "FGBuildableFactory")
	float mPowerConsumptionExponent;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mDoesHaveShutdownAnimationOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mDoesHaveShutdownAnimationOverride),
			  Category = "FGBuildableFactory")
	bool mDoesHaveShutdownAnimation;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mMinimumProducingTimeOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mMinimumProducingTimeOverride),
			  Category = "FGBuildableFactory")
	float mMinimumProducingTime;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mMinimumStoppedTimeOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mMinimumStoppedTimeOverride),
			  Category = "FGBuildableFactory")
	float mMinimumStoppedTime;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mCanChangePotentialnOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mCanChangePotentialnOverride),
			  Category = "FGBuildableFactory")
	bool mCanChangePotential;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mMinPotentialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mMinPotentialOverride),
			  Category = "FGBuildableFactory")
	float mMinPotential;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mMaxPotentialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mMaxPotentialOverride),
			  Category = "FGBuildableFactory")
	float mMaxPotential;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mBaseProductionBoostOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mBaseProductionBoostOverride),
			  Category = "FGBuildableFactory")
	float mBaseProductionBoost;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mFluidStackSizeDefaultOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mFluidStackSizeDefaultOverride),
			  Category = "FGBuildableFactory")
	EStackSize mFluidStackSizeDefault = EStackSize::SS_FLUID;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mFluidStackSizeMultiplierOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mFluidStackSizeMultiplierOverride),
			  Category = "FGBuildableFactory")
	int32 mFluidStackSizeMultiplier = 4;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mAddToSignificanceManagerOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mAddToSignificanceManagerOverride),
			  Category = "FGBuildableFactory")
	bool mAddToSignificanceManager = true;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mSignificanceRangeOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mSignificanceRangeOverride),
			  Category = "FGBuildableFactory")
	float mSignificanceRange;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mPotentialShardSlotsOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mPotentialShardSlotsOverride),
			  Category = "FGBuildableFactory")
	int32 mPotentialShardSlots;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mProductionShardSlotSizeOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mProductionShardSlotSizeOverride),
			  Category = "FGBuildableFactory")
	int32 mProductionShardSlotSize;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mProductionShardBoostMultiplierOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mProductionShardBoostMultiplierOverride),
			  Category = "FGBuildableFactory")
	float mProductionShardBoostMultiplier;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mOverridePotentialShardSlotsOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mOverridePotentialShardSlotsOverride),
			  Category = "FGBuildableFactory")
	uint8 mOverridePotentialShardSlots : 1;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mOverrideProductionShardSlotSizeOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mOverrideProductionShardSlotSizeOverride),
			  Category = "FGBuildableFactory")
	uint8 mOverrideProductionShardSlotSize : 1;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mEstimatedMininumPowerConsumptionOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mEstimatedMininumPowerConsumptionOverride),
			  Category = "FGBuildableManufacturerVariablePower")
	float mEstimatedMininumPowerConsumption = 0;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mEstimatedMaximumPowerConsumptionOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mEstimatedMaximumPowerConsumptionOverride),
			  Category = "FGBuildableManufacturerVariablePower")
	float mEstimatedMaximumPowerConsumption = 5000;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mPowerConsumptionCurveOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mPowerConsumptionCurveOverride),
			  Category = "FGBuildableManufacturerVariablePower")
	UCurveFloat* mPowerConsumptionCurve = nullptr;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mPowerProductionOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mPowerProductionOverride),
			  Category = "AFGBuildableGenerator")
	float mPowerProduction = 1.f;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mDefaultFuelClassesOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mDefaultFuelClassesOverride),
			  Category = "AFGBuildableGenerator|AFGBuildableGeneratorFuel")
	TArray<TSoftClassPtr<UFGItemDescriptor>> mDefaultFuelClasses;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mFuelLoadAmountOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mFuelLoadAmountOverride),
			  Category = "AFGBuildableGenerator|AFGBuildableGeneratorFuel")
	int32 mFuelLoadAmount = 1;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mSupplementalLoadAmountOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mSupplementalLoadAmountOverride),
			  Category = "AFGBuildableGenerator|AFGBuildableGeneratorFuel")
	int32 mSupplementalLoadAmount = 1;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mSupplementalToPowerRatioOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mSupplementalToPowerRatioOverride),
			  Category = "AFGBuildableGenerator|AFGBuildableGeneratorFuel")
	float mSupplementalToPowerRatio = 1.f;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mIsFullBlastOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mIsFullBlastOverride),
			  Category = "AFGBuildableGenerator|AFGBuildableGeneratorFuel")
	bool mIsFullBlast;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mFuelItemClassOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mFuelItemClassOverride),
			  Category = "AFGBuildablePortal")
	TSubclassOf<UFGItemDescriptor> mFuelItemClass = nullptr;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mFuelSlotSizeOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mFuelSlotSizeOverride),
			  Category = "AFGBuildablePortal")
	int32 mFuelSlotSize;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mMinFuelToStartProducingOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mMinFuelToStartProducingOverride),
			  Category = "AFGBuildablePortal")
	int32 mMinFuelToStartProducing;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mMinFuelToStartProducingAfterAbruptStopOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
			  meta = (EditCondition = mMinFuelToStartProducingAfterAbruptStopOverride), Category = "AFGBuildablePortal")
	int32 mMinFuelToStartProducingAfterAbruptStop;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mPortalDisconnectedCooldownTimeOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mPortalDisconnectedCooldownTimeOverride),
			  Category = "AFGBuildablePortal")
	float mPortalDisconnectedCooldownTime;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mHeatUpCycleTimeOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mHeatUpCycleTimeOverride),
			  Category = "AFGBuildablePortal")
	float mHeatUpCycleTime;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mHeatUpFuelConsumptionCurveOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mHeatUpFuelConsumptionCurveOverride),
			  Category = "AFGBuildablePortal")
	UCurveFloat* mHeatUpFuelConsumptionCurve;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mHeatUpPowerConsumptionCurveOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mHeatUpPowerConsumptionCurveOverride),
			  Category = "AFGBuildablePortal")
	UCurveFloat* mHeatUpPowerConsumptionCurve;
};
