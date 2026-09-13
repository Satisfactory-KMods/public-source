// Copyright Kyri123 / KMods 2026. All Rights Reserved.



#include "Network/KPCLNetworkCable.h"

#include "Network/KPCLNetwork.h"

AKPCLNetworkCable::AKPCLNetworkCable()
{
	PrimaryActorTick.bCanEverTick = false;
	mCircuitType = UKPCLNetwork::StaticClass();
}
