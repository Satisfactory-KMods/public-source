#pragma once

#include "CoreMinimal.h"
#include "FGFactoryColoringTypes.h"
#include "KBFL_CDOHelperClass_Base.h"
#include "KBFL_CDOHelperClass_Buildable.generated.h"

/**
 *@deprecated: use the new UKBFLCDOOverwrite system instead
 * This CDO should call in the Main Menu!
 */
UCLASS()
class KBFL_API UKBFL_CDOHelperClass_Buildable : public UKBFL_CDOHelperClass_Base
{
	GENERATED_BODY()

public:
	virtual void DoCDO() override;
	virtual TArray<UClass*> GetClasses() override;

	/** must be set for CDO */
	UPROPERTY(EditAnywhere, Category = "CDO Helper")
	TArray<TSoftClassPtr<AFGBuildable>> mClasses;

	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mHologramClassOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mHologramClassOverride),
			  Category = "FGBuildable")
	TSubclassOf<class AFGHologram> mHologramClass;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mDisplayNameOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mDisplayNameOverride), Category = "FGBuildable")
	FText mDisplayName;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mDescriptionOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mDescriptionOverride, MultiLine = true),
			  Category = "FGBuildable")
	FText mDescription;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mDecoratorClassOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mDecoratorClassOverride),
			  Category = "FGBuildable")
	TSubclassOf<class AFGDecorationTemplate> mDecoratorClass;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mDefaultSwatchCustomizationOverrideOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mDefaultSwatchCustomizationOverrideOverride),
			  Category = "FGBuildable")
	TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> mDefaultSwatchCustomizationOverride;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mSwatchGroupOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mSwatchGroupOverride), Category = "FGBuildable")
	TSubclassOf<class UFGSwatchGroup> mSwatchGroup;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mAllowColoringOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mAllowColoringOverride),
			  Category = "FGBuildable")
	bool mAllowColoring;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mAllowPatterningOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mAllowPatterningOverride),
			  Category = "FGBuildable")
	bool mAllowPatterning;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mFactorySkinClassOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mFactorySkinClassOverride),
			  Category = "FGBuildable")
	TSubclassOf<class UFGFactorySkinActorData> mFactorySkinClass;
	//-------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mSkipBuildEffectOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mSkipBuildEffectOverride),
			  Category = "FGBuildable")
	bool mSkipBuildEffect;
	//------------------------------------------------------------------------------------------------------------------------------------
	UPROPERTY(meta = (NoAutoJson = true))
	bool mBuildEffectSpeedOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mBuildEffectSpeedOverride),
			  Category = "FGBuildable")
	float mBuildEffectSpeed;
};
