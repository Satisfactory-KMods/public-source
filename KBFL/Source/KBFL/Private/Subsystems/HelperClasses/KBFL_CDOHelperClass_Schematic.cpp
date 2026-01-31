#pragma once
#include "Subsystems/HelperClasses/KBFL_CDOHelperClass_Schematic.h"

#include "FGSchematicCategory.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Unlocks/FGUnlock.h"


DECLARE_LOG_CATEGORY_EXTERN(CDOHelperSchematicLog, Log, All)

DEFINE_LOG_CATEGORY(CDOHelperSchematicLog)

void UKBFL_CDOHelperClass_Schematic::DoCDO()
{
	UE_LOG(CDOHelperSchematicLog, Log, TEXT("CDOHelperSchematic > Called %s"), *this->GetName());
	for (TSubclassOf<UFGSchematic> Class : GetClasses())
	{
		if (UFGSchematic* DefaultObject = Class.GetDefaultObject())
		{
			// DefaultObject->AddToRoot();
			UE_LOG(CDOHelperSchematicLog, Log, TEXT("CDOHelperSchematic > DoSchematicCDO > %s"),
				   *DefaultObject->GetName());
			if (mTypeOverride)
			{
				DefaultObject->mType = mType;
			}

			if (mDisplayNameOverride)
			{
				DefaultObject->mDisplayName = mDisplayName;
			}

			if (mDescriptionOverride)
			{
				DefaultObject->mDescription = mDescription;
			}

			if (mSchematicCategoryOverride)
			{
				if (IsValid(mSchematicCategory))
				{
					// mSchematicCategory->AddToRoot();
				}

				DefaultObject->mSchematicCategory = mSchematicCategory;
			}

			if (mSubCategoriesOverride)
			{
				for (UClass* SubCategory : mSubCategories)
				{
					if (IsValid(SubCategory))
					{
						// SubCategory->AddToRoot();
					}
				}

				DefaultObject->mSubCategories = mSubCategories;
			}

			if (mMenuPriorityOverride)
			{
				DefaultObject->mMenuPriority = mMenuPriority;
			}

			if (mTechTierOverride)
			{
				DefaultObject->mTechTier = mTechTier;
			}

			if (mCostOverride)
			{
				DefaultObject->mCost = mCost;
			}

			if (mTimeToCompleteOverride)
			{
				DefaultObject->mTimeToComplete = mTimeToComplete;
			}

			if (mRelevantShopSchematicsOverride)
			{
				DefaultObject->mRelevantShopSchematics = mRelevantShopSchematics;
			}

			if (mUnlocksOverride)
			{
				for (UFGUnlock* Unlock : mUnlocks)
				{
					if (IsValid(Unlock))
					{
						// Unlock->AddToRoot();
					}
				}

				DefaultObject->mUnlocks = mUnlocks;
			}

			if (mSchematicIconOverride)
			{
				DefaultObject->mSchematicIcon = mSchematicIcon;
			}

			if (mSmallSchematicIconOverride)
			{
				if (IsValid(mSmallSchematicIcon))
				{
					// mSmallSchematicIcon->AddToRoot();
				}

				DefaultObject->mSmallSchematicIcon = mSmallSchematicIcon;
			}

			if (mSchematicDependenciesOverride)
			{
				/*for ( UFGAvailabilityDependency* Dependency : mSchematicDependencies)
				{
					if(IsValid(Dependency))
					{
						Dependency->AddToRoot();
					}
				}*/

				DefaultObject->mSchematicDependencies = mSchematicDependencies;
			}

			if (mDependenciesBlocksSchematicAccessOverride)
			{
				DefaultObject->mDependenciesBlocksSchematicAccess = mDependenciesBlocksSchematicAccess;
			}

			if (mRelevantEventsOverride)
			{
				DefaultObject->mRelevantEvents = mRelevantEvents;
			}
		}
	}

	Super::DoCDO();
}

TArray<UClass*> UKBFL_CDOHelperClass_Schematic::GetClasses()
{
	TArray<UClass*> Re;

	if (UKBFLAssetDataSubsystem* AssetDataSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld()))
	{
		if (!mOfPath.IsEmpty())
		{
			TArray<TSubclassOf<UFGSchematic>> Schematics = AssetDataSubsystem->GetAllSchematics();
			for (TSubclassOf<UFGSchematic> Schematic : Schematics)
			{
				if (Schematic && Schematic->GetPathName().StartsWith(mOfPath) &&
					!mExcludeSchematics.Contains(Schematic))
				{
					Re.Add(Schematic);
				}
			}
		}

		if (IsValidSoftClass(mAllOfSubclass))
		{
			TSubclassOf<UFGSchematic> Subclass = mAllOfSubclass.LoadSynchronous();
			if (Subclass)
			{
				for (TSubclassOf<UFGSchematic> Schematic : AssetDataSubsystem->GetAllSchematics())
				{
					if (Schematic && Schematic->IsChildOf(Subclass) && !mExcludeSchematics.Contains(Schematic))
					{
						Re.Add(Schematic);
					}
				}
			}
		}
	}

	for (TSoftClassPtr<UFGSchematic> Class : mSchematics)
	{
		if (IsValidSoftClass(Class) && !mExcludeSchematics.Contains(Class.LoadSynchronous()))
		{
			Re.Add(Class.LoadSynchronous());
		}
	}

	return Re;
}
