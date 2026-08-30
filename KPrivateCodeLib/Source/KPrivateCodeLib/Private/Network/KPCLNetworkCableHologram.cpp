

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

	TArray<UActorComponent*> TaggedComponents =
		GetComponentsByTag(UStaticMeshComponent::StaticClass(), FName("ConInd"));
	if (TaggedComponents.Num() > 0)
	{
		mConnectionMesh = Cast<UStaticMeshComponent>(TaggedComponents[0]);
	}
}

bool AKPCLNetworkCableHologram::TryUpgrade(const FHitResult& hitResult)
{

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

}

bool AKPCLNetworkCableHologram::IsConnectedToChild() const
{
	if (GetConnectionToSet() == 0)
	{
		return false;
	}
	return true;
}
