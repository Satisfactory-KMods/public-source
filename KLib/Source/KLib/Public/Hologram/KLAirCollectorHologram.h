#pragma once

#include "CoreMinimal.h"

#include "FGGameUserSettings.h"
#include "Hologram/FGFactoryHologram.h"

#include "KLAirCollectorHologram.generated.h"

UCLASS()
class KLIB_API AKLAirCollectorHologram : public AFGFactoryHologram
{
	GENERATED_BODY()

public:
	AKLAirCollectorHologram();

	virtual void GetSupportedBuildModes_Implementation(
		TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const override;
	virtual void OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode) override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	virtual bool TrySnapToActor(const FHitResult& hitResult) override;

	virtual void BeginPlay() override;

	float GetScanRange() const;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UFGHologramBuildModeDescriptor> mSphereMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMeshComponent> mSphere;

	UPROPERTY(Transient)
	TObjectPtr<UFGGameUserSettings> mGameUserSettings;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> mStaticMesh;

	UPROPERTY(Transient)
	TMap<int, TObjectPtr<class UStaticMesh>> mFoundationMeshMapping;

private:

	void UpdateFoundationMesh(const AActor* HitActor);

	float mCachedScanRange = 0.f;
};
