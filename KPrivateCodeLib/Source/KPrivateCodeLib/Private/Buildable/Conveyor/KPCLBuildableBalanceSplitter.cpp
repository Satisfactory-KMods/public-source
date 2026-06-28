//

#include "Buildable/Conveyor/KPCLBuildableBalanceSplitter.h"

#include "Cpp/KBFLCppInventoryHelper.h"
#include "Kismet/KismetMathLibrary.h"
#include "Logging/StructuredLog.h"
#include "Net/UnrealNetwork.h"
#include "Replication/KPCLDefaultRCO.h"
#include "Resources/FGAnyUndefinedDescriptor.h"
#include "Resources/FGNoneDescriptor.h"
#include "Resources/FGOverflowDescriptor.h"
#include "Resources/FGWildCardDescriptor.h"

AKPCLBuildableBalanceSplitter::AKPCLBuildableBalanceSplitter()
{
	this->mFactoryTickFunction.TickGroup = TG_PrePhysics;
	this->mFactoryTickFunction.EndTickGroup = TG_PrePhysics;
	this->mFactoryTickFunction.bTickEvenWhenPaused = false;
	this->mFactoryTickFunction.bCanEverTick = true;
	this->mFactoryTickFunction.bStartWithTickEnabled = true;
	this->mFactoryTickFunction.bAllowTickOnDedicatedServer = true;
	this->mFactoryTickFunction.TickInterval = 0.0;
	this->mToggleDormancyOnInteraction = true;
	this->NetDormancy = DORM_Initial;
	mInventorySize = 4;
}

bool FKPCLSplitterTimer::CanPushItem(TSubclassOf<UFGItemDescriptor> ItemClass, bool bOnlyOverflowing,
									 bool bIsItemDefinedElsewhere) const
{
	// Kein Filter gesetzt = alles erlaubt (normaler Splitter-Modus)
	if (mFilteredItems.IsEmpty())
	{
		return !bOnlyOverflowing;
	}

	bool bHasOverflow = HasOverrule();
	if (bOnlyOverflowing)
	{
		return bHasOverflow;
	}

	// Wenn nur None-Einträge vorhanden sind, nichts durchlassen
	if (HasNone() || bHasOverflow)
	{
		return false;
	}

	// Wildcard: alles durchlassen
	if (HasWildcard())
	{
		return true;
	}

	for (const TSubclassOf<UFGItemDescriptor>& FilteredItem : mFilteredItems)
	{
		if (FilteredItem == ItemClass)
		{
			return true;
		}
	}

	// AnyUndefined: nur für Items die auf keinem anderen Output spezifisch definiert sind
	if (!bIsItemDefinedElsewhere && HasAnyUndefined())
	{
		return true;
	}

	return false;
}

void FKPCLSplitterTimer::Tick(float dt)
{
	// Nur ticken wenn ein Rate-Limit gesetzt ist
	if (mTargetPerMin <= 0.0f)
	{
		// Kein Limit = immer bereit
		mCurrentTimeLeft = 0.0f;
		return;
	}

	// dt vollständig abziehen – auch wenn der Timer bereits bereit ist.
	// Das ermöglicht mehrere Pushes pro Frame wenn dt > TargetDuration
	// (z.B. Frame 50ms, Periode 12ms → ~4 Pushes erlaubt).
	// Ungenutztes Guthaben (Belt voll etc.) wird nach der Verteilung
	// in Factory_Tick auf 0 zurückgesetzt, damit es sich nicht aufstaut.
	mCurrentTimeLeft -= dt;
}

bool FKPCLSplitterTimer::CanPush() const { return mCurrentTimeLeft <= 0.0f; }

void FKPCLSplitterTimer::HasPushed() { mCurrentTimeLeft += UKismetMathLibrary::SafeDivide(60.0f, mTargetPerMin); }

void FKPCLSplitterTimer::Reset() { mCurrentTimeLeft = 0.0f; }

bool FKPCLSplitterTimer::HasOverrule() const { return HasItemOfClass(UFGOverflowDescriptor::StaticClass()); }

bool FKPCLSplitterTimer::HasWildcard() const { return HasItemOfClass(UFGWildCardDescriptor::StaticClass()); }

bool FKPCLSplitterTimer::HasNone() const { return HasItemOfClass(UFGNoneDescriptor::StaticClass()); }

bool FKPCLSplitterTimer::HasAnyUndefined() const { return HasItemOfClass(UFGAnyUndefinedDescriptor::StaticClass()); }

bool FKPCLSplitterTimer::HasItemOfClass(TSubclassOf<UFGItemDescriptor> ItemClass) const
{
	for (const TSubclassOf<UFGItemDescriptor>& FilteredItem : mFilteredItems)
	{
		if (IsValid(FilteredItem) && FilteredItem->IsChildOf(ItemClass))
		{
			return true;
		}
	}
	return false;
}

FString FKPCLSplitterTimer::ToString() const
{
	return FString::Printf(TEXT("FKPCLSplitterTimer :: TargetPerMin: %.2f, CurrentTimeLeft: %.2f, FilterCount: %d"),
						   mTargetPerMin, mCurrentTimeLeft, mFilteredItems.Num());
}

bool AKPCLBuildableBalanceSplitter::CanUseFactoryClipboard_Implementation() { return true; }

UFGFactoryClipboardSettings* AKPCLBuildableBalanceSplitter::CopySettings_Implementation()
{
	UKPCLBalanceSplitterClipboardSettings* ClipboardSettings =
		NewObject<UKPCLBalanceSplitterClipboardSettings>(this, UKPCLBalanceSplitterClipboardSettings::StaticClass());
	for (const FKPCLSplitterTimer& Timer : mOutputTimers)
	{
		ClipboardSettings->mRules.Add(Timer);
	}
	return ClipboardSettings;
}

bool AKPCLBuildableBalanceSplitter::PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
																 class AFGPlayerController* player)
{
	UKPCLBalanceSplitterClipboardSettings* ClipboardSettings =
		Cast<UKPCLBalanceSplitterClipboardSettings>(factoryClipboard);
	if (!ClipboardSettings)
	{
		return false;
	}

	int32 Idx = 0;
	for (const FKPCLSplitterTimer& Rule : ClipboardSettings->mRules)
	{
		if (!mOutputTimers.IsValidIndex(Idx))
		{
			break;
		}

		FKPCLSplitterTimer* Timer = &mOutputTimers[Idx];
		Timer->mTargetPerMin = Rule.mTargetPerMin;
		Timer->mFilteredItems = Rule.mFilteredItems;
		Timer->HasPushed();
		Idx++;
	}
	// Actor uses DORM_Initial dormancy; wake it so the replicated mOutputTimers change is sent.
	FlushNetDormancy();
	return true;
}

void AKPCLBuildableBalanceSplitter::GetDismantleRefund_Implementation(TArray<FInventoryStack>& out_refund,
																	  bool noBuildCostEnabled) const
{
	Super::GetDismantleRefund_Implementation(out_refund, noBuildCostEnabled);
}

float AKPCLBuildableBalanceSplitter::GetProgressForTimer(const FKPCLSplitterTimer& Timer)
{
	if (Timer.mTargetPerMin <= 0.0f)
	{
		return 1.0f;
	}
	return FMath::Clamp(1.0f - (Timer.mCurrentTimeLeft / (60.0f / Timer.mTargetPerMin)), 0.0f, 1.0f);
}

bool AKPCLBuildableBalanceSplitter::CanPushForTimer(const FKPCLSplitterTimer& Timer) { return Timer.CanPush(); }

void AKPCLBuildableBalanceSplitter::SetFilteredItems(int32 Idx, TArray<TSubclassOf<UFGItemDescriptor>> Items)
{
	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this))
		{
			RCO->Server_Splitter_SetFilteredItems(this, Idx, Items);
		}
		return;
	}

	if (!mOutputTimers.IsValidIndex(Idx))
	{
		return;
	}
	FKPCLSplitterTimer* Timer = &mOutputTimers[Idx];

	switch (mSplitterType)
	{
	case EKPCLBalanceSplitterType::NORMAL:
		return;
	case EKPCLBalanceSplitterType::SMART:
		Items.SetNum(1);
		break;
	default:
		break;
	}

	Timer->mFilteredItems = Items;

	mOnSplitterRulesChanged.Broadcast(true, *Timer, Idx);
	// Actor uses DORM_Initial dormancy; wake it so the replicated mOutputTimers change is sent.
	FlushNetDormancy();
}

void AKPCLBuildableBalanceSplitter::SetItemsPerMin(int32 Idx, float ItemsPerMin)
{
	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this))
		{
			RCO->Server_Splitter_SetItemsPerMin(this, Idx, ItemsPerMin);
		}
		return;
	}

	if (!mOutputTimers.IsValidIndex(Idx))
	{
		return;
	}
	FKPCLSplitterTimer* Timer = &mOutputTimers[Idx];
	Timer->mTargetPerMin = FMath::Clamp(ItemsPerMin, 0.0f, 1200.0f);
	Timer->HasPushed();
	Timer->Reset();

	mOnSplitterRulesChanged.Broadcast(false, *Timer, Idx);
	// Actor uses DORM_Initial dormancy; wake it so the replicated mOutputTimers change is sent.
	FlushNetDormancy();
}

void AKPCLBuildableBalanceSplitter::RemoveFromFilter(int32 Idx, TSubclassOf<UFGItemDescriptor> Item)
{
	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this))
		{
			RCO->Server_Splitter_RemoveFromFilter(this, Idx, Item);
		}
		return;
	}

	if (!mOutputTimers.IsValidIndex(Idx))
	{
		return;
	}
	FKPCLSplitterTimer* Timer = &mOutputTimers[Idx];
	Timer->mFilteredItems.Remove(Item);

	if (mSplitterType != EKPCLBalanceSplitterType::NORMAL)
	{
		if (Timer->mFilteredItems.IsEmpty())
		{
			Timer->mFilteredItems.Add(UFGNoneDescriptor::StaticClass());
		}
	}

	mOnSplitterRulesChanged.Broadcast(true, *Timer, Idx);
	// Actor uses DORM_Initial dormancy; wake it so the replicated mOutputTimers change is sent.
	FlushNetDormancy();
}

void AKPCLBuildableBalanceSplitter::AddOrSetFilter(int32 Idx, TSubclassOf<UFGItemDescriptor> Item)
{
	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this))
		{
			RCO->Server_Splitter_AddOrSetFilter(this, Idx, Item);
		}
		return;
	}

	if (!mOutputTimers.IsValidIndex(Idx))
	{
		return;
	}

	if (!IsValid(Item))
	{
		Item = UFGNoneDescriptor::StaticClass();
	}

	FKPCLSplitterTimer* Timer = &mOutputTimers[Idx];
	if (mSplitterType == EKPCLBalanceSplitterType::SMART)
	{
		Timer->mFilteredItems.SetNum(1);
		Timer->mFilteredItems[0] = Item;
		mOnSplitterRulesChanged.Broadcast(true, *Timer, Idx);
	}
	else if (mSplitterType == EKPCLBalanceSplitterType::PROGRAMMABLE)
	{
		if (Timer->mFilteredItems.Num() == 1 && Timer->mFilteredItems[0]->IsChildOf(UFGNoneDescriptor::StaticClass()))
		{
			Timer->mFilteredItems.Empty();
		}

		if (!Timer->mFilteredItems.Contains(Item))
		{
			Timer->mFilteredItems.Add(Item);
			mOnSplitterRulesChanged.Broadcast(true, *Timer, Idx);
		}
	}
	else
	{
		Timer->mFilteredItems.Empty();
	}
	// Actor uses DORM_Initial dormancy; wake it so the replicated mOutputTimers change is sent.
	FlushNetDormancy();
}

void AKPCLBuildableBalanceSplitter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, mOutputTimers);
}

void AKPCLBuildableBalanceSplitter::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);
	if (HasAuthority())
	{
		for (FKPCLSplitterTimer& Timer : mOutputTimers)
		{
			Timer.Tick(dt);
		}

		Factory_HandleBelts(dt);

		// Verteile Items von Slot 0 zu den Output-Slots (1+)
		DistributeItemsToOutputSlots(dt);

		// Ungenutztes Zeit-Guthaben zurücksetzen (verhindert Aufstauung
		// wenn das Belt voll war und kein Item gepusht werden konnte)
		for (FKPCLSplitterTimer& Timer : mOutputTimers)
		{
			if (Timer.CanPush())
			{
				Timer.mCurrentTimeLeft = 0.0f;
			}
		}
	}
}

void AKPCLBuildableBalanceSplitter::Factory_HandleBelts(float dt)
{
	// B14: Null-check buffer inventory before use in the per-tick path.
	UFGInventoryComponent* BufferInventory = GetBufferInventory();
	if (!IsValid(BufferInventory))
	{
		return;
	}

	// Hole Items vom Input-Belt in Slot 0
	for (UFGFactoryConnectionComponent* Input : mInputs)
	{
		// B13: Use `continue` so later inputs are still processed when this
		//      one is unconnected — `return` was skipping all remaining inputs.
		if (!IsValid(Input) || !Input->IsConnected())
		{
			continue;
		}

		int32 NumToGrab = Input->MaxNumGrab(dt);
		for (int32 I = 0; I < NumToGrab; I++)
		{
			TArray<FInventoryItem> out_items;
			if (!Input->Factory_PeekOutput(out_items) || out_items.IsEmpty())
			{
				// B13: `break` to stop grabbing from *this* input; outer loop
				//      continues to the next input belt.
				break;
			}

			FInventoryItem Item = out_items[0];
			if (!Item.IsValid())
			{
				break; // B13: same — stop inner grab loop, not the whole function
			}

			FInventoryStack Stack;
			BufferInventory->GetStackFromIndex(0, Stack);
			if (Stack.HasItems() && Stack.Item.GetItemClass() != Item.GetItemClass())
			{
				break; // B13: buffer holds a different item; try next input
			}

			int32 AmountAdded = BufferInventory->AddStackToIndex(0, FInventoryStack(Item), true);
			if (AmountAdded <= 0)
			{
				break; // B13: buffer full for this item type; try next input
			}

			float offset = 0.0f;
			FInventoryItem grabbedItem;
			if (!Input->Factory_GrabOutput(grabbedItem, offset, Item.GetItemClass()))
			{
				BufferInventory->RemoveFromIndex(0, AmountAdded);
			}
		}
	}
}

void AKPCLBuildableBalanceSplitter::DistributeItemsToOutputSlots(float dt)
{
	UFGInventoryComponent* BufferInventory = GetBufferInventory();
	if (!IsValid(BufferInventory))
	{
		return;
	}

	// Slot 0 ist der Input-Buffer
	constexpr int32 InputSlotIndex = 0;

	// Prüfe ob Items im Input-Slot sind
	FInventoryStack InputStack;
	if (!BufferInventory->GetStackFromIndex(InputSlotIndex, InputStack) || !InputStack.HasItems())
	{
		return;
	}

	// Verteile basierend auf dem Splitter-Typ
	switch (mSplitterType)
	{
	case EKPCLBalanceSplitterType::NORMAL:
		// Schleife: mehrere Pushes pro Frame erlaubt wenn dt > TargetDuration
		while (DistributeNormalMode(BufferInventory, InputSlotIndex, dt))
		{
		}
		break;
	case EKPCLBalanceSplitterType::SMART:
	case EKPCLBalanceSplitterType::PROGRAMMABLE:
		DistributeSmartOrProgrammableMode(BufferInventory, InputSlotIndex, dt);
		break;
	}
}

bool AKPCLBuildableBalanceSplitter::DistributeNormalMode(UFGInventoryComponent* BufferInventory, int32 InputSlotIndex,
														 float dt)
{
	FInventoryStack InputStack;
	if (!BufferInventory->GetStackFromIndex(InputSlotIndex, InputStack) || !InputStack.HasItems())
	{
		return false;
	}

	for (int32 i = 0; i < mOutputs.Num(); ++i)
	{
		int32 OutputIndex = (mCurrentOutputIndex + i) % mOutputs.Num();

		if (!mOutputs.IsValidIndex(OutputIndex) || !mOutputTimers.IsValidIndex(OutputIndex))
		{
			continue;
		}

		UFGFactoryConnectionComponent* Output = mOutputs[OutputIndex];
		if (!Output || !Output->IsConnected())
		{
			continue;
		}

		FKPCLSplitterTimer* Timer = &mOutputTimers[OutputIndex];
		if (!Timer || !Timer->CanPush())
		{
			continue;
		}

		int32 NumToGrab = Output->MaxNumGrab(dt);
		for (int32 I = 0; I < NumToGrab; I++)
		{
			// Output-Slot-Index = OutputIndex + 1 (da Slot 0 der Input-Buffer ist)
			int32 OutputSlotIndex = OutputIndex + 1;

			// Verschiebe ein Item vom Input-Slot zum Output-Slot
			FInventoryStack StackToMove(InputStack.Item);
			StackToMove.NumItems = 1;

			// Prüfe nochmal die ItemClass vor dem Verschieben
			if (!StackToMove.HasItems())
			{
				return false;
			}

			int32 AddedItems = BufferInventory->AddStackToIndex(OutputSlotIndex, StackToMove, true);
			if (AddedItems > 0)
			{
				BufferInventory->RemoveFromIndex(InputSlotIndex, AddedItems);

				// Wechsle zum nächsten Output für Round-Robin (überspringe nicht-verbundene)
				mCurrentOutputIndex = (OutputIndex + 1) % mOutputs.Num();
				Timer->HasPushed();
				return true; // Item erfolgreich gepusht → Schleife kann erneut aufrufen
			}
		}
	}
	return false; // Kein Item konnte gepusht werden
}

void AKPCLBuildableBalanceSplitter::DistributeSmartOrProgrammableMode(UFGInventoryComponent* BufferInventory,
																	  int32 InputSlotIndex, float dt)
{
	FInventoryStack InputStack;
	if (!BufferInventory->GetStackFromIndex(InputSlotIndex, InputStack) || !InputStack.HasItems())
	{
		return;
	}

	// Prüfe, ob dieses Item an einem anderen Output spezifisch definiert ist (für AnyUndefined)
	bool bIsItemDefinedElsewhere = IsItemDefinedOnAnyOutput(InputStack.Item.GetItemClass());

	bool IsOverflowingNeeded = true;
	while (true)
	{
		bool HadPushedThisIteration = false;
		// Erster Durchlauf: Suche einen Output der das Item gemäß seinen Regeln akzeptiert
		for (int32 i = 0; i < mOutputs.Num(); ++i)
		{
			int32 OutputIndex = (mCurrentOutputIndex + i) % mOutputs.Num();

			if (!mOutputs.IsValidIndex(OutputIndex) || !mOutputTimers.IsValidIndex(OutputIndex))
			{
				continue;
			}

			UFGFactoryConnectionComponent* Output = mOutputs[OutputIndex];
			if (!Output || !Output->IsConnected())
			{
				continue;
			}

			FKPCLSplitterTimer* Timer = &mOutputTimers[OutputIndex];
			if (!Timer || !Timer->CanPush())
			{
				continue;
			}

			if (!CanOutputAcceptItem(OutputIndex, InputStack.Item.GetItemClass(), bIsItemDefinedElsewhere, false))
			{
				continue;
			}

			if (TryMoveItemToOutput(BufferInventory, InputSlotIndex, InputStack, OutputIndex))
			{
				// Erfolg - wechsle zum nächsten Output
				mCurrentOutputIndex = (OutputIndex + 1) % mOutputs.Num();
				Timer->HasPushed();

				IsOverflowingNeeded = false;
				HadPushedThisIteration = true;
				if (!BufferInventory->GetStackFromIndex(InputSlotIndex, InputStack) || !InputStack.HasItems())
				{
					return;
				}
			}
		}

		if (!HadPushedThisIteration)
		{
			break;
		}
	}

	if (!IsOverflowingNeeded)
	{
		return;
	}

	// Zweiter Durchlauf: Overflow-Modus
	for (int32 i = 0; i < mOutputs.Num(); ++i)
	{
		int32 OutputIndex = (mCurrentOutputIndex + i) % mOutputs.Num();

		if (!mOutputs.IsValidIndex(OutputIndex) || !mOutputTimers.IsValidIndex(OutputIndex))
		{
			continue;
		}

		UFGFactoryConnectionComponent* Output = mOutputs[OutputIndex];
		if (!Output || !Output->IsConnected())
		{
			continue;
		}

		FKPCLSplitterTimer* Timer = &mOutputTimers[OutputIndex];
		if (!Timer || !Timer->CanPush())
		{
			continue;
		}

		bool WasSuccessful = false;
		int32 NumToGrab = Output->MaxNumGrab(dt);
		for (int32 I = 0; I < NumToGrab; I++)
		{
			if (!CanOutputAcceptItem(OutputIndex, InputStack.Item.GetItemClass(), bIsItemDefinedElsewhere, true))
			{
				continue;
			}

			if (TryMoveItemToOutput(BufferInventory, InputSlotIndex, InputStack, OutputIndex))
			{
				mCurrentOutputIndex = (OutputIndex + 1) % mOutputs.Num();
				Timer->HasPushed();

				WasSuccessful = true;
				if (!BufferInventory->GetStackFromIndex(InputSlotIndex, InputStack) || !InputStack.HasItems())
				{
					return;
				}
			}
		}

		if (WasSuccessful)
		{
			return;
		}
	}
}

bool AKPCLBuildableBalanceSplitter::CanOutputAcceptItem(int32 OutputIndex, TSubclassOf<UFGItemDescriptor> ItemClass,
														bool bIsItemDefinedElsewhere, bool bOverflowMode) const
{
	if (!mOutputs.IsValidIndex(OutputIndex) || !mOutputTimers.IsValidIndex(OutputIndex))
	{
		return false;
	}

	UFGFactoryConnectionComponent* Output = mOutputs[OutputIndex];
	if (!Output || !Output->IsConnected())
	{
		return false;
	}

	const FKPCLSplitterTimer* Timer = GetTimerForConnection(Output);
	if (!Timer || !Timer->CanPush())
	{
		return false;
	}

	return Timer->CanPushItem(ItemClass, bOverflowMode, bIsItemDefinedElsewhere);
}

bool AKPCLBuildableBalanceSplitter::TryMoveItemToOutput(UFGInventoryComponent* BufferInventory, int32 InputSlotIndex,
														const FInventoryStack& InputStack, int32 OutputIndex)
{
	// Validiere dass das Item eine gültige Klasse hat
	if (!InputStack.Item.GetItemClass())
	{
		return false;
	}

	// Output-Slot-Index = OutputIndex + 1 (da Slot 0 der Input-Buffer ist)
	int32 OutputSlotIndex = OutputIndex + 1;

	// Prüfe ob der Output-Slot leer ist
	FInventoryStack OutputStack;
	BufferInventory->GetStackFromIndex(OutputSlotIndex, OutputStack);
	if (OutputStack.HasItems() && OutputStack.Item.GetItemClass() != InputStack.Item.GetItemClass())
	{
		return false;
	}

	int32 SlotSize = BufferInventory->GetSlotSize(OutputSlotIndex);
	if (SlotSize <= 0 || OutputStack.NumItems >= SlotSize)
	{
		return false;
	}

	// Verschiebe ein Item vom Input-Slot zum Output-Slot
	FInventoryStack StackToMove(InputStack.Item);
	StackToMove.NumItems = 1;

	int32 AddedItems = BufferInventory->AddStackToIndex(OutputSlotIndex, StackToMove, true);
	if (AddedItems > 0)
	{
		BufferInventory->RemoveFromIndex(InputSlotIndex, AddedItems);
		return true;
	}

	return false;
}

void AKPCLBuildableBalanceSplitter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		UFGInventoryComponent* BufferInventory = GetBufferInventory();
		if (!IsValid(BufferInventory))
		{
			return;
		}

		// BufferInventory->SetSuppressOnItemAddedDelegate(true);
		// BufferInventory->SetSuppressOnItemRemovedDelegate(true);

		BufferInventory->AddArbitrarySlotSize(0, mInputBufferSize);
		for (int32 i = 1; i < BufferInventory->GetSizeLinear(); ++i)
		{
			BufferInventory->AddArbitrarySlotSize(i, mOutputBufferSize);
		}

		mOutputs.Empty();
		mInputs.Empty();

		if (IsValid(GetOutputByName(TEXT("TopConnection"))))
		{
			UFGFactoryConnectionComponent* TopConnection = GetOutputByName(TEXT("TopConnection"));
			UFGFactoryConnectionComponent* BottomConnection = GetOutputByName(TEXT("BottomConnection"));

			if (IsValid(TopConnection) && IsValid(BottomConnection))
			{
				UFGFactoryConnectionComponent* MiddleConnection =
					TopConnection->GetDirection() == EFactoryConnectionDirection::FCD_OUTPUT ? TopConnection
																							 : BottomConnection;
				UFGFactoryConnectionComponent* Input =
					TopConnection->GetDirection() == EFactoryConnectionDirection::FCD_OUTPUT ? BottomConnection
																							 : TopConnection;

				if (UFGFactoryConnectionComponent* Out1 = GetOutputByName(TEXT("Output1")))
				{
					mOutputs.Add(Out1);
				}
				mOutputs.Add(MiddleConnection);
				if (UFGFactoryConnectionComponent* Out2 = GetOutputByName(TEXT("Output2")))
				{
					mOutputs.Add(Out2);
				}
				mInputs.Add(Input);
			}
		}
		else
		{
			if (UFGFactoryConnectionComponent* Out3 = GetOutputByName(TEXT("Output3")))
			{
				mOutputs.Add(Out3);
			}
			if (UFGFactoryConnectionComponent* Out1 = GetOutputByName(TEXT("Output1")))
			{
				mOutputs.Add(Out1);
			}
			if (UFGFactoryConnectionComponent* Out2 = GetOutputByName(TEXT("Output2")))
			{
				mOutputs.Add(Out2);
			}
			if (UFGFactoryConnectionComponent* In1 = GetOutputByName(TEXT("Input1")))
			{
				mInputs.Add(In1);
			}
		}

		for (int32 i = 0; i < mInputs.Num(); ++i)
		{
			UFGFactoryConnectionComponent* Input = mInputs[i];
			if (Input)
			{
				Input->SetInventory(BufferInventory);
				Input->SetInventoryAccessIndex(0);
			}
		}

		if (mOutputTimers.Num() != mOutputs.Num())
		{
			mOutputTimers.SetNum(mOutputs.Num());
		}

		// Clamp saved round-robin index to valid range after load (B11).
		if (mOutputs.Num() > 0)
		{
			mCurrentOutputIndex = FMath::Clamp(mCurrentOutputIndex, 0, mOutputs.Num() - 1);
		}

		// Leere die Map und befülle sie neu
		mOutputConnectionToIndexMap.Empty();

		// Verknüpfe Timer mit Connections
		for (int32 i = 0; i < mOutputs.Num() && i < mOutputTimers.Num(); ++i)
		{
			mOutputTimers[i].mConnection = mOutputs[i];
			mOutputs[i]->SetForwardPeekAndGrabToBuildable(true);
			mOutputs[i]->SetInventory(nullptr);
			mOutputs[i]->SetInventoryAccessIndex(INDEX_NONE);

			// Füge zur Map hinzu für schnellen Zugriff
			mOutputConnectionToIndexMap.Add(mOutputs[i], i);
		}

		// For Smart und Programmable: Initialisiere Filter mit None, damit standardmäßig nichts durchgelassen wird
		for (FKPCLSplitterTimer& Timer : mOutputTimers)
		{
			if (mSplitterType != EKPCLBalanceSplitterType::NORMAL)
			{
				if (Timer.mFilteredItems.IsEmpty())
				{
					Timer.mFilteredItems.Add(UFGNoneDescriptor::StaticClass());
				}
			}
			else
			{
				Timer.mFilteredItems.Empty();
			}
		}
	}
}

bool AKPCLBuildableBalanceSplitter::Factory_GrabOutput_Implementation(class UFGFactoryConnectionComponent* connection,
																	  FInventoryItem& out_item, float& out_OffsetBeyond,
																	  TSubclassOf<UFGItemDescriptor> type)
{
	UFGInventoryComponent* BufferInventory = GetBufferInventory();
	if (!IsValid(BufferInventory) || !IsValid(connection))
	{
		return false;
	}

	// Finde den Index der Connection über die Map
	int32 ConnectionIndex = INDEX_NONE;

	// Prüfe zuerst direkt in der Map
	if (const int32* FoundIndex = mOutputConnectionToIndexMap.Find(connection))
	{
		ConnectionIndex = *FoundIndex;
	}
	// Falls nicht gefunden, prüfe die verbundene Connection
	else if (UFGFactoryConnectionComponent* ConnectedConnection = connection->GetConnection())
	{
		if (const int32* FoundConnectedIndex = mOutputConnectionToIndexMap.Find(ConnectedConnection))
		{
			ConnectionIndex = *FoundConnectedIndex;
		}
	}

	if (ConnectionIndex == INDEX_NONE)
	{
		return false;
	}

	// Output-Slot-Index = ConnectionIndex + 1 (Slot 0 ist Input-Buffer)
	int32 InventoryIdx = ConnectionIndex + 1;

	// Sicherheitsprüfung: InventoryIdx muss gültig sein
	if (InventoryIdx <= 0 || InventoryIdx >= BufferInventory->GetSizeLinear())
	{
		return false;
	}

	FInventoryStack Stack;
	if (!BufferInventory->GetStackFromIndex(InventoryIdx, Stack))
	{
		return false;
	}

	// Prüfe ob der Stack tatsächlich Items hat
	if (!Stack.HasItems() || Stack.NumItems <= 0)
	{
		return false;
	}

	// Prüfe ob das Item und seine Klasse gültig sind BEVOR wir es entfernen
	TSubclassOf<UFGItemDescriptor> ItemClass = Stack.Item.GetItemClass();
	if (!ItemClass)
	{
		// Item hat keine gültige Klasse - korrupter State, nicht entfernen
		return false;
	}

	// Wenn ein spezifischer Typ angefordert wird, prüfe ob es passt
	if (type && ItemClass != type)
	{
		return false;
	}

	// Alles valide - Item zurückgeben und entfernen
	out_item = Stack.Item;
	out_OffsetBeyond = 0.0f;

	// Entferne das Item aus dem Inventory
	BufferInventory->RemoveFromIndex(InventoryIdx, 1);

	return true;
}

bool AKPCLBuildableBalanceSplitter::Factory_PeekOutput_Implementation(
	const class UFGFactoryConnectionComponent* connection, TArray<FInventoryItem>& out_items,
	TSubclassOf<UFGItemDescriptor> type) const
{
	UFGInventoryComponent* BufferInventory = GetBufferInventory();
	if (!IsValid(BufferInventory) || !IsValid(connection))
	{
		return false;
	}

	// Finde den Index der Connection über die Map
	int32 ConnectionIndex = INDEX_NONE;

	// Prüfe zuerst direkt in der Map (const_cast nötig weil Find nicht const ist für TMap)
	if (const int32* FoundIndex = mOutputConnectionToIndexMap.Find(connection))
	{
		ConnectionIndex = *FoundIndex;
	}
	// Falls nicht gefunden, prüfe die verbundene Connection
	else if (UFGFactoryConnectionComponent* ConnectedConnection = connection->GetConnection())
	{
		if (const int32* FoundConnectedIndex = mOutputConnectionToIndexMap.Find(ConnectedConnection))
		{
			ConnectionIndex = *FoundConnectedIndex;
		}
	}

	if (ConnectionIndex == INDEX_NONE)
	{
		return false;
	}

	// Output-Slot-Index = ConnectionIndex + 1 (Slot 0 ist Input-Buffer)
	int32 InventoryIdx = ConnectionIndex + 1;

	// Sicherheitsprüfung: InventoryIdx muss gültig sein
	if (InventoryIdx <= 0 || InventoryIdx >= BufferInventory->GetSizeLinear())
	{
		return false;
	}

	out_items.Empty();

	FInventoryStack Stack;
	if (!BufferInventory->GetStackFromIndex(InventoryIdx, Stack) || !Stack.HasItems())
	{
		return false;
	}

	// Prüfe ob das Item gültig ist
	if (!Stack.Item.IsValid() || !Stack.Item.GetItemClass())
	{
		return false;
	}

	// Wenn ein spezifischer Typ angefordert wird, prüfe ob es passt
	if (type && Stack.Item.GetItemClass() != type)
	{
		return false;
	}

	out_items.Add(Stack.Item);
	return true;
}

UFGFactoryConnectionComponent* AKPCLBuildableBalanceSplitter::GetOutputByName(FName ConnectionName) const
{
	UFGFactoryConnectionComponent* FoundConnection = nullptr;
	TArray<UFGFactoryConnectionComponent*> Components;
	GetComponents<UFGFactoryConnectionComponent>(Components);
	for (UFGFactoryConnectionComponent* Comp : Components)
	{
		if (Comp && Comp->GetFName() == ConnectionName)
		{
			FoundConnection = Comp;
			break;
		}
	}

	return FoundConnection;
}


FKPCLSplitterTimer* AKPCLBuildableBalanceSplitter::GetTimerForConnection(UFGFactoryConnectionComponent* Connection)
{
	// Phase 3: O(1) lookup via the map built in BeginPlay instead of a linear scan.
	if (!Connection)
	{
		return nullptr;
	}

	if (const int32* IndexPtr = mOutputConnectionToIndexMap.Find(Connection))
	{
		if (mOutputTimers.IsValidIndex(*IndexPtr))
		{
			return &mOutputTimers[*IndexPtr];
		}
	}
	return nullptr;
}

const FKPCLSplitterTimer*
AKPCLBuildableBalanceSplitter::GetTimerForConnection(UFGFactoryConnectionComponent* Connection) const
{
	// Phase 3: O(1) lookup via the map built in BeginPlay instead of a linear scan.
	if (!Connection)
	{
		return nullptr;
	}

	if (const int32* IndexPtr = mOutputConnectionToIndexMap.Find(Connection))
	{
		if (mOutputTimers.IsValidIndex(*IndexPtr))
		{
			return &mOutputTimers[*IndexPtr];
		}
	}
	return nullptr;
}

bool AKPCLBuildableBalanceSplitter::IsItemDefinedOnAnyOutput(TSubclassOf<UFGItemDescriptor> ItemClass) const
{
	if (!ItemClass)
	{
		return false;
	}

	for (const FKPCLSplitterTimer& Timer : mOutputTimers)
	{
		for (const TSubclassOf<UFGItemDescriptor>& FilteredItem : Timer.mFilteredItems)
		{
			if (!FilteredItem)
			{
				continue;
			}

			// Ignoriere spezielle Descriptor-Typen
			if (FilteredItem->IsChildOf(UFGWildCardDescriptor::StaticClass()) ||
				FilteredItem->IsChildOf(UFGOverflowDescriptor::StaticClass()) ||
				FilteredItem->IsChildOf(UFGNoneDescriptor::StaticClass()) ||
				FilteredItem->IsChildOf(UFGAnyUndefinedDescriptor::StaticClass()))
			{
				continue;
			}

			// Spezifischer Match gefunden
			if (FilteredItem == ItemClass)
			{
				return true;
			}
		}
	}

	return false;
}
