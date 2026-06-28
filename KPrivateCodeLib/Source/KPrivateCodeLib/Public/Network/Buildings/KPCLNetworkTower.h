// ILikeBanas

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

	/** Factory_Tick runs on the factory worker thread; ticks are throttled by significance. */
	virtual void Factory_Tick(float dt) override;

	/** Radar tower this network tower extends. Assigned by the hologram's ConfigureActor. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "KPrivateCode|Network")
	TWeakObjectPtr<AFGBuildableRadarTower> mConnectedRadarTower = nullptr;

	/** Displayed on the map / compass for this tower. */
	UPROPERTY(EditDefaultsOnly, Category = "KMods|Representation")
	FText mTowerRepresentationText;

private:
	/** Set to true once a self-dismantle has been queued so we don't spam the game thread. */
	bool bPendingDismantle = false;
};
