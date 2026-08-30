#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Unlocks/KLGiveItemToAllPlayerUnlock.h"
#include "Unlocks/KPCLRepeatPurchaseUnlock.h"

#include "KLUnlockMaxRepeatPurchases.generated.h"

USTRUCT(BlueprintType)
struct KLIB_API FKLRepeatPurchaseRewardLevel
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repeat Purchases", meta = (ClampMin = "1", UIMin = "1"))
	int32 mPurchaseLevel = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repeat Purchases")
	TArray<FItemAmount> mItemsToGive;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repeat Purchases")
	bool bOverrideCosts = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repeat Purchases",
			  meta = (EditCondition = "bOverrideCosts"))
	TArray<FItemAmount> mCosts;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repeat Purchases")
	TArray<FItemAmount> mAdditionalCosts;
};

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class KLIB_API UKLUnlockMaxRepeatPurchases : public UKLGiveItemToAllPlayerUnlock, public IKPCLRepeatPurchaseUnlock
{
	GENERATED_BODY()

public:

	virtual bool IsRepeatPurchasesAllowed_Implementation() const override;

	virtual int32 GetMaxPurchaseCount() const override { return mMaxPurchaseCount; }
	virtual float GetCostMultiplier(int32 CompletedPurchaseCount) const override;
	virtual float GetRewardMultiplier(int32 CompletedPurchaseCount) const override;
	virtual void ApplyLevelState(TSubclassOf<UFGSchematic> SchematicClass, int32 CompletedPurchaseCount) override;

	UFUNCTION(BlueprintPure, Category = "KMods|Unlocks|Repeat Purchases")
	static UKLUnlockMaxRepeatPurchases* FindOnSchematic(TSubclassOf<UFGSchematic> SchematicClass);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Unlocks|Repeat Purchases")
	int32 mMaxPurchaseCount = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Unlocks|Repeat Purchases")
	TObjectPtr<UCurveFloat> mCostMultiplierCurve = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Unlocks|Repeat Purchases")
	TObjectPtr<UCurveFloat> mRewardMultiplierCurve = nullptr;

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
