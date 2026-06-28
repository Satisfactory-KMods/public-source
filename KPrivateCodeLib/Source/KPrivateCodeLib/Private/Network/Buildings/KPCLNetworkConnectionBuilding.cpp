// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Network/Buildings/KPCLNetworkConnectionBuilding.h"

#include "Cpp/KBFLCppInventoryHelper.h"
#include "FGCentralStorageSubsystem.h"
#include "KPrivateCodeLibModule.h"
#include "Net/UnrealNetwork.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Replication/KPCLDefaultRCO.h"
#include "Resources/FGNoneDescriptor.h"

void AKPCLNetworkConnectionBuilding::UI_ApplyRelevantItems_Implementation(
	TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots)
{
	Super::UI_ApplyRelevantItems_Implementation(OutSlots);

	if (mIsUpload && IsSolidFaxit())
	{
		OutSlots.Add(mDepotRequiredItem);
		OutSlots.Add(mSinkRequiredItem);
	}
}

AKPCLNetworkConnectionBuilding::AKPCLNetworkConnectionBuilding()
{
	PrimaryActorTick.bCanEverTick = false;
	bBindNetworkComponent = true;

	mInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::InputName);

	mDistanceTransferModifier.mValue = 1.f;
	mDistanceTransferModifier.mMaxValue = 0.5f;
	mDistanceTransferModifier.mEveryMeter = 0.05f;
	mDistanceTransferModifier.bNegative = true;
}

void AKPCLNetworkConnectionBuilding::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		mCentralStorageSubsystem = AFGCentralStorageSubsystem::Get(GetWorld());

		TryToConnectToNearstCore();
		UpdateInventoryState();
		UpdateProductionSpeed(false);
	}
}

bool AKPCLNetworkConnectionBuilding::CanProduce_Implementation() const
{
	if (!Super::CanProduce_Implementation())
	{
		return false;
	}

	if (IsStoredItemBlocked())
	{
		return false;
	}

	if (mIsUpload)
	{
		return CanUploadStorage();
	}

	return CanDownloadStorage();
}

void AKPCLNetworkConnectionBuilding::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);

	FG_DOREPCONDITIONAL(ThisClass, mSpeedOverride);
	FG_DOREPCONDITIONAL(ThisClass, mFilterItem);
}

UFGFactoryClipboardSettings* AKPCLNetworkConnectionBuilding::CopySettings_Implementation()
{
	UKPCLFaxitNetworkConnectionClipboardSettings* Settings = NewObject<UKPCLFaxitNetworkConnectionClipboardSettings>();
	Settings->mNetworkId = mNetworkId;
	Settings->mFilterItem = mFilterItem;
	Settings->mConnectedNetworkId = mConnectedNetworkId;
	Settings->mSpeedOverride = mSpeedOverride;
	Settings->bIsUpload = mIsUpload;
	Settings->bIsPaused = IsProductionPaused();
	return Settings;
}

bool AKPCLNetworkConnectionBuilding::PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
																  class AFGPlayerController* player)
{
	bool bResult = false;

	// Fixed: the second block (UKPCLFaxitNetworkConnectionClipboardSettings) must be 'else if'.
	// UKPCLFaxitNetworkConnectionClipboardSettings derives from UKPCLFaxitBasicClipboardSettings,
	// so without 'else if', a connection clipboard would run both blocks, double-applying the
	// network attachment and pause state.
	if (UKPCLFaxitNetworkConnectionClipboardSettings* ConnectionSettings =
			Cast<UKPCLFaxitNetworkConnectionClipboardSettings>(factoryClipboard))
	{
		// Fixed: was dereferencing mFaxitSubsystem without null check.
		if (IsValid(mFaxitSubsystem))
		{
			bool Success;
			FKPCLFaxitNetwork Network = mFaxitSubsystem->GetByNetworkId(ConnectionSettings->mNetworkId, Success);
			if (Success)
			{
				mFaxitSubsystem->AddBuildingToCore(this, Network.mCore);
			}
		}

		SetIsProductionPaused(ConnectionSettings->bIsPaused);

		if (!ConnectionSettings->bIsUpload && !mIsUpload)
		{
			SetFilterItem(ConnectionSettings->mFilterItem);
		}

		// Fixed: was passing own mSpeedOverride instead of ConnectionSettings->mSpeedOverride.
		SetSpeedOverride(ConnectionSettings->mSpeedOverride);
		bResult = true;
	}
	else if (UKPCLFaxitBasicClipboardSettings* Settings = Cast<UKPCLFaxitBasicClipboardSettings>(factoryClipboard))
	{
		// Fixed: was dereferencing mFaxitSubsystem without null check.
		if (IsValid(mFaxitSubsystem))
		{
			bool Success;
			FKPCLFaxitNetwork Network = mFaxitSubsystem->GetByNetworkId(Settings->mNetworkId, Success);
			if (Success)
			{
				mFaxitSubsystem->AddBuildingToCore(this, Network.mCore);
			}
		}

		SetIsProductionPaused(Settings->bIsPaused);
		bResult = true;
	}

	return bResult;
}

void AKPCLNetworkConnectionBuilding::onProducingFinal_Implementation()
{
	Super::onProducingFinal_Implementation();
	UFGInventoryComponent* Inventory = GetInventory();
	if (!IsValid(Inventory))
	{
		return;
	}

	AKPCLNetworkCore* Core = GetFaxitCore();
	if (!IsValid(Core))
	{
		return;
	}

	int32 Amount = 0;
	if (mIsUpload)
	{
		FInventoryStack Stack;
		Inventory->GetStackFromIndex(INV_SLOT, Stack);
		if (Stack.HasItems())
		{
			TSubclassOf<UFGItemDescriptor> ItemClass = Stack.Item.GetItemClass();

			// TODO: test if this now store > depot > sink in one cycle instead only one of them per cycle
			int32 LeftOver = FMath::Min(GetTransferAmount(), Stack.NumItems);
			Amount += Core->TryToStoreItem(Inventory, Stack.Item.GetItemClass(), LeftOver, INV_SLOT);
			LeftOver -= Amount;

			if (LeftOver <= 0)
			{
				AddUploadToStats(ItemClass, Amount);
				return;
			}

			int32 DepotAmount = UploadToDepot(LeftOver);
			LeftOver -= DepotAmount;
			Amount += DepotAmount;

			if (LeftOver <= 0)
			{
				AddUploadToStats(ItemClass, Amount);
				return;
			}

			int32 SinkAmount = Sink(LeftOver);
			Amount += SinkAmount;

			AddUploadToStats(ItemClass, Amount);
		}
	}
	else if (IsValid(GetFilterItem()) && !mIsUpload)
	{
		Amount = Core->TryToGrabItem(Inventory, GetFilterItem(), GetTransferAmount(), INV_SLOT);
		AddDownloadToStats(GetFilterItem(), Amount);
	}

	UpdateProductionSpeed(false);
}

void AKPCLNetworkConnectionBuilding::SetBelts()
{
	Super::SetBelts();

	UFGFactoryConnectionComponent* Component = mIsUpload ? GetConv(0, KPCLInput) : GetConv(0, KPCLOutput);
	if (IsValid(Component))
	{
		Component->SetInventory(GetInventory());
		Component->SetInventoryAccessIndex(-1);
	}
}

void AKPCLNetworkConnectionBuilding::CollectBelts()
{
	UFGFactoryConnectionComponent* Component = GetConv(0, KPCLInput);
	if (mIsUpload && IsValid(Component))
	{
		UKBFLCppInventoryHelper::PullBelt(GetInventory(), Component);
	}
}

void AKPCLNetworkConnectionBuilding::CollectAndPushPipes(float dt, bool IsPush)
{
	Super::CollectAndPushPipes(dt, IsPush);

	UFGPipeConnectionFactory* Component = mIsUpload ? GetPipe(0, KPCLInput) : GetPipe(0, KPCLOutput);
	if (!IsValid(Component))
	{
		return;
	}

	if (!mIsUpload)
	{
		UKBFLCppInventoryHelper::PushPipe(GetInventory(), INV_SLOT, dt, Component);
	}
	else if (mIsUpload)
	{
		UKBFLCppInventoryHelper::PullAllFromPipe(GetInventory(), INV_SLOT, dt, Component);
	}
}

void AKPCLNetworkConnectionBuilding::Server_DoFlush()
{
	Super::Server_DoFlush();

	if (IsSolidFaxit())
	{
		return;
	}

	UFGInventoryComponent* Inventory = GetInventory();
	if (IsValid(Inventory))
	{
		Inventory->Empty();
	}
}

bool AKPCLNetworkConnectionBuilding::CanStoreInDepot() const
{
	if (!CanPushToDepot())
	{
		return false;
	}

	FInventoryStack Stack;
	GetInventory()->GetStackFromIndex(INV_SLOT, Stack);
	if (!Stack.HasItems())
	{
		return false;
	}

	return IsValid(mCentralStorageSubsystem) &&
		mCentralStorageSubsystem->CanUploadInventoryItemToCentralStorage(Stack.Item);
}

bool AKPCLNetworkConnectionBuilding::CanSinkItem(TSubclassOf<UFGItemDescriptor> Item) const
{
	if (!IsValid(Item))
	{
		return false;
	}
	AFGResourceSinkSubsystem* SinkSub = AFGResourceSinkSubsystem::Get(GetWorld());
	return IsValid(SinkSub) && (SinkSub->GetResourceSinkPointsForItem(Item) > 0);
}

void AKPCLNetworkConnectionBuilding::UpdateInventoryState()
{
	UFGInventoryComponent* Inventory = GetInventory();
	if (!IsValid(Inventory))
	{
		return;
	}

	if (!IsSolidFaxit() || !mIsUpload)
	{
		Inventory->Resize(1);

		if (IsValid(GetFilterItem()) && !mIsUpload)
		{
			Inventory->SetAllowedItemOnIndex(INV_SLOT, GetFilterItem());
		}

		return;
	}

	Inventory->Resize(3);

	Inventory->SetAllowedItemOnIndex(INV_SLOT, nullptr);

	Inventory->SetAllowedItemOnIndex(DEPOT_ITEM_SLOT, GetDepotRequiredItem());
	Inventory->SetAllowedItemOnIndex(SINK_ITEM_SLOT, GetSinkRequiredItem());

	Inventory->AddArbitrarySlotSize(INV_SLOT, GetTransferAmount() * 10);
	Inventory->AddArbitrarySlotSize(DEPOT_ITEM_SLOT, 1);
	Inventory->AddArbitrarySlotSize(SINK_ITEM_SLOT, 1);

	SetBelts();
}

int32 AKPCLNetworkConnectionBuilding::UploadToDepot(int32 Amount) const
{
	if (!CanPushToDepot() || Amount <= 0)
	{
		return 0;
	}

	FInventoryStack Stack;
	GetInventory()->GetStackFromIndex(INV_SLOT, Stack);

	if (!IsValid(mCentralStorageSubsystem) || !Stack.HasItems() ||
		!mCentralStorageSubsystem->CanUploadInventoryItemToCentralStorage(Stack.Item))
	{
		return 0;
	}

	int32 CurrentItemLimit = mCentralStorageSubsystem->GetCentralStorageItemLimit(Stack.Item.GetItemClass());
	int32 CurrentStoredAmount = mCentralStorageSubsystem->GetNumItemsFromCentralStorage(Stack.Item.GetItemClass());

	int32 AvailableSpace = CurrentItemLimit - CurrentStoredAmount;
	int32 UploadAmount = FMath::Min3(Amount, Stack.NumItems, AvailableSpace);

	if (UploadAmount <= 0)
	{
		return 0;
	}

	// Fixed: was wrapping the upload loop in an AsyncTask with a `[&]` capture — UAF since
	// `this`, `Stack`, and `mCentralStorageSubsystem` could all be invalid by game-thread dispatch.
	// UploadToDepot is always called server-side from onProducingFinal (via Factory_TickAuthOnly),
	// which itself already runs on the factory worker thread and is safe to call subsystem APIs
	// synchronously. The synchronous path also returns the true transferred count instead of an
	// optimistic pre-count that could diverge from the actual depot state.
	int32 Transferred = 0;
	for (int32 Count = 0; Count < UploadAmount; ++Count)
	{
		if (!mCentralStorageSubsystem->CanUploadInventoryItemToCentralStorage(Stack.Item))
		{
			break;
		}
		mCentralStorageSubsystem->UploadItemFromInventoryToCentralStorage(GetInventory(), INV_SLOT);
		++Transferred;
	}
	return Transferred;
}

int32 AKPCLNetworkConnectionBuilding::Sink(int32 Amount)
{
	if (!CanSink() || Amount <= 0)
	{
		return 0;
	}

	FInventoryStack Stack;
	GetInventory()->GetStackFromIndex(INV_SLOT, Stack);
	if (Stack.HasItems())
	{
		FItemAmount ItemAmount = FItemAmount(Stack.Item.GetItemClass(), Stack.NumItems);
		int32 Removed = SinkItems(ItemAmount, Amount);
		if (Removed > 0)
		{
			GetInventory()->RemoveFromIndex(INV_SLOT, Removed);
			return Removed;
		}
	}
	return 0;
}

void AKPCLNetworkConnectionBuilding::UpdateProductionSpeed(bool ShouldReset)
{
	if (mSpeedOverride > 0.f)
	{
		mProductionHandle.SetNewTime(60 / mSpeedOverride * ITEM_AMOUNT, ShouldReset);
		return;
	}

	mProductionHandle.SetNewTime(60 / GetItemsPerMin() * ITEM_AMOUNT, ShouldReset);
}

void AKPCLNetworkConnectionBuilding::UpdateInventoryFilter() const
{
	UFGInventoryComponent* Inventory = GetInventory();
	if (!IsValid(Inventory))
	{
		return;
	}

	if (mIsUpload)
	{
		Inventory->SetAllowedItemOnIndex(INV_SLOT, nullptr);
	}

	else if (IsValid(GetFilterItem()))
	{
		Inventory->SetAllowedItemOnIndex(INV_SLOT, GetFilterItem());
	}
}

float AKPCLNetworkConnectionBuilding::GetPowerConsume() const { return GetRealPowerConsume(); }

float AKPCLNetworkConnectionBuilding::GetRealPowerConsume() const
{
	float BaseConsume = mPowerOptions.mNormalPowerConsume;
	return BaseConsume + GetPowerConsumeOnDistance();
}

bool AKPCLNetworkConnectionBuilding::CanPushToDepot() const
{
	if (!IsValid(mFaxitSubsystem) || !mFaxitSubsystem->mDepotUnlocked || !IsSolidFaxit() || !mIsUpload)
	{
		return false;
	}

	UFGInventoryComponent* Inventory = GetInventory();
	if (!IsValid(Inventory) || Inventory->IsIndexEmpty(DEPOT_ITEM_SLOT))
	{
		return false;
	}

	return true;
}

bool AKPCLNetworkConnectionBuilding::CanSink() const
{
	if (!IsValid(mFaxitSubsystem) || !mFaxitSubsystem->mSinkUnlocked || !IsSolidFaxit() || !mIsUpload)
	{
		return false;
	}

	UFGInventoryComponent* Inventory = GetInventory();
	if (!IsValid(Inventory) || Inventory->IsIndexEmpty(SINK_ITEM_SLOT))
	{
		return false;
	}

	return true;
}

bool AKPCLNetworkConnectionBuilding::IsSolidFaxit() const { return mItemForm == EResourceForm::RF_SOLID; }

int32 AKPCLNetworkConnectionBuilding::GetTransferAmount() const
{
	if (IsSolidFaxit())
	{
		return ITEM_AMOUNT;
	}
	return FLUID_AMOUNT;
}

bool AKPCLNetworkConnectionBuilding::CanUploadStorage() const
{
	if (!IsValid(GetFaxitCoreConst()))
	{
		return false;
	}

	const TSubclassOf<UFGItemDescriptor> StoredItemClass = GetStoredItemClass();
	if (!IsValid(StoredItemClass))
	{
		return false;
	}

	const bool IsFull = GetFaxitCoreConst()->IsStorageFull(StoredItemClass);
	if (IsFull)
	{
		return (CanSink() && CanSinkItem(StoredItemClass)) || CanStoreInDepot();
	}

	return !IsFull;
}

float AKPCLNetworkConnectionBuilding::GetItemsPerMin(bool Converted) const
{
	float Amount = 1.f;
	if (IsValid(mFaxitSubsystem))
	{
		if (IsSolidFaxit())
		{
			Amount = mFaxitSubsystem->GetItemsPerMinute();
		}
		else if (Converted)
		{
			Amount = mFaxitSubsystem->GetFluidPerMinute() / 1000.f;
		}
		else
		{
			Amount = mFaxitSubsystem->GetFluidPerMinute();
		}
	}

	return bHasCableBoost ? Amount * mCableBoostAmount : Amount;
}

bool AKPCLNetworkConnectionBuilding::CanDownloadStorage() const
{
	if (GetFilterItem()->IsChildOf(UFGNoneDescriptor::StaticClass()))
	{
		return false;
	}

	if (!IsValid(GetFaxitCoreConst()))
	{
		return false;
	}
	TSubclassOf<UFGItemDescriptor> FilterItem = GetFilterItem();

	if (!IsValid(FilterItem) || GetFaxitCoreConst()->IsStorageEmpty(FilterItem))
	{
		return false;
	}

	FInventoryStack Stack;
	GetInventory()->GetStackFromIndex(INV_SLOT, Stack);
	if (Stack.HasItems())
	{
		if (Stack.HasItems() && Stack.Item.GetItemClass() != FilterItem)
		{
			return false;
		}

		return Stack.NumItems < GetInventory()->GetSlotSize(INV_SLOT, FilterItem);
	}

	return true;
}

bool AKPCLNetworkConnectionBuilding::StorageIsEmpty() const
{
	if (GetInventory())
	{
		return GetInventory()->IsEmpty();
	}
	return false;
}

void AKPCLNetworkConnectionBuilding::ClearSpeedOverride()
{
	if (!HasAuthority())
	{
		UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKPCLDefaultRCO>(GetWorld());
		if (IsValid(RCO))
		{
			RCO->Server_Faxit_ClearSpeedOverride(this);
		}
		return;
	}

	SetSpeedOverride(-1.f);
}

void AKPCLNetworkConnectionBuilding::SetSpeedOverride(float NewSpeed)
{
	if (!HasAuthority())
	{
		UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKPCLDefaultRCO>(GetWorld());
		if (IsValid(RCO))
		{
			RCO->Server_Faxit_SetSpeedOverride(this, NewSpeed);
		}
		return;
	}

	mSpeedOverride = FMath::Clamp(NewSpeed, -1.f, GetItemsPerMin());
	if (mSpeedOverride <= 0.f)
	{
		mSpeedOverride = -1.f;
	}
	mPropertyReplicator.MarkPropertyDirty(FName("mSpeedOverride"));

	UpdateProductionSpeed(true);
}

void AKPCLNetworkConnectionBuilding::SetFilterItem(TSubclassOf<UFGItemDescriptor> NewItem)
{
	if (!HasAuthority())
	{
		UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKPCLDefaultRCO>(GetWorld());
		if (IsValid(RCO))
		{
			RCO->Server_Faxit_SetFilterItem(this, NewItem);
		}
		return;
	}

	mFilterItem = NewItem;
	mPropertyReplicator.MarkPropertyDirty(FName("mFilterItem"));

	UFGInventoryComponent* Inventory = GetInventory();
	if (IsValid(Inventory))
	{
		FInventoryStack Stack;
		Inventory->GetStackFromIndex(INV_SLOT, Stack);
		if (Stack.HasItems() && Stack.Item.GetItemClass() != NewItem)
		{
			AKPCLNetworkCore* Core = GetFaxitCore();
			if (IsValid(Core))
			{
				Core->TryToStoreItemAmount(Stack.Item.GetItemClass(), Stack.NumItems);
			}
			Inventory->RemoveFromIndex(INV_SLOT, Stack.NumItems);
		}
	}

	UpdateInventoryFilter();
}

TSubclassOf<UFGItemDescriptor> AKPCLNetworkConnectionBuilding::GetSinkRequiredItem() const
{
	return IsValid(mSinkRequiredItem) ? mSinkRequiredItem
									  : TSubclassOf<UFGItemDescriptor>{UFGNoneDescriptor::StaticClass()};
}

TSubclassOf<UFGItemDescriptor> AKPCLNetworkConnectionBuilding::GetDepotRequiredItem() const
{
	return IsValid(mDepotRequiredItem) ? mDepotRequiredItem
									   : TSubclassOf<UFGItemDescriptor>{UFGNoneDescriptor::StaticClass()};
}

bool AKPCLNetworkConnectionBuilding::IsStoredItemBlocked() const
{
	const AKPCLNetworkCore* Core = GetFaxitCoreConst();
	if (!IsValid(Core))
	{
		return true;
	}

	if (!mIsUpload)
	{
		return Core->IsItemBlacklisted(GetFilterItem());
	}

	if (!IsValid(GetInventory()))
	{
		return true;
	}

	FInventoryStack Stack;
	GetInventory()->GetStackFromIndex(INV_SLOT, Stack);

	if (!Stack.HasItems())
	{
		return true;
	}

	TSubclassOf<UFGItemDescriptor> StoredItem = Stack.Item.GetItemClass();

	return Core->IsItemBlacklisted(StoredItem);
}

TSubclassOf<UFGItemDescriptor> AKPCLNetworkConnectionBuilding::GetFilterItem() const
{
	if (!IsValid(mFilterItem))
	{
		return TSubclassOf<UFGItemDescriptor>{UFGNoneDescriptor::StaticClass()};
	}
	return mFilterItem;
}

TSubclassOf<UFGItemDescriptor> AKPCLNetworkConnectionBuilding::GetStoredItemClass() const
{
	if (!IsValid(GetInventory()))
	{
		return nullptr;
	}

	FInventoryStack Stack;
	GetInventory()->GetStackFromIndex(INV_SLOT, Stack);
	if (!Stack.HasItems())
	{
		return nullptr;
	}
	return Stack.Item.GetItemClass();
}

bool AKPCLNetworkConnectionBuilding::HasItemStored(int32 Slot) const
{
	if (!IsValid(GetInventory()))
	{
		return false;
	}

	FInventoryStack Stack;
	GetInventory()->GetStackFromIndex(Slot, Stack);
	return Stack.HasItems() && IsValid(Stack.Item.GetItemClass());
}
