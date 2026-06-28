//

#include "Network/KPCLNetworkTowerHologram.h"

#include "Network/Buildings/KPCLNetworkTower.h"

// Sets default values
AKPCLNetworkTowerHologram::AKPCLNetworkTowerHologram() : mLinkedRadarTower(nullptr) {}

void AKPCLNetworkTowerHologram::CheckValidPlacement()
{
	Super::CheckValidPlacement();

	if (!IsValid(mLinkedRadarTower))
	{
		AddConstructDisqualifier(mNoRadarDisqualifier);
	}
}

void AKPCLNetworkTowerHologram::ConfigureActor(class AFGBuildable* inBuildable) const
{
	Super::ConfigureActor(inBuildable);

	if (AKPCLNetworkTower* NetworkTower = Cast<AKPCLNetworkTower>(inBuildable))
	{
		NetworkTower->mConnectedRadarTower = mLinkedRadarTower;
	}
}

void AKPCLNetworkTowerHologram::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	Super::SetHologramLocationAndRotation(hitResult);

	if (AFGBuildableRadarTower* Tower = Cast<AFGBuildableRadarTower>(hitResult.GetActor()))
	{
		AKPCLFaxitSubsystem* FaxitSubsystem = AKPCLFaxitSubsystem::Get(GetWorld());
		if (!IsValid(FaxitSubsystem))
		{
			mLinkedRadarTower = nullptr;
		}
		else if (Tower != mLinkedRadarTower && !FaxitSubsystem->IsTowerRegistered(Tower))
		{
			OnSnap();
			mLinkedRadarTower = Tower;
		}
		else if (FaxitSubsystem->IsTowerRegistered(Tower))
		{
			mLinkedRadarTower = nullptr;
		}

		SetActorLocationAndRotation(Tower->GetActorLocation(), Tower->GetActorRotation());
	}
	else
	{
		mLinkedRadarTower = nullptr;
	}
}

bool AKPCLNetworkTowerHologram::IsValidHitActor(AActor* hitActor) const
{
	return IsValid(Cast<AFGBuildableRadarTower>(hitActor));
}

bool AKPCLNetworkTowerHologram::IsValidHitResult(const FHitResult& hitResult) const
{
	return IsValid(Cast<AFGBuildableRadarTower>(hitResult.GetActor()));
}
