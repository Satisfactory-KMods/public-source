// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildableWire.h"

#include "KPCLNetworkCable.generated.h"

UCLASS(Abstract)
class KPRIVATECODELIB_API AKPCLNetworkCable : public AFGBuildableWire
{
	GENERATED_BODY()

public:
	AKPCLNetworkCable();
};
