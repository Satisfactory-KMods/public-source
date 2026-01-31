// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/KPCLNetworkInfoComponent.h"

#include "Net/UnrealNetwork.h"

#include "Network/KPCLNetwork.h"

void UKPCLNetworkInfoComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKPCLNetworkInfoComponent, mNetworkCoresInNetwork);
}

bool UKPCLNetworkInfoComponent::HasCore() const
{
	if (!IsConnected())
	{
		return false;
	}
	return mNetworkCoresInNetwork.Num() >= 1;
}

int32 UKPCLNetworkInfoComponent::CoreCount() const
{
	return mNetworkCoresInNetwork.Num();
}

void UKPCLNetworkInfoComponent::SetCors(TArray<AKPCLNetworkCore*> Cores)
{
	if (mNetworkCoresInNetwork.Num() != Cores.Num())
	{
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(FSimpleDelegateGraphTask::FDelegate::CreateLambda(
			                                                     [&, Cores]()
			                                                     {
				                                                     if (CoreStateChanged.IsBound())
				                                                     {
					                                                     CoreStateChanged.Broadcast(Cores.Num() > 0);
				                                                     }
			                                                     }), TStatId(), nullptr, ENamedThreads::GameThread);
	}
	mNetworkCoresInNetwork = Cores;
}

UKPCLNetwork* UKPCLNetworkInfoComponent::GetNetwork() const
{
	if (UKPCLNetwork* Network = Cast<UKPCLNetwork>(GetPowerCircuit()))
	{
		return Network;
	}
	return nullptr;
}

class AKPCLNetworkCore* UKPCLNetworkInfoComponent::GetFirstCores() const
{
	return mNetworkCoresInNetwork.Num() > 0 ? mNetworkCoresInNetwork[0] : nullptr;
}

class AKPCLNetworkCore* UKPCLNetworkInfoComponent::GetFirstCores(bool& Valid) const
{
	AKPCLNetworkCore* Core = mNetworkCoresInNetwork.Num() > 0 ? mNetworkCoresInNetwork[0] : nullptr;
	Valid = IsValid(Core);
	return Core;
}

TArray<class AKPCLNetworkCore*> UKPCLNetworkInfoComponent::GetCores() const
{
	return mNetworkCoresInNetwork;
}
