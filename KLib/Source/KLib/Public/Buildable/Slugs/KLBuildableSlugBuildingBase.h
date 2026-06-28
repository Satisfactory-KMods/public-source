// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildable/Modular/KPCLModularBuildingBase.h"
#include "Replication/KLDefaultRCO.h"
#include "Subsystem/KLSlugSubsystem.h"

#include "KLBuildableSlugBuildingBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHumidityChanged, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTempChanged, float, NewValue);

UCLASS()
class KLIB_API AKLBuildableSlugBuildingBase : public AKPCLModularBuildingBase
{
	GENERATED_BODY()

public:
	AKLBuildableSlugBuildingBase();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

	//~ Begin AKPCLModularBuildingBase Interface
	virtual void OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
								  UFGInventoryComponent* sourceInventory) override;
	virtual void OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
									UFGInventoryComponent* sourceInventory) override;
	virtual void OnOutputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
								   UFGInventoryComponent* sourceInventory) override;
	virtual void OnOutputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
									 UFGInventoryComponent* sourceInventory) override;
	virtual void SetBelts() override;
	//~ End AKPCLModularBuildingBase Interface

	FInventoryStack GetStackFromIndex(int32 Index, bool OutPutInventory = false) const;
	FORCEINLINE virtual void OnSlotChecking(TSubclassOf<UFGItemDescriptor> Item, bool WasRemoved) {}
	FORCEINLINE virtual void OnSlotOutputChecking(TSubclassOf<UFGItemDescriptor> Item, bool WasRemoved) {}

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	TObjectPtr<UFGInventoryComponent> mInputInventory;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	TObjectPtr<UFGInventoryComponent> mOutputInventory;

	UPROPERTY(Transient)
	TObjectPtr<AKLSlugSubsystem> mSlugSubsystem;
};
