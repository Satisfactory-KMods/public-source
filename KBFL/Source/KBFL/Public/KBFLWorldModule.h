// Copyright Kyri123 / KMods 2026. All Rights Reserved.



#pragma once

#include "CoreMinimal.h"
#include "DataAssets/KBFLCustomizationProvider.h"
#include "Module/GameWorldModule.h"
#include "Module/WorldModule.h"
#include "UObject/Object.h"

#include "KBFLWorldModule.generated.h"

UCLASS(Blueprintable)
class KBFL_API UKBFLWorldModule : public UGameWorldModule
{
	GENERATED_BODY()

public:
	UKBFLWorldModule();

	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void ConstructionPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void InitPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void PostInitPhase();

	UFUNCTION(BlueprintCallable, Category = "KMods|AssetRegistry")
	void ScanSchematics();

	UFUNCTION(BlueprintCallable, Category = "KMods|AssetRegistry")
	void ScanResearchTrees();

	UFUNCTION(BlueprintCallable, Category = "KMods|AssetRegistry")
	void ScanChatCommands();

private:

	template <typename T>
	void ScanModAssetsInto(TArray<TSubclassOf<T>>& OutArray, const TCHAR* Label);

public:

	UPROPERTY(VisibleDefaultsOnly, Category = "DEPRECATED|Swatches")
	TArray<FKBFLMaterialDescriptorInformation> mMaterialInformation = {};

	UPROPERTY(VisibleDefaultsOnly, Category = "DEPRECATED|Swatches")
	TArray<FKBFLSwatchInformation> mSwatchDescriptionInformation = {};

	UPROPERTY(VisibleDefaultsOnly, Category = "DEPRECATED|Swatches")
	TMap<TSubclassOf<UFGSwatchGroup>, TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> mSwatchGroups;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|AssetRegistry")
	TArray<TSubclassOf<UObject>> mBlacklistedClasses;
};
