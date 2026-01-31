#include "Buildable/Slugs/KLBuildableSlugBuildingBase.h"

#include "FGFactoryConnectionComponent.h"

AKLBuildableSlugBuildingBase::AKLBuildableSlugBuildingBase()
	: mSlugSubsystem(nullptr)
{
	mInputInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::InputName);
	mOutputInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::OutputName);
}

void AKLBuildableSlugBuildingBase::BeginPlay()
{
	Super::BeginPlay();
	mSlugSubsystem = AKLSlugSubsystem::Get(this);
}

void AKLBuildableSlugBuildingBase::SetBelts()
{
	TArray<UFGFactoryConnectionComponent*> BeltsToSet = GetAllConv();
	for (UFGFactoryConnectionComponent* BeltConnection : BeltsToSet)
	{
		if (BeltConnection)
		{
			BeltConnection->SetInventory(BeltConnection->GetDirection() == EFactoryConnectionDirection::FCD_INPUT
				                             ? GetInventory()
				                             : GetOutputInventory());
		}
	}

	/*TArray<UFGPipeConnectionFactory*> PipesToSet = GetAllPipes();
	for (UFGPipeConnectionFactory* PipeConnection : PipesToSet)
	{
		if (PipeConnection)
		{
			PipeConnection->SetInventory(PipeConnection->GetPipeConnectionType() == EPipeConnectionType::PCT_CONSUMER
				? GetInventory()
				: GetOutputInventory());
		}
	}*/
}

void AKLBuildableSlugBuildingBase::OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
                                                    UFGInventoryComponent* sourceInventory)
{
	Super::OnInputItemAdded(itemClass, numRemoved, sourceInventory);
	OnSlotChecking(itemClass, false);
}

void AKLBuildableSlugBuildingBase::OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
                                                      UFGInventoryComponent* sourceInventory)
{
	Super::OnInputItemRemoved(itemClass, numRemoved, sourceInventory);
	OnSlotChecking(itemClass, true);
}

void AKLBuildableSlugBuildingBase::OnOutputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
                                                     UFGInventoryComponent* sourceInventory)
{
	Super::OnOutputItemAdded(itemClass, numRemoved, sourceInventory);
	OnSlotOutputChecking(itemClass, false);
}

void AKLBuildableSlugBuildingBase::OnOutputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
                                                       UFGInventoryComponent* sourceInventory)
{
	Super::OnOutputItemRemoved(itemClass, numRemoved, sourceInventory);
	OnSlotOutputChecking(itemClass, true);
}

FInventoryStack AKLBuildableSlugBuildingBase::GetStackFromIndex(int32 Index, bool OutPutInventory) const
{
	UFGInventoryComponent* Inventory = OutPutInventory ? GetOutputInventory() : GetInventory();
	FInventoryStack Stack;
	if (Inventory)
	{
		if (Inventory->IsValidIndex(Index))
		{
			Inventory->GetStackFromIndex(Index, Stack);
		}
	}
	return Stack;
}

/*
void AKLBuildableSlugBuildingBase::Multicast_TempChanged_Implementation(float NewValue)
{
	OnTempChanged.Broadcast(NewValue);
}

void AKLBuildableSlugBuildingBase::Multicast_HumidityChanged_Implementation(float NewValue)
{
	OnHumidityChanged.Broadcast(NewValue);
}*/
