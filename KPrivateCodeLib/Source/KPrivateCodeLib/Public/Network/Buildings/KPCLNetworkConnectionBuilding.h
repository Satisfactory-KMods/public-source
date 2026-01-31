#pragma once

#include "CoreMinimal.h"
#include "FGCentralStorageSubsystem.h"
#include "FGResourceSinkSubsystem.h"
#include "KPCLNetworkCore.h"
#include "Network/KPCLNetworkBuildingBase.h"
#include "Resources/FGNoneDescriptor.h"
#include "KPCLNetworkConnectionBuilding.generated.h"

UCLASS()
class KPRIVATECODELIB_API UKPCLFaxitNetworkConnectionClipboardSettings : public UKPCLFaxitBasicClipboardSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	bool bIsUpload = true;

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UFGItemDescriptor> mFilterItem;

	UPROPERTY(BlueprintReadWrite)
	float mSpeedOverride = -1.f;
};

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkConnectionBuilding : public AKPCLNetworkBuildingBase
{
	GENERATED_BODY()

public:
	friend class AKPCLNetworkSink;

	virtual void UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots) override;
	AKPCLNetworkConnectionBuilding();

	// START: AActor
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// END: AActor

	// START: FGBuildable
	virtual bool CanProduce_Implementation() const override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	// END: FGBuildable

	virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override;
	virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
	                                          class AFGPlayerController* player) override;

protected:
	virtual void onProducingFinal_Implementation() override;

	virtual void SetBelts() override;
	virtual void CollectBelts() override;
	virtual void CollectAndPushPipes(float dt, bool IsPush) override;
	virtual void Server_DoFlush() override;

	bool CanStoreInDepot() const;
	bool CanSinkItem(TSubclassOf<UFGItemDescriptor> Item) const;

	void UpdateInventoryState();
	int32 UploadToDepot(int32 Amount) const;
	int32 Sink(int32 Amount);
	void UpdateProductionSpeed(bool ShouldReset = false);

	void UpdateInventoryFilter() const;

public:
	virtual float GetPowerConsume() const override;
	virtual float GetRealPowerConsume() const override;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	bool CanPushToDepot() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	bool CanSink() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	bool IsSolidFaxit() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	int32 GetTransferAmount() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	bool CanUploadStorage() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	float GetItemsPerMin(bool Converted = true) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	bool CanDownloadStorage() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	bool StorageIsEmpty() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	void ClearSpeedOverride();

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	void SetSpeedOverride(float NewSpeed);

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	void SetFilterItem(TSubclassOf<UFGItemDescriptor> NewItem);

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	TSubclassOf<UFGItemDescriptor> GetSinkRequiredItem() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	TSubclassOf<UFGItemDescriptor> GetDepotRequiredItem() const;

	UFUNCTION()
	bool IsStoredItemBlocked() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	TSubclassOf<UFGItemDescriptor> GetFilterItem() const;
	TSubclassOf<UFGItemDescriptor> GetStoredItemClass() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	bool HasItemStored(int32 Slot = 0) const;

private:
	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	UFGInventoryComponent* mInventory;

	UPROPERTY(SaveGame, meta = ( FGReplicated ))
	TSubclassOf<UFGItemDescriptor> mFilterItem = TSubclassOf<UFGItemDescriptor>{UFGNoneDescriptor::StaticClass()};

	UPROPERTY(SaveGame, meta = ( FGReplicated ))
	float mSpeedOverride = -1.f;

	UPROPERTY()
	TMap<TSubclassOf<UFGItemDescriptor>, FItemAmount> mCurrentStateCache;

	int32 ITEM_AMOUNT = 5;
	int32 FLUID_AMOUNT = 5000;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Faxit")
	FKPCLNetworkDistanceModifier mDistanceTransferModifier;

	UPROPERTY(BlueprintReadOnly, Category = "KMods|Faxit")
	int32 INV_SLOT = 0;

	UPROPERTY(BlueprintReadOnly, Category = "KMods|Faxit")
	int32 DEPOT_ITEM_SLOT = 1;

	UPROPERTY(BlueprintReadOnly, Category = "KMods|Faxit")
	int32 SINK_ITEM_SLOT = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Faxit")
	bool mIsUpload = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Faxit")
	EResourceForm mItemForm = EResourceForm::RF_SOLID;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Faxit")
	TSubclassOf<UFGItemDescriptor> mSinkRequiredItem;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Faxit")
	TSubclassOf<UFGItemDescriptor> mDepotRequiredItem;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Faxit")
	float mCableBoostAmount = 2.f;

protected:
	UPROPERTY(Transient)
	AFGCentralStorageSubsystem* mCentralStorageSubsystem;
};
