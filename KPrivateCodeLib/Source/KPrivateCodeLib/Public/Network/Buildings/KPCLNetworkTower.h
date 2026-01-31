// 

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
	virtual FText GetActorRepresentationText() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Factory_Tick(float dt) override;

	// Sets default values for this actor's properties
	AKPCLNetworkTower();

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "KPrivateCode|Network")
	TWeakObjectPtr<AFGBuildableRadarTower> mConnectedRadarTower = nullptr;
};
