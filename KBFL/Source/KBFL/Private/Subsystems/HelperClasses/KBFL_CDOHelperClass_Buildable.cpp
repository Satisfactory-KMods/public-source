#pragma once
#include "Subsystems/HelperClasses/KBFL_CDOHelperClass_Buildable.h"

#include "Buildables/FGBuildable.h"
#include "Buildables/FGBuildablePipeline.h"
#include "FGSwatchGroup.h"
#include "Hologram/FGHologram.h"

void UKBFL_CDOHelperClass_Buildable::DoCDO()
{
	for (TSubclassOf<AFGBuildable> Class : GetClasses())
	{
		if (Class)
		{
			if (AFGBuildable* DefaultObject = Class.GetDefaultObject())
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

				if (mHologramClassOverride)
				{
					if (IsValid(mHologramClass))
					{
						// mHologramClass->AddToRoot();
					}

					DefaultObject->mHologramClass = this->mHologramClass;
				}

				if (mDecoratorClassOverride)
				{
					if (IsValid(mDecoratorClass))
					{
						// mDefaultSwatchCustomizationOverride->AddToRoot();
					}

					DefaultObject->mDecoratorClass = this->mDecoratorClass;
				}

				if (mDefaultSwatchCustomizationOverrideOverride)
				{
					if (IsValid(mDefaultSwatchCustomizationOverride))
					{
						// mDefaultSwatchCustomizationOverride->AddToRoot();
					}

					DefaultObject->mDefaultSwatchCustomizationOverride = this->mDefaultSwatchCustomizationOverride;
				}

				if (mSwatchGroupOverride)
				{
					if (IsValid(mSwatchGroup))
					{
						// mSwatchGroup->AddToRoot();
					}

					DefaultObject->mSwatchGroup = this->mSwatchGroup;
				}

				if (mAllowColoringOverride)
				{
					DefaultObject->mAllowColoring = this->mAllowColoring;
				}

				if (mAllowPatterningOverride)
				{
					DefaultObject->mAllowPatterning = this->mAllowPatterning;
				}

				if (mFactorySkinClassOverride)
				{
					if (IsValid(mFactorySkinClass))
					{
						// mFactorySkinClass->AddToRoot();
					}

					DefaultObject->mFactorySkinClass = this->mFactorySkinClass;
				}

				if (mSkipBuildEffectOverride)
				{
					DefaultObject->mSkipBuildEffect = this->mSkipBuildEffect;
				}

				if (mBuildEffectSpeedOverride)
				{
					DefaultObject->mBuildEffectSpeed = this->mBuildEffectSpeed;
				}
			}
		}
	}

	Super::DoCDO();
}

TArray<UClass*> UKBFL_CDOHelperClass_Buildable::GetClasses()
{
	TArray<UClass*> Re;

	for (TSoftClassPtr<AFGBuildable> Class : mClasses)
	{
		if (IsValidSoftClass(Class))
		{
			Re.Add(Class.LoadSynchronous());
		}
	}

	return Re;
}
