

#include "Network/Buildings/KPCLNetworkConnectionBuilding.h"

#include "Cpp/KBFLCppInventoryHelper.h"
#include "FGCentralStorageSubsystem.h"
#include "KPrivateCodeLibModule.h"
#include "Net/UnrealNetwork.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Network/KPCLNetworkAsyncHelpers.h"
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

	bCustomFactoryTickPreCustomLogic = true;

	mInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::InputName);
	mDownloadBufferInventory = CreateDefaultSubobject<UFGInventoryComponent>(TEXT("DownloadBufferInventory"));
	mDownloadBufferInventory->SetIsReplicated(true);

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
		if (IsValid(mDownloadBufferInventory))
		{
			mDownloadBufferInventory->SetReplicationRelevancyOwner(this);
		}

		TryToConnectToNearstCore();
		UpdateInventoryState();
		MoveDownloadBufferToOutput();
		UpdateProductionSpeed(false);

		if (IsValid(mFaxitSubsystem))
		{
			mFaxitSubsystem->mOnFaxitUnlocksUpdated.AddUniqueDynamic(
				this, &AKPCLNetworkConnectionBuilding::HandleFaxitUnlocksUpdated);
		}
	}
}

void AKPCLNetworkConnectionBuilding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(mFaxitSubsystem))
	{
		mFaxitSubsystem->mOnFaxitUnlocksUpdated.RemoveDynamic(
			this, &AKPCLNetworkConnectionBuilding::HandleFaxitUnlocksUpdated);
	}

	Super::EndPlay(EndPlayReason);
}

void AKPCLNetworkConnectionBuilding::HandleFaxitUnlocksUpdated()
{
	if (!HasAuthority())
	{
		return;
	}

	if (mSpeedOverride > 0.f && mSpeedOverride >= GetItemsPerMin())
	{
		mSpeedOverride = -1.f;
		mPropertyReplicator.MarkPropertyDirty(FName("mSpeedOverride"));
	}

	UpdateProductionSpeed(false);
	UpdateDownloadBufferInventoryState();
	MoveDownloadBufferToOutput();
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

	if (UKPCLFaxitNetworkConnectionClipboardSettings* ConnectionSettings =
			Cast<UKPCLFaxitNetworkConnectionClipboardSettings>(factoryClipboard))
	{

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

		SetSpeedOverride(ConnectionSettings->mSpeedOverride);
		bResult = true;
	}
	else if (UKPCLFaxitBasicClipboardSettings* Settings = Cast<UKPCLFaxitBasicClipboardSettings>(factoryClipboard))
	{

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

			int32 LeftOver = FMath::Min(GetTransferAmount(), Stack.NumItems);
			Amount += Core->TryToStoreItem(Inventory, Stack.Item.GetItemClass(), LeftOver, INV_SLOT);
			LeftOver -= Amount;

			if (LeftOver <= 0)
			{
				AddUploadToStats(ItemClass, Amount);
				return;
			}

			LeftOver -= UploadToDepot(LeftOver);

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
		if (SupportsDownloadBufferInventory())
		{
			MoveDownloadBufferToOutput();
			Amount = DownloadToBuffer(Core, GetTransferAmount());
			MoveDownloadBufferToOutput();
		}
		else
		{
			Amount = Core->TryToGrabItem(Inventory, GetFilterItem(), GetTransferAmount(), INV_SLOT);
		}
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

		UFGInventoryComponent* ConnectionInventory = GetInventory();
		if (mIsUpload && SupportsDownloadBufferInventory() && IsValid(GetDownloadBufferInventory()))
		{
			ConnectionInventory = GetDownloadBufferInventory();
		}
		Component->SetInventory(ConnectionInventory);
		Component->SetInventoryAccessIndex(-1);
	}
}

void AKPCLNetworkConnectionBuilding::CollectBelts()
{
	UFGFactoryConnectionComponent* Component = GetConv(0, KPCLInput);
	if (mIsUpload && IsValid(Component))
	{
		UFGInventoryComponent* InputInventory = GetInventory();
		if (SupportsDownloadBufferInventory() && IsValid(GetDownloadBufferInventory()))
		{
			InputInventory = GetDownloadBufferInventory();
		}
		UKBFLCppInventoryHelper::PullBelt(InputInventory, Component);
		MoveDownloadBufferToOutput();
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

void AKPCLNetworkConnectionBuilding::Factory_TickAuthOnly(float dt)
{
	Super::Factory_TickAuthOnly(dt);
	UpdateDownloadBufferInventoryState();
	MoveDownloadBufferToOutput();
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

		UpdateDownloadBufferInventoryState();
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
	UpdateDownloadBufferInventoryState();
}

void AKPCLNetworkConnectionBuilding::UpdateDownloadBufferInventoryState()
{
	UFGInventoryComponent* BufferInventory = GetDownloadBufferInventory();
	if (!IsValid(BufferInventory))
	{
		return;
	}

	if (!SupportsDownloadBufferInventory())
	{

		if (!mIsUpload || !IsSolidFaxit())
		{
			ReturnDownloadBufferToNetwork();
		}

		if (BufferInventory->GetSizeLinear() != 1 && BufferInventory->IsEmpty())
		{
			BufferInventory->Resize(1);
		}
		return;
	}

	if (!mIsUpload)
	{
		ReturnMismatchedDownloadBufferItemsToNetwork();
	}

	const int32 DesiredSize = GetDownloadBufferInventorySize();
	const int32 CurrentSize = BufferInventory->GetSizeLinear();
	if (CurrentSize < DesiredSize)
	{
		BufferInventory->Resize(DesiredSize);
	}
	else if (CurrentSize > DesiredSize)
	{

		if (!mIsUpload)
		{
			ReturnDownloadBufferToNetwork(DesiredSize);
		}
		if (IsDownloadBufferRangeEmpty(DesiredSize))
		{
			BufferInventory->Resize(DesiredSize);
		}
	}

	const int32 ActiveSlotCount = FMath::Min(DesiredSize, BufferInventory->GetSizeLinear());
	for (int32 Slot = 0; Slot < ActiveSlotCount; ++Slot)
	{
		const TSubclassOf<UFGItemDescriptor> AllowedItem = mIsUpload ? nullptr : GetFilterItem();
		if (BufferInventory->GetAllowedItemOnIndex(Slot) != AllowedItem)
		{
			BufferInventory->SetAllowedItemOnIndex(Slot, AllowedItem);
		}
	}
}

int32 AKPCLNetworkConnectionBuilding::MoveDownloadBufferToOutput()
{

	if (!IsSolidFaxit())
	{
		return 0;
	}

	UFGInventoryComponent* TargetInventory = GetInventory();
	UFGInventoryComponent* BufferInventory = GetDownloadBufferInventory();
	if (!IsValid(TargetInventory) || !IsValid(BufferInventory))
	{
		return 0;
	}

	int32 MovedAmount = 0;
	for (int32 Slot = 0; Slot < BufferInventory->GetSizeLinear(); ++Slot)
	{
		FInventoryStack Stack;
		if (!BufferInventory->GetStackFromIndex(Slot, Stack) || !Stack.HasItems())
		{
			continue;
		}

		const TSubclassOf<UFGItemDescriptor> ItemClass = mIsUpload ? Stack.Item.GetItemClass() : GetFilterItem();
		if (!IsValid(ItemClass) || ItemClass->IsChildOf(UFGNoneDescriptor::StaticClass()) ||
			Stack.Item.GetItemClass() != ItemClass || !InventorySlotHasSpace(TargetInventory, INV_SLOT, ItemClass))
		{
			continue;
		}

		FInventoryStack TargetStack;
		const int32 TargetAmount = TargetInventory->GetStackFromIndex(INV_SLOT, TargetStack) && TargetStack.HasItems()
			? TargetStack.NumItems
			: 0;
		const int32 SlotSpace = TargetInventory->GetSlotSize(INV_SLOT, ItemClass) - TargetAmount;
		const int32 AmountToMove = FMath::Min(Stack.NumItems, SlotSpace);
		if (AmountToMove <= 0)
		{
			continue;
		}

		const int32 AddedAmount =
			TargetInventory->AddStackToIndex(INV_SLOT, FInventoryStack(AmountToMove, ItemClass), true, BufferInventory);
		if (AddedAmount <= 0)
		{
			break;
		}

		BufferInventory->RemoveFromIndex(Slot, AddedAmount, TargetInventory);
		MovedAmount += AddedAmount;
	}

	return MovedAmount;
}

int32 AKPCLNetworkConnectionBuilding::DownloadToBuffer(AKPCLNetworkCore* Core, int32 Amount)
{
	UFGInventoryComponent* BufferInventory = GetDownloadBufferInventory();
	TSubclassOf<UFGItemDescriptor> FilterItem = GetFilterItem();
	if (!IsValid(Core) || !IsValid(BufferInventory) || !IsValid(FilterItem) || Amount <= 0)
	{
		return 0;
	}

	int32 DownloadedAmount = 0;
	const int32 ActiveSlotCount = FMath::Min(GetDownloadBufferInventorySize(), BufferInventory->GetSizeLinear());
	for (int32 Slot = 0; Slot < ActiveSlotCount && DownloadedAmount < Amount; ++Slot)
	{
		if (!InventorySlotHasSpace(BufferInventory, Slot, FilterItem))
		{
			continue;
		}

		const int32 GrabbedAmount = Core->TryToGrabItem(BufferInventory, FilterItem, Amount - DownloadedAmount, Slot);
		if (GrabbedAmount <= 0)
		{
			continue;
		}

		DownloadedAmount += GrabbedAmount;
	}

	return DownloadedAmount;
}

int32 AKPCLNetworkConnectionBuilding::ReturnDownloadBufferToNetwork(int32 FirstSlot)
{
	UFGInventoryComponent* BufferInventory = GetDownloadBufferInventory();
	AKPCLNetworkCore* Core = GetFaxitCore();
	if (!IsValid(BufferInventory) || !IsValid(Core))
	{
		return 0;
	}

	int32 StoredAmount = 0;
	for (int32 Slot = FMath::Max(0, FirstSlot); Slot < BufferInventory->GetSizeLinear(); ++Slot)
	{
		FInventoryStack Stack;
		if (!BufferInventory->GetStackFromIndex(Slot, Stack) || !Stack.HasItems())
		{
			continue;
		}

		StoredAmount += Core->TryToStoreItem(BufferInventory, Stack.Item.GetItemClass(), Stack.NumItems, Slot);
	}

	return StoredAmount;
}

int32 AKPCLNetworkConnectionBuilding::ReturnMismatchedDownloadBufferItemsToNetwork()
{
	UFGInventoryComponent* BufferInventory = GetDownloadBufferInventory();
	AKPCLNetworkCore* Core = GetFaxitCore();
	const TSubclassOf<UFGItemDescriptor> FilterItem = GetFilterItem();
	if (!IsValid(BufferInventory) || !IsValid(Core))
	{
		return 0;
	}

	int32 StoredAmount = 0;
	for (int32 Slot = 0; Slot < BufferInventory->GetSizeLinear(); ++Slot)
	{
		FInventoryStack Stack;
		if (!BufferInventory->GetStackFromIndex(Slot, Stack) || !Stack.HasItems() ||
			Stack.Item.GetItemClass() == FilterItem)
		{
			continue;
		}

		StoredAmount += Core->TryToStoreItem(BufferInventory, Stack.Item.GetItemClass(), Stack.NumItems, Slot);
	}

	return StoredAmount;
}

bool AKPCLNetworkConnectionBuilding::IsDownloadBufferRangeEmpty(int32 FirstSlot) const
{
	UFGInventoryComponent* BufferInventory = GetDownloadBufferInventory();
	if (!IsValid(BufferInventory))
	{
		return true;
	}

	for (int32 Slot = FMath::Max(0, FirstSlot); Slot < BufferInventory->GetSizeLinear(); ++Slot)
	{
		FInventoryStack Stack;
		if (BufferInventory->GetStackFromIndex(Slot, Stack) && Stack.HasItems())
		{
			return false;
		}
	}

	return true;
}

bool AKPCLNetworkConnectionBuilding::DownloadBufferHasSpace(TSubclassOf<UFGItemDescriptor> Item) const
{
	UFGInventoryComponent* BufferInventory = GetDownloadBufferInventory();
	if (!SupportsDownloadBufferInventory() || !IsValid(BufferInventory) || !IsValid(Item))
	{
		return false;
	}

	const int32 ActiveSlotCount = FMath::Min(GetDownloadBufferInventorySize(), BufferInventory->GetSizeLinear());
	for (int32 Slot = 0; Slot < ActiveSlotCount; ++Slot)
	{
		if (InventorySlotHasSpace(BufferInventory, Slot, Item))
		{
			return true;
		}
	}

	return false;
}

bool AKPCLNetworkConnectionBuilding::DownloadBufferHasMovableItems(TSubclassOf<UFGItemDescriptor> Item) const
{
	UFGInventoryComponent* BufferInventory = GetDownloadBufferInventory();
	if (!SupportsDownloadBufferInventory() || !IsValid(BufferInventory) || !IsValid(Item))
	{
		return false;
	}

	for (int32 Slot = 0; Slot < BufferInventory->GetSizeLinear(); ++Slot)
	{
		FInventoryStack Stack;
		if (BufferInventory->GetStackFromIndex(Slot, Stack) && Stack.HasItems() && Stack.Item.GetItemClass() == Item)
		{
			return true;
		}
	}

	return false;
}

bool AKPCLNetworkConnectionBuilding::InventorySlotHasSpace(UFGInventoryComponent* Inventory, int32 Slot,
														   TSubclassOf<UFGItemDescriptor> Item) const
{
	if (!IsValid(Inventory) || !IsValid(Item) || Slot < 0 || Slot >= Inventory->GetSizeLinear())
	{
		return false;
	}

	FInventoryStack Stack;
	if (!Inventory->GetStackFromIndex(Slot, Stack) || !Stack.HasItems())
	{
		return true;
	}

	return Stack.Item.GetItemClass() == Item && Stack.NumItems < Inventory->GetSlotSize(Slot, Item);
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

	TSubclassOf<UFGItemDescriptor> ItemClass = Stack.Item.GetItemClass();
	RunOnGameThreadIfValid(this,
						   [UploadAmount, ItemClass](const AKPCLNetworkConnectionBuilding* Self)
						   {
							   AFGCentralStorageSubsystem* Subsystem = Self->mCentralStorageSubsystem;
							   UFGInventoryComponent* Inventory = Self->GetInventory();
							   if (!IsValid(Subsystem) || !IsValid(Inventory))
							   {
								   return;
							   }

							   int32 Transferred = 0;
							   for (int32 Count = 0; Count < UploadAmount; ++Count)
							   {
								   FInventoryStack CurrentStack;
								   Inventory->GetStackFromIndex(Self->INV_SLOT, CurrentStack);
								   if (!CurrentStack.HasItems() || CurrentStack.Item.GetItemClass() != ItemClass ||
									   !Subsystem->CanUploadInventoryItemToCentralStorage(CurrentStack.Item))
								   {
									   break;
								   }
								   Subsystem->UploadItemFromInventoryToCentralStorage(Inventory, Self->INV_SLOT);
								   ++Transferred;
							   }

							   if (Transferred > 0)
							   {
								   const_cast<AKPCLNetworkConnectionBuilding*>(Self)->AddUploadToStats(ItemClass,
																									   Transferred);
							   }
						   });
	return UploadAmount;
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

	if (SupportsDownloadBufferInventory())
	{
		return DownloadBufferHasSpace(FilterItem) ||
			(InventorySlotHasSpace(GetInventory(), INV_SLOT, FilterItem) && DownloadBufferHasMovableItems(FilterItem));
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
	if (IsValid(GetDownloadBufferInventory()) && !GetDownloadBufferInventory()->IsEmpty())
	{
		return false;
	}

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

	if (!mIsUpload && NewItem != mFilterItem && IsValid(GetDownloadBufferInventory()) &&
		!GetDownloadBufferInventory()->IsEmpty())
	{
		ReturnDownloadBufferToNetwork();
		MoveDownloadBufferToOutput();
		if (!GetDownloadBufferInventory()->IsEmpty())
		{
			return;
		}
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
				Core->TryToStoreItem(Inventory, Stack.Item.GetItemClass(), Stack.NumItems, INV_SLOT);
			}
		}
	}

	UpdateInventoryFilter();
	UpdateDownloadBufferInventoryState();
	MoveDownloadBufferToOutput();
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

bool AKPCLNetworkConnectionBuilding::HasBufferInventory() const { return bHasBufferInventory; }

int32 AKPCLNetworkConnectionBuilding::GetInitialBufferInventorySize() const
{
	return FMath::Max(0, mInitialBufferInventorySize);
}

bool AKPCLNetworkConnectionBuilding::SupportsDownloadBufferInventory() const
{

	return GetDownloadBufferInventorySize() > 0;
}

UFGInventoryComponent* AKPCLNetworkConnectionBuilding::GetDownloadBufferInventory() const
{
	return mDownloadBufferInventory;
}

int32 AKPCLNetworkConnectionBuilding::GetDownloadBufferInventorySize() const
{
	if (!bHasBufferInventory || !IsSolidFaxit())
	{
		return 0;
	}

	const int32 UnlockSize = IsValid(mFaxitSubsystem) ? mFaxitSubsystem->GetNetworkBuildingBufferInventorySize() : 0;
	return FMath::Max(0, GetInitialBufferInventorySize() + UnlockSize);
}
