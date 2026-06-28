// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Network/KPCLNetworkInfoComponent.h"

#include "Net/UnrealNetwork.h"

#include "Network/Buildings/KPCLNetworkCore.h"
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

int32 UKPCLNetworkInfoComponent::CoreCount() const { return mNetworkCoresInNetwork.Num(); }

void UKPCLNetworkInfoComponent::SetCors(TArray<AKPCLNetworkCore*> Cores)
{
	if (mNetworkCoresInNetwork.Num() != Cores.Num())
	{
		// Fixed: previous capture [&, Cores] captured `this` by reference into a deferred
		// game-thread task — use-after-free if the component is destroyed before the task runs.
		// Replaced with an explicit TWeakObjectPtr capture so the broadcast is safely skipped
		// when the component is gone.
		TWeakObjectPtr<UKPCLNetworkInfoComponent> WeakThis(this);
		const bool bHasCores = Cores.Num() > 0;
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateLambda(
				[WeakThis, bHasCores]()
				{
					UKPCLNetworkInfoComponent* StrongThis = WeakThis.Get();
					if (StrongThis && StrongThis->CoreStateChanged.IsBound())
					{
						StrongThis->CoreStateChanged.Broadcast(bHasCores);
					}
				}),
			TStatId(), nullptr, ENamedThreads::GameThread);
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
	return mNetworkCoresInNetwork.Num() > 0 ? mNetworkCoresInNetwork[0].Get() : nullptr;
}

class AKPCLNetworkCore* UKPCLNetworkInfoComponent::GetFirstCores(bool& Valid) const
{
	AKPCLNetworkCore* Core = mNetworkCoresInNetwork.Num() > 0 ? mNetworkCoresInNetwork[0].Get() : nullptr;
	Valid = IsValid(Core);
	return Core;
}

TArray<class AKPCLNetworkCore*> UKPCLNetworkInfoComponent::GetCores() const { return mNetworkCoresInNetwork; }
