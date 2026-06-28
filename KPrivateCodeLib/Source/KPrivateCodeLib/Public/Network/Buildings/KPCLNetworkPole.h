// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildablePowerPole.h"

#include "Interfaces/KPCLNetworkDataInterface.h"
#include "Network/KPCLNetworkConnectionComponent.h"

#include "KPCLNetworkPole.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkPole : public AFGBuildablePowerPole, public IKPCLNetworkDataInterface
{
	GENERATED_BODY()

public:
	//~ Begin IKPCLNetworkDataInterface
	virtual bool HasCore_Implementation() const override;
	virtual bool CanUseFactoryClipboard_Implementation() override;
	virtual class AKPCLNetworkCore* GetCore_Implementation() override;
	virtual UKPCLNetwork* GetNetwork_Implementation() const override;
	virtual FNetworkUIData GetUIDData_Implementation() const override;
	virtual TArray<EKPCLNetworkError> GetNetworkErrors_Implementation() const override;
	//~ End IKPCLNetworkDataInterface

	virtual void BeginPlay() override;
	void ConnectLocalPowerConnections() const;

	virtual void RegisterInteractingPlayer_Implementation(AFGCharacterPlayer* player) override;
	virtual void UnregisterInteractingPlayer_Implementation(AFGCharacterPlayer* player) override;

	UFUNCTION(BlueprintPure, Category = "KMods|Network")
	virtual UKPCLNetworkConnectionComponent* GetNetworkConnectionComponent() const;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|UI")
	FNetworkUIData mNetworkUIData;
};
