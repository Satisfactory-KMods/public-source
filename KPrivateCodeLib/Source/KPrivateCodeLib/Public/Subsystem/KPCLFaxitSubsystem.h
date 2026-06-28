// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildableRadarTower.h"
#include "FGInventoryComponent.h"
#include "ItemAmount.h"
#include "Resources/FGItemDescriptor.h"

#include "KPCLModSubsystem.h"

#include "KPCLFaxitSubsystem.generated.h"

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

	/**
	 * Must be resolved after loading
	 * by calling AKPCLFaxitSubsystem::ResolveBundleMap or AKPCLFaxitSubsystem::ResolveBundleMapArray
	 */
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

/**
 * Maps item descriptors to amounts.
 * Thread-safe: all public methods are protected by an FRWLock.
 */
USTRUCT()
struct KPRIVATECODELIB_API FKPCLMappedItemAmount
{
	GENERATED_BODY()

	FKPCLMappedItemAmount() = default;

	/** Copy/move constructors that don't copy the non-copyable FRWLock. */
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

	/** @return The amount that can be stored. */
	int32 GetSpaceFor(FItemAmount Amount) const;
	int32 GetSpaceFor(TSubclassOf<UFGItemDescriptor> InItem, int32 Amount) const;

	bool CanAddNewItemSlot(TSubclassOf<UFGItemDescriptor> InItem) const;

	UPROPERTY(SaveGame)
	int32 mMaxUniqueItems = -1;

	UPROPERTY(SaveGame)
	int32 mStackMultiplier = 0;

private:
	bool IsItemAllowed(TSubclassOf<UFGItemDescriptor> InItem) const;

	/** No-lock versions for internal use — must only be called while holding mItemAmountsLock. */
	bool HasItem_NoLock(TSubclassOf<UFGItemDescriptor> InItem) const;
	int32 GetSpaceFor_NoLock(TSubclassOf<UFGItemDescriptor> InItem, int32 Amount) const;
	bool CanAddNewItemSlot_NoLock(TSubclassOf<UFGItemDescriptor> InItem) const;
	void RemoveItemFromStorage_NoLock(TSubclassOf<UFGItemDescriptor> InItem);

	/** Replication vehicle: server populates this from mItemAmounts via TickReplication();
	 *  clients rebuild mItemAmounts from it in OnRep_Storage (on the owning actor). */
	UPROPERTY()
	TArray<FItemAmount> mReplicatedItemAmounts;

	/** Authoritative storage map. Saved to disk. Excluded from net serialization because
	 *  UE's RepLayout does not support TMap — replication goes through mReplicatedItemAmounts. */
	UPROPERTY(SaveGame, NotReplicated)
	TMap<TSubclassOf<UFGItemDescriptor>, int32> mItemAmounts;

	/** Guards all accesses to mItemAmounts. mutable so const methods can acquire read locks. */
	mutable FRWLock mItemAmountsLock;
};

UCLASS()
class KPRIVATECODELIB_API AKPCLFaxitSubsystem : public AKPCLModSubsystem
{
	GENERATED_BODY()

public:
	/** Resolve map and create a copy of the bundle. */
	static void ResolveBundleMap(FKPCLFaxitNetworkStatDataBundle& InBundle);

	/** Resolve map and create a copy of the bundles. */
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
	TArray<FKPCLFaxitNetwork> GetNetworks() const;

	void UnlockNetworkFeature();
	void GetNearstNetwork(AActor* Actor, FKPCLFaxitNetwork& Network);

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	bool mSinkUnlocked = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	bool mDepotUnlocked = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	bool mRemoteAccessUnlocked = false;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	int32 mNetworkSolidSpeedLevel = 1;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	int32 mNetworkFluidSpeedLevel = 1;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	int32 mNetworkLevel = 1;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Faxit")
	int32 mDriveLevel = 1;

	/** Base value for max building count per network (x NetworkMachineLevel). */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Faxit")
	TObjectPtr<UCurveFloat> mNetworkCountsPerTier = nullptr;

	/** Base value for max building count per network (x NetworkSolidSpeedLevel). */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Faxit")
	TObjectPtr<UCurveFloat> mBeltSpeedPerTier = nullptr;

	/** Base value for max building count per network (x NetworkFluidSpeedLevel). */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Faxit")
	TObjectPtr<UCurveFloat> mPipeSpeedPerTier = nullptr;

	/** Base value for drive limit per network (x NetworkFluidSpeedLevel). */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Faxit")
	TObjectPtr<UCurveFloat> mDriveLimitPerTier = nullptr;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;

	void CleanupNetworks();

private:
	void DestoryNetwork(AKPCLNetworkCore* Core);

	UPROPERTY(SaveGame, Replicated)
	TArray<FKPCLFaxitNetwork> mNetworks;

	TMap<FString, int32> mNetworkMap;

	UPROPERTY()
	TMap<TObjectPtr<AFGBuildableRadarTower>, TObjectPtr<AKPCLNetworkTower>> mRegisteredTowers;
};
