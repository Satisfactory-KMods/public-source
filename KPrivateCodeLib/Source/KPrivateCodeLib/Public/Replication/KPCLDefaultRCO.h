#pragma once

#include "CoreMinimal.h"

#include "FGPlayerController.h"
#include "FGRemoteCallObject.h"

#include "OutlineSystem/KPCLOutlineSubsystem.h"
#include "Structures/KPCLNetworkStructures.h"
#include "Subsystem/KPCLSwatchSystem.h"

#include "KPCLDefaultRCO.generated.h"

class AKPCLBuildableBalanceSplitter;
class AKPCLDeliveryTaskSubsystem;
class AKPCLFaxitSubsystem;
class AKPCLNetworkBuildingBase;
class AKPCLNetworkCore;
class UKAPIDeliveryTask;

UCLASS()
class KPRIVATECODELIB_API UKPCLDefaultRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Networking", meta = (WorldContext = "WorldContext"))
	static UKPCLDefaultRCO* GetKPCLDefaultRCO(UObject* WorldContext) { return Get(WorldContext); }

	static UKPCLDefaultRCO* Get(UObject* WorldContext)
	{
		if (IsValid(WorldContext))
		{
			UWorld* World = WorldContext->GetWorld();
			if (IsValid(World))
			{
				if (AFGPlayerController* Controller = Cast<AFGPlayerController>(World->GetFirstPlayerController()))
				{
					if (UKPCLDefaultRCO* RCO = Controller->GetRemoteCallObjectOfClass<UKPCLDefaultRCO>())
					{
						return RCO;
					}
				}
			}
		}
		return nullptr;
	};

	template <class T>
	static T* GetRCO(UObject* WorldContext)
	{
		if (WorldContext)
		{
			if (AFGPlayerController* Controller =
					Cast<AFGPlayerController>(WorldContext->GetWorld()->GetFirstPlayerController()))
			{
				if (T* RCO = Controller->GetRemoteCallObjectOfClass<T>())
				{
					return RCO;
				}
			}
		}
		return nullptr;
	};

	UFUNCTION(Server, BlueprintCallable, WithValidation, Reliable)
	void Server_CreateOutlineForActor(AKPCLOutlineSubsystem* Subsystem, FOutlineData Data);
	FORCEINLINE bool Server_CreateOutlineForActor_Validate(AKPCLOutlineSubsystem* Subsystem, FOutlineData Data)
	{
		return true;
	}

	UFUNCTION(Server, BlueprintCallable, WithValidation, Reliable)
	void Server_ClearOutlines(AKPCLOutlineSubsystem* Subsystem);
	FORCEINLINE bool Server_ClearOutlines_Validate(AKPCLOutlineSubsystem* Subsystem) { return true; }

	UFUNCTION(Server, BlueprintCallable, WithValidation, Reliable)
	void Server_SetOutlineColor(AKPCLOutlineSubsystem* Subsystem, FLinearColor Color, EOutlineColorSlot ColorSlot);
	FORCEINLINE bool Server_SetOutlineColor_Validate(AKPCLOutlineSubsystem* Subsystem, FLinearColor Color,
													 EOutlineColorSlot ColorSlot)
	{
		return true;
	}

	UFUNCTION(Server, BlueprintCallable, WithValidation, Reliable)
	void Server_ClearOutlineForActor(AKPCLOutlineSubsystem* Subsystem, AActor* Actor);
	FORCEINLINE bool Server_ClearOutlineForActor_Validate(AKPCLOutlineSubsystem* Subsystem, AActor* Actor)
	{
		return true;
	}

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_FlushFluids(AFGBuildable* Building);
	bool Server_FlushFluids_Validate(AFGBuildable* Building) { return true; }
	virtual void Server_FlushFluids_Implementation(AFGBuildable* Building);

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_RemoveCustomSwatchData(AKPCLSwatchSystem* Target, int32 Idx);
	bool Server_RemoveCustomSwatchData_Validate(AKPCLSwatchSystem* Target, int32 Idx) { return true; }

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_AddCustomSwatchData(AKPCLSwatchSystem* Target, FCustomSwatchData Data);
	bool Server_AddCustomSwatchData_Validate(AKPCLSwatchSystem* Target, FCustomSwatchData Data) { return true; }

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_UpdateCustomSwatchData(AKPCLSwatchSystem* Target, FCustomSwatchData Data, int32 Idx);

	bool Server_UpdateCustomSwatchData_Validate(AKPCLSwatchSystem* Target, FCustomSwatchData Data, int32 Idx)
	{
		return true;
	}

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_LootChest(class AKPCLLootChest* Target, AFGCharacterPlayer* Player);
	bool Server_LootChest_Validate(class AKPCLLootChest* Target, AFGCharacterPlayer* Player) { return true; }

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_MoveItemAmount(class UFGInventoryComponent* Source, int32 SourceIndex, UFGInventoryComponent* Target,
							   FItemAmount Amount, bool ResizeToFit);

	bool Server_MoveItemAmount_Validate(class UFGInventoryComponent* Source, int32 SourceIndex,
										UFGInventoryComponent* Target, FItemAmount Amount, bool ResizeToFit)
	{
		return true;
	}

	UFUNCTION(BlueprintCallable)
	static int32 MoveItemAmount(class UFGInventoryComponent* Source, int32 SourceIndex, UFGInventoryComponent* Target,
								FItemAmount Amount, bool ResizeToFit);

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_DeliveryTask_TryToQueue(AKPCLDeliveryTaskSubsystem* Target, UKAPIDeliveryTask* Task);

	bool Server_DeliveryTask_TryToQueue_Validate(AKPCLDeliveryTaskSubsystem* Target, UKAPIDeliveryTask* Task);

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_DeliveryTask_TryGrabCurrentTaskItems(AKPCLDeliveryTaskSubsystem* Target);

	bool Server_DeliveryTask_TryGrabCurrentTaskItems_Validate(AKPCLDeliveryTaskSubsystem* Target);

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_DeliveryTask_TryToRemoveFromQueue(AKPCLDeliveryTaskSubsystem* Target, UKAPIDeliveryTask* Task);

	bool Server_DeliveryTask_TryToRemoveFromQueue_Validate(AKPCLDeliveryTaskSubsystem* Target, UKAPIDeliveryTask* Task);

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_DeliveryTask_TryToRemoveFromQueueAt(AKPCLDeliveryTaskSubsystem* Target, UKAPIDeliveryTask* Task,
													int32 QueueIndex);

	bool Server_DeliveryTask_TryToRemoveFromQueueAt_Validate(AKPCLDeliveryTaskSubsystem* Target,
															 UKAPIDeliveryTask* Task, int32 QueueIndex);

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_DeliveryTask_TryToDequeueFrom(AKPCLDeliveryTaskSubsystem* Target, UKAPIDeliveryTask* Task,
											  int32 QueueIndex);

	bool Server_DeliveryTask_TryToDequeueFrom_Validate(AKPCLDeliveryTaskSubsystem* Target, UKAPIDeliveryTask* Task,
													   int32 QueueIndex);

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_DeliveryTask_TryToMoveQueueEntry(AKPCLDeliveryTaskSubsystem* Target, UKAPIDeliveryTask* Task,
												 int32 FromIndex, int32 ToIndex);

	bool Server_DeliveryTask_TryToMoveQueueEntry_Validate(AKPCLDeliveryTaskSubsystem* Target, UKAPIDeliveryTask* Task,
														  int32 FromIndex, int32 ToIndex);

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Delivery_SetMinimumStoragePercentage(AKPCLNetworkCore* Target, float NewPercentage);

	bool Server_Delivery_SetMinimumStoragePercentage_Validate(AKPCLNetworkCore* Target, float NewPercentage)
	{
		return Target != nullptr && FMath::IsFinite(NewPercentage);
	}

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Faxit_SetFilterItem(class AKPCLNetworkConnectionBuilding* Target,
									TSubclassOf<UFGItemDescriptor> NewItem);

	bool Server_Faxit_SetFilterItem_Validate(class AKPCLNetworkConnectionBuilding* Target,
											 TSubclassOf<UFGItemDescriptor> NewItem)
	{
		return true;
	}

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Faxit_ClearSpeedOverride(class AKPCLNetworkConnectionBuilding* Target);
	bool Server_Faxit_ClearSpeedOverride_Validate(class AKPCLNetworkConnectionBuilding* Target) { return true; }

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Faxit_SetSpeedOverride(class AKPCLNetworkConnectionBuilding* Target, float Value);

	bool Server_Faxit_SetSpeedOverride_Validate(class AKPCLNetworkConnectionBuilding* Target, float Value)
	{
		return true;
	}

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Faxit_GrabFromNetwork(class AKPCLNetworkCore* Target, AFGCharacterPlayer* Player, FItemAmount Amount);

	bool Server_Faxit_GrabFromNetwork_Validate(class AKPCLNetworkCore* Target, AFGCharacterPlayer* Player,
											   FItemAmount Amount)
	{
		return true;
	}

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Faxit_StorageFromPlayerToNetwork(class AKPCLNetworkCore* Target, AFGCharacterPlayer* Player,
												 FItemAmount Amount);

	bool Server_Faxit_StorageFromPlayerToNetwork_Validate(class AKPCLNetworkCore* Target, AFGCharacterPlayer* Player,
														  FItemAmount Amount)
	{
		return true;
	}

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Faxit_UpdateNetworkName(class AKPCLFaxitSubsystem* Target, class AKPCLNetworkCore* Core,
										const FString& NewName);

	bool Server_Faxit_UpdateNetworkName_Validate(class AKPCLFaxitSubsystem* Target, class AKPCLNetworkCore* Core,
												 const FString& NewName)
	{
		return true;
	}

	UFUNCTION(Server, WithValidation, Reliable)
	void Server_Faxit_AddBuildingToCore(AKPCLNetworkBuildingBase* Building, AKPCLNetworkCore* Core);

	bool Server_Faxit_AddBuildingToCore_Validate(AKPCLNetworkBuildingBase* Building, AKPCLNetworkCore* Core);

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Valve_SetOnThreshold(class AKPCLPressureRegulatorValve* Target, float NewThreshold);

	bool Server_Valve_SetOnThreshold_Validate(class AKPCLPressureRegulatorValve* Target, float NewThreshold)
	{
		return true;
	}

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Valve_SetOffThreshold(class AKPCLPressureRegulatorValve* Target, float NewThreshold);

	bool Server_Valve_SetOffThreshold_Validate(class AKPCLPressureRegulatorValve* Target, float NewThreshold)
	{
		return true;
	}

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Valve_SetThresholds(class AKPCLPressureRegulatorValve* Target, float NewOnThreshold,
									float NewOffThreshold);

	bool Server_Valve_SetThresholds_Validate(class AKPCLPressureRegulatorValve* Target, float NewOnThreshold,
											 float NewOffThreshold)
	{
		return true;
	}

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Splitter_SetFilteredItems(AKPCLBuildableBalanceSplitter* Target, int32 Idx,
										  const TArray<TSubclassOf<UFGItemDescriptor>>& Items);

	bool Server_Splitter_SetFilteredItems_Validate(AKPCLBuildableBalanceSplitter* Target, int32 Idx,
												   const TArray<TSubclassOf<UFGItemDescriptor>>& Items)
	{
		return true;
	}

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Splitter_SetItemsPerMin(AKPCLBuildableBalanceSplitter* Target, int32 Idx, float ItemsPerMin);

	bool Server_Splitter_SetItemsPerMin_Validate(AKPCLBuildableBalanceSplitter* Target, int32 Idx, float ItemsPerMin)
	{
		return true;
	}

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Splitter_RemoveFromFilter(AKPCLBuildableBalanceSplitter* Target, int32 Idx,
										  TSubclassOf<UFGItemDescriptor> Item);

	bool Server_Splitter_RemoveFromFilter_Validate(AKPCLBuildableBalanceSplitter* Target, int32 Idx,
												   TSubclassOf<UFGItemDescriptor> Item)
	{
		return true;
	}

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Splitter_AddOrSetFilter(AKPCLBuildableBalanceSplitter* Target, int32 Idx,
										TSubclassOf<UFGItemDescriptor> Item);

	bool Server_Splitter_AddOrSetFilter_Validate(AKPCLBuildableBalanceSplitter* Target, int32 Idx,
												 TSubclassOf<UFGItemDescriptor> Item)
	{
		return true;
	}

	UPROPERTY(Replicated)
	bool mDummy = true;
};
