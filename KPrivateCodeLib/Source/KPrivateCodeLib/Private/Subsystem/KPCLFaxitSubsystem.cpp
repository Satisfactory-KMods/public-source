// Fill out your copyright notice in the Description page of Project Settings.
#include "Subsystem/KPCLFaxitSubsystem.h"

#include "Async/ParallelFor.h"
#include "BFL/KBFL_Util.h"
#include "EntitySystem/MovieSceneEntitySystem.h"
#include "FGInventoryLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Network/Buildings/KPCLNetworkTower.h"
#include "Replication/KPCLDefaultRCO.h"
#include "Resources/FGNoneDescriptor.h"
#include "Schematic/KPCLSchematic.h"

// ─── Copy / Move (FRWLock is non-copyable, so we handle it manually) ──────────

FKPCLMappedItemAmount::FKPCLMappedItemAmount(const FKPCLMappedItemAmount& Other)
{
	FRWScopeLock ReadLock(Other.mItemAmountsLock, SLT_ReadOnly);
	mItemAmounts = Other.mItemAmounts;
	mReplicatedItemAmounts = Other.mReplicatedItemAmounts;
	mMaxUniqueItems = Other.mMaxUniqueItems;
	mStackMultiplier = Other.mStackMultiplier;
}

FKPCLMappedItemAmount& FKPCLMappedItemAmount::operator=(const FKPCLMappedItemAmount& Other)
{
	if (this != &Other)
	{
		FRWScopeLock WriteLock(mItemAmountsLock, SLT_Write);
		FRWScopeLock ReadLock(Other.mItemAmountsLock, SLT_ReadOnly);
		mItemAmounts = Other.mItemAmounts;
		mReplicatedItemAmounts = Other.mReplicatedItemAmounts;
		mMaxUniqueItems = Other.mMaxUniqueItems;
		mStackMultiplier = Other.mStackMultiplier;
	}
	return *this;
}

FKPCLMappedItemAmount::FKPCLMappedItemAmount(FKPCLMappedItemAmount&& Other) noexcept
{
	FRWScopeLock WriteLock(Other.mItemAmountsLock, SLT_Write);
	mItemAmounts = MoveTemp(Other.mItemAmounts);
	mReplicatedItemAmounts = MoveTemp(Other.mReplicatedItemAmounts);
	mMaxUniqueItems = Other.mMaxUniqueItems;
	mStackMultiplier = Other.mStackMultiplier;
}

FKPCLMappedItemAmount& FKPCLMappedItemAmount::operator=(FKPCLMappedItemAmount&& Other) noexcept
{
	if (this != &Other)
	{
		FRWScopeLock WriteLock(mItemAmountsLock, SLT_Write);
		FRWScopeLock OtherWriteLock(Other.mItemAmountsLock, SLT_Write);
		mItemAmounts = MoveTemp(Other.mItemAmounts);
		mReplicatedItemAmounts = MoveTemp(Other.mReplicatedItemAmounts);
		mMaxUniqueItems = Other.mMaxUniqueItems;
		mStackMultiplier = Other.mStackMultiplier;
	}
	return *this;
}

// ─── Private no-lock helpers (caller must hold the appropriate lock) ───────────

bool FKPCLMappedItemAmount::HasItem_NoLock(TSubclassOf<UFGItemDescriptor> InItem) const
{
	if (!IsValid(InItem))
	{
		return false;
	}
	return mItemAmounts.Contains(InItem);
}

int32 FKPCLMappedItemAmount::GetSpaceFor_NoLock(TSubclassOf<UFGItemDescriptor> InItem, int32 Amount) const
{
	if (!IsValid(InItem) || Amount <= 0)
	{
		return 0;
	}

	int32 ItemStackSize = UFGItemDescriptor::GetStackSize(InItem);
	int32 TotalStackSize = ItemStackSize * mStackMultiplier;
	if (HasItem_NoLock(InItem))
	{
		int32 CurrentAmount = mItemAmounts[InItem];
		int32 AvailableSpace = TotalStackSize - CurrentAmount;
		return FMath::Min(Amount, AvailableSpace);
	}
	if (CanAddNewItemSlot_NoLock(InItem))
	{
		return FMath::Min(Amount, TotalStackSize);
	}
	return 0;
}

bool FKPCLMappedItemAmount::CanAddNewItemSlot_NoLock(TSubclassOf<UFGItemDescriptor> InItem) const
{
	if (!IsValid(InItem))
	{
		return false;
	}
	if (HasItem_NoLock(InItem))
	{
		return true;
	}
	return mItemAmounts.Num() < mMaxUniqueItems || mMaxUniqueItems < 0;
}

void FKPCLMappedItemAmount::RemoveItemFromStorage_NoLock(TSubclassOf<UFGItemDescriptor> InItem)
{
	mItemAmounts.Remove(InItem);
}

// ─── Public API (all guarded by mItemAmountsLock) ────────────────────────────

void FKPCLMappedItemAmount::GetDismantleAmounts(TArray<FInventoryStack>& out_returns) const
{
	FRWScopeLock ReadLock(mItemAmountsLock, SLT_ReadOnly);
	for (auto ItemAmount : mItemAmounts)
	{
		FInventoryStack TempStack(ItemAmount.Value, ItemAmount.Key);
		if (!TempStack.HasItems())
		{
			continue;
		}

		const EResourceForm form = UFGItemDescriptor::GetForm(TempStack.Item.GetItemClass());

		if (form == EResourceForm::RF_SOLID)
		{
			UFGInventoryLibrary::MergeInventoryItem(out_returns, TempStack);
		}
	}
}

int32 FKPCLMappedItemAmount::AddItemAmount(TSubclassOf<UFGItemDescriptor> InItem, int32 InAmount)
{
	if (!IsItemAllowed(InItem))
	{
		return 0;
	}

	FRWScopeLock WriteLock(mItemAmountsLock, SLT_Write);
	int32 Amount = GetSpaceFor_NoLock(InItem, InAmount);
	if (mItemAmounts.Contains(InItem))
	{
		mItemAmounts[InItem] += Amount;
	}
	else
	{
		mItemAmounts.Add(InItem, Amount);
	}
	return Amount;
}

int32 FKPCLMappedItemAmount::AddItemAmount(const FItemAmount& ItemAmount)
{
	return AddItemAmount(ItemAmount.ItemClass, ItemAmount.Amount);
}

int32 FKPCLMappedItemAmount::RemoveItemAmount(TSubclassOf<UFGItemDescriptor> InItem, int32 InAmount)
{
	if (!IsItemAllowed(InItem))
	{
		return 0;
	}

	FRWScopeLock WriteLock(mItemAmountsLock, SLT_Write);
	if (mItemAmounts.Contains(InItem))
	{
		int32 AvailableAmount = mItemAmounts[InItem];
		int32 RemovedAmount = FMath::Min(InAmount, AvailableAmount);
		mItemAmounts[InItem] -= RemovedAmount;
		if (mItemAmounts[InItem] <= 0)
		{
			RemoveItemFromStorage_NoLock(InItem);
		}
		return RemovedAmount;
	}
	return 0;
}

int32 FKPCLMappedItemAmount::RemoveItemAmount(const FItemAmount& ItemAmount)
{
	if (!IsItemAllowed(ItemAmount.ItemClass))
	{
		return 0;
	}

	FRWScopeLock WriteLock(mItemAmountsLock, SLT_Write);
	if (mItemAmounts.Contains(ItemAmount.ItemClass))
	{
		int32 AvailableAmount = mItemAmounts[ItemAmount.ItemClass];
		int32 RemovedAmount = FMath::Min(ItemAmount.Amount, AvailableAmount);
		mItemAmounts[ItemAmount.ItemClass] -= RemovedAmount;
		if (mItemAmounts[ItemAmount.ItemClass] <= 0)
		{
			RemoveItemFromStorage_NoLock(ItemAmount.ItemClass);
		}
		return RemovedAmount;
	}
	return 0;
}

FItemAmount FKPCLMappedItemAmount::GetAmountOrCreateAmount(TSubclassOf<UFGItemDescriptor> InItem)
{
	FRWScopeLock WriteLock(mItemAmountsLock, SLT_Write);
	if (!mItemAmounts.Contains(InItem))
	{
		mItemAmounts.Add(InItem, 0);
	}

	return FItemAmount(InItem, mItemAmounts[InItem]);
}

FItemAmount FKPCLMappedItemAmount::GetAmountConst(TSubclassOf<UFGItemDescriptor> InItem, bool& HasItem) const
{
	FRWScopeLock ReadLock(mItemAmountsLock, SLT_ReadOnly);
	HasItem = mItemAmounts.Contains(InItem);
	if (HasItem)
	{
		const int32* ValuePtr = mItemAmounts.Find(InItem);
		if (!ValuePtr)
		{
			return FItemAmount(InItem, 0);
		}
		return FItemAmount(InItem, *ValuePtr);
	}

	return FItemAmount(InItem, 0);
}

TArray<FItemAmount> FKPCLMappedItemAmount::ToItemAmountArray() const
{
	FRWScopeLock ReadLock(mItemAmountsLock, SLT_ReadOnly);
	TArray<FItemAmount> result;
	for (const auto& pair : mItemAmounts)
	{
		result.Add(FItemAmount(pair.Key, pair.Value));
	}
	return result;
}

int32 FKPCLMappedItemAmount::GetSlotSize() const
{
	FRWScopeLock ReadLock(mItemAmountsLock, SLT_ReadOnly);
	return mItemAmounts.Num();
}

bool FKPCLMappedItemAmount::HasItem(TSubclassOf<UFGItemDescriptor> InItem) const
{
	if (!IsValid(InItem))
	{
		return false;
	}
	FRWScopeLock ReadLock(mItemAmountsLock, SLT_ReadOnly);
	return HasItem_NoLock(InItem);
}

void FKPCLMappedItemAmount::CleanUpEmptySlots(TSet<TSubclassOf<UFGItemDescriptor>> BlacklistedItems)
{
	FRWScopeLock WriteLock(mItemAmountsLock, SLT_Write);
	TArray<TSubclassOf<UFGItemDescriptor>> KeysToRemove;
	for (const auto& pair : mItemAmounts)
	{
		bool bIsBlacklisted = false;
		for (TSubclassOf<UFGItemDescriptor> BlacklistedItem : BlacklistedItems)
		{
			if (pair.Value <= 0 || !IsItemAllowed(pair.Key) ||
				(IsValid(BlacklistedItem) && pair.Key->IsChildOf(BlacklistedItem)))
			{
				bIsBlacklisted = true;
				break;
			}
		}

		if (pair.Value <= 0 || !IsValid(pair.Key) || pair.Key->IsChildOf(UFGNoneDescriptor::StaticClass()) ||
			bIsBlacklisted)
		{
			KeysToRemove.Add(pair.Key);
		}
	}

	for (const TSubclassOf<UFGItemDescriptor>& Key : KeysToRemove)
	{
		RemoveItemFromStorage_NoLock(Key);
	}

	if (mMaxUniqueItems > -1)
	{
		while (mItemAmounts.Num() > mMaxUniqueItems)
		{
			TSubclassOf<UFGItemDescriptor> LastKey;
			for (const auto& pair : mItemAmounts)
			{
				LastKey = pair.Key;
			}
			RemoveItemFromStorage_NoLock(LastKey);
		}
	}
}

void FKPCLMappedItemAmount::RemoveItemFromStorage(TSubclassOf<UFGItemDescriptor> InItem)
{
	FRWScopeLock WriteLock(mItemAmountsLock, SLT_Write);
	RemoveItemFromStorage_NoLock(InItem);
}

void FKPCLMappedItemAmount::TickReplication()
{
	FRWScopeLock ReadLock(mItemAmountsLock, SLT_ReadOnly);
	TArray<FItemAmount> ReplicatedArray;
	for (TPair<TSubclassOf<UFGItemDescriptor>, int> Amount : mItemAmounts)
	{
		ReplicatedArray.Add(FItemAmount(Amount.Key, Amount.Value));
	}
	mReplicatedItemAmounts = ReplicatedArray;
}

void FKPCLMappedItemAmount::Client_BuildMap()
{
	FRWScopeLock WriteLock(mItemAmountsLock, SLT_Write);
	mItemAmounts.Empty();
	for (FItemAmount ItemAmount : mReplicatedItemAmounts)
	{
		mItemAmounts.Add(ItemAmount.ItemClass, ItemAmount.Amount);
	}
}

int32 FKPCLMappedItemAmount::GetSpaceFor(FItemAmount Amount) const
{
	return GetSpaceFor(Amount.ItemClass, Amount.Amount);
}

int32 FKPCLMappedItemAmount::GetSpaceFor(TSubclassOf<UFGItemDescriptor> InItem, int32 Amount) const
{
	if (!IsValid(InItem) || Amount <= 0)
	{
		return 0;
	}

	FRWScopeLock ReadLock(mItemAmountsLock, SLT_ReadOnly);
	return GetSpaceFor_NoLock(InItem, Amount);
}

bool FKPCLMappedItemAmount::CanAddNewItemSlot(TSubclassOf<UFGItemDescriptor> InItem) const
{
	if (!IsValid(InItem))
	{
		return false;
	}
	FRWScopeLock ReadLock(mItemAmountsLock, SLT_ReadOnly);
	return CanAddNewItemSlot_NoLock(InItem);
}

bool FKPCLMappedItemAmount::IsItemAllowed(TSubclassOf<UFGItemDescriptor> InItem) const
{
	if (!IsValid(InItem))
	{
		return false;
	}

	if (InItem->IsChildOf(UFGNoneDescriptor::StaticClass()))
	{
		return false;
	}

	return true;
}

void AKPCLFaxitSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLFaxitSubsystem, mNetworks);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mSinkUnlocked);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mDepotUnlocked);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mRemoteAccessUnlocked);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mNetworkSolidSpeedLevel);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mNetworkFluidSpeedLevel);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mNetworkLevel);
	DOREPLIFETIME(AKPCLFaxitSubsystem, mDriveLevel);
}

void AKPCLFaxitSubsystem::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CleanupNetworks();
		UnlockNetworkFeature();
	}

	RebuildMap();
}

void AKPCLFaxitSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	Super::PostLoadGame_Implementation(saveVersion, gameVersion);

	CleanupNetworks();
}

void AKPCLFaxitSubsystem::CleanupNetworks()
{
	TMap<AKPCLNetworkCore*, FString> FoundNetworks;
	TSet<FString> PendingToRemove;

	for (FKPCLFaxitNetwork& Network : mNetworks)
	{
		FString* Found = FoundNetworks.Find(Network.mCore);
		if (Found)
		{
			PendingToRemove.Add(Network.mNetworkId);
			FKPCLFaxitNetwork* NetworkRef = GetNetworkRef(*Found);

			if (NetworkRef)
			{
				Network.mNetworkBuildings.Append(Network.mNetworkTowers);
				for (class AKPCLNetworkBuildingBase* NetworkBuilding : Network.mNetworkBuildings)
				{
					NetworkRef->AddActorToNetwork(NetworkBuilding);
				}
			}

			continue;
		}
		if (!IsValid(Network.mCore))
		{
			PendingToRemove.Add(Network.mNetworkId);
			continue;
		}
		FoundNetworks.Add(Network.mCore, Network.mNetworkId);
	}

	mNetworks.RemoveAll([PendingToRemove](const FKPCLFaxitNetwork& Network)
						{ return PendingToRemove.Contains(Network.mNetworkId); });

	RebuildMap();
}

void AKPCLFaxitSubsystem::ResolveBundleMap(FKPCLFaxitNetworkStatDataBundle& InBundle)
{
	for (FKPCLFaxitNetworkStatData Stat : InBundle.mStats)
	{
		InBundle.mStatsMapped.Add(Stat.mItem, Stat);
	}
	InBundle.CleanUpNullItems();
}

void AKPCLFaxitSubsystem::ResolveBundleMapArray(TArray<FKPCLFaxitNetworkStatDataBundle>& InBundle)
{
	for (FKPCLFaxitNetworkStatDataBundle& Bundle : InBundle)
	{
		ResolveBundleMap(Bundle);
	}
}

AKPCLFaxitSubsystem::AKPCLFaxitSubsystem()
{
	mShouldSave = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_LastDemotable;
	PrimaryActorTick.EndTickGroup = TG_LastDemotable;
}

FKPCLFaxitNetwork* AKPCLFaxitSubsystem::GetNetworkRef(const AKPCLNetworkBuildingBase* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	const FString Id = Actor->GetNetworkId();
	if (Actor->GetNetworkId().IsEmpty())
	{
		return nullptr;
	}

	// Ensure map is built
	if (mNetworkMap.Num() == 0 && mNetworks.Num() > 0)
	{
		RebuildMap();
	}

	int32* IndexPtr = mNetworkMap.Find(Id);
	if (!IndexPtr || !mNetworks.IsValidIndex(*IndexPtr))
	{
		return nullptr;
	}

	return &mNetworks[*IndexPtr];
}

FKPCLFaxitNetwork* AKPCLFaxitSubsystem::GetNetworkRef(const FString& NetworkId)
{
	if (NetworkId.IsEmpty())
	{
		return nullptr;
	}

	// Ensure map is built
	if (mNetworkMap.Num() == 0 && mNetworks.Num() > 0)
	{
		RebuildMap();
	}

	int32* IndexPtr = mNetworkMap.Find(NetworkId);
	if (!IndexPtr || !mNetworks.IsValidIndex(*IndexPtr))
	{
		return nullptr;
	}

	return &mNetworks[*IndexPtr];
}

void AKPCLFaxitSubsystem::RegisterNetworkTower(AKPCLNetworkTower* NetworkTower)
{
	if (NetworkTower->mConnectedRadarTower.IsValid())
	{
		mRegisteredTowers.Add(NetworkTower->mConnectedRadarTower.Get(), NetworkTower);
	}
}

void AKPCLFaxitSubsystem::UnRegisterNetworkTower(class AKPCLNetworkTower* NetworkTower)
{
	if (auto Key = mRegisteredTowers.FindKey(NetworkTower))
	{
		mRegisteredTowers.Remove(*Key);
	}
}

bool AKPCLFaxitSubsystem::HasToManyNetworks() const
{
	const int32 Limit = GetNetworkLimit();
	const int32 Current = GetNetworkCount();

	return Current > Limit;
}

int32 AKPCLFaxitSubsystem::NetworksLeft() const
{
	const int32 Limit = GetNetworkLimit();
	const int32 Current = GetNetworkCount();

	return FMath::Max(Limit - Current, 0);
}

bool AKPCLFaxitSubsystem::IsTowerRegistered(class AFGBuildableRadarTower* Tower) const
{
	if (!IsValid(Tower))
	{
		return false;
	}
	return mRegisteredTowers.Contains(Tower);
}

AKPCLNetworkTower* AKPCLFaxitSubsystem::GetTowerByRadarTower(const AFGBuildableRadarTower* Tower)
{
	if (!IsValid(Tower))
	{
		return nullptr;
	}

	const TObjectPtr<AKPCLNetworkTower>* Found = mRegisteredTowers.Find(Tower);
	if (!Found)
	{
		return nullptr;
	}

	return *Found;
}

AKPCLFaxitSubsystem* AKPCLFaxitSubsystem::Get(UObject* worldContext)
{
	return UKBFL_Util::GetSubsystem<AKPCLFaxitSubsystem>(worldContext);
}

FKPCLFaxitNetwork AKPCLFaxitSubsystem::CreateOrAddNetworkCoreNative(AKPCLNetworkCore* Core)
{
	for (FKPCLFaxitNetwork Network : mNetworks)
	{
		if (Network.mCore == Core)
		{
			Core->SetNetworkIdDirect(Network.mNetworkId);
			return Network;
		}
	}

	FKPCLFaxitNetwork NewNetworkData = FKPCLFaxitNetwork(FGuid::NewGuid().ToString(), Core);
	Core->SetNetworkIdDirect(NewNetworkData.mNetworkId);
	mNetworks.Add(NewNetworkData);

	// Rebuild map BEFORE calling AddBuildingToCore to ensure the map is up to date
	RebuildMap();

	AddBuildingToCore(Core, Core);

	return NewNetworkData;
}

FKPCLFaxitNetwork AKPCLFaxitSubsystem::CreateOrAddNetworkNative(FString& NetworkId, AKPCLNetworkCore* Core)
{
	FKPCLFaxitNetwork OutNetwork;
	if (NetworkId.IsEmpty() || !GetNetwork(Core, OutNetwork))
	{
		FKPCLFaxitNetwork NewNetworkData = FKPCLFaxitNetwork(FGuid::NewGuid().ToString(), Core);
		mNetworks.Add(NewNetworkData);
		NetworkId = NewNetworkData.mNetworkId;
		RebuildMap();

		int32* IndexPtr = mNetworkMap.Find(NewNetworkData.mNetworkId);
		if (IndexPtr && mNetworks.IsValidIndex(*IndexPtr))
		{
			return mNetworks[*IndexPtr];
		}
		return NewNetworkData;
	}

	NetworkId = OutNetwork.mNetworkId;
	return OutNetwork;
}

FKPCLFaxitNetworkInfo AKPCLFaxitSubsystem::GetNetworkByIdWithInfo(FString NetworkId, bool& bSuccess)
{
	FKPCLFaxitNetwork Network = GetByNetworkId(NetworkId, bSuccess);
	if (!bSuccess)
	{
		return FKPCLFaxitNetworkInfo();
	}

	FKPCLFaxitNetworkInfo Info;

	Info.mRelatedNetwork = Network;
	Info.bHasReachedLimit = Network.mCore->HasBuildingLimitReached();
	Info.mCurrentBuildingCount = Network.mCore->GetBuildingCount();
	Info.mBuildingLimit = Network.mCore->GetBuildingLimit();
	Info.mStackCount = Network.mCore->GetStackLimit();
	Info.mBuildingPower = Network.mCore->GetRealPowerConsume();

	return Info;
}

FKPCLFaxitNetwork AKPCLFaxitSubsystem::GetByNetworkId(FString NetworkId, bool& bSuccess)
{
	FKPCLFaxitNetwork* Found =
		mNetworks.FindByPredicate([NetworkId](const FKPCLFaxitNetwork& item) { return item.mNetworkId == NetworkId; });

	if (Found)
	{
		bSuccess = true;
		return *Found;
	}

	bSuccess = false;
	return FKPCLFaxitNetwork();
}

void AKPCLFaxitSubsystem::RebuildMap()
{
	mNetworkMap.Empty();
	for (int32 i = 0; i < mNetworks.Num(); i++)
	{
		mNetworkMap.Add(mNetworks[i].mNetworkId, i);
	}
}

void AKPCLFaxitSubsystem::DestoryNetwork(AKPCLNetworkCore* Core)
{
	FString Id = Core->mNetworkId;
	if (!mNetworkMap.Contains(Id))
	{
		return;
	}

	mNetworks.RemoveAll([Id](const FKPCLFaxitNetwork& item) { return item.mNetworkId == Id; });
	RebuildMap();
}

void AKPCLFaxitSubsystem::DestroyNetworkBuilding(AKPCLNetworkBuildingBase* Building)
{
	if (!IsValid(Building))
	{
		return;
	}

	AKPCLNetworkCore* CoreToDestroy = nullptr;
	for (FKPCLFaxitNetwork& Network : mNetworks)
	{
		if (Network.mCore == Building)
		{
			CoreToDestroy = CastChecked<AKPCLNetworkCore>(Building);
		}

		Network.RemoveActorFromNetwork(Building);
	}

	if (IsValid(CoreToDestroy))
	{
		// If the core is destroyed, destroy the whole network after iteration is complete.
		DestoryNetwork(CoreToDestroy);
	}
}

bool AKPCLFaxitSubsystem::CanAddToNetwork(FString NetworkId)
{
	FKPCLFaxitNetwork* FaxitNetwork = GetNetworkRef(NetworkId);
	if (FaxitNetwork && IsValid(FaxitNetwork->mCore))
	{
		int32 BuildingLimit = FaxitNetwork->mCore->GetBuildingLimit();
		int32 CurrentCount = FaxitNetwork->mNetworkBuildings.Num();
		return CurrentCount < BuildingLimit;
	}

	return false;
}

bool AKPCLFaxitSubsystem::AddBuildingToCore(AKPCLNetworkBuildingBase* Building, AKPCLNetworkCore* Core)
{
	if (!IsValid(Building) || !IsValid(Core) || Core == Building)
	{
		return false;
	}

	if (FKPCLFaxitNetwork* FaxitNetwork = GetNetworkRef(Core))
	{
		// First, remove building from all other networks
		for (FKPCLFaxitNetwork& Network : mNetworks)
		{
			if (Network.mNetworkId != FaxitNetwork->mNetworkId)
			{
				Network.RemoveActorFromNetwork(Building);
			}
		}

		// Add to target network
		FaxitNetwork->AddActorToNetwork(Building);
		Building->SetNetworkCore(FaxitNetwork->mCore, FaxitNetwork);

		return true;
	}

	return false;
}

bool AKPCLFaxitSubsystem::HasNetwork(AKPCLNetworkBuildingBase* Actor)
{
	FKPCLFaxitNetwork OutNetwork;
	return GetNetwork(Actor, OutNetwork);
}

bool AKPCLFaxitSubsystem::GetNetwork(const AKPCLNetworkBuildingBase* Actor, FKPCLFaxitNetwork& OutNetwork)
{
	if (!IsValid(Actor))
	{
		return false;
	}

	FString Id = Actor->GetNetworkId();
	if (Id.IsEmpty())
	{
		return false;
	}

	// Ensure map is built
	if (mNetworkMap.Num() == 0 && mNetworks.Num() > 0)
	{
		RebuildMap();
	}

	int32* IndexPtr = mNetworkMap.Find(Id);
	if (!IndexPtr || !mNetworks.IsValidIndex(*IndexPtr))
	{
		return false;
	}

	OutNetwork = mNetworks[*IndexPtr];
	return true;
}

bool AKPCLFaxitSubsystem::GetNetworkByCore(const AKPCLNetworkCore* Core, FKPCLFaxitNetwork& OutNetwork)
{
	FKPCLFaxitNetwork* Network =
		mNetworks.FindByPredicate([Core](const FKPCLFaxitNetwork& item) { return item.mCore == Core; });
	if (!Network)
	{
		return false;
	}

	OutNetwork = *Network;
	return true;
}

void AKPCLFaxitSubsystem::UpdateNetworkName(AKPCLNetworkCore* Core, FString NewName)
{
	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKPCLDefaultRCO>(GetWorld()))
		{
			RCO->Server_Faxit_UpdateNetworkName(this, Core, NewName);
		}
		return;
	}

	FKPCLFaxitNetwork* network = GetNetworkRef(Core);
	if (network)
	{
		network->mNetworkName = NewName;

		Core->UpdateRepresentation();
		for (AKPCLNetworkBuildingBase* Building : network->mNetworkBuildings)
		{
			Building->UpdateRepresentation();
		}
	}
}

int32 AKPCLFaxitSubsystem::GetItemsPerMinute() const
{
	if (!IsValid(mBeltSpeedPerTier))
	{
		return 0;
	}
	return FMath::Max(mBeltSpeedPerTier->GetFloatValue(mNetworkSolidSpeedLevel), 15);
}

int32 AKPCLFaxitSubsystem::GetFluidPerMinute() const
{
	if (!IsValid(mPipeSpeedPerTier))
	{
		return 0;
	}
	return FMath::Max(mPipeSpeedPerTier->GetFloatValue(mNetworkFluidSpeedLevel), 30000);
}

int32 AKPCLFaxitSubsystem::GetNetworkLimit() const
{
	if (!IsValid(mNetworkCountsPerTier))
	{
		return 0;
	}
	return mNetworkCountsPerTier->GetFloatValue(mNetworkLevel);
}

int32 AKPCLFaxitSubsystem::GetNetworkCount() const { return mNetworks.Num(); }

int32 AKPCLFaxitSubsystem::GetNetworkDriveLimit() const
{
	if (!IsValid(mDriveLimitPerTier))
	{
		return 0;
	}
	return FMath::Max(mDriveLimitPerTier->GetFloatValue(FMath::Max(mDriveLevel, 1)), 1);
}

TArray<FKPCLFaxitNetwork> AKPCLFaxitSubsystem::GetNetworks() const { return mNetworks; }

void AKPCLFaxitSubsystem::UnlockNetworkFeature()
{
	UKPCLSchematic::Faxit_GetLevel(GetWorld(), EKPCLNetworkLevelType::Drive, mDriveLevel);
	UKPCLSchematic::Faxit_GetLevel(GetWorld(), EKPCLNetworkLevelType::Network, mNetworkLevel);
	UKPCLSchematic::Faxit_GetLevel(GetWorld(), EKPCLNetworkLevelType::SolidSpeed, mNetworkSolidSpeedLevel);
	UKPCLSchematic::Faxit_GetLevel(GetWorld(), EKPCLNetworkLevelType::FluidSpeed, mNetworkFluidSpeedLevel);

	UKPCLSchematic::Faxit_IsUnlocked(GetWorld(), EKPCLNetworkLevelType::Sink, mSinkUnlocked);
	UKPCLSchematic::Faxit_IsUnlocked(GetWorld(), EKPCLNetworkLevelType::Depot, mDepotUnlocked);

	for (FKPCLFaxitNetwork& Network : mNetworks)
	{
		if (IsValid(Network.mCore))
		{
			Network.mCore->OnTiersUpdated();
		}
	}
}

void AKPCLFaxitSubsystem::GetNearstNetwork(AActor* Actor, FKPCLFaxitNetwork& Network)
{
	bool IsTower = IsValid(Cast<AKPCLNetworkTower>(Actor));
	float Last = TNumericLimits<float>::Max();
	for (FKPCLFaxitNetwork FaxitNetwork : mNetworks)
	{
		if (IsValid(FaxitNetwork.mCore) && CanAddToNetwork(FaxitNetwork.mNetworkId))
		{
			if (!IsValid(Network.mCore))
			{
				Network = FaxitNetwork;
				Last = FaxitNetwork.GetNearstDistanceToAccessPoint(Actor->GetActorLocation(), IsTower);
				continue;
			}

			float Distance = FaxitNetwork.GetNearstDistanceToAccessPoint(Actor->GetActorLocation(), IsTower);
			if (Distance < Last)
			{
				Last = Distance;
				Network = FaxitNetwork;
			}
		}
	}
}

void FKPCLFaxitNetworkStatData::Merge(FKPCLFaxitNetworkStatData& Other, bool ResetOther)
{
	if (mItem != Other.mItem)
	{
		return;
	}

	mDownload += Other.mDownload;
	mUpload += Other.mUpload;

	if (!ResetOther)
	{
		return;
	}
	Other.mDownload = 0;
	Other.mUpload = 0;
}

void FKPCLFaxitNetworkStatData::Merge(FKPCLFaxitNetworkStatData* Other, bool ResetOther)
{
	if (!Other)
	{
		return;
	}
	Merge(*Other, ResetOther);
}

void FKPCLFaxitNetworkStatDataBundle::CleanUpNullItems()
{
	mStats.RemoveAll([](const FKPCLFaxitNetworkStatData& item)
					 { return !IsValid(item.mItem) || (item.mDownload <= 0 && item.mUpload <= 0); });
}

void FKPCLFaxitNetwork::SetCore(AKPCLNetworkCore* Core)
{
	mCore = Core;
	mIsValid = IsValid(Core);
}

void FKPCLFaxitNetwork::UpdateName(FString NetworkName) { mNetworkName = NetworkName; }

void FKPCLFaxitNetwork::RemoveActorFromNetwork(AKPCLNetworkBuildingBase* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}
	mNetworkBuildings.Remove(Actor);
	if (AKPCLNetworkTower* Tower = Cast<AKPCLNetworkTower>(Actor))
	{
		mNetworkTowers.Remove(Tower);
	}
}

void FKPCLFaxitNetwork::AddActorToNetwork(AKPCLNetworkBuildingBase* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	mNetworkBuildings.AddUnique(Actor);

	if (AKPCLNetworkTower* Tower = Cast<AKPCLNetworkTower>(Actor))
	{
		mNetworkTowers.AddUnique(Tower);
	}
}

bool FKPCLFaxitNetwork::IsActorInNetwork(AKPCLNetworkBuildingBase* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}
	return mNetworkBuildings.Contains(Actor);
}

float FKPCLFaxitNetwork::GetNearstDistanceToAccessPoint(FVector Location, bool IsTower) const
{
	if (!IsValid(mCore))
	{
		return FLT_MAX;
	}

	float NearestDistance = FVector::Distance(Location, mCore->GetActorLocation());
	if (!IsTower)
	{
		for (AKPCLNetworkTower* Tower : mNetworkTowers)
		{
			if (!IsValid(Tower))
			{
				continue;
			}
			float Distance = FVector::Distance(Location, Tower->GetActorLocation());
			NearestDistance = FMath::Min(NearestDistance, Distance);
		}
	}
	return NearestDistance;
}

void FKPCLFaxitNetwork::Merge(FKPCLFaxitNetwork& OtherNetwork)
{
	if (!OtherNetwork.mIsValid)
	{
		return;
	}

	for (AKPCLNetworkBuildingBase* NetworkBuilding : OtherNetwork.mNetworkBuildings)
	{
		if (IsValid(NetworkBuilding) && !IsActorInNetwork(NetworkBuilding))
		{
			mNetworkBuildings.Add(NetworkBuilding);
		}
	}

	OtherNetwork.mNetworkBuildings.Empty();
}
