//
#include "World/KLToxicResourceNode.h"

#include "EngineUtils.h"

AKLToxicResourceNode::AKLToxicResourceNode() {}

FText AKLToxicResourceNode::GetLookAtDecription_Implementation(class AFGCharacterPlayer* byCharacter,
															   const FUseState& state) const
{
	FText ItemName = UFGItemDescriptor::GetItemName(mResourceClass);
	return FText::Format(NSLOCTEXT("KLib", "ToxicResourceNode_LookAtDescription",
								   "Containing {0}. Place a Air Collector nearby to harvest the toxic gas."),
						 ItemName);
}

void AKLToxicResourceNode::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		InitializePillar();
		EnsureGasCloud();
	}
}

void AKLToxicResourceNode::InitializePillar()
{
	if (bGasPillarSpawned && mGasPillarRef.IsValid())
	{
		return;
	}

	TSubclassOf<AFGGasPillar> Class = GetRandomGasPillarClass();
	if (AFGGasPillar* Pillar = GetWorld()->SpawnActorDeferred<AFGGasPillar>(Class, GetActorTransform()))
	{
		Pillar->FinishSpawning(GetActorTransform());
		mGasPillarRef = Pillar;
		bGasPillarSpawned = true;
	}
}
void AKLToxicResourceNode::EnsureGasCloud()
{
	if (!mGasCloudRef.IsValid() && IsValid(mGasCloudClass) && mGasPillarRef.IsValid())
	{
		if (AFGGasPillarCloud* GasCloud =
				GetWorld()->SpawnActorDeferred<AFGGasPillarCloud>(mGasCloudClass, GetActorTransform()))
		{
			GasCloud->FinishSpawning(GetActorTransform());
			mGasCloudRef = GasCloud;
			mGasPillarRef->mNearbyGasCloud = GasCloud;
			if (mGasPillarRef->GetDestructibleActorState() == EDestructibleActorState::DSS_Destroyed)
			{
				mGasPillarRef->NotifyGasCloudOfRemoval();
			}
			else
			{
				GasCloud->mProximityPillarWorldLocations.Empty();
				for (TActorIterator<AFGGasPillar> ActorItr(GetWorld()); ActorItr; ++ActorItr)
				{
					if (AFGGasPillar* gasPillar = *ActorItr)
					{
						if (GetDistanceTo(gasPillar) <= GasCloud->mOverlapRadius)
						{
							GasCloud->mProximityPillarWorldLocations.Add(
								gasPillar->GetActorLocation() + FVector(0, 0, gasPillar->GetEffectHeightOffset()));
							gasPillar->mNearbyGasCloud = GasCloud;
						}
					}
				}
			}
		}
	}
}

TSubclassOf<AFGGasPillar> AKLToxicResourceNode::GetRandomGasPillarClass() const
{
	TArray<TSubclassOf<AFGGasPillar>> ValidClasses =
		mGasPillarClasses.FilterByPredicate([](TSubclassOf<AFGGasPillar> Class) { return IsValid(Class); });
	fgcheckf(!ValidClasses.IsEmpty(),
			 TEXT("AKLToxicResourceNode::GetRandomGasPillarClass: No GasPillarClasses found for %s"), *GetName());
	const int32 Index = FMath::RandRange(0, ValidClasses.Num() - 1);
	return ValidClasses[Index];
}

bool AKLToxicResourceNode::IsOccupied() const
{
	if (!mGasPillarRef.IsValid())
	{
		return false;
	}

	if (AFGDestructibleActor* DestructibleActor = Cast<AFGDestructibleActor>(mGasPillarRef.Get()))
	{
		return DestructibleActor->GetDestructibleActorState() == EDestructibleActorState::DSS_Intact;
	}

	return true;
}

bool AKLToxicResourceNode::CanBecomeOccupied() const { return false; }

bool AKLToxicResourceNode::CanPlaceResourceExtractor() const { return false; }
