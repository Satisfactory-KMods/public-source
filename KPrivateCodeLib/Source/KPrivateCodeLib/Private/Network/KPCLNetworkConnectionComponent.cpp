// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Network/KPCLNetworkConnectionComponent.h"

#include "FGCircuitSubsystem.h"
#include "Network/KPCLNetwork.h"

UKPCLNetwork* UKPCLNetworkConnectionComponent::GetNetwork() const
{
	if (GetCircuitID() >= 0)
	{
		AFGCircuitSubsystem* CircuitSubsystem = AFGCircuitSubsystem::Get(GetWorld());
		if (IsValid(CircuitSubsystem))
		{
			return Cast<UKPCLNetwork>(CircuitSubsystem->FindCircuit(GetCircuitID()));
		}
	}
	return nullptr;
}

AKPCLNetworkCore* UKPCLNetworkConnectionComponent::GetCore() const
{
	UKPCLNetwork* Network = GetNetwork();
	if (IsValid(Network))
	{
		return Network->GetCore();
	}
	return nullptr;
}

bool UKPCLNetworkConnectionComponent::IsNetworkOk() const
{
	UKPCLNetwork* Network = GetNetwork();

	if (!IsValid(Network))
	{
		return false;
	}
	if (Network->NetworkHasCoreToMuchCores() || !Network->NetworkHasCore())
	{
		return false;
	}
	if (!Network->CoreStateIsOk())
	{
		return false;
	}

	return true;
}

UKPCLNetworkConnectionComponent::UKPCLNetworkConnectionComponent() { mCircuitType = UKPCLNetwork::StaticClass(); }
