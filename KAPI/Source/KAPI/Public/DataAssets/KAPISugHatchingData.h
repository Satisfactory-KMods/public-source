// 

#pragma once

#include "CoreMinimal.h"
#include "ItemAmount.h"
#include "KAPIDataAssetBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "Resources/FGItemDescriptor.h"

#include "KAPISugHatchingData.generated.h"

USTRUCT(BlueprintType)
struct KAPI_API FKAPISlugIncubation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UFGItemDescriptor> mSlug;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int mProductionCount = 1;

	/**
	 * Chance to breed this Slug
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0.01f, ClampMax=100.f))
	float mProbability = 100.f;

	/**
	 * Chance multiplier applied on failed breed attempts
	 * += mProbability * mFixedChanceMultiplier
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bShouldUseFixedChance", EditConditionHides))
	float mFixedChanceMultiplier = 0.1f;

	/**
	 * If true the fixed chance if be applied
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShouldUseFixedChance = true;

	bool Roll(float ChanceOverwrite = -1.f) const;
	bool Valid() const;

	friend bool operator==(FKAPISlugIncubation A, FKAPISlugIncubation B) { return A.mSlug == B.mSlug; }
	friend bool operator!=(FKAPISlugIncubation A, FKAPISlugIncubation B) { return A.mSlug != B.mSlug; }
};

UENUM(BlueprintType)
enum class EKAPISlugTime : uint8
{
	NONE = 0 UMETA(DisplayName = "Invalid"),
	Any = 1 UMETA(DisplayName = "Any Time"),
	Day = 2 UMETA(DisplayName = "Day Time"),
	Night = 3 UMETA(DisplayName = "Night Time")
};

USTRUCT(BlueprintType)
struct KAPI_API FKAPISlugFeeling
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mMinHumidity = .2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mMaxHumidity = .8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mMinHeat = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mMaxHeat = 38.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EKAPISlugTime mDayTime = EKAPISlugTime::Any;

	bool IsTempInRange(float Temp) const;

	bool IsHumidityInRange(float Humidity) const;

	bool IsDayTimeValid(EKAPISlugTime Time) const;
};

/**
 * 
 */
UCLASS()
class KAPI_API UKAPISugHatchingData : public UKAPIDataAssetBase
{
	GENERATED_BODY()

public:
	// ================================
	// Functions
	// ================================
	UFUNCTION(BlueprintPure)
	bool IncubationFluidRequired() const;

	UFUNCTION(BlueprintPure)
	void GetComfortableSlugs(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlugs) const;

	bool RollSlugs(TArray<FItemAmount>& OutSlugs,
	               TMap<TSubclassOf<UFGItemDescriptor>, float>& FixedChance, bool bUseFixedChance = true) const;

	UFUNCTION(Blueprintable)
	TArray<TSubclassOf<UFGItemDescriptor>> GetPossibleSlugs() const;

	UFUNCTION(BlueprintPure)
	FKAPISlugFeeling GetFeeling() const;

	UFUNCTION(BlueprintCallable)
	TArray<FKAPISlugIncubation> GetSlugIncubationsSortedByChance() const;

	UFUNCTION(BlueprintCallable)
	TArray<FItemAmount> GetSlugsForThisCycle() const;

	UFUNCTION(BlueprintPure)
	static UKAPISugHatchingData* GetHighterSlugStatic(UKAPISugHatchingData* A, UKAPISugHatchingData* B);

	UFUNCTION(BlueprintPure)
	UKAPISugHatchingData* GetHighterSlug(UKAPISugHatchingData* Other) const;

	UFUNCTION(BlueprintPure)
	bool IsComfortableWith(UKAPISugHatchingData* Other) const;

	// ================================
	// Base
	// ================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slug")
	TSubclassOf<UFGItemDescriptor> mSlug;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slug")
	TSubclassOf<UFGItemDescriptor> mEgg;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slug")
	FLinearColor mEggColor = FLinearColor::White;

	/**
	 * This tier is used to determine which slugs has a higher priority for the breeding process
	 * the higher the tier, the more priority and will use for temp, humidity and day time checks first
	 * also the food of the higher tier slugs will be preferred
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slug",
		meta=(EditCondition="bRequireFluid", EditConditionHides))
	int32 mSlugTier = -1;

	// ================================
	// Incubation
	// ================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Incubation")
	float mHatchDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Incubation")
	float mPowerConsume = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Incubation")
	uint8 mIncubatorTier = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Incubation")
	bool bRequireFluid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Incubation")
	TArray<FKAPISlugIncubation> mPossibleSlugs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Incubation",
		meta=(EditCondition="bRequireFluid", EditConditionHides))
	uint8 mTankTier = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Incubation",
		meta=(EditCondition="bRequireFluid", EditConditionHides))
	int32 mFluidConsume = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Incubation",
		meta=(EditCondition="bRequireFluid", EditConditionHides))
	TSubclassOf<UFGItemDescriptor> mFluidClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Incubation",
		meta=(EditCondition="bRequireFluid", EditConditionHides, ClampMin=1, ClampMax=20))
	float mFluidConsumeTime = 2.0f;

	// ================================
	// Breeding
	// ================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breeding", meta=(ClampMin=1, ClampMax=20))
	int mProductionCountEggs = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breeding", meta=(ClampMin=0.1, ClampMax=3600.f))
	float mBreedingTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breeding", meta=(ClampMin=0.1, ClampMax=3600.f))
	float mDieTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breeding")
	TSubclassOf<UFGItemDescriptor> mRequiredFood;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breeding", meta=(ClampMin=1, ClampMax=20))
	int32 mFoodConsumePerCycle = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breeding", meta=(ClampMin=0.1, ClampMax=3600.f))
	float mFoodDuration = 2.5f;

	UPROPERTY(EditAnywhere, Category="Breeding")
	TArray<TSubclassOf<UFGItemDescriptor>> mComfortableWith;

	UPROPERTY(EditAnywhere, Category="Feeling")
	float mMinHumidity = .2f;

	UPROPERTY(EditAnywhere, Category="Feeling")
	float mMaxHumidity = .8f;

	UPROPERTY(EditAnywhere, Category="Feeling")
	float mMinHeat = 22.0f;

	UPROPERTY(EditAnywhere, Category="Feeling")
	float mMaxHeat = 38.0f;

	UPROPERTY(EditAnywhere, Category="Feeling")
	EKAPISlugTime mDayTime = EKAPISlugTime::Any;
};
