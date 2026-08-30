#pragma once

#include "CoreMinimal.h"

#include "FGSchematic.h"

#include "Subsystem/KPCLFaxitSubsystem.h"

#include "KPCLSchematic.generated.h"

UENUM(BlueprintType)
enum class EKPCLSchematicType : uint8
{
	Faxit UMETA(DisplayName = "Faxit")

};

UCLASS(Blueprintable)
class KPRIVATECODELIB_API UKPCLSchematic : public UFGSchematic
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMods|Deprecated")
	EKPCLSchematicType mSchematicType = EKPCLSchematicType::Faxit;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMods|Deprecated")
	EKPCLNetworkLevelType mFaxitLevel = EKPCLNetworkLevelType::RemoveAccess;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KMods|Deprecated")
	int32 mLevel = 1;
};
