// ILikeBanas

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Unlocks/KLGiveItemToAllPlayerUnlock.h"
#include "Unlocks/KPCLRepeatPurchaseUnlock.h"

#include "KLUnlockMaxRepeatPurchases.generated.h"

/** Replaces complete item rewards and optionally schematic costs from specified purchase level onward. */
USTRUCT(BlueprintType)
struct KLIB_API FKLRepeatPurchaseRewardLevel
{
	GENERATED_BODY()

	/** 1 is the first purchase. The highest configured level not above the current level wins. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repeat Purchases", meta = (ClampMin = "1", UIMin = "1"))
	int32 mPurchaseLevel = 1;

	/** Complete reward list used from this purchase level onward. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repeat Purchases")
	TArray<FItemAmount> mItemsToGive;

	/** Whether this level starts a new schematic cost list. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repeat Purchases")
	bool bOverrideCosts = false;

	/** Complete raw schematic cost list used from this level onward, before mCostMultiplierCurve. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repeat Purchases",
			  meta = (EditCondition = "bOverrideCosts"))
	TArray<FItemAmount> mCosts;

	/** Raw costs added only to this exact purchase level, before mCostMultiplierCurve. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repeat Purchases")
	TArray<FItemAmount> mAdditionalCosts;
};

/**
 * Gives level-dependent repeat-purchase rewards to the Trading Post storage. Overflow spawns as a crate.
 *
 * Curve input is the number of already completed purchases. X=0 therefore controls the first
 * purchase, X=1 the second, and so on. A null curve keeps the configured value unchanged.
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class KLIB_API UKLUnlockMaxRepeatPurchases : public UKLGiveItemToAllPlayerUnlock, public IKPCLRepeatPurchaseUnlock
{
	GENERATED_BODY()

public:
	//~ Begin UFGUnlock Interface
	virtual bool IsRepeatPurchasesAllowed_Implementation() const override;
	//~ End UFGUnlock Interface

	//~ Begin IKPCLRepeatPurchaseUnlock Interface
	virtual int32 GetMaxPurchaseCount() const override { return mMaxPurchaseCount; }
	virtual float GetCostMultiplier(int32 CompletedPurchaseCount) const override;
	virtual float GetRewardMultiplier(int32 CompletedPurchaseCount) const override;
	virtual void ApplyLevelState(TSubclassOf<UFGSchematic> SchematicClass, int32 CompletedPurchaseCount) override;
	//~ End IKPCLRepeatPurchaseUnlock Interface

	/** Finds this unlock on supplied schematic CDO. Returns null when absent. */
	UFUNCTION(BlueprintPure, Category = "KMods|Unlocks|Repeat Purchases")
	static UKLUnlockMaxRepeatPurchases* FindOnSchematic(TSubclassOf<UFGSchematic> SchematicClass);

	/** Maximum total purchases. Values <= 0 mean unlimited. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Unlocks|Repeat Purchases")
	int32 mMaxPurchaseCount = 2;

	/** Optional multiplier curve for schematic cost. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Unlocks|Repeat Purchases")
	TObjectPtr<UCurveFloat> mCostMultiplierCurve = nullptr;

	/** Optional multiplier curve for inherited mAmounts item rewards. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Unlocks|Repeat Purchases")
	TObjectPtr<UCurveFloat> mRewardMultiplierCurve = nullptr;

	/** Optional full item-list replacements, persistent cost overrides, and exact-level extra costs. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Unlocks|Repeat Purchases")
	TArray<FKLRepeatPurchaseRewardLevel> mRewardItemsByLevel;

private:
	float EvaluateMultiplier(const UCurveFloat* Curve, int32 CompletedPurchaseCount) const;

	UPROPERTY(Transient)
	TArray<FItemAmount> mCachedBaseRewards;

	UPROPERTY(Transient)
	bool bHasCachedBaseRewards = false;

	UPROPERTY(Transient)
	TArray<FItemAmount> mCachedBaseCosts;

	UPROPERTY(Transient)
	bool bHasCachedBaseCosts = false;

	UPROPERTY(Transient)
	FText mCachedBaseDisplayName;

	UPROPERTY(Transient)
	bool bHasCachedBaseDisplayName = false;
};
