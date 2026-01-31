// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGPowerConnectionComponent.h"
#include "KPCLNetwork.h"
#include "UObject/Object.h"
#include "KPCLNetworkConnectionComponent.generated.h"

/**
 * only for indicator
 */
UCLASS(ClassGroup = ( Custom ), meta = ( BlueprintSpawnableComponent ))
class KPRIVATECODELIB_API UKPCLNetworkConnectionComponent : public UFGPowerConnectionComponent
{
	GENERATED_BODY()

public:
	UKPCLNetworkConnectionComponent();

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	UKPCLNetwork* GetNetwork() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	AKPCLNetworkCore* GetCore() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	bool IsNetworkOk() const;
};
