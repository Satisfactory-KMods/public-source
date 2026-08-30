
#include "DataAssets/KAPIDeliveryTask.h"

#include "Curves/CurveFloat.h"
#include "FGSchematic.h"
#include "FGSchematicManager.h"
#include "FGStorySubsystem.h"
#include "FGUnlockSubsystem.h"
#include "KAPIGameplayTags.h"
#include "KAPIModule.h"
#include "Resources/FGItemDescriptor.h"
#include "Unlocks/FGUnlock.h"
#include "Unlocks/FGUnlockArmEquipmentSlot.h"
#include "Unlocks/FGUnlockInventorySlot.h"

namespace
{
	constexpr int32 ResolveDirectSlotUnlockTarget(const int32 CurrentTotal, const int32 RequestedSlots)
	{
		if (RequestedSlots <= 0)
		{
			return FMath::Max(0, CurrentTotal);
		}
		return static_cast<int32>(FMath::Min<int64>(
			static_cast<int64>(FMath::Max(0, CurrentTotal)) + static_cast<int64>(RequestedSlots), MAX_int32));
	}

	static_assert(ResolveDirectSlotUnlockTarget(143, 1) == 144);
	static_assert(ResolveDirectSlotUnlockTarget(144, 1) == 145);
	static_assert(ResolveDirectSlotUnlockTarget(143, 0) == 143);
	static_assert(ResolveDirectSlotUnlockTarget(MAX_int32, 1) == MAX_int32);

	constexpr int32 ResolvePoolSelectionCount(const int32 EligibleElementCount, const int32 MaxPoolItems)
	{
		if (EligibleElementCount <= 0)
		{
			return 0;
		}
		if (MaxPoolItems == INDEX_NONE || MaxPoolItems >= EligibleElementCount)
		{
			return EligibleElementCount;
		}
		return MaxPoolItems > 0 ? MaxPoolItems : 0;
	}

	static_assert(ResolvePoolSelectionCount(14, INDEX_NONE) == 14);
	static_assert(ResolvePoolSelectionCount(14, 5) == 5);
	static_assert(ResolvePoolSelectionCount(3, 5) == 3);
}

UKAPIDeliveryTask::UKAPIDeliveryTask() { mParentTask = 0; }

FItemAmount UKAPIDeliveryTask::GetPoolElementAmount(const FKAPIItemPoolElement& Element, int32 MaxAmount,
													float Multiplier)
{
	FItemAmount NewAmount;
	NewAmount.ItemClass = Element.mItem;

	const FInt32RangeBound& SourceLowerRangeBound = Element.mAmountRange.GetLowerBound();
	const FInt32RangeBound& SourceUpperRangeBound = Element.mAmountRange.GetUpperBound();
	if (SourceLowerRangeBound.IsOpen() || SourceUpperRangeBound.IsOpen())
	{
		NewAmount.Amount = 0;
		return NewAmount;
	}
	const int32 SourceLowerBound = SourceLowerRangeBound.GetValue() + (SourceLowerRangeBound.IsExclusive() ? 1 : 0);
	const int32 SourceUpperBound = SourceUpperRangeBound.GetValue() - (SourceUpperRangeBound.IsExclusive() ? 1 : 0);
	if (SourceLowerBound > SourceUpperBound)
	{
		NewAmount.Amount = 0;
		return NewAmount;
	}
	const int32 ClampedMaxAmount = FMath::Max(0, MaxAmount);
	const double MaxMultiplier =
		SourceUpperBound > 0 ? static_cast<double>(ClampedMaxAmount) / static_cast<double>(SourceUpperBound) : 0.0;
	const double EffectiveMultiplier = FMath::Min(FMath::Max(0.0, static_cast<double>(Multiplier)), MaxMultiplier);

	const int32 LowerBound =
		FMath::Clamp(FMath::RoundToInt32(SourceLowerBound * EffectiveMultiplier), 0, ClampedMaxAmount);
	const int32 UpperBound =
		FMath::Clamp(FMath::RoundToInt32(SourceUpperBound * EffectiveMultiplier), LowerBound, ClampedMaxAmount);
	NewAmount.Amount = FMath::RandRange(LowerBound, UpperBound);

	return NewAmount;
}

void UKAPIDeliveryTask::AssertTask(const TMap<FIntVector2, TObjectPtr<UKAPIDeliveryTask>>& TaskMap) const
{
	const auto FatalIf = [this](bool bFailed, const FString& Reason)
	{
		if (bFailed)
		{
			UE_LOG(LogKApi, Fatal, TEXT("Invalid delivery task '%s': %s"), *GetPathName(), *Reason);
		}
	};

	FatalIf(mName.IsEmpty(), TEXT("mName is empty"));
	FatalIf(mDescription.IsEmpty(), TEXT("mDescription is empty"));
	FatalIf(!IsValid(mIcon), TEXT("mIcon is invalid"));
	for (int32 Index = 0; Index < mTaskDependencies.Num(); ++Index)
	{
		FatalIf(!IsValid(mTaskDependencies[Index]), FString::Printf(TEXT("mTaskDependencies[%d] is invalid"), Index));
	}
	for (int32 Index = 0; Index < mUnlocks.Num(); ++Index)
	{
		FatalIf(!IsValid(mUnlocks[Index]), FString::Printf(TEXT("mUnlocks[%d] is invalid"), Index));
	}

	const TObjectPtr<UKAPIDeliveryTask>* RegisteredTask = TaskMap.Find(mCoordinate);
	FatalIf(!RegisteredTask || RegisteredTask->Get() != this,
			FString::Printf(TEXT("mCoordinate (%d, %d) is missing or occupied by another task"), mCoordinate.X,
							mCoordinate.Y));
	FatalIf(mRequiredParentCompletions < 1, TEXT("mRequiredParentCompletions must be at least one"));

	for (const FIntVector2& ParentCoordinate : GetParentCoordinates())
	{
		const TObjectPtr<UKAPIDeliveryTask>* ParentTask = TaskMap.Find(ParentCoordinate);
		FatalIf(ParentTask && ParentTask->Get() == this, TEXT("a delivery task cannot be its own parent"));
	}

	TSet<const UKAPIDeliveryTask*> VisitingTasks;
	TSet<const UKAPIDeliveryTask*> VisitedTasks;
	TFunction<bool(const UKAPIDeliveryTask*)> HasParentCycle =
		[&TaskMap, &VisitingTasks, &VisitedTasks, &HasParentCycle](const UKAPIDeliveryTask* Task)
	{
		if (VisitedTasks.Contains(Task))
		{
			return false;
		}
		if (VisitingTasks.Contains(Task))
		{
			return true;
		}

		VisitingTasks.Add(Task);
		for (const FIntVector2& ParentCoordinate : Task->GetParentCoordinates())
		{
			const TObjectPtr<UKAPIDeliveryTask>* Parent = TaskMap.Find(ParentCoordinate);
			if (Parent && IsValid(*Parent) && HasParentCycle(Parent->Get()))
			{
				return true;
			}
		}
		VisitingTasks.Remove(Task);
		VisitedTasks.Add(Task);
		return false;
	};
	FatalIf(HasParentCycle(this), TEXT("delivery task parent graph contains a cycle"));

	const auto AssertItemAmounts =
		[&FatalIf](const TArray<FItemAmount>& ItemAmounts, const TCHAR* FieldName, bool bMustNotBeEmpty)
	{
		FatalIf(bMustNotBeEmpty && ItemAmounts.IsEmpty(), FString::Printf(TEXT("%s is empty"), FieldName));
		for (int32 Index = 0; Index < ItemAmounts.Num(); ++Index)
		{
			const FItemAmount& ItemAmount = ItemAmounts[Index];
			FatalIf(!IsValid(ItemAmount.ItemClass),
					FString::Printf(TEXT("%s[%d].ItemClass is invalid"), FieldName, Index));
			FatalIf(ItemAmount.Amount <= 0,
					FString::Printf(TEXT("%s[%d].Amount must be greater than zero"), FieldName, Index));
		}
	};

	if (!bIsEndless)
	{
		AssertItemAmounts(mCosts, TEXT("mCosts"), true);

		AssertItemAmounts(mRewards, TEXT("mRewards"), false);

		FatalIf(!IsGateUnlockTask() && mRewards.IsEmpty() && mUnlocks.IsEmpty() && mSchematics.IsEmpty(),
				TEXT("a one-time task must grant at least one of mRewards, mUnlocks or mSchematics, or carry the "
					 "KMods.Delivery.GateUnlock tag"));

		for (int32 Index = 0; Index < mSchematics.Num(); ++Index)
		{
			FatalIf(!IsValid(mSchematics[Index]), FString::Printf(TEXT("mSchematics[%d] is invalid"), Index));
		}
		return;
	}

	FatalIf(mPoolElements.IsEmpty(), TEXT("mPoolElements is empty"));
	FatalIf(mMaxPoolItems != INDEX_NONE && mMaxPoolItems <= 0,
			TEXT("mMaxPoolItems must be INDEX_NONE or greater than zero"));
	if (mMaxPoolItems != INDEX_NONE)
	{
		UE_LOG(LogKApi, Display, TEXT("Delivery task '%s' limits each pool roll to %d items."), *mName.ToString(),
			   mMaxPoolItems);
	}
	FatalIf(mTaskRewards.IsEmpty(), TEXT("mTaskRewards is empty"));
	FatalIf(mMaxTasks != INDEX_NONE && mMaxTasks <= 0, TEXT("mMaxTasks must be INDEX_NONE or greater than zero"));
	FatalIf(!IsValid(mAmountMultiplierCurve) || mAmountMultiplierCurve->FloatCurve.GetNumKeys() == 0,
			TEXT("mAmountMultiplierCurve is invalid or has no keys"));
	FatalIf(mAbsolutMaxItemAmount.IsEmpty(), TEXT("mAbsolutMaxItemAmount is empty"));

	for (const TPair<EResourceForm, TObjectPtr<UCurveFloat>>& MaxAmount : mAbsolutMaxItemAmount)
	{
		FatalIf(MaxAmount.Key == EResourceForm::RF_INVALID || MaxAmount.Key == EResourceForm::RF_LAST_ENUM,
				TEXT("mAbsolutMaxItemAmount contains an invalid resource form"));
		FatalIf(!IsValid(MaxAmount.Value) || MaxAmount.Value->FloatCurve.GetNumKeys() == 0,
				TEXT("mAbsolutMaxItemAmount values must be valid curves with at least one key"));
	}

	for (int32 Index = 0; Index < mPoolElements.Num(); ++Index)
	{
		const FKAPIItemPoolElement& PoolElement = mPoolElements[Index];
		FatalIf(!IsValid(PoolElement.mItem), FString::Printf(TEXT("mPoolElements[%d].mItem is invalid"), Index));
		FatalIf(!PoolElement.mAmountRange.HasLowerBound() || !PoolElement.mAmountRange.HasUpperBound() ||
					PoolElement.mAmountRange.IsEmpty(),
				FString::Printf(TEXT("mPoolElements[%d].mAmountRange must be finite and non-empty"), Index));
		FatalIf(PoolElement.mAmountRange.HasLowerBound() && PoolElement.mAmountRange.GetLowerBoundValue() <= 0,
				FString::Printf(TEXT("mPoolElements[%d].mAmountRange lower bound must be greater than zero"), Index));
		FatalIf(PoolElement.mRequiredTas < INDEX_NONE,
				FString::Printf(TEXT("mPoolElements[%d].mRequiredTas must be INDEX_NONE or non-negative"), Index));
		FatalIf(mMaxTasks != INDEX_NONE && PoolElement.mRequiredTas >= mMaxTasks,
				FString::Printf(TEXT("mPoolElements[%d].mRequiredTas exceeds mMaxTasks"), Index));

		if (IsValid(PoolElement.mItem))
		{
			const EResourceForm ResourceForm = UFGItemDescriptor::GetForm(PoolElement.mItem);
			FatalIf(!mAbsolutMaxItemAmount.Contains(ResourceForm),
					FString::Printf(TEXT("mPoolElements[%d] has no mAbsolutMaxItemAmount entry for resource form %d"),
									Index, static_cast<int32>(ResourceForm)));
		}
	}

	for (int32 Index = 0; Index < mTaskRewards.Num(); ++Index)
	{
		const FKAPITaskReward& TaskReward = mTaskRewards[Index];
		AssertItemAmounts(TaskReward.mRewards, *FString::Printf(TEXT("mTaskRewards[%d].mRewards"), Index), true);
		FatalIf(TaskReward.mRequiredTask < INDEX_NONE,
				FString::Printf(TEXT("mTaskRewards[%d].mRequiredTask must be INDEX_NONE or non-negative"), Index));
		FatalIf(mMaxTasks != INDEX_NONE && TaskReward.mRequiredTask >= mMaxTasks,
				FString::Printf(TEXT("mTaskRewards[%d].mRequiredTask exceeds mMaxTasks"), Index));
	}
}

void UKAPIDeliveryTask::GetPoolElementsAmount(TArray<FItemAmount>& OutElements, int32 TaskCount) const
{
	OutElements.Reset();

	if (!bIsEndless || TaskCount < 0 || (mMaxTasks != INDEX_NONE && TaskCount >= mMaxTasks) ||
		!IsValid(mAmountMultiplierCurve))
	{
		return;
	}

	TArray<int32> EligiblePoolElementIndices;
	EligiblePoolElementIndices.Reserve(mPoolElements.Num());
	for (int32 PoolElementIndex = 0; PoolElementIndex < mPoolElements.Num(); ++PoolElementIndex)
	{
		const FKAPIItemPoolElement& PoolElement = mPoolElements[PoolElementIndex];
		if (!PoolElement.Valid() || (PoolElement.mRequiredTas != INDEX_NONE && PoolElement.mRequiredTas > TaskCount))
		{
			continue;
		}

		const EResourceForm ResourceForm = UFGItemDescriptor::GetForm(PoolElement.mItem);
		const TObjectPtr<UCurveFloat>* MaxAmountCurve = mAbsolutMaxItemAmount.Find(ResourceForm);
		if (!MaxAmountCurve || !IsValid(*MaxAmountCurve))
		{
			continue;
		}
		EligiblePoolElementIndices.Add(PoolElementIndex);
	}

	const int32 EligiblePoolElementCount = EligiblePoolElementIndices.Num();
	const int32 SelectionCount = ResolvePoolSelectionCount(EligiblePoolElementCount, mMaxPoolItems);
	if (SelectionCount < EligiblePoolElementCount)
	{
		for (int32 SelectionIndex = 0; SelectionIndex < SelectionCount; ++SelectionIndex)
		{
			const int32 SwapIndex = FMath::RandRange(SelectionIndex, EligiblePoolElementIndices.Num() - 1);
			EligiblePoolElementIndices.Swap(SelectionIndex, SwapIndex);
		}
		EligiblePoolElementIndices.SetNum(SelectionCount, EAllowShrinking::No);
		EligiblePoolElementIndices.Sort();
		UE_LOG(LogKApi, Display, TEXT("Delivery task '%s' selected %d of %d eligible pool items (limit %d)."),
			   *GetPathName(), SelectionCount, EligiblePoolElementCount, mMaxPoolItems);
	}

	const float Multiplier = mAmountMultiplierCurve->GetFloatValue(static_cast<float>(TaskCount));
	for (const int32 PoolElementIndex : EligiblePoolElementIndices)
	{
		const FKAPIItemPoolElement& PoolElement = mPoolElements[PoolElementIndex];
		const EResourceForm ResourceForm = UFGItemDescriptor::GetForm(PoolElement.mItem);
		const TObjectPtr<UCurveFloat>* MaxAmountCurve = mAbsolutMaxItemAmount.Find(ResourceForm);
		if (!MaxAmountCurve || !IsValid(*MaxAmountCurve))
		{
			continue;
		}

		const int32 MaxAmount =
			FMath::Max(0, FMath::RoundToInt32((*MaxAmountCurve)->GetFloatValue(static_cast<float>(TaskCount))));
		FItemAmount ItemAmount = GetPoolElementAmount(PoolElement, MaxAmount, Multiplier);
		if (ItemAmount.Amount > 0)
		{
			OutElements.Add(MoveTemp(ItemAmount));
		}
	}
}

void UKAPIDeliveryTask::GetAllUnlocks(TArray<UFGUnlock*>& OutUnlocks) const
{
	OutUnlocks.Reset();

	for (const TObjectPtr<UFGUnlock>& Unlock : mUnlocks)
	{
		if (IsValid(Unlock))
		{
			OutUnlocks.AddUnique(Unlock.Get());
		}
	}

	for (const TSubclassOf<UFGSchematic>& Schematic : mSchematics)
	{
		if (!IsValid(Schematic))
		{
			continue;
		}
		for (UFGUnlock* SchematicUnlock : UFGSchematic::GetUnlocks(Schematic))
		{
			if (IsValid(SchematicUnlock))
			{
				OutUnlocks.AddUnique(SchematicUnlock);
			}
		}
	}
}

void UKAPIDeliveryTask::GetAllDependencies(TArray<UFGAvailabilityDependency*>& OutDependencies) const
{
	OutDependencies.Reset();

	for (const TObjectPtr<UFGAvailabilityDependency>& Dependency : mTaskDependencies)
	{
		if (IsValid(Dependency))
		{
			OutDependencies.AddUnique(Dependency.Get());
		}
	}

	for (const TSubclassOf<UFGSchematic>& Schematic : mSchematics)
	{
		if (!IsValid(Schematic))
		{
			continue;
		}

		TArray<UFGAvailabilityDependency*> SchematicDependencies;
		UFGSchematic::GetSchematicDependencies(Schematic, SchematicDependencies);
		for (UFGAvailabilityDependency* Dependency : SchematicDependencies)
		{
			if (IsValid(Dependency))
			{
				OutDependencies.AddUnique(Dependency);
			}
		}
	}
}

bool UKAPIDeliveryTask::AreTaskDependenciesMet(UObject* WorldContextObject) const
{
	TArray<UFGAvailabilityDependency*> Dependencies;
	GetAllDependencies(Dependencies);

	for (const UFGAvailabilityDependency* Dependency : Dependencies)
	{
		if (!Dependency->AreDependenciesMet(WorldContextObject))
		{
			return false;
		}
	}
	return true;
}

TArray<FIntVector2> UKAPIDeliveryTask::GetParentCoordinates() const
{
	TArray<FIntVector2> ParentCoordinates;
	const auto AddParent = [this, &ParentCoordinates](EKAPIDeliveryTaskParent Direction, const FIntVector2& Offset)
	{
		if (EnumHasAnyFlags(static_cast<EKAPIDeliveryTaskParent>(mParentTask), Direction))
		{
			ParentCoordinates.Add(mCoordinate + Offset);
		}
	};

	AddParent(EKAPIDeliveryTaskParent::Above, FIntVector2(0, -1));
	AddParent(EKAPIDeliveryTaskParent::Left, FIntVector2(-1, 0));
	AddParent(EKAPIDeliveryTaskParent::Right, FIntVector2(1, 0));
	AddParent(EKAPIDeliveryTaskParent::Bottom, FIntVector2(0, 1));
	AddParent(EKAPIDeliveryTaskParent::AboveLeft, FIntVector2(-1, -1));
	AddParent(EKAPIDeliveryTaskParent::AboveRight, FIntVector2(1, -1));
	AddParent(EKAPIDeliveryTaskParent::BelowLeft, FIntVector2(-1, 1));
	AddParent(EKAPIDeliveryTaskParent::BelowRight, FIntVector2(1, 1));
	return ParentCoordinates;
}

bool UKAPIDeliveryTask::IsSchematicOnlyTask() const
{
	if (bIsEndless || !mRewards.IsEmpty())
	{
		return false;
	}

	for (const TObjectPtr<UFGUnlock>& Unlock : mUnlocks)
	{
		if (IsValid(Unlock))
		{
			return false;
		}
	}

	for (const TSubclassOf<UFGSchematic>& Schematic : mSchematics)
	{
		if (IsValid(Schematic))
		{
			return true;
		}
	}
	return false;
}

bool UKAPIDeliveryTask::IsGateUnlockTask() const { return !bIsEndless && HasTag(TAG_KMods_DeliveryGateUnlock); }

void UKAPIDeliveryTask::ForwardAdaMessages(UObject* WorldContextObject) const
{
	if (mAdaMessages.IsEmpty())
	{
		return;
	}

	AFGStorySubsystem* StorySubsystem = AFGStorySubsystem::Get(WorldContextObject);
	if (IsValid(StorySubsystem))
	{
		StorySubsystem->ForwardMessagesToGameUI(mAdaMessages);
	}
}

void UKAPIDeliveryTask::HandleUnlock(UObject* WorldContextObject) const
{
	ForwardAdaMessages(WorldContextObject);

	if (!mSchematics.IsEmpty() && !bIsEndless)
	{
		AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(WorldContextObject);
		if (IsValid(SchematicManager))
		{
			SchematicManager->GiveAccessToSchematics(
				mSchematics, nullptr, ESchematicUnlockFlags::Force | ESchematicUnlockFlags::SuppressNarrativeMessages);
		}
	}

	if (AFGUnlockSubsystem* UnlockSubsystem = AFGUnlockSubsystem::Get(WorldContextObject))
	{
		if (!UnlockSubsystem->HasAuthority())
		{
			UE_LOG(LogKApi, Error, TEXT("Cannot grant delivery task '%s' unlocks without authority."), *GetPathName());
			return;
		}

		for (const TObjectPtr<UFGUnlock>& Unlock : mUnlocks)
		{
			if (IsValid(Unlock))
			{
				const UFGUnlockInventorySlot* InventorySlotUnlock = Cast<UFGUnlockInventorySlot>(Unlock);
				const UFGUnlockArmEquipmentSlot* ArmSlotUnlock = Cast<UFGUnlockArmEquipmentSlot>(Unlock);
				const bool bIsSlotUnlock = InventorySlotUnlock || ArmSlotUnlock;
				const int32 PreviousSlots = InventorySlotUnlock
					? UnlockSubsystem->GetNumTotalInventorySlots()
					: (ArmSlotUnlock ? UnlockSubsystem->GetNumTotalArmEquipmentSlots() : 0);
				const int32 RequestedSlots = InventorySlotUnlock
					? InventorySlotUnlock->GetNumInventorySlotsToUnlock()
					: (ArmSlotUnlock ? ArmSlotUnlock->GetNumArmEquipmentSlotsToUnlock() : 0);
				const int32 GuaranteedSlots = ResolveDirectSlotUnlockTarget(PreviousSlots, RequestedSlots);
				if (bIsSlotUnlock && RequestedSlots > 0 && GuaranteedSlots > PreviousSlots)
				{

					if (InventorySlotUnlock)
					{
						UnlockSubsystem->SetTotalNumInventorySlots(GuaranteedSlots);
					}
					else
					{
						UnlockSubsystem->SetTotalNumArmEquipmentSlots(GuaranteedSlots);
					}
				}
				Unlock->Unlock(UnlockSubsystem);

				if (bIsSlotUnlock)
				{
					const int32 NewSlots = InventorySlotUnlock ? UnlockSubsystem->GetNumTotalInventorySlots()
													   : UnlockSubsystem->GetNumTotalArmEquipmentSlots();
					const TCHAR* SlotType = InventorySlotUnlock ? TEXT("player inventory") : TEXT("arm equipment");
					if (RequestedSlots <= 0)
					{
						UE_LOG(LogKApi, Error,
							   TEXT("Delivery task '%s' has invalid %s slot unlock '%s' amount %d."), *GetPathName(),
							   SlotType, *Unlock->GetPathName(), RequestedSlots);
					}
					else if (NewSlots < GuaranteedSlots)
					{
						UE_LOG(LogKApi, Error,
							   TEXT("Delivery task '%s' failed to grant %d %s slots through '%s' "
									"(expected at least %d, reported %d)."),
							   *GetPathName(), RequestedSlots, SlotType, *Unlock->GetPathName(), GuaranteedSlots, NewSlots);
					}
				}
			}
		}
	}
}

void UKAPIDeliveryTask::ApplyUnlocks(UObject* WorldContextObject) const
{
	if (mUnlocks.IsEmpty())
	{
		return;
	}

	AFGUnlockSubsystem* UnlockSubsystem = AFGUnlockSubsystem::Get(WorldContextObject);
	if (!IsValid(UnlockSubsystem) || !UnlockSubsystem->HasAuthority())
	{
		return;
	}

	for (const TObjectPtr<UFGUnlock>& Unlock : mUnlocks)
	{
		if (IsValid(Unlock))
		{
			Unlock->Apply(UnlockSubsystem);
		}
	}
}
