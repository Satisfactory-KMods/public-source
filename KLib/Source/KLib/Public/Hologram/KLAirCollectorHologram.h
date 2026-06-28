// ILikeBanas

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

	//~ Begin AFGFactoryHologram Interface
	virtual void GetSupportedBuildModes_Implementation(
		TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const override;
	virtual void OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode) override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	virtual bool TrySnapToActor(const FHitResult& hitResult) override;
	//~ End AFGFactoryHologram Interface

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

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
	/**
	 * Swaps mStaticMesh to the foundation-height-appropriate mesh.
	 * If HitActor is not an AFGBuildableFoundation (or is null), resets to the key-1 mesh.
	 * No-ops if mStaticMesh is null or key 1 is missing.
	 */
	void UpdateFoundationMesh(const AActor* HitActor);

	/** Cached scan range populated in BeginPlay — avoids re-fetching the CDO each frame. */
	float mCachedScanRange = 0.f;
};
