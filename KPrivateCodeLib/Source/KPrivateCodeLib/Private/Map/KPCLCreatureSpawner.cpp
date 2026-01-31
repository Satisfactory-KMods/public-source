// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Map/KPCLCreatureSpawner.h"

#include "FGHealthComponent.h"

AKPCLCreatureSpawner::AKPCLCreatureSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AKPCLCreatureSpawner::BeginPlay()
{
	Super::BeginPlay();

	CheckNodesInRange();
}

void AKPCLCreatureSpawner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckNodesInRange();
}

void AKPCLCreatureSpawner::CheckNodesInRange()
{
	if (!mShouldCheckNodesInRange || !HasAuthority())
	{
		return;
	}

	bool NewActive = true;
	for (AFGResourceNodeBase* ResourceNode : mBoundedNodes)
	{
		if (IsValid(ResourceNode))
		{
			if (ResourceNode->IsOccupied())
			{
				NewActive = false;
				break;
			}
		}
	}

	mIsActive = NewActive;
	mRespawnTimeIndays = 1;
	//mRespawnTimeIndays = mIsActive ? 1 : -1;
}

void AKPCLCreatureSpawner::ReApplyChangesOnCreatures()
{
	for (FSpawnData SpawnData : mSpawnData)
	{
		if (SpawnData.Creature && !SpawnData.WasKilled)
		{
			ReApplyChangesOnCreature(SpawnData.Creature);
		}
	}
}

void AKPCLCreatureSpawner::ReApplyChangesOnCreature_Implementation(AFGCreature* Creature)
{
	if (!ensureAlwaysMsgf(Creature, TEXT("ReApplyChangesOnCreature_Implementation received a invalid Creature!")))
	{
		return;
	}

	if (HasAuthority())
	{
		if (Creature->GetHealthComponent())
		{
			if (FMath::TruncToInt(Creature->GetHealthComponent()->mMaxHealth) != FMath::TruncToInt(mHealthOverwrite))
			{
				Creature->GetHealthComponent()->mMaxHealth = mHealthOverwrite;
				Creature->GetHealthComponent()->mCurrentHealth = mHealthOverwrite;
			}
		}
	}

	for (const TTuple<int32, UMaterialInterface*> MaterialOverwrite : mMaterialOverwrite)
	{
		Creature->FindComponentByClass<USkeletalMeshComponent>()->SetMaterial(
			MaterialOverwrite.Key, MaterialOverwrite.Value);
	}
}
