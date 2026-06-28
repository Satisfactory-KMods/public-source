// Fill out your copyright notice in the Description page of Project Settings.

#include "Subsystem/KLUnlockSubsystem.h"

#include <Net/UnrealNetwork.h>

#include "FGSchematicManager.h"
#include "FGUnlockSubsystem.h"
#include "Logging.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

// Sets default values
AKLUnlockSubsystem::AKLUnlockSubsystem() : mAssetSubsystem(nullptr)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

void AKLUnlockSubsystem::BeginPlay()
{
	Super::BeginPlay();

	mAssetSubsystem = UKAPIDataAssetSubsystem::GetChecked(GetWorld());

	CleanerDescUnlocked();
}

void AKLUnlockSubsystem::Init()
{
	Super::Init();

	if (HasAuthority())
	{
		mUnlockedCleanerDescriptor.Remove(nullptr);
	}

	CleanerDescUnlocked();
}

void AKLUnlockSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKLUnlockSubsystem, mUnlockedCleanerDescriptor);
}

void AKLUnlockSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	Super::PostLoadGame_Implementation(saveVersion, gameVersion);

	if (HasAuthority())
	{
		mUnlockedCleanerDescriptor.Remove(nullptr);
		CleanerDescUnlocked();
	}
}

void AKLUnlockSubsystem::UnlockCleanerDesc(TSubclassOf<UFGItemDescriptor> CleanerDesc)
{
	if (!HasAuthority())
	{
		return;
	}
	if (mUnlockedCleanerDescriptor.AddUnique(CleanerDesc) >= 0)
	{
		onCleanerDescUnlocked.Broadcast(CleanerDesc);
		CleanerDescUnlocked();
	}
}

void AKLUnlockSubsystem::OnRep_UnlockedCleanerDescriptor()
{
	// The replicated unlock list just changed on this client; rebuild the transient cache so cleaner UIs
	// reflect the new unlock state without waiting for another trigger (join, reopen, etc.).
	CleanerDescUnlocked();
}

UKAPIModularMinerDescription* AKLUnlockSubsystem::GetInformationAboutOre(const TSubclassOf<UFGResourceDescriptor> Desc,
																		 bool bSkipUnlockCheck) const
{
	UKAPIModularMinerDescription* OutAsset = nullptr;
	if (!bSkipUnlockCheck && !HasInformationAboutOre(Desc))
	{
		GetAssetSubsystem()->Miner_GetForKey(Desc, OutAsset);
		return OutAsset;
	}

	if (!bSkipUnlockCheck)
	{
		return nullptr;
	}
	GetAssetSubsystem()->Miner_GetForKey(Desc, OutAsset);
	return OutAsset;
}

bool AKLUnlockSubsystem::HasInformationAboutOre(const TSubclassOf<UFGResourceDescriptor> Desc) const
{
	if (!Desc)
	{
		return false;
	}
	AFGUnlockSubsystem* Subsystem = AFGUnlockSubsystem::Get(GetWorld());
	if (!IsValid(Subsystem))
	{
		return false;
	}
	return Subsystem->GetScannableResources().Contains(Desc);
}

bool AKLUnlockSubsystem::IsCleanerDescUnlock(TSubclassOf<UFGItemDescriptor> CleanerDesc)
{
	if (!IsValid(CleanerDesc))
	{
		return false;
	}

	UKAPICleanerItemDescription* Desc = nullptr;
	GetAssetSubsystem()->Cleaner_GetForKey(CleanerDesc, Desc);

	return IsValid(Desc) && (Desc->bSkipUnlockCheck || mUnlockedCleanerDescriptor.Contains(CleanerDesc));
}

UKAPICleanerItemDescription* AKLUnlockSubsystem::GetUnlockedCleanerDesc(TSubclassOf<UFGItemDescriptor> CleanerDesc)
{
	if (!IsValid(CleanerDesc))
	{
		return nullptr;
	}

	if (!IsCleanerDescUnlock(CleanerDesc))
	{
		return nullptr;
	}
	UKAPICleanerItemDescription* Desc = nullptr;
	GetAssetSubsystem()->Cleaner_GetForKey(CleanerDesc, Desc);
	return Desc;
}

TArray<UKAPIModularMinerDescription*> AKLUnlockSubsystem::GetUnlockedExtractDescriptor() const
{
	return GetAssetSubsystem()->Miner_GetAllAssets();
}

TArray<UKAPICleanerItemDescription*> AKLUnlockSubsystem::GetAllUnlockedCleanerDesc()
{
	return mCachedUnlockedCleanerDesc;
}

void AKLUnlockSubsystem::CleanerDescUnlocked()
{
	TArray<UKAPICleanerItemDescription*> All = GetAssetSubsystem()->Cleaner_GetAllAssets();
	mCachedUnlockedCleanerDesc = All.FilterByPredicate(
		[this](UKAPICleanerItemDescription* Desc)
		{
			if (!IsValid(Desc) || !IsValid(Desc->mInFluid.ItemClass))
			{
				return false;
			}

			return IsCleanerDescUnlock(Desc->mInFluid.ItemClass);
		});
}

UKAPIDataAssetSubsystem* AKLUnlockSubsystem::GetAssetSubsystem() const
{
	if (!IsValid(mAssetSubsystem))
	{
		return UKAPIDataAssetSubsystem::GetChecked(GetWorld());
	}
	return mAssetSubsystem;
}

AKLUnlockSubsystem* AKLUnlockSubsystem::Get(UObject* worldContext)
{
	return UKBFL_Util::GetSubsystem<AKLUnlockSubsystem>(worldContext);
}
