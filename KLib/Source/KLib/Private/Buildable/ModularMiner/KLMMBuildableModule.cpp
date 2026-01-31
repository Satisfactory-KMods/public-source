// ILikeBanas


#include "Buildable/ModularMiner/KLMMBuildableModule.h"

#include "FGFactoryConnectionComponent.h"
#include "FGGasPillar.h"
#include "FGPowerInfoComponent.h"
#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Buildable/ModularMiner/KLMMBuildableMiner.h"
#include "Cpp/KBFLCppInventoryHelper.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Net/UnrealNetwork.h"

AKLMMBuildableModule::AKLMMBuildableModule()
{
	bReplicates = true;
	mFactoryTickFunction.bCanEverTick = true;
	PrimaryActorTick.bCanEverTick = true;

	mInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::InputName);
	mInventory->SetDefaultSize(1);
}

void AKLMMBuildableModule::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && !HasMiner())
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AKLMMBuildableModule::TryToRegister);
	}
}

void AKLMMBuildableModule::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool AKLMMBuildableModule::CanProduce_Implementation() const
{
	if (IsPlayingBuildEffect())
	{
		return false;
	}

	if (HasMiner() && GetMiner()->GetHasPower())
	{
		return GetMiner()->IsProducing();
	}

	return false;
}

void AKLMMBuildableModule::HandlePower(float dt)
{
	// override to disable power handling
	// is handles in the miner
	if (HasMiner())
	{
		mPowerOptions.bIsRunning = GetMiner()->IsProducing();
	}

	GetPowerInfo()->SetTargetConsumption(0.0f);
	GetPowerInfo()->SetBaseProduction(0.0f);
	GetPowerInfo()->SetMaximumTargetConsumption(0.0f);
}

bool AKLMMBuildableModule::Factory_HasPower() const
{
	if (HasMiner())
	{
		return GetMiner()->Factory_HasPower();
	}
	return Super::Factory_HasPower();
}

float AKLMMBuildableModule::GetPowerConsume() const
{
	return Super::GetPowerConsume();
}

void AKLMMBuildableModule::CollectAndPushPipes(float dt, bool IsPush)
{
	Super::CollectAndPushPipes(dt, IsPush);

	if (IsPush && HasPipeConnection())
	{
		UKBFLCppInventoryHelper::PushPipe(GetInventory(), 0, dt, GetPipe(0, KPCLOutput));
	}
}

void AKLMMBuildableModule::OnModuleProductionCompleted_Implementation()
{
}

void AKLMMBuildableModule::TryToRegister()
{
	TArray<AActor*> OutActors;
	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = TArray<TEnumAsByte<EObjectTypeQuery>>{
		ObjectTypeQuery1, ObjectTypeQuery2, ObjectTypeQuery3, ObjectTypeQuery4, ObjectTypeQuery5
	};
	const bool MinerFound = UKismetSystemLibrary::SphereOverlapActors(GetWorld(), GetActorLocation(), 500.0f,
	                                                                  ObjectTypes, AKLMMBuildableMiner::StaticClass(),
	                                                                  {this}, OutActors);

	UE_LOG(LogTemp, Warning, TEXT("Miner in Area: %d because miner module '%s' need a new miner!"), MinerFound,
	       *GetName());

	// Grab Miner
	if (MinerFound)
	{
		if (AKLMMBuildableMiner* lMiner = Cast<AKLMMBuildableMiner>(OutActors[0]))
		{
			Execute_AttachedActor(lMiner, this, mAttachmentClass, GetActorTransform(), 500);
		}
	}
}

void AKLMMBuildableModule::SetBelts()
{
	if (UFGFactoryConnectionComponent* lBelt = GetConv(0, KPCLOutput))
	{
		lBelt->SetInventory(GetInventory());

		if (mHasBeltOutput || HasPipeConnection())
		{
			if (GetMasterBuildable<AKLMMBuildableMiner>())
			{
				if (GetMasterBuildable<AKLMMBuildableMiner>()->GetTrashDescriptor())
				{
					if (GetInventory())
					{
						UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(
							GetInventory(), 0, GetMasterBuildable<AKLMMBuildableMiner>()->GetTrashDescriptor());
						return;
					}
				}
			}
		}
	}

	//if (UFGPipeConnectionFactory* lPipe = GetPipe(0, KPCLOutput))
	//{
	//	lPipe->SetInventory(GetInventory());
	//}

	if (GetInventory())
	{
		if (GetMasterBuildable<AKLMMBuildableMiner>())
		{
			if (const TSubclassOf<UFGItemDescriptor> ItemClass = GetMasterBuildable<AKLMMBuildableMiner>()->
				GetTrashDescriptor())
			{
				UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), 0, ItemClass);
			}
		}
	}
}

void AKLMMBuildableModule::StorageItem(TSubclassOf<UFGItemDescriptor> Item, int Count) const
{
	UKBFLCppInventoryHelper::StoreItemAmountInInventory(GetInventory(), 0, Item, Count);
}

void AKLMMBuildableModule::SetAllowedItem(TSubclassOf<UFGItemDescriptor> Item) const
{
	UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), 0, Item);
}

bool AKLMMBuildableModule::CanStorageOutput(TSubclassOf<UFGItemDescriptor> Item, int Count) const
{
	return UKBFLCppInventoryHelper::CanStoreItem(GetInventory(), 0, Item, Count);
}

TSubclassOf<UFGItemDescriptor> AKLMMBuildableModule::GetAllowedItem() const
{
	return GetInventory()->GetAllowedItemOnIndex(0);
}

TSubclassOf<UKAPIModularAttachmentDescriptor> AKLMMBuildableModule::GetType() const
{
	return mAttachmentClass;
}

bool AKLMMBuildableModule::HasMiner() const
{
	return GetMiner() != nullptr;
}

bool AKLMMBuildableModule::IsVariable() const
{
	return GetPowerOption().IsPowerVariable();
}

AKLMMBuildableBase* AKLMMBuildableModule::GetMiner() const
{
	return GetMasterBuildable<AKLMMBuildableBase>();
}

int AKLMMBuildableModule::GetTier() const
{
	return mTier;
}

float AKLMMBuildableModule::GetMalus() const
{
	return mMalus;
}

float AKLMMBuildableModule::GetBonus() const
{
	return mBonus;
}

bool AKLMMBuildableModule::HasPipeConnection() const
{
	return GetPipe(0, KPCLOutput) != nullptr;
}
