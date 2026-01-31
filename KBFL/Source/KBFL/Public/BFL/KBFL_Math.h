#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "KBFL_Math.generated.h"

UCLASS()
class KBFL_API UKBFL_Math : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Placeholder */
	// UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category="KMods|Math")
	// static XXX XXX(UObject* WorldContext);

	/** Caclulate Orbit Points for Splines */
	UFUNCTION(BlueprintPure, Category = "KMods|Math")
	static TArray<FVector> CalculateOrbitPoints(float Ellipse = 1.0f, float Radius = 1000.0f, float Tilt = 0.0f,
												int PointCount = 12);

	UFUNCTION(BlueprintPure, Category = "KMods|Math")
	static float CalculateItemsPerMinFloat(float ItemCount, float ProductionTime);

	UFUNCTION(BlueprintPure, Category = "KMods|Math")
	static float CalculateM3PerMinFloat(float ItemCount, float ProductionTime);

	UFUNCTION(BlueprintPure, Category = "KMods|Math")
	static float CalculateItemsPerMin(int ItemCount, float ProductionTime);

	UFUNCTION(BlueprintPure, Category = "KMods|Math")
	static float CalculateM3PerMin(int ItemCount, float ProductionTime);

	UFUNCTION(BlueprintPure, Category = "KMods|Math")
	static float CalculateM3PerMinForWidget(int ItemCount, float ProductionTime);
};
