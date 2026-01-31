#pragma once
#include "Subsystems/HelperClasses/KBFL_CDOHelperClass_Items.h"

#include "FGCategory.h"
#include "FGQuickSwitchGroup.h"
#include "Resources/FGItemDescriptorNuclearFuel.h"
#include "Resources/FGPowerShardDescriptor.h"
#include "Resources/FGResourceDescriptor.h"

void UKBFL_CDOHelperClass_Items::DoCDO()
{
	for (TSubclassOf<UFGItemDescriptor> Class : GetClasses())
	{
		if (Class)
		{
			if (UFGItemDescriptor* DefaultObject = Class.GetDefaultObject())
			{
				// DefaultObject->AddToRoot();
				if (mDisplayNameOverride)
				{
					DefaultObject->mDisplayName = this->mDisplayName;
				}

				if (mDescriptionOverride)
				{
					DefaultObject->mDescription = this->mDescription;
				}

				if (mAbbreviatedDisplayNameOverride)
				{
					DefaultObject->mAbbreviatedDisplayName = this->mAbbreviatedDisplayName;
				}

				if (mStackSizeOverride)
				{
					DefaultObject->mStackSize = this->mStackSize;
					DefaultObject->mCachedStackSize = UFGItemDescriptor::GetStackSize(Class);
				}

				if (mRememberPickUpOverride)
				{
					DefaultObject->mRememberPickUp = this->mRememberPickUp;
				}

				if (mCanBeDiscardedOverride)
				{
					DefaultObject->mCanBeDiscarded = this->mCanBeDiscarded;
				}

				if (mRadioactiveDecayOverride)
				{
					DefaultObject->mRadioactiveDecay = this->mRadioactiveDecay;
				}

				if (mEnergyValueOverride)
				{
					DefaultObject->mEnergyValue = this->mEnergyValue;
				}

				if (mFormOverride)
				{
					DefaultObject->mForm = this->mForm;
				}

				if (mConveyorMeshOverride)
				{
					DefaultObject->mConveyorMesh = this->mConveyorMesh;
				}

				if (mSmallIconOverride)
				{
					DefaultObject->mSmallIcon = this->mSmallIcon;
				}

				if (mPersistentBigIconOverride)
				{
					DefaultObject->mPersistentBigIcon = this->mPersistentBigIcon;
				}

				if (mCategoryOverride)
				{
					if (IsValid(mCategory))
					{
						// mCategory->AddToRoot();
					}

					DefaultObject->mCategory = this->mCategory;
				}

				if (mSubCategoriesOverride)
				{
					for (UClass* SubCategory : this->mSubCategories)
					{
						if (IsValid(SubCategory))
						{
							// SubCategory->AddToRoot();
						}
					}

					DefaultObject->mSubCategories = this->mSubCategories;
				}

				if (mQuickSwitchGroupOverride)
				{
					if (IsValid(mQuickSwitchGroup))
					{
						// mQuickSwitchGroup->AddToRoot();
					}

					DefaultObject->mQuickSwitchGroup = this->mQuickSwitchGroup;
				}

				if (mFluidColorOverride)
				{
					DefaultObject->mFluidColor = this->mFluidColor;
				}

				if (mGasColorOverride)
				{
					DefaultObject->mGasColor = this->mGasColor;
				}

				UFGResourceDescriptor* ResDefault = Cast<UFGResourceDescriptor>(DefaultObject);
				if (ResDefault)
				{
					if (mDepositMeshOverride)
					{
						ResDefault->mDepositMesh = this->mDepositMesh;
					}

					if (mDepositMaterialOverride)
					{
						if (IsValid(mDepositMaterial))
						{
							// mDepositMaterial->AddToRoot();
						}

						ResDefault->mDepositMaterial = this->mDepositMaterial;
					}

					if (mDecalSizeOverride)
					{
						ResDefault->mDecalSize = this->mDecalSize;
					}

					if (mCompassTextureOverride)
					{
						if (IsValid(mCompassTexture))
						{
							// mCompassTexture->AddToRoot();
						}

						ResDefault->mCompassTexture = this->mCompassTexture;
					}

					if (mCollectSpeedMultiplierOverride)
					{
						ResDefault->mCollectSpeedMultiplier = this->mCollectSpeedMultiplier;
					}
				}

				if (UFGItemDescriptorNuclearFuel* NuclearFuelDefault =
						Cast<UFGItemDescriptorNuclearFuel>(DefaultObject))
				{
					if (mSpentFuelClassOverride)
					{
						if (IsValid(mSpentFuelClass))
						{
							// mSpentFuelClass->AddToRoot();
						}

						NuclearFuelDefault->mSpentFuelClass = this->mSpentFuelClass;
					}

					if (mAmountOfWasteOverride)
					{
						NuclearFuelDefault->mAmountOfWaste = this->mAmountOfWaste;
					}
				}

				if (UFGPowerShardDescriptor* ShardDescriptor = Cast<UFGPowerShardDescriptor>(DefaultObject))
				{
					if (this->mPowerShardTypeOverride)
					{
						ShardDescriptor->mPowerShardType = this->mPowerShardType;
					}
					if (this->mExtraPotentialOverride)
					{
						ShardDescriptor->mExtraPotential = this->mExtraPotential;
					}
					if (this->mExtraProductionBoostOverride)
					{
						ShardDescriptor->mExtraProductionBoost = this->mExtraProductionBoost;
					}
				}
			}
		}
	}

	Super::DoCDO();
}

TArray<UClass*> UKBFL_CDOHelperClass_Items::GetClasses()
{
	TArray<UClass*> Re;

	for (TSoftClassPtr<UFGItemDescriptor> Class : mItems)
	{
		if (IsValidSoftClass(Class))
		{
			Re.Add(Class.LoadSynchronous());
		}
	}

	return Re;
}
