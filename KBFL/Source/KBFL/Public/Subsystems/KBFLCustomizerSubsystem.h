// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DataAssets/KBFLCustomizationProvider.h"
#include "FGFactoryColoringTypes.h"
#include "FGGameMode.h"
#include "FGSwatchGroup.h"
#include "Module/WorldModule.h"
#include "Subsystems/WorldSubsystem.h"

#include "KBFLCustomizerSubsystem.generated.h"

UCLASS(Blueprintable, BlueprintType)
class KBFL_API UKBFLactoryCustomizationDescriptor_Swatch : public UFGFactoryCustomizationDescriptor_Swatch
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Swatch")
	FFactoryCustomizationColorSlot mDefaultColorSlot;
};

UCLASS()
class KBFL_API UKBFLCustomizerSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	bool GatherDefaultCollections();
	bool GatherProviders();
	bool
	RegisterSwatchGroups(TMap<TSubclassOf<UFGSwatchGroup>, TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> Map);
	void CDOMaterials(TArray<FKBFLMaterialDescriptorInformation> CDOInformation);

	UFUNCTION(BlueprintCallable)
	void BeginForProvider(UKBFLCustomizationProvider* Provider);

	UFUNCTION(BlueprintCallable, Category = "KMods|Customizer Subsystem")
	bool SetDefaultToSwatchGroup(TSubclassOf<UFGSwatchGroup> SwatchGroup,
								 TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch> Swatch);

	UFUNCTION(BlueprintPure, Category = "KMods|Customizer Subsystem")
	TMap<int32, TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> GetSwatchMap() const;

private:
	UPROPERTY()
	TMap<int32, TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> mSwatchIDMap;

	UPROPERTY()
	TSubclassOf<UFGFactoryCustomizationCollection> mDefaultSwatchCollection;

	UPROPERTY()
	TSubclassOf<UFGFactoryCustomizationCollection> mDefaultMaterialCollection;

	UPROPERTY()
	TSubclassOf<UFGFactoryCustomizationCollection> mDefaultPatternCollection;

	UPROPERTY()
	TSubclassOf<UFGFactoryCustomizationCollection> mDefaultSkinCollection;

	UPROPERTY()
	TObjectPtr<AFGGameMode> mGameMode;

	UPROPERTY(Transient)
	TSet<TObjectPtr<UKBFLCustomizationProvider>> mCustomizationProviders;

	bool bDefaultGathered = false;
	bool bInitialized = false;
};
