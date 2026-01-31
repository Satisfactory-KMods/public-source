#pragma once

#include "CoreMinimal.h"
#include "FGSchematic.h"
#include "KBFL_CDOHelperClass_Base.h"
#include "KBFL_CDOHelperClass_Schematic.generated.h"

/**
 *@deprecated: use the new UKBFLCDOOverwrite system instead
 */
UCLASS()
class KBFL_API UKBFL_CDOHelperClass_Schematic : public UKBFL_CDOHelperClass_Base
{
	GENERATED_BODY()

public:
	virtual void DoCDO() override;
	virtual TArray<UClass*> GetClasses() override;

	/** must be set for CDO */
	UPROPERTY(EditDefaultsOnly, Category = "CDO Helper")
	TArray<TSoftClassPtr<UFGSchematic>> mSchematics;

	UPROPERTY(EditDefaultsOnly, Category = "CDO Helper")
	TSoftClassPtr<UFGSchematic> mAllOfSubclass;

	UPROPERTY(EditDefaultsOnly, Category = "CDO Helper")
	FString mOfPath = FString();

	UPROPERTY(EditDefaultsOnly, Category = "CDO Helper")
	TArray<TSubclassOf<UFGSchematic>> mExcludeSchematics;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mTypeOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mTypeOverride), Category = "FG Schematic")
	ESchematicType mType = ESchematicType::EST_Custom;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mDisplayNameOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mDisplayNameOverride),
			  Category = "FG Schematic")
	FText mDisplayName = FText::GetEmpty();

	UPROPERTY(meta = (NoAutoJson = true))
	bool mDescriptionOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mDescriptionOverride),
			  Category = "FG Schematic")
	FText mDescription = FText::GetEmpty();

	UPROPERTY(meta = (NoAutoJson = true))
	bool mSchematicCategoryOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mSchematicCategoryOverride),
			  Category = "FG Schematic")
	TSubclassOf<class UFGSchematicCategory> mSchematicCategory = nullptr;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mSubCategoriesOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mSubCategoriesOverride),
			  Category = "FG Schematic")
	TArray<TSubclassOf<class UFGSchematicCategory>> mSubCategories = {};

	UPROPERTY(meta = (NoAutoJson = true))
	bool mMenuPriorityOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mMenuPriorityOverride),
			  Category = "FG Schematic")
	float mMenuPriority = 0.0f;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mTechTierOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mTechTierOverride),
			  Category = "FG Schematic")
	int32 mTechTier = 1;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mCostOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mCostOverride), Category = "FG Schematic")
	TArray<FItemAmount> mCost = {};

	UPROPERTY(meta = (NoAutoJson = true))
	bool mTimeToCompleteOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mTimeToCompleteOverride),
			  Category = "FG Schematic")
	float mTimeToComplete = 120.0f;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mRelevantShopSchematicsOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mRelevantShopSchematicsOverride),
			  Category = "FG Schematic")
	TArray<TSubclassOf<UFGSchematic>> mRelevantShopSchematics = {};

	UPROPERTY(meta = (NoAutoJson = true))
	bool mUnlocksOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Instanced, meta = (EditCondition = mUnlocksOverride),
			  Category = "FG Schematic")
	TArray<class UFGUnlock*> mUnlocks = {};

	UPROPERTY(meta = (NoAutoJson = true))
	bool mSchematicIconOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mSchematicIconOverride),
			  Category = "FG Schematic")
	FSlateBrush mSchematicIcon = FSlateBrush();

	UPROPERTY(meta = (NoAutoJson = true))
	bool mSmallSchematicIconOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mSmallSchematicIconOverride),
			  Category = "FG Schematic")
	UTexture2D* mSmallSchematicIcon = nullptr;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mSchematicDependenciesOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Instanced, meta = (EditCondition = mSchematicDependenciesOverride),
			  Category = "FG Schematic")
	TArray<class UFGAvailabilityDependency*> mSchematicDependencies = {};

	UPROPERTY(meta = (NoAutoJson = true))
	bool mDependenciesBlocksSchematicAccessOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mDependenciesBlocksSchematicAccessOverride),
			  Category = "FG Schematic")
	bool mDependenciesBlocksSchematicAccess = true;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mRelevantEventsOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = mRelevantEventsOverride),
			  Category = "FG Schematic")
	TArray<EEvents> mRelevantEvents = {};
};
