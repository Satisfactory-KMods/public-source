#pragma once

#include "CoreMinimal.h"
#include "Buildable/Modular/KPCLModularHologram.h"
#include "Buildable/ModularMiner/KLMMBuildableModule.h"

#include "Hologram/FGBuildableHologram.h"
#include "Resources/FGResourceNode.h"

#include "KLHologramModularMinerModule.generated.h"

/**
 * 
 */
UCLASS()
class KLIB_API AKLHologramModularMinerModule : public AKPCLModularHologram
{
	GENERATED_BODY()

public:
	AKLHologramModularMinerModule();

	virtual bool IsModuleAllowed(UKPCLModularBuildingHandlerBase* Handler, AFGBuildable* TargetBuildable,
	                             const FHitResult& hitResult) override;

	virtual AActor* GetUpgradedActor() const override;
	virtual bool TryUpgrade(const FHitResult& hitResult) override;

	virtual void ConfigureComponents(AFGBuildable* inBuildable) const override;

	bool mCanUpdate;

	UPROPERTY(Transient)
	AKLMMBuildableModule* mUpgradedActor = nullptr;
};
