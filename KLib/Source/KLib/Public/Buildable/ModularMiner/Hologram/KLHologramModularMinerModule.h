// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildable/Modular/KPCLModularHologram.h"
#include "Hologram/FGBuildableHologram.h"
#include "Resources/FGResourceNode.h"

#include "Buildable/ModularMiner/KLMMBuildableModule.h"

#include "KLHologramModularMinerModule.generated.h"

UCLASS()
class KLIB_API AKLHologramModularMinerModule : public AKPCLModularHologram
{
	GENERATED_BODY()

public:
	AKLHologramModularMinerModule();

	//~ Begin AKPCLModularHologram Interface
	virtual void ConfigureComponents(AFGBuildable* inBuildable) const override;
	virtual AActor* GetUpgradedActor() const override;
	virtual bool IsModuleAllowed(UKPCLModularBuildingHandlerBase* Handler, AFGBuildable* TargetBuildable,
								 const FHitResult& hitResult) override;
	virtual bool TryUpgrade(const FHitResult& hitResult) override;
	//~ End AKPCLModularHologram Interface

	bool mCanUpdate;

	UPROPERTY(Transient)
	TObjectPtr<AKLMMBuildableModule> mUpgradedActor = nullptr;
};
