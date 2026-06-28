#include "DataAssets/KAPISugHatchingData.h"
#include "Kismet/KismetMathLibrary.h"

bool FKAPISlugIncubation::Roll(float ChanceOverwrite) const
{
	if (!Valid())
	{
		return false;
	}
	const float Chance = ChanceOverwrite >= 0.0f ? ChanceOverwrite : mProbability;
	return FMath::FRandRange(0.0f, 100.0f) <= Chance;
}

bool FKAPISlugIncubation::Valid() const { return IsValid(mSlug) && mProductionCount > 0 && mProbability > 0.0f; }

bool FKAPISlugFeeling::IsDayTimeValid(EKAPISlugTime Time) const
{
	if (mDayTime == EKAPISlugTime::NONE || Time == EKAPISlugTime::NONE)
	{
		return false;
	}
	if (mDayTime == EKAPISlugTime::Any || Time == EKAPISlugTime::Any)
	{
		return true;
	}

	return Time == mDayTime;
}

bool FKAPISlugFeeling::IsHumidityInRange(float Humidity) const
{
	return UKismetMathLibrary::InRange_FloatFloat(Humidity, mMinHumidity, mMaxHumidity);
}

bool FKAPISlugFeeling::IsTempInRange(float Temp) const
{
	return UKismetMathLibrary::InRange_FloatFloat(Temp, mMinHeat, mMaxHeat);
}

void UKAPISugHatchingData::GetComfortableSlugs(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlugs) const
{
	OutSlugs.Append(mComfortableWith);
	OutSlugs.AddUnique(mSlug);
}

FKAPISlugFeeling UKAPISugHatchingData::GetFeeling() const
{
	return FKAPISlugFeeling{mMinHumidity, mMaxHumidity, mMinHeat, mMaxHeat, mDayTime};
}

UKAPISugHatchingData* UKAPISugHatchingData::GetHighterSlug(UKAPISugHatchingData* Other) const
{
	if (!IsValid(Other))
	{
		return const_cast<UKAPISugHatchingData*>(this);
	}
	if (mSlugTier < Other->mSlugTier)
	{
		return Other;
	}
	return const_cast<UKAPISugHatchingData*>(this);
}

UKAPISugHatchingData* UKAPISugHatchingData::GetHighterSlugStatic(UKAPISugHatchingData* A, UKAPISugHatchingData* B)
{
	if (!IsValid(A) && !IsValid(B))
	{
		return nullptr;
	}
	if (!IsValid(A) && IsValid(B))
	{
		return B;
	}
	if (IsValid(A) && !IsValid(B))
	{
		return A;
	}

	return A->GetHighterSlug(B);
}

TArray<TSubclassOf<UFGItemDescriptor>> UKAPISugHatchingData::GetPossibleSlugs() const
{
	TArray<TSubclassOf<UFGItemDescriptor>> Result;
	for (const FKAPISlugIncubation& Incubation : mPossibleSlugs)
	{
		if (Incubation.Valid())
		{
			Result.AddUnique(Incubation.mSlug);
		}
	}
	return Result;
}

TArray<FKAPISlugIncubation> UKAPISugHatchingData::GetSlugIncubationsSortedByChance() const
{
	TArray<FKAPISlugIncubation> FilteredSlugs =
		mPossibleSlugs.FilterByPredicate([](const FKAPISlugIncubation& Incubation) { return Incubation.Valid(); });

	FilteredSlugs.Sort([](const FKAPISlugIncubation& A, const FKAPISlugIncubation& B)
					   { return A.mProbability > B.mProbability; });

	return FilteredSlugs;
}

TArray<FItemAmount> UKAPISugHatchingData::GetSlugsForThisCycle() const
{
	TArray<FItemAmount> Result;

	TArray<FKAPISlugIncubation> SortedSlugs = GetSlugIncubationsSortedByChance();

	for (const FKAPISlugIncubation& Incubation : SortedSlugs)
	{
		if (Incubation.Roll())
		{
			Result.Add(FItemAmount(Incubation.mSlug, Incubation.mProductionCount));
		}
	}

	return Result;
}

bool UKAPISugHatchingData::IncubationFluidRequired() const { return bRequireFluid && IsValid(mFluidClass); }

bool UKAPISugHatchingData::IsComfortableWith(UKAPISugHatchingData* Other) const
{
	if (!IsValid(Other))
	{
		return false;
	}

	if (Other == this)
	{
		return true;
	}

	TArray<TSubclassOf<UFGItemDescriptor>> ComfortableSlugs;
	GetComfortableSlugs(ComfortableSlugs);
	TArray<TSubclassOf<UFGItemDescriptor>> OtherComfortableSlugs;
	Other->GetComfortableSlugs(OtherComfortableSlugs);

	// Intersect if the two arrays have at least one common slug
	return ComfortableSlugs.ContainsByPredicate([&OtherComfortableSlugs](TSubclassOf<UFGItemDescriptor> Slug)
												{ return OtherComfortableSlugs.Contains(Slug); });
}

bool UKAPISugHatchingData::RollSlugs(TArray<FItemAmount>& OutSlugs, TArray<FKAPISlugFixedChance>& FixedChance,
									 bool bUseFixedChance) const
{
	OutSlugs.Empty();
	for (const FKAPISlugIncubation& Incubation : mPossibleSlugs)
	{
		const int32 ExistingIndex = FixedChance.IndexOfByPredicate([&Incubation](const FKAPISlugFixedChance& Entry)
																   { return Entry.mSlug == Incubation.mSlug; });

		float Chance = Incubation.mProbability;
		if (bUseFixedChance && ExistingIndex != INDEX_NONE)
		{
			Chance = FixedChance[ExistingIndex].mChance;
		}
		if (Incubation.Roll(Chance))
		{
			OutSlugs.Add(FItemAmount(Incubation.mSlug, Incubation.mProductionCount));
			if (ExistingIndex != INDEX_NONE)
			{
				FixedChance.RemoveAt(ExistingIndex);
			}
		}
		else if (bUseFixedChance && Incubation.bShouldUseFixedChance)
		{
			float CurrentBonus = ExistingIndex != INDEX_NONE ? FixedChance[ExistingIndex].mChance : 0.f;
			CurrentBonus = FMath::Max(CurrentBonus, Incubation.mProbability);
			CurrentBonus += Incubation.mProbability * Incubation.mFixedChanceMultiplier;
			CurrentBonus = FMath::Clamp(CurrentBonus, 0.0f, 100.0f);

			if (ExistingIndex != INDEX_NONE)
			{
				FixedChance[ExistingIndex].mChance = CurrentBonus;
			}
			else
			{
				FixedChance.Emplace(Incubation.mSlug, CurrentBonus);
			}
		}
	}
	return !OutSlugs.IsEmpty();
}
