// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGPlayerController.h"
#include "FGRemoteCallObject.h"
#include "Components/KPCLNetworkPlayerComponent.h"
#include "OutlineSystem/KPCLOutlineSubsystem.h"
#include "Structures/KPCLNetworkStructures.h"
#include "Subsystem/KPCLSwatchSystem.h"
#include "KPCLDefaultRCO.generated.h"

/**
 * 
 */
UCLASS()
class KPRIVATECODELIB_API UKPCLDefaultRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Networking", meta = (WorldContext = "WorldContext"))
	static UKPCLDefaultRCO* GetKPCLDefaultRCO(UObject* WorldContext) { return Get(WorldContext); }

	static UKPCLDefaultRCO* Get(UObject* WorldContext)
	{
		if (WorldContext)
		{
			if (AFGPlayerController* Controller = Cast<AFGPlayerController>(
				WorldContext->GetWorld()->GetFirstPlayerController()))
			{
				if (UKPCLDefaultRCO* RCO = Controller->GetRemoteCallObjectOfClass<UKPCLDefaultRCO>())
				{
					return RCO;
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
			if (AFGPlayerController* Controller = Cast<AFGPlayerController>(
				WorldContext->GetWorld()->GetFirstPlayerController()))
			{
				if (T* RCO = Controller->GetRemoteCallObjectOfClass<T>())
				{
					return RCO;
				}
			}
		}
		return nullptr;
	};

	// Start Outline

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
	                                                 EOutlineColorSlot ColorSlot) { return true; }

	UFUNCTION(Server, BlueprintCallable, WithValidation, Reliable)
	void Server_ClearOutlineForActor(AKPCLOutlineSubsystem* Subsystem, AActor* Actor);
	FORCEINLINE bool Server_ClearOutlineForActor_Validate(AKPCLOutlineSubsystem* Subsystem, AActor* Actor)
	{
		return true;
	}

	// End Outline

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


	// START: Faxit

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_Faxit_SetFilterItem(class AKPCLNetworkConnectionBuilding* Target,
	                                TSubclassOf<UFGItemDescriptor> NewItem);

	bool Server_Faxit_SetFilterItem_Validate(class AKPCLNetworkConnectionBuilding* Target,
	                                         TSubclassOf<UFGItemDescriptor> NewItem) { return true; }

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
	                                           FItemAmount Amount) { return true; }

	// END: Faxit

	UPROPERTY(Replicated)
	bool mDummy = true;
};
