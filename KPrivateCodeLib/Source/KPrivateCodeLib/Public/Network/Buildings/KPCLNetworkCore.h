// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Resources/FGItemDescriptor.h"

#include "Network/KPCLNetworkBuildingBase.h"

#include "KPCLNetworkCore.generated.h"

USTRUCT(BlueprintType)
struct FKPCLNetworkMaxData
{
	GENERATED_BODY()

	FKPCLNetworkMaxData()
	{
		mItemClass = nullptr;
		mMaxItemCount = -1;
	}

	FKPCLNetworkMaxData(TSubclassOf<UFGItemDescriptor> Class, int32 Count)
	{
		mItemClass = Class;
		mMaxItemCount = Count;
	}

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	TSubclassOf<UFGItemDescriptor> mItemClass;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	int32 mMaxItemCount;

	friend bool operator==(const FKPCLNetworkMaxData& A, const FKPCLNetworkMaxData& B)
	{
		return A.mItemClass == B.mItemClass;
	}

	friend bool operator!=(const FKPCLNetworkMaxData& A, const FKPCLNetworkMaxData& B)
	{
		return A.mItemClass != B.mItemClass;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCoreItemStateStateChanged);

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkCore : public AKPCLNetworkBuildingBase
{
	GENERATED_BODY()

public:
	AKPCLNetworkCore();

	virtual void UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots) override;
	virtual void OnTiersUpdated();
	virtual FText GetActorRepresentationText() override;
	virtual bool CanUseFactoryClipboard_Implementation() override;

	virtual const AKPCLNetworkCore* GetFaxitCoreConst() const override;
	virtual AKPCLNetworkCore* GetFaxitCore() override;
	virtual AKPCLNetworkCore* GetCore_Implementation() override;
	virtual FString GetNetworkId() const override;
	virtual bool IsCore() const override;
	void SaveStateBundle();

	UFUNCTION(BlueprintCallable)
	static bool CanItemBeStoredInNetwork(TSubclassOf<AKPCLNetworkCore> CoreClass, TSubclassOf<UFGItemDescriptor> Item);

	virtual float GetPowerConsume() const override;
	virtual float GetRealPowerConsume() const override;
	virtual bool Factory_HasPower() const override;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	float GetBuildingPower() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	bool HasStorageForClass(TSubclassOf<UFGItemDescriptor> Item) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	float GetSlavesPower() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	float GetDrivePower() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	int32 GetUniqueItemLimit() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	int32 GetUniqueItemsCount() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	int32 GetStackLimit() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	int32 GetBuildingCount() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	int32 GetBuildingLimit() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	bool HasBuildingLimitReached() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Network")
	float GetDistanceFrom(const AActor* SourceActor) const;

	//~ Begin Item Handle
	int32 GetMaxItemAmount(TSubclassOf<UFGItemDescriptor> Item) const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	TArray<FItemAmount> GetItemAmounts() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	FItemAmount GetAmountForItem(TSubclassOf<UFGItemDescriptor> Item) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	void UpdateItemList(UPARAM(ref) TArray<TSubclassOf<UFGItemDescriptor>>& Items,
						TArray<TSubclassOf<UFGItemDescriptor>>& OutNewItems) const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	bool IsOverloaded() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	bool HasDrives() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	void GetItemAmountsFiltered(bool Fluid, TArray<FItemAmount>& Out) const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	void GrabFromNetwork(AFGCharacterPlayer* Player, FItemAmount Amount);

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	void StorageFromPlayerToNetwork(AFGCharacterPlayer* Player, FItemAmount Amount);

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	bool IsItemBlacklisted(TSubclassOf<UFGItemDescriptor> Item) const;

	/** Returns true if the storage is full and can't store any more. */
	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	bool IsStorageFull(TSubclassOf<UFGItemDescriptor> Item) const;

	/** Returns true if the storage is empty. */
	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	bool IsStorageEmpty(TSubclassOf<UFGItemDescriptor> Item) const;

	/** Returns the amount that was stored in the network. */
	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	int32 TryToStoreItem(UFGInventoryComponent* Inventory, TSubclassOf<UFGItemDescriptor> Item, int32 Amount = 0,
						 int32 InventorySlot = 0);

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	int32 TryToStoreItemAmount(TSubclassOf<UFGItemDescriptor> Item, int32 Amount);

	/** Returns the amount that was grabbed from the network. */
	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	int32 TryToGrabItem(UFGInventoryComponent* Inventory, TSubclassOf<UFGItemDescriptor> Item, int32 Amount = 0,
						int32 InventorySlot = 0);

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	int32 TryToGrabItemAmount(TSubclassOf<UFGItemDescriptor> Item, int32 Amount);
	//~ End Item Handle

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	UFGInventoryComponent* GetPlayerBufferInventory() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Faxit")
	TArray<FKPCLFaxitNetworkStatDataBundle> GetStateBundles();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Faxit")
	int32 mDefaultUniqueItems = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Faxit")
	int32 mDefaultBuildingLimit = 15;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Faxit")
	TSet<TSubclassOf<UFGItemDescriptor>> mBlacklistedItems;

	UPROPERTY(BlueprintAssignable, Category = "KMods|Faxit")
	FOnCoreItemStateStateChanged OnStorageChanged;

	UPROPERTY(BlueprintAssignable, Category = "KMods|Faxit")
	FOnCoreItemStateStateChanged OnStorageItemsChanged;

	bool bStatBundleChanged = true;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated))
	TArray<FKPCLFaxitNetworkStatDataBundle> mStateBundels;

	UPROPERTY(SaveGame, meta = (FGReplicatedUsing = OnRep_Storage))
	FKPCLMappedItemAmount mStorage;

	/** Rebuilds mStorage.mItemAmounts from the replicated array on clients. */
	UFUNCTION()
	void OnRep_Storage();

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated))
	float mNetworkPower = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated))
	int32 mDiskUniqueBuildings = 1;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated))
	int32 mDiskBuildingLimit = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated))
	int32 mDiskStacks = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated))
	float mDrivePower = 0.f;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated))
	float mBuildingPower = 0.f;

protected:
	//~ Begin AActor
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	//~ End AActor

	virtual UFGInventoryComponent* GetInventory() const override;

	void TryConnectNetworks(AFGBuildable* OtherBuildable) const;

	//~ Begin KPCL
	virtual bool HasCoreInNetwork_Implementation() const override;

	void DebugInventory() const;

	/** Overwrite the power handle to translate network to power. */
	void UpdatePowerConsume();
	virtual void HandlePower(float dt) override;
	//~ End KPCL

	//~ Begin AFGFactoryBuilding
	virtual void GetDismantleInventoryReturns(TArray<FInventoryStack>& out_returns) const override;
	virtual void GetDismantleRefund_Implementation(TArray<FInventoryStack>& out_refund,
												   bool noBuildCostEnabled) const override;
	virtual void Factory_TickAuthOnly(float dt) override;
	virtual bool CanProduce_Implementation() const override;
	//~ End AFGFactoryBuilding

	virtual bool FormFilterOutputInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const override;
	virtual bool FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const override;

	virtual void OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
									UFGInventoryComponent* sourceInventory) override;
	virtual void OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
								  UFGInventoryComponent* sourceInventory) override;

	void UpdateStorageState();

	void FlushOverflow();
	void NotifyStorageChange(bool ItemsChanged = false);

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	TObjectPtr<UFGInventoryComponent> mInventory;

private:
	friend class UKPCLNetwork;
	friend class AKPCLUnlockSubsystem;

	/** Setters for FGReplicated properties — each centralises the MarkPropertyDirty call. */
	void SetNetworkPower(float NewPower);
	void SetBuildingPower(float NewPower);
	void SetDiskStacks(int32 NewStacks);
	void SetDrivePower(float NewPower);
	void SetDiskBuildingLimit(int32 NewLimit);
	/** Fixed: drive-derived unique-item count must be stored in the replicated mDiskUniqueBuildings,
	 *  not overwritten into the non-replicated mDefaultUniqueItems base constant. */
	void SetDiskUniqueBuildings(int32 NewCount);

	FCriticalSection mMutex;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<class AKPCLNetworkConnectionBuilding>> mNetworkConnections;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Faxit")
	FSmartTimer mItemFlushTimer = FSmartTimer(30.f, false);

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Faxit")
	FSmartTimer mNetworkStatsArchiver = FSmartTimer(60.f, false);
};
