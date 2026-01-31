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

	UPROPERTY()
	FVector Scale = FVector(1);
};
