// Fill out your copyright notice in the Description page of Project Settings.
#include "Replication/KPCLDefaultRCO.h"

#include "Buildable/Conveyor/KPCLBuildableBalanceSplitter.h"
#include "Buildable/KPCLExtractorBase.h"
#include "Buildable/KPCLPressureRegulatorValve.h"
#include "Buildable/KPCLProducerBase.h"
#include "Looting/KPCLLootChest.h"
#include "Net/UnrealNetwork.h"
#include "Network/Buildings/KPCLNetworkConnectionBuilding.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Subsystem/KPCLFaxitSubsystem.h"

// Start Outline

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

//~ End Outline

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

//~ Begin PressureRegulatorValve

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

void UKPCLDefaultRCO::Server_Valve_SetInvertThresholdBehavior_Implementation(AKPCLPressureRegulatorValve* Target,
																			 bool bInvert)
{
	if (IsValid(Target))
	{
		Target->SetInvertThresholdBehavior(bInvert);
	}
}

//~ End PressureRegulatorValve

//~ Begin BalanceSplitter

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

//~ End BalanceSplitter
