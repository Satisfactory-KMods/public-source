// Copyright Kyri123 / KMods 2026. All Rights Reserved.


#include "Replication/KPCLDefaultRCO.h"

#include "Buildable/Conveyor/KPCLBuildableBalanceSplitter.h"
#include "Buildable/KPCLExtractorBase.h"
#include "Buildable/KPCLPressureRegulatorValve.h"
#include "Buildable/KPCLProducerBase.h"
#include "FGCharacterPlayer.h"
#include "Looting/KPCLLootChest.h"
#include "Net/UnrealNetwork.h"
#include "Network/Buildings/KPCLNetworkConnectionBuilding.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Network/KPCLNetworkBuildingBase.h"
#include "Subsystem/KPCLDeliveryTaskSubsystem.h"
#include "Subsystem/KPCLFaxitSubsystem.h"

void UKPCLDefaultRCO::Server_CreateOutlineForActor_Implementation(AKPCLOutlineSubsystem* Subsystem, FOutlineData Data)
{
	if (Subsystem && Data.mActorToOutline)
	{
		Subsystem->MultiCast_CreateOutlineForActor(Data);
		Subsystem->ForceNetUpdate();
	}
}

void UKPCLDefaultRCO::Server_ClearOutlines_Implementation(AKPCLOutlineSubsystem* Subsystem)
{
	if (Subsystem)
	{
		Subsystem->MultiCast_ClearOutlines();
		Subsystem->ForceNetUpdate();
	}
}

void UKPCLDefaultRCO::Server_SetOutlineColor_Implementation(AKPCLOutlineSubsystem* Subsystem, FLinearColor Color,
															EOutlineColorSlot ColorSlot)
{
	if (Subsystem)
	{
		Subsystem->MultiCast_SetOutlineColor(Color, ColorSlot);
		Subsystem->ForceNetUpdate();
	}
}

void UKPCLDefaultRCO::Server_ClearOutlineForActor_Implementation(AKPCLOutlineSubsystem* Subsystem, AActor* Actor)
{
	if (Subsystem && Actor)
	{
		Subsystem->MultiCast_ClearOutlinesForActor(Actor);
		Subsystem->ForceNetUpdate();
	}
}

void UKPCLDefaultRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKPCLDefaultRCO, mDummy);
}

void UKPCLDefaultRCO::Server_FlushFluids_Implementation(AFGBuildable* Building)
{
	if (AKPCLProducerBase* Producer = Cast<AKPCLProducerBase>(Building))
	{
		Producer->FlushFluids();
		Producer->ForceNetUpdate();
		return;
	}

	if (AKPCLExtractorBase* Extractor = Cast<AKPCLExtractorBase>(Building))
	{
		Extractor->FlushFluids();
		Extractor->ForceNetUpdate();
	}
}

void UKPCLDefaultRCO::Server_LootChest_Implementation(AKPCLLootChest* Target, AFGCharacterPlayer* Player)
{
	if (ensure(Target))
	{
		Target->Loot(Player);
	}
}

int32 UKPCLDefaultRCO::MoveItemAmount(UFGInventoryComponent* Source, int32 SourceIndex, UFGInventoryComponent* Target,
									  FItemAmount Amount, bool ResizeToFit)
{
	if (ensure(Source && Target && Amount.ItemClass && Amount.Amount > 0))
	{
		FInventoryStack Stack;
		if (Source->GetStackFromIndex(SourceIndex, Stack))
		{
			if (Stack.NumItems >= Amount.Amount || ResizeToFit)
			{
				if (ResizeToFit && Stack.NumItems < Amount.Amount)
				{
					Amount.Amount = Stack.NumItems;
				}

				int32 GrabedAmount = Target->AddStack(FInventoryStack(Amount.Amount, Amount.ItemClass));

				if (GrabedAmount > 0)
				{
					Source->RemoveFromIndex(SourceIndex, GrabedAmount);
				}

				return GrabedAmount;
			}
		}
	}
	return 0;
}

void UKPCLDefaultRCO::Server_MoveItemAmount_Implementation(UFGInventoryComponent* Source, int32 SourceIndex,
														   UFGInventoryComponent* Target, FItemAmount Amount,
														   bool ResizeToFit)
{
	MoveItemAmount(Source, SourceIndex, Target, Amount, ResizeToFit);
}

void UKPCLDefaultRCO::Server_UpdateCustomSwatchData_Implementation(AKPCLSwatchSystem* Target, FCustomSwatchData Data,
																   int32 Idx)
{
	if (ensureMsgf(Target, TEXT("Cant Found AKPCLSwatchSystem Server_AddCustomSwatchData")))
	{
		Target->UpdateCustomSwatchData(Data, Idx);
		Target->ForceNetUpdate();
	}
}

void UKPCLDefaultRCO::Server_AddCustomSwatchData_Implementation(AKPCLSwatchSystem* Target, FCustomSwatchData Data)
{
	if (ensureMsgf(Target, TEXT("Cant Found AKPCLSwatchSystem Server_AddCustomSwatchData")))
	{
		Target->AddCustomSwatchData(Data);
		Target->ForceNetUpdate();
	}
}

void UKPCLDefaultRCO::Server_RemoveCustomSwatchData_Implementation(AKPCLSwatchSystem* Target, int32 Idx)
{
	if (ensureMsgf(Target, TEXT("Cant Found AKPCLSwatchSystem Server_RemoveCustomSwatchData")))
	{
		Target->RemoveCustomSwatchData(Idx);
		Target->ForceNetUpdate();
	}
}

namespace
{
	bool IsCanonicalDeliveryTaskTarget(const UKPCLDefaultRCO* RCO, const AKPCLDeliveryTaskSubsystem* Target)
	{
		return IsValid(RCO) && IsValid(Target) && Target == AKPCLDeliveryTaskSubsystem::Get(RCO->GetWorld());
	}

	AFGCharacterPlayer* GetRequestingPlayer(const UKPCLDefaultRCO* RCO)
	{
		AFGPlayerController* Controller = IsValid(RCO) ? RCO->GetTypedOuter<AFGPlayerController>() : nullptr;
		AFGCharacterPlayer* Player = IsValid(Controller) ? Cast<AFGCharacterPlayer>(Controller->GetPawn()) : nullptr;
		return IsValid(Player) && Player->GetWorld() == RCO->GetWorld() ? Player : nullptr;
	}

	bool IsQueuedTaskAt(const UKPCLDefaultRCO* RCO, AKPCLDeliveryTaskSubsystem* Target, const UKAPIDeliveryTask* Task,
						int32 QueueIndex)
	{
		if (!IsCanonicalDeliveryTaskTarget(RCO, Target) || !IsValid(Task))
		{
			return false;
		}

		const TArray<UKAPIDeliveryTask*> QueuedTasks = Target->GetQueuedTasks();
		return QueuedTasks.IsValidIndex(QueueIndex) && QueuedTasks[QueueIndex] == Task;
	}
}

bool UKPCLDefaultRCO::Server_DeliveryTask_TryToQueue_Validate(AKPCLDeliveryTaskSubsystem* Target,
															  UKAPIDeliveryTask* Task)
{
	return IsCanonicalDeliveryTaskTarget(this, Target);
}

bool UKPCLDefaultRCO::Server_DeliveryTask_TryGrabCurrentTaskItems_Validate(AKPCLDeliveryTaskSubsystem* Target)
{
	return IsCanonicalDeliveryTaskTarget(this, Target);
}

bool UKPCLDefaultRCO::Server_DeliveryTask_TryToRemoveFromQueue_Validate(AKPCLDeliveryTaskSubsystem* Target,
																		UKAPIDeliveryTask* Task)
{
	return IsCanonicalDeliveryTaskTarget(this, Target);
}

bool UKPCLDefaultRCO::Server_DeliveryTask_TryToRemoveFromQueueAt_Validate(AKPCLDeliveryTaskSubsystem* Target,
																		  UKAPIDeliveryTask* Task, int32 QueueIndex)
{
	return IsCanonicalDeliveryTaskTarget(this, Target) && QueueIndex >= 0;
}

bool UKPCLDefaultRCO::Server_DeliveryTask_TryToDequeueFrom_Validate(AKPCLDeliveryTaskSubsystem* Target,
																	UKAPIDeliveryTask* Task, int32 QueueIndex)
{
	return IsCanonicalDeliveryTaskTarget(this, Target) && QueueIndex >= 0;
}

bool UKPCLDefaultRCO::Server_DeliveryTask_TryToMoveQueueEntry_Validate(AKPCLDeliveryTaskSubsystem* Target,
																	   UKAPIDeliveryTask* Task, int32 FromIndex,
																	   int32 ToIndex)
{
	return IsCanonicalDeliveryTaskTarget(this, Target) && FromIndex >= 0 && ToIndex >= 0;
}

void UKPCLDefaultRCO::Server_DeliveryTask_TryToQueue_Implementation(AKPCLDeliveryTaskSubsystem* Target,
																	UKAPIDeliveryTask* Task)
{
	if (IsCanonicalDeliveryTaskTarget(this, Target) && IsValid(Task))
	{
		TArray<FText> ErrorMessages;
		Target->TryToQueueTask(Task, ErrorMessages);
	}
}

void UKPCLDefaultRCO::Server_DeliveryTask_TryGrabCurrentTaskItems_Implementation(
	AKPCLDeliveryTaskSubsystem* Target)
{
	AFGCharacterPlayer* Player = GetRequestingPlayer(this);
	if (IsCanonicalDeliveryTaskTarget(this, Target) && IsValid(Player))
	{
		Target->TryGrabCurrentTaskItemsFromPlayer(Player);
	}
}

void UKPCLDefaultRCO::Server_DeliveryTask_TryToRemoveFromQueue_Implementation(AKPCLDeliveryTaskSubsystem* Target,
																			  UKAPIDeliveryTask* Task)
{
	if (IsCanonicalDeliveryTaskTarget(this, Target) && IsValid(Task))
	{
		Target->TryToRemoveFromQueue(Task);
	}
}

void UKPCLDefaultRCO::Server_DeliveryTask_TryToRemoveFromQueueAt_Implementation(AKPCLDeliveryTaskSubsystem* Target,
																				UKAPIDeliveryTask* Task,
																				int32 QueueIndex)
{
	if (IsQueuedTaskAt(this, Target, Task, QueueIndex))
	{
		Target->TryToRemoveFromQueueAt(QueueIndex);
	}
}

void UKPCLDefaultRCO::Server_DeliveryTask_TryToDequeueFrom_Implementation(AKPCLDeliveryTaskSubsystem* Target,
																		  UKAPIDeliveryTask* Task, int32 QueueIndex)
{
	if (IsQueuedTaskAt(this, Target, Task, QueueIndex))
	{
		Target->TryToDequeueFrom(QueueIndex);
	}
}

void UKPCLDefaultRCO::Server_DeliveryTask_TryToMoveQueueEntry_Implementation(AKPCLDeliveryTaskSubsystem* Target,
																			 UKAPIDeliveryTask* Task, int32 FromIndex,
																			 int32 ToIndex)
{
	if (IsQueuedTaskAt(this, Target, Task, FromIndex) && Target->GetQueuedTasks().IsValidIndex(ToIndex))
	{
		Target->TryToMoveQueueEntry(FromIndex, ToIndex);
	}
}

void UKPCLDefaultRCO::Server_Delivery_SetMinimumStoragePercentage_Implementation(AKPCLNetworkCore* Target,
																				 float NewPercentage)
{
	if (IsValid(Target))
	{
		Target->SetMinimumStoragePercentage(NewPercentage);
	}
}

void UKPCLDefaultRCO::Server_Faxit_GrabFromNetwork_Implementation(class AKPCLNetworkCore* Target,
																  AFGCharacterPlayer* Player, FItemAmount Amount)
{
	if (IsValid(Target))
	{
		Target->GrabFromNetwork(Player, Amount);
	}
}

void UKPCLDefaultRCO::Server_Faxit_StorageFromPlayerToNetwork_Implementation(class AKPCLNetworkCore* Target,
																			 AFGCharacterPlayer* Player,
																			 FItemAmount Amount)
{
	if (IsValid(Target))
	{
		Target->StorageFromPlayerToNetwork(Player, Amount);
	}
}

void UKPCLDefaultRCO::Server_Faxit_UpdateNetworkName_Implementation(class AKPCLFaxitSubsystem* Target,
																	class AKPCLNetworkCore* Core,
																	const FString& NewName)
{
	if (IsValid(Target))
	{
		Target->UpdateNetworkName(Core, NewName);
	}
}

namespace
{
	bool CanProcessFaxitAddBuildingToCore(const UKPCLDefaultRCO* RCO, AKPCLNetworkBuildingBase* Building,
										 AKPCLNetworkCore* Core)
	{
		if (!IsValid(RCO) || !IsValid(Building) || !IsValid(Core) || Building == Core ||
			Building->GetWorld() != RCO->GetWorld() || Core->GetWorld() != RCO->GetWorld())
		{
			return false;
		}

		AFGCharacterPlayer* Player = GetRequestingPlayer(RCO);
		if (!IsValid(Player) || !Building->GetInteractingPlayers().Contains(Player))
		{
			return false;
		}

		AKPCLFaxitSubsystem* FaxitSubsystem = AKPCLFaxitSubsystem::Get(RCO->GetWorld());
		if (!IsValid(FaxitSubsystem))
		{
			return false;
		}

		FKPCLFaxitNetwork Network;
		if (!FaxitSubsystem->GetNetworkByCore(Core, Network) || !Network.mIsValid || Network.mCore != Core)
		{
			return false;
		}

		return Network.IsActorInNetwork(Building) || FaxitSubsystem->CanAddToNetwork(Network.mNetworkId);
	}
}

bool UKPCLDefaultRCO::Server_Faxit_AddBuildingToCore_Validate(AKPCLNetworkBuildingBase*, AKPCLNetworkCore*)
{

	return true;
}

void UKPCLDefaultRCO::Server_Faxit_AddBuildingToCore_Implementation(AKPCLNetworkBuildingBase* Building,
																	AKPCLNetworkCore* Core)
{
	AKPCLFaxitSubsystem* FaxitSubsystem = AKPCLFaxitSubsystem::Get(this);
	if (IsValid(FaxitSubsystem) && CanProcessFaxitAddBuildingToCore(this, Building, Core))
	{
		FaxitSubsystem->AddBuildingToCore(Building, Core);
	}
}

void UKPCLDefaultRCO::Server_Faxit_SetSpeedOverride_Implementation(class AKPCLNetworkConnectionBuilding* Target,
																   float Value)
{
	if (IsValid(Target))
	{
		Target->SetSpeedOverride(Value);
	}
}

void UKPCLDefaultRCO::Server_Faxit_ClearSpeedOverride_Implementation(class AKPCLNetworkConnectionBuilding* Target)
{
	if (IsValid(Target))
	{
		Target->ClearSpeedOverride();
	}
}

void UKPCLDefaultRCO::Server_Faxit_SetFilterItem_Implementation(class AKPCLNetworkConnectionBuilding* Target,
																TSubclassOf<UFGItemDescriptor> NewItem)
{
	if (IsValid(Target))
	{
		Target->SetFilterItem(NewItem);
	}
}

void UKPCLDefaultRCO::Server_Valve_SetOnThreshold_Implementation(AKPCLPressureRegulatorValve* Target,
																 float NewThreshold)
{
	if (IsValid(Target))
	{
		Target->SetOnThreshold(NewThreshold);
	}
}

void UKPCLDefaultRCO::Server_Valve_SetOffThreshold_Implementation(AKPCLPressureRegulatorValve* Target,
																  float NewThreshold)
{
	if (IsValid(Target))
	{
		Target->SetOffThreshold(NewThreshold);
	}
}

void UKPCLDefaultRCO::Server_Valve_SetThresholds_Implementation(AKPCLPressureRegulatorValve* Target,
																float NewOnThreshold, float NewOffThreshold)
{
	if (IsValid(Target))
	{
		Target->SetThresholds(NewOnThreshold, NewOffThreshold);
	}
}

void UKPCLDefaultRCO::Server_Splitter_SetFilteredItems_Implementation(
	AKPCLBuildableBalanceSplitter* Target, int32 Idx, const TArray<TSubclassOf<UFGItemDescriptor>>& Items)
{
	if (IsValid(Target))
	{
		Target->SetFilteredItems(Idx, Items);
	}
}

void UKPCLDefaultRCO::Server_Splitter_SetItemsPerMin_Implementation(AKPCLBuildableBalanceSplitter* Target, int32 Idx,
																	float ItemsPerMin)
{
	if (IsValid(Target))
	{
		Target->SetItemsPerMin(Idx, ItemsPerMin);
	}
}

void UKPCLDefaultRCO::Server_Splitter_RemoveFromFilter_Implementation(AKPCLBuildableBalanceSplitter* Target, int32 Idx,
																	  TSubclassOf<UFGItemDescriptor> Item)
{
	if (IsValid(Target))
	{
		Target->RemoveFromFilter(Idx, Item);
	}
}

void UKPCLDefaultRCO::Server_Splitter_AddOrSetFilter_Implementation(AKPCLBuildableBalanceSplitter* Target, int32 Idx,
																	TSubclassOf<UFGItemDescriptor> Item)
{
	if (IsValid(Target))
	{
		Target->AddOrSetFilter(Idx, Item);
	}
}
