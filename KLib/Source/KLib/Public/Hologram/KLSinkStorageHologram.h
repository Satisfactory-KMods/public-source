// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Hologram/FGStackableStorageHologram.h"
#include "UObject/Object.h"
#include "KLSinkStorageHologram.generated.h"

/**
 * 
 */
UCLASS()
class KLIB_API AKLSinkStorageHologram : public AFGStackableStorageHologram
{
	GENERATED_BODY()

	virtual void GetSupportedBuildModes_Implementation(
		TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const override;
	virtual void OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode) override;

	virtual AActor* GetUpgradedActor() const override;
	virtual bool TryUpgrade(const FHitResult& hitResult) override;
	virtual void ConfigureActor(AFGBuildable* inBuildable) const override;

	UPROPERTY()
	AActor* mUpgradeActor;

	UPROPERTY(EditDefaultsOnly, Category="KMods")
	TSubclassOf<UFGHologramBuildModeDescriptor> mUpgradeBuildMode;
};
