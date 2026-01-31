// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/KPCLNetworkCableHologram.h"
#include "Network/KPCLNetworkCable.h"

AKPCLNetworkCableHologram::AKPCLNetworkCableHologram(): mConnectionMesh(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;
}

void AKPCLNetworkCableHologram::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AKPCLNetworkCableHologram::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	Super::SetHologramLocationAndRotation(hitResult);

	if (IsConnectedToChild())
	{
		return;
	}
	return Super::SetHologramLocationAndRotation(hitResult);
}

void AKPCLNetworkCableHologram::BeginPlay()
{
	Super::BeginPlay();

	mConnectionMesh = Cast<UStaticMeshComponent>(
		GetComponentsByTag(UStaticMeshComponent::StaticClass(), FName("ConInd"))[0]);
}

bool AKPCLNetworkCableHologram::TryUpgrade(const FHitResult& hitResult)
{
	bool Super = Super::TryUpgrade(hitResult);

	if (hitResult.IsValidBlockingHit() && Super)
	{
		AKPCLNetworkCable* OtherCable = Cast<AKPCLNetworkCable>(hitResult.GetActor());
		if (IsValid(OtherCable))
		{
			SetConnection(0, OtherCable->GetConnection(0));
			SetConnection(1, OtherCable->GetConnection(1));
		}
	}

	return Super;
}

int32 AKPCLNetworkCableHologram::GetConnectionToSet() const
{
	return mCurrentConnection;
	//return IsValid(GetConnection(0)) ? 0 : 1;
}

bool AKPCLNetworkCableHologram::IsConnectedToChild() const
{
	if (GetConnectionToSet() == 0)
	{
		return false;
	}
	return true;
}
