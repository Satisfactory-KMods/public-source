// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "ItemAmount.h"
#include "KPCLLootChest.h"
#include "Subsystems/ResourceNodes/KBFLActorSpawnDescriptor.h"

#include "KPCLLootChestSpawnDesc.generated.h"

UCLASS()
class KPRIVATECODELIB_API UKPCLLootChestSpawnDesc : public UKBFLActorSpawnDescriptor
{
	GENERATED_BODY()

public:
	UKPCLLootChestSpawnDesc() { mNeedAuth = true; };
};
