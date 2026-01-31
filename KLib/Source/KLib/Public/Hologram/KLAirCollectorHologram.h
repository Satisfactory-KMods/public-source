// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FGGameUserSettings.h"
#include "Hologram/FGFactoryHologram.h"
#include "KLAirCollectorHologram.generated.h"

UCLASS()
class KLIB_API AKLAirCollectorHologram : public AFGFactoryHologram
{
	GENERATED_BODY()

public:
	virtual void GetSupportedBuildModes_Implementation(
		TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const override;
	virtual void OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode) override;

	AKLAirCollectorHologram();
	virtual void BeginPlay() override;
	virtual bool TrySnapToActor(const FHitResult& hitResult) override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;

	float GetScanRange() const;

	UPROPERTY(Transient)
	TMap<int, class UStaticMesh*> mFoundationMeshMapping;

	UPROPERTY(Transient)
	UStaticMeshComponent* mStaticMesh;

	UPROPERTY(Transient)
	UFGGameUserSettings* mGameUserSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* mSphere;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UFGHologramBuildModeDescriptor> mSphereMode;
};
