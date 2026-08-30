#pragma once

#include "CoreMinimal.h"

#include "Equipment/FGBuildGun.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "KBFL_Player.generated.h"

UCLASS()
class KBFL_API UKBFL_Player : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "KMods|Player")
	static AFGBuildGun* GetBuildingGun(UObject* WorldContext);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "KMods|Player")
	static AFGPlayerController* GetFGController(UObject* WorldContext);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "KMods|Player")
	static AFGCharacterPlayer* GetFGCharacter(UObject* WorldContext);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "KMods|Player")
	static AFGPlayerState* GetFgPlayerState(UObject* WorldContext);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "KMods|Player")
	static void GetBuildingGunHitResult(UObject* WorldContext, bool& IsInBuildOrDismantleState, FHitResult& HitResult);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "KMods|Player")
	static EBuildGunState GetPlayerBuildState(UObject* WorldContext);
};
