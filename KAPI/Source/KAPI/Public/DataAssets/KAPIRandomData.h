// 

#pragma once

#include "CoreMinimal.h"
#include "Math/UnrealMathUtility.h"

#include "KAPIRandomData.generated.h"

/**
 * Generic template structure for random data with probability-based rolling
 * Can be used for any item or data that needs to be randomly selected based on probability
 */
USTRUCT(BlueprintType)
struct FKAPIRandomData
{
	GENERATED_BODY()

	/**
	 * Probability/Chance for this item to be selected (0-100)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0.0f, ClampMax=100.0f))
	float mProbability = 100.0f;

	/**
	 * Performs a probability roll to determine if this item should be selected
	 * @return true if the random roll succeeds, false otherwise
	 */
	bool Roll() const
	{
		return FMath::FRandRange(0.0f, 100.0f) <= mProbability;
	}

	/**
	 * Checks if this random data entry is valid (has a valid probability)
	 * @return true if probability is greater than 0, false otherwise
	 */
	bool IsValid() const
	{
		return mProbability > 0.0f;
	}
};
