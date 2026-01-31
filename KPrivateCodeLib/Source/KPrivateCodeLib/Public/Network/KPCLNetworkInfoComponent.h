// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGPowerInfoComponent.h"
#include "KPCLNetworkInfoComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FCoreStateChanged, bool);
DECLARE_MULTICAST_DELEGATE(FMaxTransferChanged);
/**
 * Component to store and set Bytes
 */
UCLASS(ClassGroup = ( Custom ), meta = ( BlueprintSpawnableComponent ))
class KPRIVATECODELIB_API UKPCLNetworkInfoComponent : public UFGPowerInfoComponent
{
	GENERATED_BODY()

	// START: UObject
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// END: UObject

public:
	FMaxTransferChanged MaxTransferChanged;
	FCoreStateChanged CoreStateChanged;

	UFUNCTION(BlueprintPure, Category="Network")
	bool HasCore() const;

	UFUNCTION(BlueprintPure, Category="Network")
	int32 CoreCount() const;

	void SetCors(TArray<class AKPCLNetworkCore*> Cores);

	UFUNCTION(BlueprintPure, Category="Network")
	class UKPCLNetwork* GetNetwork() const;

	UFUNCTION(BlueprintPure, Category="Network")
	TArray<class AKPCLNetworkCore*> GetCores() const;

	UFUNCTION(BlueprintPure, Category="Network")
	class AKPCLNetworkCore* GetFirstCores(bool& Valid) const;
	class AKPCLNetworkCore* GetFirstCores() const;

private:
	// is written by the Circuit!
	UPROPERTY(SaveGame, Replicated)
	TArray<class AKPCLNetworkCore*> mNetworkCoresInNetwork;
};
