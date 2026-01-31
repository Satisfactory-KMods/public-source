// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/KPCLNetwork.h"

#include "KPrivateCodeLibModule.h"
#include "Net/UnrealNetwork.h"
#include "Network/KPCLNetworkInfoComponent.h"
#include "Network/Buildings/KPCLNetworkConnectionBuilding.h"
#include "Network/Buildings/KPCLNetworkCore.h"

void UKPCLNetwork::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKPCLNetwork, mConnectionBuildings);
	DOREPLIFETIME(UKPCLNetwork, mCoreBuildings);
}

void UKPCLNetwork::TickCircuit(float dt)
{
	mHasPower = true;
	Super::TickCircuit(dt);
	mHasPower = true;
	mIsFuseTriggered = false;
	mPowerProduced = 50000000.f;

	if (HasAuthority() && CoreStateIsOk())
	{
		if (IsFuseTriggered())
		{
			ResetFuse();
		}
	}
}

void UKPCLNetwork::UpdateDataInInfos()
{
	for (UFGPowerInfoComponent* Group : mPowerInfos)
	{
		UKPCLNetworkInfoComponent* InfoComponent = Cast<UKPCLNetworkInfoComponent>(Group);

		InfoComponent->SetCors(mCoreBuildings);
	}
}

FString UKPCLNetwork::GetNetworkId() const
{
	AKPCLNetworkCore* Core = GetCore();
	return IsValid(Core) ? Core->GetNetworkId() : FString();
}

void UKPCLNetwork::OnCircuitChanged()
{
	Super::OnCircuitChanged();

	FCriticalSection Mutex;

	TArray<AKPCLNetworkConnectionBuilding*> AllBuildings;
	TArray<AKPCLNetworkCore*> AllCores;

	const int32 NumPerGroup = FMath::Max(FMath::DivideAndRoundUp(mPowerInfos.Num(), 7), 1);
	ParallelFor(7, [&](int32 Index)
	{
		TArray<AKPCLNetworkCore*> Cores;
		TArray<AKPCLNetworkConnectionBuilding*> Buildings;

		for (int32 Member = Index * NumPerGroup; Member < FMath::Min((Index + 1) * NumPerGroup, mPowerInfos.Num());
		     Member++)
		{
			if (const UKPCLNetworkInfoComponent* NetWorkInfo = Cast<UKPCLNetworkInfoComponent>(mPowerInfos[Member]))
			{
				if (ensure(NetWorkInfo->GetOwner()))
				{
					if (AKPCLNetworkCore* ManuOwner = Cast<AKPCLNetworkCore>(
						mPowerInfos[Member]->GetOwner()))
					{
						Cores.Add(ManuOwner);
					}
					else if (AKPCLNetworkConnectionBuilding* Owner = Cast<AKPCLNetworkConnectionBuilding>(
						mPowerInfos[Member]->GetOwner()))
					{
						Buildings.Add(Owner);
					}
					else
					{
						UE_LOG(LogKPCL, Warning, TEXT(" Unknown Class: %s "),
						       *mPowerInfos[ Member ]->GetOwner()->GetName());
					}
				}
			}
		}

		Mutex.Lock();
		AllBuildings.Append(Buildings);
		AllCores.Append(Cores);
		Mutex.Unlock();
	});

	mConnectionBuildings = AllBuildings;
	mCoreBuildings = AllCores;
	UpdateDataInInfos();
}

bool UKPCLNetwork::NetworkHasCore() const
{
	return mCoreBuildings.Num() >= 1;
}

bool UKPCLNetwork::NetworkHasCoreToMuchCores() const
{
	return mCoreBuildings.Num() > 1;
}

bool UKPCLNetwork::CoreStateIsOk() const
{
	if (IsValid(GetCore()))
	{
		return GetCore()->IsProducing() && !NetworkHasCoreToMuchCores();
	}
	return false;
}

AKPCLNetworkCore* UKPCLNetwork::GetCore() const
{
	return mCoreBuildings.Num() > 0 ? mCoreBuildings[0] : nullptr;
}

TArray<AKPCLNetworkConnectionBuilding*> UKPCLNetwork::GetNetworkConnectionBuildings() const
{
	return mConnectionBuildings;
}
