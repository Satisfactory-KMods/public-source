// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "Hologram/FGFactoryBuildingHologram.h"
#include "KBFLUtilBuildable.generated.h"

UCLASS()
class KBFL_API AKBFLUtilBuildable : public AFGBuildable
{
	GENERATED_BODY()

public:
	virtual bool ShouldSave_Implementation() const override { return false; }
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	/** Applies the replicated scale on clients once it arrives (actor scale is not part of replicated movement). */
	UFUNCTION()
	void OnRep_Scale();

	UPROPERTY(ReplicatedUsing = OnRep_Scale)
	FVector Scale = FVector(1);
};
