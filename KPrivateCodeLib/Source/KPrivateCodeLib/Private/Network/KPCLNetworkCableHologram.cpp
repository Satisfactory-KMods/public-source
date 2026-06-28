// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Network/KPCLNetworkCableHologram.h"
#include "Network/KPCLNetworkCable.h"

AKPCLNetworkCableHologram::AKPCLNetworkCableHologram() : mConnectionMesh(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;
}

void AKPCLNetworkCableHologram::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AKPCLNetworkCableHologram::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	// Fixed: previously called Super twice on the common path (once unconditionally at the
	// top, then again via the return-Super branch), causing double-applied snap logic.
	// Now Super is called exactly once regardless of connection state.
	if (IsConnectedToChild())
	{
		Super::SetHologramLocationAndRotation(hitResult);
		return;
	}
	Super::SetHologramLocationAndRotation(hitResult);
}

void AKPCLNetworkCableHologram::BeginPlay()
{
	Super::BeginPlay();

	// Fixed: raw [0] index access would crash if no component carries the "ConInd" tag
	// (e.g. a Blueprint subclass omits it). Guard with a bounds check.
	TArray<UActorComponent*> TaggedComponents =
		GetComponentsByTag(UStaticMeshComponent::StaticClass(), FName("ConInd"));
	if (TaggedComponents.Num() > 0)
	{
		mConnectionMesh = Cast<UStaticMeshComponent>(TaggedComponents[0]);
	}
}

bool AKPCLNetworkCableHologram::TryUpgrade(const FHitResult& hitResult)
{
	// Fixed: local variable named "Super" shadowed the keyword (legal but confusing). Renamed to bSuperResult.
	const bool bSuperResult = Super::TryUpgrade(hitResult);

	if (hitResult.IsValidBlockingHit() && bSuperResult)
	{
		AKPCLNetworkCable* OtherCable = Cast<AKPCLNetworkCable>(hitResult.GetActor());
		if (IsValid(OtherCable))
		{
			SetConnection(0, OtherCable->GetConnection(0));
			SetConnection(1, OtherCable->GetConnection(1));
		}
	}

	return bSuperResult;
}

int32 AKPCLNetworkCableHologram::GetConnectionToSet() const
{
	return mCurrentConnection;
	// return IsValid(GetConnection(0)) ? 0 : 1;
}

bool AKPCLNetworkCableHologram::IsConnectedToChild() const
{
	if (GetConnectionToSet() == 0)
	{
		return false;
	}
	return true;
}
