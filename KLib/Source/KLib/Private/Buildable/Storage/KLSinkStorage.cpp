// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildable/Storage/KLSinkStorage.h"

#include "FGPowerInfoComponent.h"
#include "FGResourceSinkSubsystem.h"


AKLSinkStorage::AKLSinkStorage()
	: mResourceSinkSubsystem(nullptr)
	  , mPowerConnection(nullptr)
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
	}
}

void AKLSinkStorage::CacheSubsystem()
{
	mResourceSinkSubsystem = AFGResourceSinkSubsystem::Get(this);
	if (IsValid(mResourceSinkSubsystem))
	{
		const auto Component = Cast<UFGPowerConnectionComponent>(
			GetComponentByClass(UFGPowerConnectionComponent::StaticClass()));
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

// Called every frame
void AKLSinkStorage::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);

	if (HasAuthority() && IsProducing() && GetStorageInventory())
	{
		HandleLastSlot();
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

void AKLSinkStorage::HandleLastSlot()
{
	const int LastIdx = GetStorageInventory()->GetSizeLinear() - 1;
	if (GetStorageInventory()->IsValidIndex(LastIdx) && !GetStorageInventory()->IsIndexEmpty(LastIdx))
	{
		FInventoryStack Stack;
		GetStorageInventory()->GetStackFromIndex(LastIdx, Stack);
		if (Stack.HasItems())
		{
			if (mResourceSinkSubsystem->AddPoints_ThreadSafe(Stack.Item.GetItemClass()))
			{
				GetStorageInventory()->RemoveFromIndex(LastIdx, 1);
			}
		}
	}
}
