// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildable/Modular/KPCLModularBuildingBase.h"
#include "Replication/KLDefaultRCO.h"
#include "Subsystem/KLSlugSubsystem.h"

#include "KLBuildableSlugBuildingBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTempChanged, float, NewValue);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHumidityChanged, float, NewValue);

/**
 * Main Class for Slug Buildings (mainly use to add a second inventory)
 */
UCLASS()
class KLIB_API AKLBuildableSlugBuildingBase : public AKPCLModularBuildingBase
{
	GENERATED_BODY()

public:
	AKLBuildableSlugBuildingBase();

	virtual void BeginPlay() override;

	// Overwrite to bind the output belts to Output Inventory
	virtual void SetBelts() override;

	// Will called if item added or removed
	FORCEINLINE virtual void OnSlotChecking(TSubclassOf<UFGItemDescriptor> Item, bool WasRemoved)
	{
	}

	FORCEINLINE virtual void OnSlotOutputChecking(TSubclassOf<UFGItemDescriptor> Item, bool WasRemoved)
	{
	}

	virtual void OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
	                              UFGInventoryComponent* sourceInventory) override;
	virtual void OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
	                                UFGInventoryComponent* sourceInventory) override;

	virtual void OnOutputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
	                               UFGInventoryComponent* sourceInventory) override;
	virtual void OnOutputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
	                                 UFGInventoryComponent* sourceInventory) override;

	// Native Helper
	FInventoryStack GetStackFromIndex(int32 Index, bool OutPutInventory = false) const;

	UPROPERTY(Transient)
	AKLSlugSubsystem* mSlugSubsystem;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	UFGInventoryComponent* mInputInventory;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	UFGInventoryComponent* mOutputInventory;
};
