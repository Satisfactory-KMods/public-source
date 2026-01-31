// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGPowerCircuit.h"
#include "KPCLNetworkBuildingBase.h"
#include "KPCLNetwork.generated.h"

/**
 * 
 */
UCLASS()
class KPRIVATECODELIB_API UKPCLNetwork : public UFGPowerCircuit
{
	GENERATED_BODY()

	// START: UObject
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// END: UObject

	virtual void TickCircuit(float dt) override;
	virtual void OnCircuitChanged() override;
	void UpdateDataInInfos();

public:
	UFUNCTION(BlueprintPure, Category = "Circuits|Network")
	FString GetNetworkId() const;

	UFUNCTION(BlueprintPure, Category = "Circuits|Network")
	bool NetworkHasCore() const;

	UFUNCTION(BlueprintPure, Category = "Circuits|Network")
	bool NetworkHasCoreToMuchCores() const;

	UFUNCTION(BlueprintPure, Category = "Circuits|Network")
	bool CoreStateIsOk() const;

	UFUNCTION(BlueprintPure, Category = "Circuits|Network")
	class AKPCLNetworkCore* GetCore() const;

	TArray<class AKPCLNetworkConnectionBuilding*> GetNetworkConnectionBuildings() const;

private:
	UPROPERTY(Replicated, SaveGame)
	TArray<class AKPCLNetworkCore*> mCoreBuildings;

	UPROPERTY(Replicated, SaveGame)
	TArray<class AKPCLNetworkConnectionBuilding*> mConnectionBuildings;
};
