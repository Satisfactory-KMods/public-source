// Fill out your copyright notice in the Description page of Project Settings.

#include "Subsystem/KLSlugSubsystem.h"

#include <Net/UnrealNetwork.h>

#include "Subsystems/KAPIDataAssetSubsystem.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

// Sets default values
AKLSlugSubsystem::AKLSlugSubsystem()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

void AKLSlugSubsystem::BeginPlay()
{
	Super::BeginPlay();

	KPCL_ASG_INIT(mSkipUnlock);
}

void AKLSlugSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	KPCL_ASG_DEINIT(mSkipUnlock);
}

void AKLSlugSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKLSlugSubsystem, mBreededEggs);
	DOREPLIFETIME(AKLSlugSubsystem, mBreededSlugs);
}

bool AKLSlugSubsystem::ShouldSave_Implementation() const { return true; }

AKLSlugSubsystem* AKLSlugSubsystem::Get(UObject* worldContext)
{
	return UKBFL_Util::GetSubsystem<AKLSlugSubsystem>(worldContext);
}

void AKLSlugSubsystem::AddBreededEgg(TSubclassOf<UFGItemDescriptor> Egg)
{
	if (HasAuthority() && IsValid(Egg))
	{
		mBreededEggs.AddUnique(Egg);
	}
}

TArray<TSubclassOf<UFGItemDescriptor>> AKLSlugSubsystem::GetAllBreededEgg() const
{
	if (mSkipUnlock.GetValue())
	{
		TArray<TSubclassOf<UFGItemDescriptor>> Slugs;
		UKAPIDataAssetSubsystem* DataSubsystem = UKAPIDataAssetSubsystem::Get(GetWorld());
		if (IsValid(DataSubsystem))
		{
			for (UKAPISugHatchingData* HatchingData : DataSubsystem->Slug_GetAll())
			{
				Slugs.AddUnique(HatchingData->mEgg);
			}
		}
	}
	return mBreededEggs;
}

bool AKLSlugSubsystem::WasEggBreeded(TSubclassOf<UFGItemDescriptor> Egg) const
{
	if (mSkipUnlock.GetValue())
	{
		return true;
	}
	return mBreededEggs.Contains(Egg);
}

bool AKLSlugSubsystem::WasSlugBreeded(TSubclassOf<UFGItemDescriptor> Slug) const
{
	if (mSkipUnlock.GetValue())
	{
		return true;
	}
	return mBreededSlugs.Contains(Slug);
}

TArray<TSubclassOf<UFGItemDescriptor>> AKLSlugSubsystem::GetAllBreededSlugs() const
{
	if (mSkipUnlock.GetValue())
	{
		TArray<TSubclassOf<UFGItemDescriptor>> Slugs;
		UKAPIDataAssetSubsystem* DataSubsystem = UKAPIDataAssetSubsystem::Get(GetWorld());
		if (IsValid(DataSubsystem))
		{
			for (UKAPISugHatchingData* HatchingData : DataSubsystem->Slug_GetAll())
			{
				Slugs.AddUnique(HatchingData->mSlug);
			}
		}
	}
	return mBreededSlugs;
}

void AKLSlugSubsystem::AddBreededSlugs(TSubclassOf<UFGItemDescriptor> Slug)
{
	if (HasAuthority() && IsValid(Slug))
	{
		mBreededSlugs.AddUnique(Slug);
	}
}
