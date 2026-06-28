// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGPowerConnectionComponent.h"
#include "UObject/Object.h"

#include "KPCLNetwork.h"

#include "KPCLNetworkConnectionComponent.generated.h"

/** Connection component for the Faxit network — wraps UFGPowerConnectionComponent. */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
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
