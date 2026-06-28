// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Resources/FGResourceNode.h"
#include "Runtime/CoreUObject/Public/UObject/NoExportTypes.h"

#include "RSS_Math.generated.h"

UCLASS()
class RSS_API URSS_Math : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Caclulate Orbit Points for Splines */
	UFUNCTION(BlueprintPure, Category = "KMods|Math")
	static TArray<FVector> CalculateOrbitPoints(float Ellipse = 1.0f, float Radius = 1000.0f, float Tilt = 0.0f,
												int PointCount = 12);
};
