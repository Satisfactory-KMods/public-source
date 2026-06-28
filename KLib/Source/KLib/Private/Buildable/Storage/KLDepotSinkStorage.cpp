#include "Buildable/Storage/KLDepotSinkStorage.h"

#include "FGInventoryComponent.h"
#include "FGResourceSinkSubsystem.h"


AKLDepotSinkStorage::AKLDepotSinkStorage() : mResourceSinkSubsystem(nullptr)
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AKLDepotSinkStorage::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	CacheSubsystem();

	if (UFGInventoryComponent* Inv = GetStorageInventory())
	{
		Inv->OnItemAddedDelegate_Native.BindUObject(this, &AKLDepotSinkStorage::OnInventoryItemAdded);
	}

	TrySinkOverflow();
}

void AKLDepotSinkStorage::CacheSubsystem()
{
	mResourceSinkSubsystem = AFGResourceSinkSubsystem::Get(this);
	if (!IsValid(mResourceSinkSubsystem))
	{
		FTimerManager& TimerManager = this->GetWorldTimerManager();
		FTimerHandle TimerHandle;
		TimerManager.SetTimer(TimerHandle, this, &AKLDepotSinkStorage::CacheSubsystem, 0.1f, false);
	}
}

bool AKLDepotSinkStorage::CanProduce_Implementation() const
{
	if (IsPlayingBuildEffect())
	{
		return false;
	}

	return Super::CanProduce_Implementation();
}

void AKLDepotSinkStorage::OnInventoryItemAdded(TSubclassOf<UFGItemDescriptor> ItemClass, int32 NumAdded,
											   UFGInventoryComponent* SourceInventory)
{
	TrySinkOverflow();
}

void AKLDepotSinkStorage::TrySinkOverflow()
{
	if (!HasAuthority() || !IsValid(mResourceSinkSubsystem) || IsPlayingBuildEffect())
	{
		return;
	}

	UFGInventoryComponent* Inv = GetStorageInventory();
	if (!IsValid(Inv))
	{
		return;
	}

	// Only the last slot acts as the sink slot; the rest is left for normal depot upload.
	const int32 SlotIndex = Inv->GetSizeLinear() - 1;
	if (SlotIndex < 0 || !Inv->IsValidIndex(SlotIndex) || Inv->IsIndexEmpty(SlotIndex))
	{
		return;
	}

	FInventoryStack Stack;
	if (!Inv->GetStackFromIndex(SlotIndex, Stack) || !Stack.HasItems())
	{
		return;
	}

	// Keep the slot near-full so the depot keeps uploading; sink only the amount above the threshold.
	const int32 StackSize = UFGItemDescriptor::GetStackSize(Stack.Item.GetItemClass());
	const int32 KeepAmount = FMath::Clamp(static_cast<int32>(StackSize * mKeepStackFraction), 1, StackSize);
	const int32 Amount = Stack.NumItems;

	if (Amount <= KeepAmount)
	{
		return;
	}

	const int32 Overflow = Amount - KeepAmount;
	const int32 MaxToSink = (mMaxItemsToSinkPerCycle <= 0) ? Overflow : FMath::Min(mMaxItemsToSinkPerCycle, Overflow);

	int32 SunkCount = 0;
	for (int32 i = 0; i < MaxToSink; ++i)
	{
		if (!mResourceSinkSubsystem->AddPoints_ThreadSafe(Stack.Item.GetItemClass()))
		{
			break; // Item is not sinkable; leave the rest.
		}
		++SunkCount;
	}

	if (SunkCount > 0)
	{
		Inv->RemoveFromIndex(SlotIndex, SunkCount);
	}
}
