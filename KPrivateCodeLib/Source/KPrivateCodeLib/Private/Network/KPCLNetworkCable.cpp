

#include "Network/KPCLNetworkCable.h"

#include "Network/KPCLNetwork.h"

AKPCLNetworkCable::AKPCLNetworkCable()
{
	PrimaryActorTick.bCanEverTick = false;
	mCircuitType = UKPCLNetwork::StaticClass();
}
