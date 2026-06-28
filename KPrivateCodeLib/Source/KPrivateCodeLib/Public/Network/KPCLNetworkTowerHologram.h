// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildableRadarTower.h"
#include "FGConstructDisqualifier.h"
#include "Hologram/FGFactoryHologram.h"

#include "KPCLNetworkTowerHologram.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkTowerHologram : public AFGFactoryHologram
{
	GENERATED_BODY()

public:
	AKPCLNetworkTowerHologram();

	virtual void CheckValidPlacement() override;
	virtual void ConfigureActor(class AFGBuildable* inBuildable) const override;
	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;
	virtual bool IsValidHitActor(AActor* hitActor) const override;
	virtual bool IsValidHitResult(const FHitResult& hitResult) const override;

	UPROPERTY(EditDefaultsOnly, Category = "NetworkTower")
	TSubclassOf<UFGConstructDisqualifier> mNoRadarDisqualifier;

	UPROPERTY()
	TObjectPtr<AFGBuildableRadarTower> mLinkedRadarTower;
};
