// 

#pragma once

#include "CoreMinimal.h"
#include "FGConstructDisqualifier.h"
#include "Buildables/FGBuildableRadarTower.h"
#include "Hologram/FGFactoryHologram.h"

#include "KPCLNetworkTowerHologram.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkTowerHologram : public AFGFactoryHologram
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AKPCLNetworkTowerHologram();

	virtual void CheckValidPlacement() override;

	virtual void ConfigureActor(class AFGBuildable* inBuildable) const override;

	virtual void SetHologramLocationAndRotation(const FHitResult& hitResult) override;

	virtual bool IsValidHitActor(AActor* hitActor) const override;
	virtual bool IsValidHitResult(const FHitResult& hitResult) const override;

	UPROPERTY(EditDefaultsOnly, Category = "NetworkTower")
	TSubclassOf<UFGConstructDisqualifier> mNoRadarDisqualifier;

	UPROPERTY()
	AFGBuildableRadarTower* mLinkedRadarTower;
};
