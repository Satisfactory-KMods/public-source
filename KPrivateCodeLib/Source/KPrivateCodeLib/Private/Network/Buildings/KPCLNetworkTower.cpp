// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Network/Buildings/KPCLNetworkTower.h"

#include "KPrivateCodeLibModule.h"
#include "Network/KPCLNetworkAsyncHelpers.h"

AKPCLNetworkTower::AKPCLNetworkTower() { PrimaryActorTick.bCanEverTick = false; }

FText AKPCLNetworkTower::GetActorRepresentationText()
{

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

	if (HasAuthority() && !mConnectedRadarTower.IsValid() && !bPendingDismantle)
	{
		bPendingDismantle = true;
		RunOnGameThreadIfValid(this,
							   [](AKPCLNetworkTower* Self)
							   {

								   Execute_Dismantle(Self);
							   });
	}
}
