//

#include "Network/Buildings/KPCLNetworkTower.h"

#include "KPrivateCodeLibModule.h"
#include "Network/KPCLNetworkAsyncHelpers.h"

AKPCLNetworkTower::AKPCLNetworkTower() { PrimaryActorTick.bCanEverTick = false; }

FText AKPCLNetworkTower::GetActorRepresentationText()
{
	// Fixed: was concatenating a hardcoded non-localized " (Network Tower)" string. Now
	// delegates to the base text and appends the Blueprint-configurable mTowerRepresentationText.
	FText SuperText = Super::GetActorRepresentationText();
	if (!mTowerRepresentationText.IsEmpty())
	{
		return FText::FromString(SuperText.ToString() + TEXT(" ") + mTowerRepresentationText.ToString());
	}
	return SuperText;
}

void AKPCLNetworkTower::BeginPlay()
{
	Super::BeginPlay();

	AKPCLFaxitSubsystem* FaxitSubsystem = AKPCLFaxitSubsystem::Get(GetWorld());
	if (IsValid(FaxitSubsystem))
	{
		FaxitSubsystem->RegisterNetworkTower(this);
	}

	TryToConnectToNearstCore();
}

void AKPCLNetworkTower::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	AKPCLFaxitSubsystem* FaxitSubsystem = AKPCLFaxitSubsystem::Get(GetWorld());
	if (IsValid(FaxitSubsystem))
	{
		FaxitSubsystem->UnRegisterNetworkTower(this);
	}
}

void AKPCLNetworkTower::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);

	// Self-dismantle if the linked radar tower has been removed.
	// Fixed: the old implementation queued a new AsyncTask every tick until the
	// dismantle executed, flooding the game thread with unbounded redundant calls.
	// The `bPendingDismantle` latch ensures we dispatch exactly once.
	if (HasAuthority() && !mConnectedRadarTower.IsValid() && !bPendingDismantle)
	{
		bPendingDismantle = true;
		RunOnGameThreadIfValid(this,
							   [](AKPCLNetworkTower* Self)
							   {
								   // The latch remains set even if Dismantle fails so we don't
								   // re-queue. The actor will be cleaned up by the engine anyway.
								   Execute_Dismantle(Self);
							   });
	}
}
