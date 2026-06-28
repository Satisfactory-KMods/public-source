// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Network/KPCLNetwork.h"

#include "KPrivateCodeLibModule.h"
#include "Net/UnrealNetwork.h"
#include "Network/Buildings/KPCLNetworkConnectionBuilding.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Network/KPCLNetworkInfoComponent.h"

/** Fake power values used to keep the circuit "alive" and fuse-free. */
static constexpr float KFaxitFakePowerProduced = 50000000.f; // 50 GW
static constexpr float KFaxitFakeBaseProduction = 1000000000.f; // 1 TW (used per-info-component)

/** Number of parallel tasks used to classify power-info components in OnCircuitChanged.
 *  Falls back to serial processing when the component count is smaller than this. */
static constexpr int32 KFaxitCircuitParallelGroups = 7;

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
	// Override whatever Super wrote so the circuit never trips a fuse.
	mHasPower = true;
	mIsFuseTriggered = false;
	mPowerProduced = KFaxitFakePowerProduced;

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
		if (!InfoComponent)
		{
			// Not every power-info in mPowerInfos is a network info component; skip unknown types.
			continue;
		}

		InfoComponent->SetBaseProduction(KFaxitFakeBaseProduction);
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

	TArray<AKPCLNetworkConnectionBuilding*> AllBuildings;
	TArray<AKPCLNetworkCore*> AllCores;

	// Use serial path for small component counts to avoid ParallelFor overhead.
	if (mPowerInfos.Num() <= KFaxitCircuitParallelGroups)
	{
		for (UFGPowerInfoComponent* Info : mPowerInfos)
		{
			if (const UKPCLNetworkInfoComponent* NetWorkInfo = Cast<UKPCLNetworkInfoComponent>(Info))
			{
				if (ensure(NetWorkInfo->GetOwner()))
				{
					if (AKPCLNetworkCore* CoreOwner = Cast<AKPCLNetworkCore>(Info->GetOwner()))
					{
						AllCores.Add(CoreOwner);
					}
					else if (AKPCLNetworkConnectionBuilding* BuildingOwner =
								 Cast<AKPCLNetworkConnectionBuilding>(Info->GetOwner()))
					{
						AllBuildings.Add(BuildingOwner);
					}
					else
					{
						UE_LOG(LogKPCL, Warning, TEXT(" Unknown Class: %s "), *Info->GetOwner()->GetName());
					}
				}
			}
		}
	}
	else
	{
		FCriticalSection Mutex;
		const int32 NumPerGroup =
			FMath::Max(FMath::DivideAndRoundUp(mPowerInfos.Num(), KFaxitCircuitParallelGroups), 1);

		ParallelFor(KFaxitCircuitParallelGroups,
					[&](int32 Index)
					{
						TArray<AKPCLNetworkCore*> Cores;
						TArray<AKPCLNetworkConnectionBuilding*> Buildings;

						for (int32 Member = Index * NumPerGroup;
							 Member < FMath::Min((Index + 1) * NumPerGroup, mPowerInfos.Num()); Member++)
						{
							if (const UKPCLNetworkInfoComponent* NetWorkInfo =
									Cast<UKPCLNetworkInfoComponent>(mPowerInfos[Member]))
							{
								if (ensure(NetWorkInfo->GetOwner()))
								{
									if (AKPCLNetworkCore* ManuOwner =
											Cast<AKPCLNetworkCore>(mPowerInfos[Member]->GetOwner()))
									{
										Cores.Add(ManuOwner);
									}
									else if (AKPCLNetworkConnectionBuilding* Owner =
												 Cast<AKPCLNetworkConnectionBuilding>(mPowerInfos[Member]->GetOwner()))
									{
										Buildings.Add(Owner);
									}
									else
									{
										UE_LOG(LogKPCL, Warning, TEXT(" Unknown Class: %s "),
											   *mPowerInfos[Member]->GetOwner()->GetName());
									}
								}
							}
						}

						// Fixed: was using manual Mutex.Lock/Unlock (not exception-safe).
						// Replaced with FScopeLock for RAII safety.
						FScopeLock ScopeLock(&Mutex);
						AllBuildings.Append(Buildings);
						AllCores.Append(Cores);
					});
	}

	mConnectionBuildings = AllBuildings;
	mCoreBuildings = AllCores;
	UpdateDataInInfos();
}

bool UKPCLNetwork::NetworkHasCore() const { return mCoreBuildings.Num() >= 1; }

bool UKPCLNetwork::NetworkHasCoreToMuchCores() const { return mCoreBuildings.Num() > 1; }

bool UKPCLNetwork::CoreStateIsOk() const
{
	AKPCLNetworkCore* Core = GetCore();
	if (IsValid(Core))
	{
		return Core->IsProducing() && !NetworkHasCoreToMuchCores();
	}
	return false;
}

AKPCLNetworkCore* UKPCLNetwork::GetCore() const { return mCoreBuildings.Num() > 0 ? mCoreBuildings[0].Get() : nullptr; }

TArray<AKPCLNetworkConnectionBuilding*> UKPCLNetwork::GetNetworkConnectionBuildings() const
{
	return mConnectionBuildings;
}
