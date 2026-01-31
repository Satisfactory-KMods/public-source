// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Network/Buildings/KPCLNetworkCore.h"

#include "KPrivateCodeLibModule.h"

#include "Buildables/FGBuildableWire.h"
#include "Description/KPCLNetworkDrive.h"
#include "Net/UnrealNetwork.h"
#include "Network/Buildings/KPCLNetworkConnectionBuilding.h"
#include "Network/Buildings/KPCLNetworkTower.h"
#include "Network/KPCLNetwork.h"
#include "Network/KPCLNetworkConnectionComponent.h"
#include "Network/KPCLNetworkInfoComponent.h"
#include "Registry/ModContentRegistry.h"
#include "Replication/KPCLDefaultRCO.h"
#include "Resources/FGItemDescriptor.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

#undef GetForm

class UKPCLNetworkDrive;

AKPCLNetworkCore::AKPCLNetworkCore()
{
	PrimaryActorTick.bCanEverTick = false;
	mDismantleAllChilds = true;

	mNetworkStatsArchiver.mIsActive = true;
	mItemFlushTimer.mIsActive = true;

	mInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::InputName);
}

void AKPCLNetworkCore::UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots)
{
	Super::UI_ApplyRelevantItems_Implementation(OutSlots);

	UKBFLAssetDataSubsystem* AssetSubsystem = UKBFLAssetDataSubsystem::Get(this);
	if (IsValid(AssetSubsystem))
	{
		TArray<TSubclassOf<UFGItemDescriptor>> Items;
		AssetSubsystem->GetItemsOfChilds({UKPCLNetworkDrive::StaticClass()}, Items, true);
		for (TSubclassOf<UFGItemDescriptor> Item : Items)
		{
			if (IsValid(Item))
			{
				OutSlots.AddUnique(Item);
			}
		}
	}
}

void AKPCLNetworkCore::TryConnectNetworks(AFGBuildable* OtherBuildable) const
{
	if (const AKPCLNetworkBuildingBase* Base = Cast<AKPCLNetworkBuildingBase>(OtherBuildable))
	{
		UKPCLNetworkConnectionComponent* MainCon = GetNetworkConnectionComponent();
		UKPCLNetworkConnectionComponent* SlaveCon = Base->GetNetworkConnectionComponent();
		if (ensure(MainCon && SlaveCon))
		{
			if (!MainCon->HasHiddenConnection(SlaveCon))
			{
				MainCon->AddHiddenConnection(SlaveCon);
			}
		}
	}
}

bool AKPCLNetworkCore::HasCoreInNetwork_Implementation() const { return IsValid(Execute_GetNetwork(this)); }

// Called when the game starts or when spawned
void AKPCLNetworkCore::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		FKPCLFaxitNetwork Network = mFaxitSubsystem->CreateOrAddNetworkCoreNative(this);

		UpdateStorageState();
		UpdateRepresentation();

		UKPCLNetwork* Net = Execute_GetNetwork(this);
		if (IsValid(Net))
		{
			for (AKPCLNetworkConnectionBuilding* Building : Net->GetNetworkConnectionBuildings())
			{
				if (!IsValid(Building))
				{
					continue;
				}
				mFaxitSubsystem->AddBuildingToCore(Building, this);
			}
		}

		for (AKPCLNetworkBuildingBase* Building : Network.mNetworkBuildings)
		{
			if (!IsValid(Building))
			{
				continue;
			}
			mFaxitSubsystem->AddBuildingToCore(Building, this);
		}
	}
}

UFGInventoryComponent* AKPCLNetworkCore::GetInventory() const { return mInventory; }

void AKPCLNetworkCore::GetDismantleInventoryReturns(TArray<FInventoryStack>& out_returns) const
{
	mStorage.GetDismantleAmounts(out_returns);

	if (TSubclassOf<UFGRecipe> Recipe = GetBuiltWithRecipe())
	{
		for (FItemAmount ItemAmount : UFGRecipe::GetIngredients(Recipe))
		{
			if (ItemAmount.Amount > 0 && ItemAmount.ItemClass)
			{
				const EResourceForm form = UFGItemDescriptor::GetForm(ItemAmount.ItemClass);

				if (form == EResourceForm::RF_SOLID)
				{
					UFGInventoryLibrary::MergeInventoryItem(out_returns,
															FInventoryStack(ItemAmount.Amount, ItemAmount.ItemClass));
				}
			}
		}
	}

	TArray<FInventoryStack> Stacks;
	GetInventory()->GetInventoryStacks(Stacks, true);

	for (FInventoryStack Stack : Stacks)
	{
		if (!Stack.HasItems())
		{
			continue;
		}
		const EResourceForm form = UFGItemDescriptor::GetForm(Stack.Item.GetItemClass());

		if (form == EResourceForm::RF_SOLID)
		{
			UFGInventoryLibrary::MergeInventoryItem(out_returns, Stack);
		}
	}
}

FText AKPCLNetworkCore::GetActorRepresentationText()
{
	FText SuperName = Super::GetActorRepresentationText();
	return FText::FromString(FString(SuperName.ToString()).Append(" (Network Core)"));
}

bool AKPCLNetworkCore::CanUseFactoryClipboard_Implementation() { return false; }


void AKPCLNetworkCore::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLNetworkCore, mNetworkConnections);
}

void AKPCLNetworkCore::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);

	FG_DOREPCONDITIONAL(ThisClass, mStateBundels);
	FG_DOREPCONDITIONAL(ThisClass, mStorage);
	FG_DOREPCONDITIONAL(ThisClass, mNetworkPower);
	FG_DOREPCONDITIONAL(ThisClass, mDrivePower);
}

void AKPCLNetworkCore::GetDismantleRefund_Implementation(TArray<FInventoryStack>& out_refund,
														 bool noBuildCostEnabled) const
{
	if (noBuildCostEnabled)
	{
		return;
	}

	GetDismantleInventoryReturns(out_refund);
}

void AKPCLNetworkCore::Factory_TickAuthOnly(float dt)
{
	Super::Factory_TickAuthOnly(dt);

	mStorage.mMaxUniqueItems = GetUniqueItemLimit();
	mStorage.mStackMultiplier = GetStackLimit();

	if (mItemFlushTimer.Tick(dt))
	{
		FlushOverflow();
	}

	UKPCLNetwork* Network = GetFaxitCableNetwork();
	if (IsValid(Network) && Network->IsFuseTriggered())
	{
		AsyncTask(ENamedThreads::GameThread, [Network]() { Network->ResetFuse(); });
	}

	if (mNetworkStatsArchiver.Tick(dt))
	{
		AsyncTask(ENamedThreads::GameThread, [&]() { this->SaveStateBundle(); });
	}
}

bool AKPCLNetworkCore::IsCore() const { return true; }

void AKPCLNetworkCore::SaveStateBundle()
{
	FKPCLFaxitNetwork Network;
	if (!mFaxitSubsystem->GetNetworkByCore(this, Network))
	{
		return;
	}

	TMap<TSubclassOf<UFGItemDescriptor>, FKPCLFaxitNetworkStatData> States;
	for (class AKPCLNetworkBuildingBase* Building : Network.mNetworkBuildings)
	{
		for (FKPCLFaxitNetworkStatData Stat : Building->GetAndResetStats())
		{
			if (!IsValid(Stat.mItem))
			{
				continue;
			}
			FKPCLFaxitNetworkStatData& State = States.FindOrAdd(Stat.mItem);
			State.mItem = Stat.mItem;
			State.mDownload += Stat.mDownload;
			State.mUpload += Stat.mUpload;
		}
	}

	FKPCLFaxitNetworkStatDataBundle NewBundel;
	NewBundel.mTimestamp = FDateTime::UtcNow().ToUnixTimestamp();
	for (auto& Pair : States)
	{
		if (!IsValid(Pair.Value.mItem))
		{
			continue;
		}
		NewBundel.mStats.Add(Pair.Value);
		NewBundel.mTotalItems++;

		if (UFGItemDescriptor::GetForm(Pair.Value.mItem) == EResourceForm::RF_SOLID)
		{
			NewBundel.mTotalDownload += Pair.Value.mDownload;
			NewBundel.mTotalUpload += Pair.Value.mUpload;
		}
		else
		{
			NewBundel.mTotalDownloadM3 += static_cast<double>(Pair.Value.mDownload) / 1000.f;
			NewBundel.mTotalUploadM3 += static_cast<double>(Pair.Value.mUpload) / 1000.f;
		}
	}
	mStateBundels.Emplace(NewBundel);
	while (mStateBundels.Num() > 60)
	{
		mStateBundels.Pop(false);
	}
	mStateBundels.Sort([](const FKPCLFaxitNetworkStatDataBundle& a, const FKPCLFaxitNetworkStatDataBundle& b)
					   { return a.mTimestamp > b.mTimestamp; });
}

bool AKPCLNetworkCore::CanItemBeStoredInNetwork(TSubclassOf<AKPCLNetworkCore> CoreClass,
												TSubclassOf<UFGItemDescriptor> Item)
{
	if (!IsValid(Item) || !IsValid(CoreClass))
	{
		return false;
	}

	return !CoreClass->GetDefaultObject<AKPCLNetworkCore>()->IsItemBlacklisted(Item);
}

const AKPCLNetworkCore* AKPCLNetworkCore::GetFaxitCoreConst() const { return this; }

AKPCLNetworkCore* AKPCLNetworkCore::GetFaxitCore() { return this; }

AKPCLNetworkCore* AKPCLNetworkCore::GetCore_Implementation() { return this; }

FString AKPCLNetworkCore::GetNetworkId() const
{
	if (!mNetworkId.IsEmpty())
	{
		return mNetworkId;
	}

	FKPCLFaxitNetwork Network;
	AKPCLFaxitSubsystem::Get(GetWorld())->GetNetworkByCore(GetFaxitCoreConst(), Network);
	return Network.mIsValid ? Network.mNetworkId : mNetworkId;
}

void AKPCLNetworkCore::OnTiersUpdated()
{
	if (!HasAuthority())
	{
		return;
	}

	UpdateStorageState();
}

void AKPCLNetworkCore::DebugInventory() const
{
	UE_LOG(LogFaxit, Warning, TEXT("----- START ---> Faxit Network Core Inventory -----"));
	for (FItemAmount Storage : mStorage.ToItemAmountArray())
	{
		UE_LOG(LogFaxit, Log, TEXT("Item: %s | Amount: %d"),
			   *UFGItemDescriptor::GetItemName(Storage.ItemClass).ToString(), Storage.Amount);
	}
	UE_LOG(LogFaxit, Warning, TEXT("----- END ---> Faxit Network Core Inventory -----"));
}

void AKPCLNetworkCore::UpdatePowerConsume()
{
	bool Success;
	FKPCLFaxitNetwork Network = mFaxitSubsystem->GetByNetworkId(GetNetworkId(), Success);
	if (!Success)
	{
		return;
	}

	float TotalBuildingPowerConsume = 0.f;
	for (AKPCLNetworkBuildingBase* Building : Network.mNetworkBuildings)
	{
		if (IsValid(Building) && Building != this)
		{
			TotalBuildingPowerConsume += Building->GetRealPowerConsume();
		}
	}


	mBuildingPower = TotalBuildingPowerConsume;
	mNetworkPower = mDrivePower + TotalBuildingPowerConsume + mPowerOptions.mNormalPowerConsume;
}

void AKPCLNetworkCore::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	TArray<UFGPowerInfoComponent*> PowerInfoComponents;
	GetComponents<UFGPowerInfoComponent>(PowerInfoComponents);
	for (UFGPowerInfoComponent* PowerInfoComponent : PowerInfoComponents)
	{
		if (UKPCLNetworkInfoComponent* NetworkInfoComponent = Cast<UKPCLNetworkInfoComponent>(PowerInfoComponent))
		{
			mNetworkInfoComponent = NetworkInfoComponent;
		}
		else
		{
			mPowerInfo = PowerInfoComponent;
		}
	}

	TArray<UFGPowerConnectionComponent*> PowerConnectionComponents;
	GetComponents<UFGPowerConnectionComponent>(PowerConnectionComponents);
	for (UFGPowerConnectionComponent* PowerConnectionComponent : PowerConnectionComponents)
	{
		if (UKPCLNetworkConnectionComponent* NetworkConnectionComponent =
				Cast<UKPCLNetworkConnectionComponent>(PowerConnectionComponent))
		{
			mNetworkConnection = NetworkConnectionComponent;
		}
	}
}

bool AKPCLNetworkCore::Factory_HasPower() const
{
	if (IsValid(GetPowerInfoExplicit()))
	{
		return GetPowerInfoExplicit()->HasPower() && !GetNetworkId().IsEmpty();
	}
	return !GetNetworkId().IsEmpty();
}

float AKPCLNetworkCore::GetBuildingPower() const { return mPowerOptions.GetPowerConsume(); }

bool AKPCLNetworkCore::HasStorageForClass(TSubclassOf<UFGItemDescriptor> Item) const
{
	if (!IsValid(Item))
	{
		return false;
	}
	return mStorage.HasItem(Item);
}

float AKPCLNetworkCore::GetSlavesPower() const { return mBuildingPower; }

float AKPCLNetworkCore::GetDrivePower() const { return mDrivePower; }

int32 AKPCLNetworkCore::GetUniqueItemLimit() const { return mDiskUniqueBuildings + mDefaultUniqueItems; }

int32 AKPCLNetworkCore::GetUniqueItemsCount() const { return mStorage.GetSlotSize(); }

int32 AKPCLNetworkCore::GetStackLimit() const { return mDiskStacks; }

int32 AKPCLNetworkCore::GetBuildingCount() const
{
	FKPCLFaxitNetwork Network;
	bool Success = mFaxitSubsystem->GetNetworkByCore(this, Network);
	if (!Success)
	{
		return 0;
	}
	return Network.mNetworkBuildings.Num();
}

int32 AKPCLNetworkCore::GetBuildingLimit() const { return mDiskBuildingLimit + mDefaultBuildingLimit; }

bool AKPCLNetworkCore::HasBuildingLimitReached() const { return GetBuildingCount() >= GetBuildingLimit(); }

float AKPCLNetworkCore::GetDistanceFrom(const AActor* SourceActor) const
{
	const AKPCLNetworkCore* Core = GetFaxitCoreConst();
	if (!IsValid(SourceActor) || !IsValid(mFaxitSubsystem) || !IsValid(Core))
	{
		return FLT_MAX;
	}

	FKPCLFaxitNetwork Network;
	bool Success = mFaxitSubsystem->GetNetworkByCore(Core, Network);
	if (!Success)
	{
		return FLT_MAX;
	}

	bool IsTower = IsValid(Cast<AKPCLNetworkTower>(SourceActor));
	return Network.GetNearstDistanceToAccessPoint(SourceActor->GetActorLocation(), IsTower);
}

bool AKPCLNetworkCore::CanProduce_Implementation() const
{
	if (IsPlayingBuildEffect() || !IsValid(mFaxitSubsystem) || !HasPower() || GetCableNetworkHasToManyCores())
	{
		return false;
	}

	if (GetStackLimit() < 1)
	{
		return false;
	}

	return !IsOverloaded();
}

int32 AKPCLNetworkCore::GetMaxItemAmount(TSubclassOf<UFGItemDescriptor> Item) const
{
	return UFGItemDescriptor::GetStackSize(Item) * GetStackLimit();
}

TArray<FItemAmount> AKPCLNetworkCore::GetItemAmounts() const { return mStorage.ToItemAmountArray(); }

FItemAmount AKPCLNetworkCore::GetAmountForItem(TSubclassOf<UFGItemDescriptor> Item) const
{
	bool Success;
	return mStorage.GetAmountConst(Item, Success);
}

void AKPCLNetworkCore::UpdateItemList(TArray<TSubclassOf<UFGItemDescriptor>>& Items,
									  TArray<TSubclassOf<UFGItemDescriptor>>& OutNewItems) const
{
	for (FItemAmount AmountArray : mStorage.ToItemAmountArray())
	{
		if (!Items.Contains(AmountArray.ItemClass))
		{
			OutNewItems.Add(AmountArray.ItemClass);
			Items.Add(AmountArray.ItemClass);
		}
	}
}

bool AKPCLNetworkCore::IsOverloaded() const
{
	bool Success;
	FKPCLFaxitNetwork Network = mFaxitSubsystem->GetByNetworkId(GetNetworkId(), Success);
	if (!Success)
	{
		return true;
	}

	return Network.mNetworkBuildings.Num() > GetBuildingLimit();
}

void AKPCLNetworkCore::GetItemAmountsFiltered(bool Fluid, TArray<FItemAmount>& Out) const
{
	TArray<EResourceForm> Forms;
	if (Fluid)
	{
		Forms.Add(EResourceForm::RF_LIQUID);
		Forms.Add(EResourceForm::RF_GAS);
	}
	else
	{
		Forms.Add(EResourceForm::RF_SOLID);
		Forms.Add(EResourceForm::RF_HEAT);
		Forms.Add(EResourceForm::RF_INVALID);
		Forms.Add(EResourceForm::RF_LAST_ENUM);
	}

	Out.Empty();
	for (FItemAmount ItemAmount : mStorage.ToItemAmountArray())
	{
		if (Forms.Contains(UFGItemDescriptor::GetForm(ItemAmount.ItemClass)))
		{
			Out.Add(ItemAmount);
		}
	}
}

void AKPCLNetworkCore::GrabFromNetwork(AFGCharacterPlayer* Player, FItemAmount Amount)
{
	if (!HasAuthority())
	{
		UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKPCLDefaultRCO>(GetWorld());
		if (IsValid(RCO))
		{
			RCO->Server_Faxit_GrabFromNetwork(this, Player, Amount);
		}
		return;
	}

	if (IsValid(Player) && IsValid(Player->GetInventory()) && IsValid(Amount.ItemClass))
	{
		Amount.Amount = FMath::Min(Amount.Amount, UFGItemDescriptor::GetStackSize(Amount.ItemClass));

		int32 AddedAmount;
		{
			FScopeLock ScopeLock(&mMutex);
			FItemAmount ItemAmount = mStorage.GetAmountOrCreateAmount(Amount.ItemClass);
			if (ItemAmount.Amount <= 0)
			{
				return;
			}

			int32 MaxGrabAmount = FMath::Min(Amount.Amount, ItemAmount.Amount);

			FInventoryStack Stack = FInventoryStack(MaxGrabAmount, Amount.ItemClass);
			AddedAmount = Player->GetInventory()->AddStack(Stack, true);

			mStorage.RemoveItemAmount(Amount.ItemClass, AddedAmount);
		}

		NotifyStorageChange();
	}
}

void AKPCLNetworkCore::StorageFromPlayerToNetwork(AFGCharacterPlayer* Player, FItemAmount Amount)
{
	if (!HasAuthority())
	{
		UE_LOG(LogFaxit, Warning, TEXT("StorageFromPlayerToNetwork called on client, THIS IS NOT SUPPORTED YET!!!!."));
		return;
	}

	if (IsValid(Player) && IsValid(Player->GetInventory()) && IsValid(Amount.ItemClass))
	{
		int32 NumOfItems = Player->GetInventory()->GetNumItems(Amount.ItemClass);
		int32 AmountToStore = FMath::Min(Amount.Amount, NumOfItems);

		int32 StoredAmount = TryToStoreItemAmount(Amount.ItemClass, AmountToStore);
		if (StoredAmount <= 0)
		{
			return;
		}

		Player->GetInventory()->Remove(Amount.ItemClass, StoredAmount);
	}
}

bool AKPCLNetworkCore::IsItemBlacklisted(TSubclassOf<UFGItemDescriptor> Item) const
{
	if (!IsValid(Item))
	{
		return true;
	}

	float RadioDecay = UFGItemDescriptor::GetRadioactiveDecay(Item);
	if (RadioDecay > 0.f)
	{
		return true;
	}

	for (TSubclassOf<UFGItemDescriptor> BlacklistedItem : mBlacklistedItems)
	{
		if (IsValid(BlacklistedItem) &&
			(Item->IsChildOf(BlacklistedItem) || Item->IsChildOf(UFGBuildDescriptor::StaticClass())))
		{
			return true;
		}
	}

	return false;
}

bool AKPCLNetworkCore::IsStorageFull(TSubclassOf<UFGItemDescriptor> Item) const
{
	if (IsItemBlacklisted(Item))
	{
		return true;
	}

	bool HasItem;
	FItemAmount ItemAmount = mStorage.GetAmountConst(Item, HasItem);

	if (!HasItem)
	{
		return mStorage.GetSlotSize() >= GetUniqueItemLimit();
	}

	int32 MaxAmount = GetMaxItemAmount(Item);

	return ItemAmount.Amount >= MaxAmount;
}

bool AKPCLNetworkCore::IsStorageEmpty(TSubclassOf<UFGItemDescriptor> Item) const
{
	if (IsItemBlacklisted(Item))
	{
		return false;
	}

	bool HasItem;
	FItemAmount ItemAmount = mStorage.GetAmountConst(Item, HasItem);
	return !HasItem || ItemAmount.Amount <= 0;
}

int32 AKPCLNetworkCore::TryToStoreItem(UFGInventoryComponent* Inventory, TSubclassOf<UFGItemDescriptor> Item,
									   int32 Amount, int32 InventorySlot)
{
	if (IsItemBlacklisted(Item))
	{
		return 0;
	}

	InventorySlot = FMath::Max(InventorySlot, 0);
	FInventoryStack Stack;
	if (!Inventory->GetStackFromIndex(InventorySlot, Stack))
	{
		return 0;
	}

	int32 AmountToStore = FMath::Min(Amount, Stack.NumItems);

	int32 StoredAmount = TryToStoreItemAmount(Item, AmountToStore);
	if (StoredAmount <= 0)
	{
		return 0;
	}

	Inventory->RemoveFromIndex(InventorySlot, StoredAmount);
	return StoredAmount;
}

int32 AKPCLNetworkCore::TryToStoreItemAmount(TSubclassOf<UFGItemDescriptor> Item, int32 Amount)
{
	if (IsItemBlacklisted(Item))
	{
		return 0;
	}

	FScopeLock ScopeLock(&mMutex);
	int32 StoredAmount = mStorage.AddItemAmount(Item, Amount);
	ScopeLock.Unlock();

	NotifyStorageChange();
	return StoredAmount;
}

int32 AKPCLNetworkCore::TryToGrabItem(UFGInventoryComponent* Inventory, TSubclassOf<UFGItemDescriptor> Item,
									  int32 Amount, int32 InventorySlot)
{
	InventorySlot = FMath::Max(InventorySlot, 0);

	int32 AddedAmount;
	{
		FScopeLock ScopeLock(&mMutex);
		FItemAmount ItemAmount = mStorage.GetAmountOrCreateAmount(Item);
		if (ItemAmount.Amount <= 0)
		{
			return 0;
		}

		int32 MaxGrabAmount = FMath::Min(Amount, ItemAmount.Amount);

		FInventoryStack Stack = FInventoryStack(MaxGrabAmount, Item);
		AddedAmount = Inventory->AddStackToIndex(InventorySlot, Stack, true, Inventory);

		mStorage.RemoveItemAmount(Item, AddedAmount);
	}

	NotifyStorageChange();
	return AddedAmount;
}

int32 AKPCLNetworkCore::TryToGrabItemAmount(TSubclassOf<UFGItemDescriptor> Item, int32 Amount)
{
	int32 RemovedAmount;
	{
		FScopeLock ScopeLock(&mMutex);
		RemovedAmount = mStorage.RemoveItemAmount(Item, Amount);
	}

	NotifyStorageChange();
	return RemovedAmount;
}

UFGInventoryComponent* AKPCLNetworkCore::GetPlayerBufferInventory() const { return GetOutputInventory(); }

TArray<FKPCLFaxitNetworkStatDataBundle> AKPCLNetworkCore::GetStateBundles()
{
	if (bStatBundleChanged)
	{
		AKPCLFaxitSubsystem::ResolveBundleMapArray(mStateBundels);
	}

	return mStateBundels;
}

float AKPCLNetworkCore::GetPowerConsume() const { return GetRealPowerConsume(); }

float AKPCLNetworkCore::GetRealPowerConsume() const { return mNetworkPower; }

bool AKPCLNetworkCore::FormFilterOutputInventory(TSubclassOf<UFGItemDescriptor> object, int32 idx) const
{
	if (IsValid(object))
	{
		return UFGItemDescriptor::GetForm(object) == EResourceForm::RF_SOLID;
	}
	return false;
}

bool AKPCLNetworkCore::FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const
{
	if (IsValid(object))
	{
		if (const TSubclassOf<UKPCLNetworkDrive> Drive{object})
		{
			return true;
		}
	}
	return false;
}

void AKPCLNetworkCore::OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
										  UFGInventoryComponent* sourceInventory)
{
	Super::OnInputItemRemoved(itemClass, numRemoved, sourceInventory);
	UpdateStorageState();
	FlushOverflow();
}


void AKPCLNetworkCore::OnInputItemAdded(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
										UFGInventoryComponent* sourceInventory)
{
	Super::OnInputItemAdded(itemClass, numRemoved, sourceInventory);
	UpdateStorageState();
}

void AKPCLNetworkCore::UpdateStorageState()
{
	if (!IsValid(GetInventory()) || !IsValid(mFaxitSubsystem))
	{
		return;
	}

	int32 DriveLimit = mFaxitSubsystem->GetNetworkDriveLimit();
	GetInventory()->Resize(DriveLimit);

	for (int32 i = 0; i < GetInventory()->GetSizeLinear(); i++)
	{
		GetInventory()->AddArbitrarySlotSize(i, 1);
	}

	TArray<FInventoryStack> Stacks;
	GetInventory()->GetInventoryStacks(Stacks, false);

	int32 NewDrivePower = 0;
	int32 NewStackLimit = 0;
	int32 NewBuildingLimit = 0;
	int32 NewUniqueItemCount = 0;

	for (FInventoryStack Stack : Stacks)
	{
		if (const TSubclassOf<UKPCLNetworkDrive> Drive{Stack.Item.GetItemClass()})
		{
			NewStackLimit += UKPCLNetworkDrive::GetMultiplier(Drive);
			NewDrivePower += UKPCLNetworkDrive::GetPowerConsume(Drive);
			NewBuildingLimit += UKPCLNetworkDrive::GetBuildingLimit(Drive);
			NewUniqueItemCount += UKPCLNetworkDrive::GetUniqueItemLimit(Drive);
		}
	}

	mDiskStacks = NewStackLimit;
	mDrivePower = NewDrivePower;
	mDiskBuildingLimit = NewBuildingLimit;
	mDefaultUniqueItems = NewUniqueItemCount;
}

void AKPCLNetworkCore::FlushOverflow()
{
	mStorage.CleanUpEmptySlots(mBlacklistedItems);
	mItemFlushTimer.Reset();
}

void AKPCLNetworkCore::NotifyStorageChange(bool ItemsChanged) const
{
	OnStorageChanged.Broadcast();
	if (ItemsChanged)
	{
		OnStorageItemsChanged.Broadcast();
	}
}

void AKPCLNetworkCore::HandlePower(float dt)
{
	UpdatePowerConsume();
	Super::HandlePower(dt);
}
