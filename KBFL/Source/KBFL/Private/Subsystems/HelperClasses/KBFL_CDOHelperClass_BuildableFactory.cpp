#pragma once
#include "Subsystems/HelperClasses/KBFL_CDOHelperClass_BuildableFactory.h"

#include "Buildables/FGBuildable.h"
#include "Buildables/FGBuildableFactory.h"
#include "Buildables/FGBuildableGeneratorFuel.h"
#include "Buildables/FGBuildableManufacturerVariablePower.h"
#include "Buildables/FGBuildablePipeline.h"
#include "Buildables/FGBuildablePortal.h"


class AFGBuildableGenerator;
class AFGBuildableGeneratorFuel;
DECLARE_LOG_CATEGORY_EXTERN(CDOHelperItemsLog, Log, All)

DEFINE_LOG_CATEGORY(CDOHelperItemsLog)

void UKBFL_CDOHelperClass_BuildableFactory::DoCDO()
{
	for (TSubclassOf<AFGBuildableFactory> Class : GetClasses())
	{
		if (Class)
		{
			if (AFGBuildableFactory* DefaultObject = Class.GetDefaultObject())
			{
				if (mPowerConsumptionOverride)
				{
					DefaultObject->mPowerConsumption = this->mPowerConsumption;
				}

				if (mPowerConsumptionExponentOverride)
				{
					DefaultObject->mPowerConsumptionExponent = this->mPowerConsumptionExponent;
				}

				if (mDoesHaveShutdownAnimationOverride)
				{
					DefaultObject->mDoesHaveShutdownAnimation = this->mDoesHaveShutdownAnimation;
				}

				if (mMinimumProducingTimeOverride)
				{
					DefaultObject->mMinimumProducingTime = this->mMinimumProducingTime;
				}

				if (mFluidStackSizeMultiplierOverride)
				{
					DefaultObject->mFluidStackSizeMultiplier = this->mFluidStackSizeMultiplier;
				}

				if (mMinimumStoppedTimeOverride)
				{
					DefaultObject->mMinimumStoppedTime = this->mMinimumStoppedTime;
				}

				if (mCanChangePotentialnOverride)
				{
					DefaultObject->mCanChangePotential = this->mCanChangePotential;
				}

				if (mMinPotentialOverride)
				{
					DefaultObject->mMinPotential = this->mMinPotential;
				}

				if (mMaxPotentialOverride)
				{
					DefaultObject->mMaxPotential = this->mMaxPotential;
				}

				if (mBaseProductionBoostOverride)
				{
					DefaultObject->mBaseProductionBoost = this->mBaseProductionBoost;
				}

				if (mFluidStackSizeDefaultOverride)
				{
					DefaultObject->mFluidStackSizeDefault = this->mFluidStackSizeDefault;
				}

				if (mAddToSignificanceManagerOverride)
				{
					DefaultObject->mAddToSignificanceManager = this->mAddToSignificanceManager;
				}

				if (mSignificanceRangeOverride)
				{
					DefaultObject->mSignificanceRange = this->mSignificanceRange;
				}

				if (mPotentialShardSlotsOverride)
				{
					DefaultObject->mPotentialShardSlots = this->mPotentialShardSlots;
				}

				if (mProductionShardSlotSizeOverride)
				{
					DefaultObject->mProductionShardSlotSize = this->mProductionShardSlotSize;
				}

				if (mProductionShardBoostMultiplierOverride)
				{
					DefaultObject->mProductionShardBoostMultiplier = this->mProductionShardBoostMultiplier;
				}

				if (mOverridePotentialShardSlotsOverride)
				{
					DefaultObject->mOverridePotentialShardSlots = this->mOverridePotentialShardSlots;
				}

				if (mOverrideProductionShardSlotSizeOverride)
				{
					DefaultObject->mOverrideProductionShardSlotSize = this->mOverrideProductionShardSlotSize;
				}

				if (AFGBuildablePortal* Portal = Cast<AFGBuildablePortal>(DefaultObject))
				{
					if (mHeatUpCycleTimeOverride)
					{
						Portal->mHeatUpCycleTime = this->mHeatUpCycleTime;
					}

					if (mPortalDisconnectedCooldownTimeOverride)
					{
						Portal->mPortalDisconnectedCooldownTime = this->mPortalDisconnectedCooldownTime;
					}

					if (mMinFuelToStartProducingAfterAbruptStopOverride)
					{
						Portal->mMinFuelToStartProducingAfterAbruptStop = this->mMinFuelToStartProducingAfterAbruptStop;
					}

					if (mMinFuelToStartProducingOverride)
					{
						Portal->mMinFuelToStartProducing = this->mMinFuelToStartProducing;
					}

					if (mFuelSlotSizeOverride)
					{
						Portal->mFuelSlotSize = this->mFuelSlotSize;
					}

					if (mFuelItemClassOverride && IsValid(this->mFuelItemClass))
					{
						Portal->mFuelItemClass = this->mFuelItemClass;
					}

					if (mHeatUpFuelConsumptionCurveOverride && IsValid(this->mHeatUpFuelConsumptionCurve))
					{
						Portal->mHeatUpFuelConsumptionCurve = this->mHeatUpFuelConsumptionCurve;
					}

					if (mHeatUpPowerConsumptionCurveOverride && IsValid(this->mHeatUpPowerConsumptionCurve))
					{
						Portal->mHeatUpPowerConsumptionCurve = this->mHeatUpPowerConsumptionCurve;
					}
				}

				if (AFGBuildableManufacturerVariablePower* VariablePower =
						Cast<AFGBuildableManufacturerVariablePower>(DefaultObject))
				{
					if (mEstimatedMaximumPowerConsumptionOverride)
					{
						VariablePower->mEstimatedMaximumPowerConsumption = this->mEstimatedMaximumPowerConsumption;
					}

					if (mEstimatedMininumPowerConsumptionOverride)
					{
						VariablePower->mEstimatedMininumPowerConsumption = this->mEstimatedMininumPowerConsumption;
					}

					if (mPowerConsumptionCurveOverride && IsValid(this->mPowerConsumptionCurve))
					{
						VariablePower->mPowerConsumptionCurve = this->mPowerConsumptionCurve;
					}
				}

				if (AFGBuildableGenerator* Generator = Cast<AFGBuildableGenerator>(DefaultObject))
				{
					if (mPowerProductionOverride)
					{
						Generator->mPowerProduction = this->mPowerProduction;
					}

					if (AFGBuildableGeneratorFuel* GeneratorFuel = Cast<AFGBuildableGeneratorFuel>(Generator))
					{
						if (mDefaultFuelClassesOverride)
						{
							GeneratorFuel->mDefaultFuelClasses = this->mDefaultFuelClasses;
						}

						if (mFuelLoadAmountOverride)
						{
							GeneratorFuel->mFuelLoadAmount = this->mFuelLoadAmount;
						}

						if (mSupplementalLoadAmountOverride)
						{
							GeneratorFuel->mSupplementalLoadAmount = this->mSupplementalLoadAmount;
						}

						if (mSupplementalToPowerRatioOverride)
						{
							GeneratorFuel->mSupplementalToPowerRatio = this->mSupplementalToPowerRatio;
						}

						if (mIsFullBlastOverride)
						{
							GeneratorFuel->mIsFullBlast = this->mIsFullBlast;
						}
					}
				}
			}
		}
	}

	Super::DoCDO();
}
