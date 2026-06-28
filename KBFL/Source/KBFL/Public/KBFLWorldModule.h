// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAssets/KBFLCustomizationProvider.h"
#include "Module/GameWorldModule.h"
#include "Module/WorldModule.h"
#include "UObject/Object.h"

#include "KBFLWorldModule.generated.h"

/**
 *
 */
UCLASS(Blueprintable)
class KBFL_API UKBFLWorldModule : public UGameWorldModule
{
	GENERATED_BODY()

public:
	UKBFLWorldModule();

	// BEGIN UGameWorldModule
	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;

	// END UGameWorldModule

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void ConstructionPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void InitPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void PostInitPhase();

	/** Editor button: scan this mod's plugin path and AddUnique all schematics into mSchematics. */
	UFUNCTION(BlueprintCallable, Category = "KMods|AssetRegistry")
	void ScanSchematics();

	/** Editor button: scan this mod's plugin path and AddUnique all research trees into mResearchTrees. */
	UFUNCTION(BlueprintCallable, Category = "KMods|AssetRegistry")
	void ScanResearchTrees();

	/** Editor button: scan this mod's plugin path and AddUnique all chat commands into mChatCommands. */
	UFUNCTION(BlueprintCallable, Category = "KMods|AssetRegistry")
	void ScanChatCommands();

private:
	// Scans this module's own mod plugin path in the Asset Registry, loads each Blueprint class
	// derived from T, and AddUnique's the valid ones into OutArray (skips null/abstract/blacklisted).
	template <typename T>
	void ScanModAssetsInto(TArray<TSubclassOf<T>>& OutArray, const TCHAR* Label);

public:
	/** Material Information for add to SF Material Desc */
	UPROPERTY(VisibleDefaultsOnly, Category = "DEPRECATED|Swatches")
	TArray<FKBFLMaterialDescriptorInformation> mMaterialInformation = {};

	/** Swatches that should add to the Subsystem */
	UPROPERTY(VisibleDefaultsOnly, Category = "DEPRECATED|Swatches")
	TArray<FKBFLSwatchInformation> mSwatchDescriptionInformation = {};

	/** Default Swatches for the Swatch Group */
	UPROPERTY(VisibleDefaultsOnly, Category = "DEPRECATED|Swatches")
	TMap<TSubclassOf<UFGSwatchGroup>, TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> mSwatchGroups;

	/**
	 * Classes excluded from the editor scan buttons (ScanSchematics / ScanResearchTrees / ScanChatCommands).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|AssetRegistry")
	TArray<TSubclassOf<UObject>> mBlacklistedClasses;
};
