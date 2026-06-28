// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Hologram/FGStackableStorageHologram.h"
#include "UObject/Object.h"

#include "KLSinkStorageHologram.generated.h"

UCLASS()
class KLIB_API AKLSinkStorageHologram : public AFGStackableStorageHologram
{
	GENERATED_BODY()

public:
	//~ Begin AFGStackableStorageHologram Interface
	virtual AActor* GetUpgradedActor() const override;
	virtual void ConfigureActor(AFGBuildable* inBuildable) const override;
	virtual void GetSupportedBuildModes_Implementation(
		TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const override;
	virtual void OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode) override;
	virtual bool TryUpgrade(const FHitResult& hitResult) override;
	//~ End AFGStackableStorageHologram Interface

	UPROPERTY(EditDefaultsOnly, Category = "KMods")
	TSubclassOf<UFGHologramBuildModeDescriptor> mUpgradeBuildMode;

	UPROPERTY()
	TObjectPtr<AActor> mUpgradeActor;
};
