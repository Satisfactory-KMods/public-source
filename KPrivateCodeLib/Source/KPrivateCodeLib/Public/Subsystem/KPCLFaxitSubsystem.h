// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildableRadarTower.h"
#include "FGInventoryComponent.h"
#include "ItemAmount.h"
#include "Resources/FGItemDescriptor.h"

#include "KPCLModSubsystem.h"

#include "KPCLFaxitSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKPCLOnFaxitUnlocksUpdated);

UENUM(BlueprintType)
enum class EKPCLNetworkLevelType : uint8
{
	RemoveAccess UMETA(DisplayName = "RemoveAccess"),
	Sink UMETA(DisplayName = "Sink"),
	Depot UMETA(DisplayName = "Depot"),
	SolidSpeed UMETA(DisplayName = "Solid Speed"),
	FluidSpeed UMETA(DisplayName = "Fluid Speed"),
	MachineNetwork UMETA(DisplayName = "Machine Network"),
	Network UMETA(DisplayName = "Network"),
	Drive UMETA(DisplayName = "Drive"),
};

USTRUCT(BlueprintType)
struct FKPCLFaxitNetworkStatData
{
	GENERATED_BODY()

	FKPCLFaxitNetworkStatData() : mItem(nullptr) {}

	FKPCLFaxitNetworkStatData(TSubclassOf<UFGItemDescriptor> Item) { mItem = Item; }

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TSubclassOf<UFGItemDescriptor> mItem;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	int32 mUpload = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	int32 mDownload = 0;

	bool operator==(const FKPCLFaxitNetworkStatData& Other) const { return mItem == Other.mItem; }

	void Merge(FKPCLFaxitNetworkStatData& Other, bool ResetOther = false);
	void Merge(FKPCLFaxitNetworkStatData* Other, bool ResetOther = false);
};

USTRUCT(BlueprintType)
struct FKPCLFaxitNetworkStatDataBundle
{
	GENERATED_BODY()

	FKPCLFaxitNetworkStatDataBundle() {}

	UPROPERTY(SaveGame, BlueprintReadOnly)
	FString mId = FGuid::NewGuid().ToString();

	UPROPERTY(SaveGame, BlueprintReadOnly)
	int64 mTimestamp = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<FKPCLFaxitNetworkStatData> mStats;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	int32 mTotalUpload = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	int32 mTotalDownload = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	double mTotalUploadM3 = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	double mTotalDownloadM3 = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	int32 mTotalItems = 0;

	void CleanUpNullItems();

	bool operator==(const FKPCLFaxitNetworkStatDataBundle& Other) const
	{
		return mId == Other.mId || mTimestamp == Other.mTimestamp;
	}

	bool operator!=(const FKPCLFaxitNetworkStatDataBundle& Other) const
	{
		return mId != Other.mId && mTimestamp != Other.mTimestamp;
	}

	bool operator>(const FKPCLFaxitNetworkStatDataBundle& Other) const { return mTimestamp > Other.mTimestamp; }

	bool operator<(const FKPCLFaxitNetworkStatDataBundle& Other) const { return mTimestamp < Other.mTimestamp; }

	bool operator>=(const FKPCLFaxitNetworkStatDataBundle& Other) const { return mTimestamp >= Other.mTimestamp; }

	bool operator<=(const FKPCLFaxitNetworkStatDataBundle& Other) const { return mTimestamp <= Other.mTimestamp; }

	UPROPERTY(BlueprintReadOnly, NotReplicated)
	TMap<TSubclassOf<UFGItemDescriptor>, FKPCLFaxitNetworkStatData> mStatsMapped;
};

USTRUCT(BlueprintType)
struct FKPCLFaxitNetwork
{
	GENERATED_BODY()

	FKPCLFaxitNetwork() : mCore(nullptr) { this->mNetworkId = FGuid::NewGuid().ToString(); }

	FKPCLFaxitNetwork(FString networkName, class AKPCLNetworkCore* Core)
	{
		this->mNetworkId = FGuid::NewGuid().ToString();
		this->mNetworkName = networkName;
		this->mCore = Core;
		this->mIsValid = Core != nullptr;
	}

	void SetCore(class AKPCLNetworkCore* Core);

	void UpdateName(FString networkName);

	void RemoveActorFromNetwork(class AKPCLNetworkBuildingBase* actor);

	void AddActorToNetwork(class AKPCLNetworkBuildingBase* actor);

	bool IsActorInNetwork(class AKPCLNetworkBuildingBase* actor) const;

	float GetNearstDistanceToAccessPoint(FVector Location, bool IsTower) const;

	void Merge(FKPCLFaxitNetwork& OtherNetwork);

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FString mNetworkId;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FString mNetworkName;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TObjectPtr<class AKPCLNetworkCore> mCore;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<TObjectPtr<class AKPCLNetworkBuildingBase>> mNetworkBuildings;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<TObjectPtr<class AKPCLNetworkTower>> mNetworkTowers;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	bool mIsValid = false;

	bool operator==(const FKPCLFaxitNetwork& Other) const { return mCore == Other.mCore; }

	bool operator!=(const FKPCLFaxitNetwork& Other) const { return mCore != Other.mCore; }
};

USTRUCT(BlueprintType)
struct FKPCLFaxitNetworkInfo
{
	GENERATED_BODY()

	FKPCLFaxitNetworkInfo() {}

	UPROPERTY(BlueprintReadOnly)
	FKPCLFaxitNetwork mRelatedNetwork;

	UPROPERTY(BlueprintReadOnly)
	bool bHasReachedLimit;

	UPROPERTY(BlueprintReadOnly)
	int32 mCurrentBuildingCount;

	UPROPERTY(BlueprintReadOnly)
	int32 mBuildingLimit;

	UPROPERTY(BlueprintReadOnly)
	int32 mStackCount;

	UPROPERTY(BlueprintReadOnly)
	int32 mBuildingPower;
};

USTRUCT()
struct KPRIVATECODELIB_API FKPCLMappedItemAmount
{
	GENERATED_BODY()

	FKPCLMappedItemAmount() = default;

	FKPCLMappedItemAmount(const FKPCLMappedItemAmount& Other);
	FKPCLMappedItemAmount& operator=(const FKPCLMappedItemAmount& Other);
	FKPCLMappedItemAmount(FKPCLMappedItemAmount&& Other) noexcept;
	FKPCLMappedItemAmount& operator=(FKPCLMappedItemAmount&& Other) noexcept;

	int32 AddItemAmount(TSubclassOf<UFGItemDescriptor> InItem, int32 InAmount);
	int32 AddItemAmount(const FItemAmount& ItemAmount);

	int32 RemoveItemAmount(TSubclassOf<UFGItemDescriptor> InItem, int32 InAmount);
	int32 RemoveItemAmount(const FItemAmount& ItemAmount);

	void GetDismantleAmounts(TArray<FInventoryStack>& out_returns) const;
	FItemAmount GetAmountOrCreateAmount(TSubclassOf<UFGItemDescriptor> InItem);
	FItemAmount GetAmountConst(TSubclassOf<UFGItemDescriptor> InItem, bool& HasItem) const;

	TArray<FItemAmount> ToItemAmountArray() const;
	int32 GetSlotSize() const;
	bool HasItem(TSubclassOf<UFGItemDescriptor> InItem) const;

	void CleanUpEmptySlots(TSet<TSubclassOf<UFGItemDescriptor>> BlacklistedItems);
	void RemoveItemFromStorage(TSubclassOf<UFGItemDescriptor> InItem);

	void TickReplication();
	void Client_BuildMap();

	int32 GetSpaceFor(FItemAmount Amount) const;
	int32 GetSpaceFor(TSubclassOf<UFGItemDescriptor> InItem, int32 Amount) const;

	bool CanAddNewItemSlot(TSubclassOf<UFGItemDescriptor> InItem) const;

	UPROPERTY(SaveGame)
	int32 mMaxUniqueItems = -1;

	UPROPERTY(SaveGame)
	int32 mStackMultiplier = 0;

private:
	bool IsItemAllowed(TSubclassOf<UFGItemDescriptor> InItem) const;

	bool HasItem_NoLock(TSubclassOf<UFGItemDescriptor> InItem) const;
	int32 GetSpaceFor_NoLock(TSubclassOf<UFGItemDescriptor> InItem, int32 Amount) const;
	bool CanAddNewItemSlot_NoLock(TSubclassOf<UFGItemDescriptor> InItem) const;
	void RemoveItemFromStorage_NoLock(TSubclassOf<UFGItemDescriptor> InItem);

	UPROPERTY()
	TArray<FItemAmount> mReplicatedItemAmounts;

	UPROPERTY(SaveGame, NotReplicated)
	TMap<TSubclassOf<UFGItemDescriptor>, int32> mItemAmounts;

	mutable FRWLock mItemAmountsLock;
};

UCLASS()
class KPRIVATECODELIB_API AKPCLFaxitSubsystem : public AKPCLModSubsystem
{
	GENERATED_BODY()

public:

	static void ResolveBundleMap(FKPCLFaxitNetworkStatDataBundle& InBundle);

	static void ResolveBundleMapArray(TArray<FKPCLFaxitNetworkStatDataBundle>& InBundle);

	AKPCLFaxitSubsystem();
	FKPCLFaxitNetwork* GetNetworkRef(const AKPCLNetworkBuildingBase* Actor);
	FKPCLFaxitNetwork* GetNetworkRef(const FString& NetworkId);

	void RegisterNetworkTower(class AKPCLNetworkTower* AkpclNetworkTower);
	void UnRegisterNetworkTower(class AKPCLNetworkTower* AkpclNetworkTower);

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	bool HasToManyNetworks() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	int32 NetworksLeft() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	bool IsTowerRegistered(class AFGBuildableRadarTower* Tower) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Faxit")
	AKPCLNetworkTower* GetTowerByRadarTower(const class AFGBuildableRadarTower* Tower);

	UFUNCTION(BlueprintPure, Category = "Subsystem", DisplayName = "GetKPCLFaxitSubsystem",
			  meta = (DefaultToSelf = "WorldContext"))
	static AKPCLFaxitSubsystem* Get(UObject* WorldContext);

	FKPCLFaxitNetwork CreateOrAddNetworkCoreNative(AKPCLNetworkCore* Core);
	FKPCLFaxitNetwork CreateOrAddNetworkNative(FString& NetworkId, AKPCLNetworkCore* Core);

	UFUNCTION(BlueprintCallable, Category = "Faxit")
	FKPCLFaxitNetworkInfo GetNetworkByIdWithInfo(FString NetworkId, bool& bSuccess);

	UFUNCTION(BlueprintCallable, Category = "Faxit")
	FKPCLFaxitNetwork GetByNetworkId(FString NetworkId, bool& bSuccess);

	void RebuildMap();

	void DestroyNetworkBuilding(AKPCLNetworkBuildingBase* Building);

	UFUNCTION(BlueprintCallable, Category = "Faxit")
	bool CanAddToNetwork(FString NetworkId);

	UFUNCTION(BlueprintCallable, Category = "Faxit")
	bool AddBuildingToCore(AKPCLNetworkBuildingBase* Building, AKPCLNetworkCore* Core);

	UFUNCTION(BlueprintCallable, Category = "Faxit")
	bool HasNetwork(AKPCLNetworkBuildingBase* Actor);

	UFUNCTION(BlueprintCallable, Category = "Faxit")
	bool GetNetwork(const AKPCLNetworkBuildingBase* Actor, FKPCLFaxitNetwork& OutNetwork);
	bool GetNetworkByCore(const class AKPCLNetworkCore* Core, FKPCLFaxitNetwork& OutNetwork);

	UFUNCTION(BlueprintCallable, Category = "Faxit")
	void UpdateNetworkName(AKPCLNetworkCore* Core, FString NewName);

	UFUNCTION(BlueprintPure, Category = "Faxit")
	int32 GetItemsPerMinute() const;

	UFUNCTION(BlueprintPure, Category = "Faxit")
	int32 GetFluidPerMinute() const;

	UFUNCTION(BlueprintPure, Category = "Faxit")
	int32 GetNetworkLimit() const;

	UFUNCTION(BlueprintPure, Category = "Faxit")
	int32 GetNetworkCount() const;

	UFUNCTION(BlueprintPure, Category = "Faxit")
	int32 GetNetworkDriveLimit() const;

	UFUNCTION(BlueprintPure, Category = "Faxit")
	int32 GetNetworkBuildingBufferInventorySize() const;

	UFUNCTION(BlueprintPure, Category = "Faxit")
	TArray<FKPCLFaxitNetwork> GetNetworks() const;

	UFUNCTION(BlueprintPure, Category = "Faxit")
	bool HasReceivedInitialNetworkData() const;

	void UnlockNetworkFeature();

	void RequestUnlockNetworkFeature();

	void GetNearstNetwork(AActor* Actor, FKPCLFaxitNetwork& Network);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faxit|Unlocks")
	void SetNetworkLimit(int32 NewLimit);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faxit|Unlocks")
	void SetMachineNetworkLimit(int32 NewLimit);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faxit|Unlocks")
	void SetDriveLimit(int32 NewLimit);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faxit|Unlocks")
	void SetItemsPerMinute(int32 NewItemsPerMinute);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faxit|Unlocks")
	void SetFluidPerMinute(int32 NewFluidPerMinute);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faxit|Unlocks")
	void SetNetworkBuildingBufferInventorySize(int32 NewBufferInventorySize);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faxit|Unlocks")
	void SetSinkUnlocked(bool bUnlocked);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faxit|Unlocks")
	void SetDepotUnlocked(bool bUnlocked);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faxit|Unlocks")
	void SetRemoteAccessUnlocked(bool bUnlocked);

	UPROPERTY(BlueprintAssignable, Category = "Faxit|Unlocks|Events")
	FKPCLOnFaxitUnlocksUpdated mOnFaxitUnlocksUpdated;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	bool mSinkUnlocked = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	bool mDepotUnlocked = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	bool mRemoteAccessUnlocked = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	int32 mItemsPerMinute = 15;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	int32 mFluidPerMinute = 30000;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	int32 mNetworkLimit = 1;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	int32 mMachineNetworkLimit = 1;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	int32 mDriveLimit = 1;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	int32 mNetworkBuildingBufferInventorySize = 0;

	UPROPERTY(ReplicatedUsing = OnRep_FaxitUnlocks)
	uint8 mFaxitUnlocksVersion = 0;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Faxit|Unlocks", meta = (ClampMin = "1"))
	int32 mBaseItemsPerMinute = 15;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Faxit|Unlocks", meta = (ClampMin = "1"))
	int32 mBaseFluidPerMinute = 30000;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Faxit|Unlocks", meta = (ClampMin = "1"))
	int32 mBaseNetworkLimit = 1;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Faxit|Unlocks", meta = (ClampMin = "1"))
	int32 mBaseMachineNetworkLimit = 1;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Faxit|Unlocks", meta = (ClampMin = "1"))
	int32 mBaseDriveLimit = 1;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Faxit|Unlocks", meta = (ClampMin = "0"))
	int32 mBaseNetworkBuildingBufferInventorySize = 0;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;

	void CleanupNetworks();

	UFUNCTION()
	void OnRep_FaxitUnlocks();

	UFUNCTION()
	void OnRep_Networks();

	UFUNCTION()
	void OnRep_NetworkReplicationReady();

private:
	void DestoryNetwork(AKPCLNetworkCore* Core);

	void MarkFaxitUnlocksChanged();

	bool bIsRecomputingFaxitUnlocks = false;
	bool bFaxitUnlocksChangedWhileRecomputing = false;
	bool bFaxitRecomputeQueued = false;

	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_Networks)
	TArray<FKPCLFaxitNetwork> mNetworks;

	UPROPERTY(ReplicatedUsing = OnRep_NetworkReplicationReady)
	uint8 mNetworkReplicationReady = 0;

	TMap<FString, int32> mNetworkMap;
	bool bHasReceivedInitialNetworkData = false;

	UPROPERTY()
	TMap<TObjectPtr<AFGBuildableRadarTower>, TObjectPtr<AKPCLNetworkTower>> mRegisteredTowers;
};
