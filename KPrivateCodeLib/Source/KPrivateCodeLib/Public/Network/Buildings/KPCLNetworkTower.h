// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildableRadarTower.h"

#include "Network/KPCLNetworkBuildingBase.h"

#include "KPCLNetworkTower.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkTower : public AKPCLNetworkBuildingBase
{
	GENERATED_BODY()

public:
	AKPCLNetworkTower();

	virtual FText GetActorRepresentationText() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Factory_Tick(float dt) override;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "KPrivateCode|Network")
	TWeakObjectPtr<AFGBuildableRadarTower> mConnectedRadarTower = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Representation")
	FText mTowerRepresentationText;

private:

	bool bPendingDismantle = false;
};
