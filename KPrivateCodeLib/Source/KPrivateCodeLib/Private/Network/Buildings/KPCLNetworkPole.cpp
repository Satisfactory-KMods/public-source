// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/Buildings/KPCLNetworkPole.h"
#include "Network/KPCLNetwork.h"
#include "Network/Buildings/KPCLNetworkCore.h"

bool AKPCLNetworkPole::HasCore_Implementation() const
{
	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if (Network)
	{
		return IsValid(Cast<AKPCLNetworkCore>(Network->GetCore()));
	}
	return false;
}

bool AKPCLNetworkPole::CanUseFactoryClipboard_Implementation()
{
	return false;
}

AKPCLNetworkCore* AKPCLNetworkPole::GetCore_Implementation()
{
	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if (Network)
	{
		return Cast<AKPCLNetworkCore>(Network->GetCore());
	}
	return nullptr;
}

UKPCLNetwork* AKPCLNetworkPole::GetNetwork_Implementation() const
{
	if (UKPCLNetworkConnectionComponent* con = GetNetworkConnectionComponent())
	{
		return Cast<UKPCLNetwork>(con->GetPowerCircuit());
	}
	return nullptr;
}

FNetworkUIData AKPCLNetworkPole::GetUIDData_Implementation() const
{
	FNetworkUIData Data = mNetworkUIData.Copy();
	Data.mIsWireless = false;
	return Data;
}

TArray<EKPCLNetworkError> AKPCLNetworkPole::GetNetworkErrors_Implementation() const
{
	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if (HasAuthority() && IsValid(Network) && Network->NetworkHasCore())
	{
		return Network->GetCore()->GetNetworkErrorCodes();
	}

	return TArray<EKPCLNetworkError>();
}

void AKPCLNetworkPole::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		ConnectLocalPowerConnections();
	}
}

void AKPCLNetworkPole::ConnectLocalPowerConnections() const
{
	TArray<UKPCLNetworkConnectionComponent*> Components;
	GetComponents<UKPCLNetworkConnectionComponent>(Components);
	for (UKPCLNetworkConnectionComponent* ComponentA : Components)
	{
		if (!IsValid(ComponentA))
		{
			continue;
		}
		for (UKPCLNetworkConnectionComponent* ComponentB : Components)
		{
			if (!IsValid(ComponentB) || ComponentA == ComponentB || ComponentA->HasHiddenConnection(ComponentB))
			{
				continue;
			}

			ComponentA->AddHiddenConnection(ComponentB);
		}
	}
}

void AKPCLNetworkPole::RegisterInteractingPlayer_Implementation(AFGCharacterPlayer* player)
{
	Super::RegisterInteractingPlayer_Implementation(player);

	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if (HasAuthority() && IsValid(Network))
	{
		Network->RegisterInteractingPlayer(player);

		if (Network->NetworkHasCore())
		{
			Execute_RegisterInteractingPlayer(Network->GetCore(), player);
		}
	}
}

void AKPCLNetworkPole::UnregisterInteractingPlayer_Implementation(AFGCharacterPlayer* player)
{
	Super::UnregisterInteractingPlayer_Implementation(player);

	UKPCLNetwork* Network = Execute_GetNetwork(this);
	if (HasAuthority() && IsValid(Network))
	{
		Network->RegisterInteractingPlayer(player);

		if (Network->NetworkHasCore())
		{
			Execute_UnregisterInteractingPlayer(Network->GetCore(), player);
		}
	}
}

UKPCLNetworkConnectionComponent* AKPCLNetworkPole::GetNetworkConnectionComponent() const
{
	return FindComponentByClass<UKPCLNetworkConnectionComponent>();
}
