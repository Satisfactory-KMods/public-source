// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Network/KPCLNetworkDeliveryHologram.h"

#include "Network/Buildings/KPCLNetworkCore.h"
#include "Network/Buildings/KPCLNetworkDelivery.h"
#include "Subsystem/KPCLFaxitSubsystem.h"

void AKPCLNetworkDeliveryHologram::BeginPlay()
{
	Super::BeginPlay();
	mFaxitSubsystem = AKPCLFaxitSubsystem::Get(this);
}

void AKPCLNetworkDeliveryHologram::CheckValidPlacement()
{
	Super::CheckValidPlacement();
	RefreshTargetNexus();
	if (IsValid(mTargetNexus))
	{
		return;
	}

	const bool bHasNexus = IsValid(mFaxitSubsystem) &&
		mFaxitSubsystem->GetNetworks().ContainsByPredicate([](const FKPCLFaxitNetwork& Network)
														   { return IsValid(Network.mCore); });
	AddConstructDisqualifier(bHasNexus ? UKPCLCDNexusBuildingLimitReached::StaticClass()
									   : UKPCLCDNoNexusAvailable::StaticClass());
}

void AKPCLNetworkDeliveryHologram::ConfigureActor(AFGBuildable* InBuildable) const
{
	Super::ConfigureActor(InBuildable);
	if (AKPCLNetworkDelivery* Delivery = Cast<AKPCLNetworkDelivery>(InBuildable))
	{
		if (IsValid(mTargetNexus))
		{
			FKPCLFaxitNetwork TargetNetwork;
			if (IsValid(mFaxitSubsystem) && mFaxitSubsystem->GetNetworkByCore(mTargetNexus, TargetNetwork))
			{
				Delivery->SetNetworkCore(mTargetNexus, &TargetNetwork);
			}
			else
			{
				Delivery->SetNetworkCore(mTargetNexus, nullptr);
			}
		}
	}
}

void AKPCLNetworkDeliveryHologram::RefreshTargetNexus()
{
	mTargetNexus = nullptr;
	if (!IsValid(mFaxitSubsystem))
	{
		mFaxitSubsystem = AKPCLFaxitSubsystem::Get(this);
	}
	if (!IsValid(mFaxitSubsystem))
	{
		return;
	}

	FKPCLFaxitNetwork TargetNetwork;
	mFaxitSubsystem->GetNearstNetwork(this, TargetNetwork);
	mTargetNexus = TargetNetwork.mCore;
}
