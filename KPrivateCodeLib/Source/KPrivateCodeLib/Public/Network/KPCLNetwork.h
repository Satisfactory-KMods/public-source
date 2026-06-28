// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGPowerCircuit.h"

#include "KPCLNetworkBuildingBase.h"

#include "KPCLNetwork.generated.h"

UCLASS()
class KPRIVATECODELIB_API UKPCLNetwork : public UFGPowerCircuit
{
	GENERATED_BODY()

	//~ Begin UObject
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject

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
	TArray<TObjectPtr<class AKPCLNetworkCore>> mCoreBuildings;

	UPROPERTY(Replicated, SaveGame)
	TArray<TObjectPtr<class AKPCLNetworkConnectionBuilding>> mConnectionBuildings;
};
