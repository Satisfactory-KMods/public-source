// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Structures/KPCLFunctionalStructure.h"
#include "UObject/Object.h"
#include "KPCLPowerOptionsLib.generated.h"

/**
 * 
 */
UCLASS()
class KPRIVATECODELIB_API UKPCLPowerOptionsLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="KMods|BFL")
	static bool IsValid(FPowerOptions PowerOption);

	UFUNCTION(BlueprintCallable, Category="KMods|BFL")
	static void MergePowerOptions(UPARAM(ref)
	                              FPowerOptions& PowerOption, FPowerOptions OtherPowerOption);

	UFUNCTION(BlueprintCallable, Category="KMods|BFL")
	static void OverWritePowerOptions(UPARAM(ref)
	                                  FPowerOptions& PowerOption, FPowerOptions OtherPowerOption);

	UFUNCTION(BlueprintPure, Category="KMods|BFL")
	static float GetMaxPowerConsume(FPowerOptions PowerOption);

	UFUNCTION(BlueprintPure, Category="KMods|BFL")
	static float GetPowerConsume(FPowerOptions PowerOption);

	UFUNCTION(BlueprintPure, Category="KMods|BFL")
	static float GetCurrentVariablePower(FPowerOptions PowerOption);

	UFUNCTION(BlueprintPure, Category="KMods|BFL")
	static bool IsPowerVariable(FPowerOptions PowerOption);
};
