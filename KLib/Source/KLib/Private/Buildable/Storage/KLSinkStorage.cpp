// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Buildable/Storage/KLSinkStorage.h"

#include "FGInventoryComponent.h"
#include "FGPowerInfoComponent.h"
#include "FGResourceSinkSubsystem.h"

AKLSinkStorage::AKLSinkStorage() : mPowerConnection(nullptr), mResourceSinkSubsystem(nullptr)
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AKLSinkStorage::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && IsValid(GetStorageInventory()))
	{
		CacheSubsystem();
		GetStorageInventory()->Resize(mInventorySizeX * mInventorySizeY);
		GetStorageInventory()->OnItemAddedDelegate_Native.BindUObject(this, &AKLSinkStorage::OnInventoryItemAdded);
		mOnHasPowerChanged.AddDynamic(this, &AKLSinkStorage::OnPowerStateChanged);
		TrySinkOverflow();
	}
}

void AKLSinkStorage::CacheSubsystem()
{
	mResourceSinkSubsystem = AFGResourceSinkSubsystem::Get(this);
	if (IsValid(mResourceSinkSubsystem))
	{
		const auto Component =
			Cast<UFGPowerConnectionComponent>(GetComponentByClass(UFGPowerConnectionComponent::StaticClass()));
		const auto PowerInfo = Cast<UFGPowerInfoComponent>(GetComponentByClass(UFGPowerInfoComponent::StaticClass()));
		if (Component && PowerInfo)
		{
			mPowerConnection = Component;
			mPowerInfo = PowerInfo;
			mPowerConnection->SetPowerInfo(PowerInfo);
			mPowerInfo->SetTargetConsumption(mPowerConsumption);
			mPowerInfo->SetMaximumTargetConsumption(mPowerConsumption);
		}
	}
	else
	{
		FTimerManager& TimerManager = this->GetWorldTimerManager();
		FTimerHandle TimerHandle;
		TimerManager.SetTimer(TimerHandle, this, &AKLSinkStorage::CacheSubsystem, 0.1f, false);
	}
}

bool AKLSinkStorage::CanProduce_Implementation() const
{
	if (IsPlayingBuildEffect() || !HasPower())
	{
		return false;
	}

	return Super::CanProduce_Implementation();
}

void AKLSinkStorage::OnInventoryItemAdded(TSubclassOf<UFGItemDescriptor> ItemClass, int32 NumAdded,
										  UFGInventoryComponent* SourceInventory)
{
	TrySinkOverflow();
}

void AKLSinkStorage::OnPowerStateChanged(bool bNewHasPower)
{
	if (bNewHasPower)
	{
		TrySinkOverflow();
	}
}

void AKLSinkStorage::TrySinkOverflow()
{
	if (!HasAuthority() || !IsValid(mResourceSinkSubsystem) || !HasPower() || IsPlayingBuildEffect())
	{
		return;
	}

	UFGInventoryComponent* Inv = GetStorageInventory();
	if (!IsValid(Inv))
	{
		return;
	}

	const int32 LastIdx = Inv->GetSizeLinear() - 1;
	if (LastIdx < 0 || !Inv->IsValidIndex(LastIdx) || Inv->IsIndexEmpty(LastIdx))
	{
		return;
	}

	FInventoryStack Stack;
	if (!Inv->GetStackFromIndex(LastIdx, Stack) || !Stack.HasItems())
	{
		return;
	}

	if (!mResourceSinkSubsystem->AddPoints_ThreadSafe(Stack.Item.GetItemClass()))
	{
		return;
	}

	int32 SunkCount = 1;
	const int32 TotalItems = Stack.NumItems;
	const int32 MaxToSink =
		(mMaxItemsToSinkPerCycle <= 0) ? TotalItems : FMath::Min(mMaxItemsToSinkPerCycle, TotalItems);

	while (SunkCount < MaxToSink)
	{
		if (!mResourceSinkSubsystem->AddPoints_ThreadSafe(Stack.Item.GetItemClass()))
		{
			break;
		}
		++SunkCount;
	}

	Inv->RemoveFromIndex(LastIdx, SunkCount);
}
