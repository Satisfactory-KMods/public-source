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

bool FKAPISlugIncubation::Valid() const
{
	return IsValid(mSlug) && mProductionCount > 0 && mProbability > 0.0f;
}

bool FKAPISlugFeeling::IsTempInRange(float Temp) const
{
	return UKismetMathLibrary::InRange_FloatFloat(Temp, mMinHeat, mMaxHeat);
}

bool FKAPISlugFeeling::IsHumidityInRange(float Humidity) const
{
	return UKismetMathLibrary::InRange_FloatFloat(Humidity, mMinHumidity, mMaxHumidity);
}

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

bool UKAPISugHatchingData::IncubationFluidRequired() const
{
	return bRequireFluid && IsValid(mFluidClass);
}

void UKAPISugHatchingData::GetComfortableSlugs(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlugs) const
{
	OutSlugs.Append(mComfortableWith);
	OutSlugs.AddUnique(mSlug);
}

bool UKAPISugHatchingData::RollSlugs(TArray<FItemAmount>& OutSlugs,
                                     TMap<TSubclassOf<UFGItemDescriptor>, float>& FixedChance,
                                     bool bUseFixedChance) const
{
	OutSlugs.Empty();
	for (const FKAPISlugIncubation& Incubation : mPossibleSlugs)
	{
		float Chance = Incubation.mProbability;
		if (bUseFixedChance && FixedChance.Contains(Incubation.mSlug))
		{
			Chance = FixedChance[Incubation.mSlug];
		}
		if (Incubation.Roll(Chance))
		{
			OutSlugs.Add(FItemAmount(Incubation.mSlug, Incubation.mProductionCount));
			FixedChance.Remove(Incubation.mSlug);
		}
		else if (bUseFixedChance && Incubation.bShouldUseFixedChance)
		{
			float CurrentBonus = FixedChance.FindOrAdd(Incubation.mSlug);
			CurrentBonus = FMath::Max(CurrentBonus, Incubation.mProductionCount);
			CurrentBonus += Incubation.mProbability * Incubation.mFixedChanceMultiplier;
			FixedChance.Add(Incubation.mSlug, FMath::Clamp(CurrentBonus, 0.0f, 100.0f));
		}
	}
	return !OutSlugs.IsEmpty();
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

FKAPISlugFeeling UKAPISugHatchingData::GetFeeling() const
{
	return FKAPISlugFeeling{
		mMinHumidity,
		mMaxHumidity,
		mMinHeat,
		mMaxHeat,
		mDayTime
	};
}

TArray<FKAPISlugIncubation> UKAPISugHatchingData::GetSlugIncubationsSortedByChance() const
{
	TArray<FKAPISlugIncubation> FilteredSlugs = mPossibleSlugs.FilterByPredicate(
		[](const FKAPISlugIncubation& Incubation)
		{
			return Incubation.Valid();
		});

	FilteredSlugs.Sort([](const FKAPISlugIncubation& A, const FKAPISlugIncubation& B)
	{
		return A.mProbability > B.mProbability;
	});

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

UKAPISugHatchingData* UKAPISugHatchingData::GetHighterSlugStatic(UKAPISugHatchingData* A,
                                                                 UKAPISugHatchingData* B)
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
		return B;
	}

	return A->GetHighterSlug(B);
}

UKAPISugHatchingData* UKAPISugHatchingData::GetHighterSlug(UKAPISugHatchingData* Other) const
{
	if (mSlugTier < Other->mSlugTier)
	{
		return Other;
	}
	return const_cast<UKAPISugHatchingData*>(this);
}

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
	return ComfortableSlugs.ContainsByPredicate(
		[&OtherComfortableSlugs](TSubclassOf<UFGItemDescriptor> Slug)
		{
			return OtherComfortableSlugs.Contains(Slug);
		});
}
