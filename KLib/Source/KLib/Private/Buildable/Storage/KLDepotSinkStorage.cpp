#include "Buildable/Storage/KLDepotSinkStorage.h"

#include "FGCentralStorageSubsystem.h"
#include "FGPowerInfoComponent.h"


AKLDepotSinkStorage::AKLDepotSinkStorage()
	: mResourceSinkSubsystem(nullptr)
	  , mPowerConnection(nullptr)
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
}

void AKLDepotSinkStorage::CacheSubsystem()
{
	mResourceSinkSubsystem = AFGResourceSinkSubsystem::Get(this);
	if (mResourceSinkSubsystem && mCentralStorageSubsystem)
	{
		mTimer.mTime = mCentralStorageSubsystem->GetCentralStorageTimeToUpload();
	}
	else
	{
		FTimerManager& TimerManager = this->GetWorldTimerManager();
		FTimerHandle TimerHandle;
		TimerManager.SetTimer(TimerHandle, this, &AKLDepotSinkStorage::CacheSubsystem, 0.1f, false);
	}
}

// Called every frame
void AKLDepotSinkStorage::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);

	if (HasAuthority() && IsProducing() && GetStorageInventory())
	{
		if (mTimer.Tick(dt))
		{
			mTimer.mTime = mCentralStorageSubsystem->GetCentralStorageTimeToUpload();
			HandleStack();
		}
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

void AKLDepotSinkStorage::HandleStack()
{
	int32 SlotIndex = 0;
	FInventoryStack Stack;
	GetStorageInventory()->GetStackFromIndex(SlotIndex, Stack);

	if (Stack.HasItems())
	{
		int32 StackSize = UFGItemDescriptor::GetStackSize(Stack.Item.GetItemClass());
		int32 Amount = Stack.NumItems;
		int32 MaxStackAmount = FMath::Max(2, StackSize * 0.9f);

		if (Amount > MaxStackAmount)
		{
			if (mResourceSinkSubsystem->AddPoints_ThreadSafe(Stack.Item.GetItemClass()))
			{
				GetStorageInventory()->RemoveFromIndex(SlotIndex, 1);
			}
		}
	}
}
