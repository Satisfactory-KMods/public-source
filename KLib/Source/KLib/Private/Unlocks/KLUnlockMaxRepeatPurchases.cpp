// ILikeBanas

#include "Unlocks/KLUnlockMaxRepeatPurchases.h"

#include "FGSchematic.h"

namespace
{
	int32 ScaleAmount(int32 Amount, float Multiplier)
	{
		const double ScaledAmount = static_cast<double>(Amount) * static_cast<double>(Multiplier);
		return static_cast<int32>(FMath::Clamp<int64>(FMath::RoundToInt64(ScaledAmount), 0, MAX_int32));
	}

	void AddCosts(TArray<FItemAmount>& Costs, const TArray<FItemAmount>& AdditionalCosts)
	{
		for (const FItemAmount& AdditionalCost : AdditionalCosts)
		{
			if (!AdditionalCost.ItemClass || AdditionalCost.Amount <= 0)
			{
				continue;
			}

			FItemAmount* ExistingCost = Costs.FindByPredicate([&AdditionalCost](const FItemAmount& Cost)
															  { return Cost.ItemClass == AdditionalCost.ItemClass; });
			if (ExistingCost != nullptr)
			{
				const int64 CombinedAmount = static_cast<int64>(ExistingCost->Amount) + AdditionalCost.Amount;
				ExistingCost->Amount = static_cast<int32>(FMath::Clamp<int64>(CombinedAmount, 0, MAX_int32));
			}
			else
			{
				Costs.Add(AdditionalCost);
			}
		}
	}
} // namespace

bool UKLUnlockMaxRepeatPurchases::IsRepeatPurchasesAllowed_Implementation() const { return true; }

UKLUnlockMaxRepeatPurchases* UKLUnlockMaxRepeatPurchases::FindOnSchematic(TSubclassOf<UFGSchematic> SchematicClass)
{
	IKPCLRepeatPurchaseUnlock* RepeatUnlock = IKPCLRepeatPurchaseUnlock::FindOnSchematic(SchematicClass);
	return RepeatUnlock != nullptr ? Cast<UKLUnlockMaxRepeatPurchases>(RepeatUnlock->_getUObject()) : nullptr;
}

float UKLUnlockMaxRepeatPurchases::GetCostMultiplier(int32 CompletedPurchaseCount) const
{
	return EvaluateMultiplier(mCostMultiplierCurve, CompletedPurchaseCount);
}

float UKLUnlockMaxRepeatPurchases::GetRewardMultiplier(int32 CompletedPurchaseCount) const
{
	return EvaluateMultiplier(mRewardMultiplierCurve, CompletedPurchaseCount);
}

void UKLUnlockMaxRepeatPurchases::ApplyLevelState(TSubclassOf<UFGSchematic> SchematicClass,
												  int32 CompletedPurchaseCount)
{
	if (!bHasCachedBaseRewards)
	{
		mCachedBaseRewards = mAmounts;
		bHasCachedBaseRewards = true;
	}
	UFGSchematic* SchematicCDO = SchematicClass ? SchematicClass.GetDefaultObject() : nullptr;
	if (IsValid(SchematicCDO) && !bHasCachedBaseCosts)
	{
		mCachedBaseCosts = SchematicCDO->mCost;
		bHasCachedBaseCosts = true;
	}
	if (IsValid(SchematicCDO) && !bHasCachedBaseDisplayName)
	{
		mCachedBaseDisplayName = SchematicCDO->mDisplayName;
		bHasCachedBaseDisplayName = true;
	}

	mAmounts = mCachedBaseRewards;
	if (IsValid(SchematicCDO) && bHasCachedBaseCosts)
	{
		SchematicCDO->mCost = mCachedBaseCosts;
	}
	const int32 PurchaseLevel =
		static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(CompletedPurchaseCount) + 1, 1, MAX_int32));
	const int32 DisplayPhase = mMaxPurchaseCount > 0 ? FMath::Min(PurchaseLevel, mMaxPurchaseCount) : PurchaseLevel;
	if (IsValid(SchematicCDO) && bHasCachedBaseDisplayName)
	{
		if (mMaxPurchaseCount > 0)
		{
			SchematicCDO->mDisplayName = FText::Format(
				NSLOCTEXT("KLib", "RepeatPurchaseSchematicFinitePhaseName", "{0} | Repeat {1} / {2}"),
				mCachedBaseDisplayName, FText::AsNumber(DisplayPhase), FText::AsNumber(mMaxPurchaseCount));
		}
		else
		{
			SchematicCDO->mDisplayName =
				FText::Format(NSLOCTEXT("KLib", "RepeatPurchaseSchematicInfinitePhaseName", "{0} | Repeat {1}"),
							  mCachedBaseDisplayName, FText::AsNumber(DisplayPhase));
		}
	}
	const FKLRepeatPurchaseRewardLevel* ActiveRewardLevel = nullptr;
	const FKLRepeatPurchaseRewardLevel* ActiveCostLevel = nullptr;
	for (const FKLRepeatPurchaseRewardLevel& RewardLevel : mRewardItemsByLevel)
	{
		if (RewardLevel.mPurchaseLevel >= 1 && RewardLevel.mPurchaseLevel <= PurchaseLevel &&
			(ActiveRewardLevel == nullptr || RewardLevel.mPurchaseLevel > ActiveRewardLevel->mPurchaseLevel))
		{
			ActiveRewardLevel = &RewardLevel;
		}
		if (RewardLevel.bOverrideCosts && RewardLevel.mPurchaseLevel >= 1 &&
			RewardLevel.mPurchaseLevel <= PurchaseLevel &&
			(ActiveCostLevel == nullptr || RewardLevel.mPurchaseLevel > ActiveCostLevel->mPurchaseLevel))
		{
			ActiveCostLevel = &RewardLevel;
		}
	}
	if (ActiveRewardLevel != nullptr)
	{
		mAmounts = ActiveRewardLevel->mItemsToGive;
	}
	if (IsValid(SchematicCDO) && ActiveCostLevel != nullptr)
	{
		SchematicCDO->mCost = ActiveCostLevel->mCosts;
	}
	if (IsValid(SchematicCDO))
	{
		for (const FKLRepeatPurchaseRewardLevel& RewardLevel : mRewardItemsByLevel)
		{
			if (RewardLevel.mPurchaseLevel == PurchaseLevel)
			{
				AddCosts(SchematicCDO->mCost, RewardLevel.mAdditionalCosts);
			}
		}
	}

	if (!IsValid(mRewardMultiplierCurve))
	{
		return;
	}

	const float Multiplier = GetRewardMultiplier(CompletedPurchaseCount);
	for (FItemAmount& Reward : mAmounts)
	{
		Reward.Amount = ScaleAmount(Reward.Amount, Multiplier);
	}
}

float UKLUnlockMaxRepeatPurchases::EvaluateMultiplier(const UCurveFloat* Curve, int32 CompletedPurchaseCount) const
{
	if (!IsValid(Curve))
	{
		return 1.0f;
	}

	const float Multiplier = Curve->GetFloatValue(static_cast<float>(FMath::Max(0, CompletedPurchaseCount)));
	return FMath::IsFinite(Multiplier) ? FMath::Max(0.0f, Multiplier) : 1.0f;
}
