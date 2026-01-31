// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAssets/KBFLCustomizationProvider.h"
#include "Interfaces/KBFLContentCDOHelperInterface.h"
#include "Module/GameWorldModule.h"
#include "Module/WorldModule.h"
#include "UObject/Object.h"

#include "KBFLWorldModule.generated.h"

/**
 *
 */
UCLASS(Blueprintable)
class KBFL_API UKBFLWorldModule : public UGameWorldModule, public IKBFLContentCDOHelperInterface
{
	GENERATED_BODY()

public:
	UKBFLWorldModule();

	// BEGIN IKBFLContentCDOHelperInterface
	virtual FKBFLCDOInformation GetCDOInformationFromPhase_Implementation(ELifecyclePhase Phase,
																		  bool& HasPhase) override;

	// END IKBFLContentCDOHelperInterface

	// BEGIN UGameWorldModule
	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;

	// END UGameWorldModule

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void ConstructionPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void InitPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void PostInitPhase();

	virtual void RegisterKBFLLogicContent();

	virtual void FindAllCDOs();

	virtual bool IsAllowedToRegister(TSubclassOf<UObject> Object) const;

	bool bScanForCDOsDone = false;
	/** Information for CDO's */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DEPRECATED|CDOHelper")
	TMap<ELifecyclePhase, FKBFLCDOInformation> mCDOInformationMap;

	/** Material Information for add to SF Material Desc */
	UPROPERTY(VisibleDefaultsOnly, Category = "DEPRECATED|Swatches")
	TArray<FKBFLMaterialDescriptorInformation> mMaterialInformation = {};

	/** Swatches that should add to the Subsystem */
	UPROPERTY(VisibleDefaultsOnly, Category = "DEPRECATED|Swatches")
	TArray<FKBFLSwatchInformation> mSwatchDescriptionInformation = {};

	/** Default Swatches for the Swatch Group */
	UPROPERTY(VisibleDefaultsOnly, Category = "DEPRECATED|Swatches")
	TMap<TSubclassOf<UFGSwatchGroup>, TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> mSwatchGroups;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|AssetRegistry")
	bool mUseAssetRegistry = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DEPRECATED|CDOHelper|AssetRegistry",
			  meta = (EditCondition = mUseAssetRegistry))
	bool mRegisterCDOs = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|AssetRegistry",
			  meta = (EditCondition = mUseAssetRegistry))
	bool mRegisterRecipes = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|AssetRegistry",
			  meta = (EditCondition = mUseAssetRegistry))
	bool mRegisterSchematics = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|AssetRegistry",
			  meta = (EditCondition = mUseAssetRegistry))
	bool mRegisterResearchTrees = true;

	/**
	 * Path for automatic find classes to register
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|AssetRegistry")
	TArray<TSubclassOf<UObject>> mBlacklistedClasses;

	/**
	 * Path for automatic find classes to register
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DEPRECATED|CDOHelper|AssetRegistry",
			  meta = (EditCondition = mRegisterCDOs))
	TArray<TSubclassOf<UObject>> mBlacklistedCDOClasses;
};
