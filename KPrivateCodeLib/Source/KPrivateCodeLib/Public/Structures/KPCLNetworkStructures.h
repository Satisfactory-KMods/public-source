// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "KPCLNetworkStructures.generated.h"

USTRUCT(BlueprintType)
struct KPRIVATECODELIB_API FTeleporterInformation
{
	GENERATED_BODY()

	FTeleporterInformation() {};

	FTeleporterInformation(int32 OverwriteIconID) { mIconID = OverwriteIconID; }

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	int32 mIconID = 782;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FName mIconName = FName("New Teleporter");

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FString mTeleporterNote = FString("Teleporter Note");
};
