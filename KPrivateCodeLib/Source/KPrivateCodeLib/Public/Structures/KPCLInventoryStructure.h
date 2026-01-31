#pragma once
#include "FGInventoryComponent.h"
#include "Buildables/FGBuildableFactory.h"
#include "KPCLInventoryStructure.generated.h"

UENUM(BlueprintType)
enum class EKPCLInventoryType : uint8
{
	Input,
	Output,
	Booster
};

USTRUCT()
struct KPRIVATECODELIB_API FKPCLInventoryStructure
{
	GENERATED_BODY()

	inline static FName InputName = "InputInventory";
	inline static FName OutputName = "OutputInventory";
	inline static FName BoosterName = "BoosterInventory";
};
