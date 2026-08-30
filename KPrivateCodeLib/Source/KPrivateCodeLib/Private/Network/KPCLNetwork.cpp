#include "Network/KPCLNetwork.h"

#include "KPrivateCodeLibModule.h"
#include "Net/UnrealNetwork.h"
#include "Network/Buildings/KPCLNetworkConnectionBuilding.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Network/Buildings/KPCLNetworkDelivery.h"
#include "Network/KPCLNetworkInfoComponent.h"

static constexpr float KFaxitFakePowerProduced = 50000000.f;
static constexpr float KFaxitFakeBaseProduction = 1000000000.f;

static constexpr int32 KFaxitCircuitParallelGroups = 7;

void UKPCLNetwork::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKPCLNetwork, mConnectionBuildings);
	DOREPLIFETIME(UKPCLNetwork, mCoreBuildings);
	DOREPLIFETIME(UKPCLNetwork, mDeliveryBuildings);
}

void UKPCLNetwork::TickCircuit(float dt)
{
	mHasPower = true;
	Super::TickCircuit(dt);

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
	TArray<AKPCLNetworkDelivery*> AllDeliveries;

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
					else if (AKPCLNetworkDelivery* DeliveryOwner = Cast<AKPCLNetworkDelivery>(Info->GetOwner()))
					{
						AllDeliveries.Add(DeliveryOwner);
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
						TArray<AKPCLNetworkDelivery*> Deliveries;

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
									else if (AKPCLNetworkDelivery* DeliveryOwner =
												 Cast<AKPCLNetworkDelivery>(mPowerInfos[Member]->GetOwner()))
									{
										Deliveries.Add(DeliveryOwner);
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

						FScopeLock ScopeLock(&Mutex);
						AllBuildings.Append(Buildings);
						AllCores.Append(Cores);
						AllDeliveries.Append(Deliveries);
					});
	}

	mConnectionBuildings = AllBuildings;
	mCoreBuildings = AllCores;
	mDeliveryBuildings = AllDeliveries;
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

TArray<AKPCLNetworkDelivery*> UKPCLNetwork::GetNetworkDeliveryBuildings() const { return mDeliveryBuildings; }
