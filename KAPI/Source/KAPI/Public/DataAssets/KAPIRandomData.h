#pragma once

#include "CoreMinimal.h"

#include "Math/UnrealMathUtility.h"

#include "KAPIRandomData.generated.h"

USTRUCT(BlueprintType)
struct FKAPIRandomData
{
	GENERATED_BODY()

	bool IsValid() const { return mProbability > 0.0f; }

	bool Roll() const { return FMath::FRandRange(0.0f, 100.0f) <= mProbability; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.0f, ClampMax = 100.0f))
	float mProbability = 100.0f;
};
