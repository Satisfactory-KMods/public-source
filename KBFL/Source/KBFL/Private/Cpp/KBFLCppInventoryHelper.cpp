// Fill out your copyright notice in the Description page of Project Settings.


#include "Cpp/KBFLCppInventoryHelper.h"

#include "FGInventoryLibrary.h"


bool UKBFLCppInventoryHelper::CanStoreItem(UFGInventoryComponent* Inventory, int InvIndex,
										   TSubclassOf<UFGItemDescriptor> ItemClass, int Amount)
{
	if (Inventory)
	{
		if (!Inventory->IsValidIndex(InvIndex))
		{
			return false;
		}

		FInventoryStack lStack;
		Inventory->GetStackFromIndex(InvIndex, lStack);
		Inventory->SetLocked(false);
		if (lStack.HasItems())
		{
			if (ItemClass != lStack.Item.GetItemClass())
			{
				return false;
			}

			if (!Inventory->IsItemAllowed(ItemClass, InvIndex))
			{
				return false;
			}

			const int StackSize = Inventory->GetSlotSize(InvIndex);
			if (lStack.NumItems >= StackSize || (lStack.NumItems + Amount) > StackSize)
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}

	return Inventory->IsItemAllowed(ItemClass, InvIndex);
}

bool UKBFLCppInventoryHelper::CanStoreItemStackOnIndex(UFGInventoryComponent* Inventory, int InvIndex,
													   FInventoryStack Stack)
{
	if (Inventory && InvIndex >= 0)
	{
		return CanStoreItem(Inventory, InvIndex, Stack.Item.GetItemClass(), Stack.NumItems);
	}
	return false;
}

bool UKBFLCppInventoryHelper::CanStoreItemStacksOnIndex(UFGInventoryComponent* Inventory, int InvIndex,
														TArray<FInventoryStack> Stacks)
{
	if (Inventory && InvIndex >= 0 && Stacks.Num() > 0)
	{
		for (auto Stack : Stacks)
		{
			if (!CanStoreItemStackOnIndex(Inventory, InvIndex, Stack))
			{
				return false;
			}
		}
	}
	if (!Inventory || InvIndex < 0)
	{
		return false;
	}
	return true;
}

bool UKBFLCppInventoryHelper::CanStoreItemStacks(UFGInventoryComponent* Inventory, TArray<FInventoryStack> Stacks)
{
	if (Inventory && Stacks.Num() > 0)
	{
		for (auto Stack : Stacks)
		{
			if (!Stack.HasItems())
			{
				return false;
			}
			if (!Inventory->HasEnoughSpaceForStack(Stack))
			{
				return false;
			}
		}
	}
	if (!Inventory)
	{
		return false;
	}
	return true;
}

bool UKBFLCppInventoryHelper::CanStoreItemStack(UFGInventoryComponent* Inventory, FInventoryStack Stack)
{
	if (Inventory && Stack.HasItems())
	{
		return Inventory->HasEnoughSpaceForStack(Stack);
	}
	return false;
}


void UKBFLCppInventoryHelper::StoreItemStackInInventory(UFGInventoryComponent* inventory, const int InvIndex,
														const FInventoryStack ItemStack)
{
	if (ItemStack.HasItems() == false)
	{
		return;
	}

	StoreItemAmountInInventory(inventory, InvIndex, ItemStack.Item.GetItemClass(), ItemStack.NumItems);
}

void UKBFLCppInventoryHelper::PullBelt(UFGInventoryComponent* Inventory, UFGFactoryConnectionComponent* BeltInput)
{
	if (!IsValid(BeltInput) || !IsValid(Inventory))
	{
		return;
	}
	if (!BeltInput->IsConnected())
	{
		return;
	}

	TArray<FInventoryItem> Items;
	if (BeltInput->Factory_PeekOutput(Items))
	{
		for (FInventoryItem InventoryItem : Items)
		{
			if (!IsValid(InventoryItem.GetItemClass()))
			{
				continue;
			}

			if (Inventory->HasEnoughSpaceForItem(InventoryItem))
			{
				FInventoryItem Item;
				float offset;
				if (BeltInput->Factory_GrabOutput(Item, offset, InventoryItem.GetItemClass()))
				{
					FInventoryStack itemStack;
					itemStack.NumItems = 1;
					itemStack.Item = Item;
					Inventory->AddStack(itemStack, true);
				}
			}
		}
	}
}

void UKBFLCppInventoryHelper::AddItemsInInventory(UFGInventoryComponent* inventory,
												  TSubclassOf<UFGItemDescriptor> itemClass, int Amount)
{
	if (inventory)
	{
		FInventoryItem outItem;
		outItem.SetItemClass(itemClass);

		FInventoryStack itemStack;
		itemStack.NumItems = Amount;
		itemStack.Item = outItem;
		if (inventory->HasEnoughSpaceForStack(itemStack))
		{
			inventory->AddStack(itemStack, true);
		}
	}
}

void UKBFLCppInventoryHelper::AddStackInInventory(UFGInventoryComponent* inventory, FInventoryStack Stack)
{
	AddItemsInInventory(inventory, Stack.Item.GetItemClass(), Stack.NumItems);
}

void UKBFLCppInventoryHelper::AddStacksInInventory(UFGInventoryComponent* inventory, TArray<FInventoryStack> Stacks)
{
	if (Stacks.Num() > 0)
	{
		for (FInventoryStack Stack : Stacks)
		{
			AddItemsInInventory(inventory, Stack.Item.GetItemClass(), Stack.NumItems);
		}
	}
}

bool UKBFLCppInventoryHelper::HasItems(UFGInventoryComponent* inventory, TSubclassOf<UFGItemDescriptor> itemClass,
									   int Amount)
{
	if (inventory && itemClass && Amount > 0)
	{
		return inventory->HasItems(itemClass, Amount);
	}
	return false;
}

void UKBFLCppInventoryHelper::GatherRefundFromInventory(UFGInventoryComponent* inventory,
														TArray<FInventoryStack>& out_refund)
{
	if (inventory)
	{
		TArray<FInventoryStack> stacks;
		inventory->GetInventoryStacks(stacks);
		UFGInventoryLibrary::RemoveAllItemsNotOfResourceForm(stacks);

		for (FInventoryStack stack : stacks)
		{
			if (stack.HasItems())
			{
				out_refund.Add(stack);
			}
		}
	}
}

void UKBFLCppInventoryHelper::StoreItemAmountInInventory(UFGInventoryComponent* inventory, int InvIndex,
														 TSubclassOf<UFGItemDescriptor> itemClass, int amount)
{
	if (inventory)
	{
		if (CanStoreItem(inventory, InvIndex, itemClass, amount))
		{
			FInventoryItem loutItem;
			loutItem.SetItemClass(itemClass);

			FInventoryStack litemStack;
			litemStack.NumItems = amount;
			litemStack.Item = loutItem;
			inventory->AddStackToIndex(InvIndex, litemStack, true);
		}
		else
		{
			FInventoryStack lout_stack;
			inventory->GetStackFromIndex(InvIndex, lout_stack);
			const int stackSize = inventory->GetSlotSize(InvIndex);

			const int quantity = stackSize - lout_stack.NumItems;

			if (CanStoreItem(inventory, InvIndex, itemClass, quantity))
			{
				StoreItemAmountInInventory(inventory, InvIndex, itemClass, quantity);
			}
		}
	}
}

void UKBFLCppInventoryHelper::PushPipe(UFGInventoryComponent* Inventory, int InventoryIndex, float dt,
									   UFGPipeConnectionFactory* PipeOutput)
{
	if (Inventory)
	{
		if (PipeOutput && PipeOutput->IsConnected())
		{
			FInventoryStack FluidStack;
			Inventory->GetStackFromIndex(InventoryIndex, FluidStack);

			if (FluidStack.NumItems > 400)
			{
				FluidStack.NumItems = 400;
			}

			if (FluidStack.HasItems())
			{
				int32 amount = PipeOutput->Factory_PushPipeOutput(dt, FluidStack);
				if (amount > 0)
				{
					Inventory->RemoveFromIndex(InventoryIndex, amount);
				}
			}
		}
	}
}

void UKBFLCppInventoryHelper::PullPipe(UFGInventoryComponent* Inventory, int InventoryIndex, float dt,
									   TSubclassOf<UFGItemDescriptor> AllowedItem,
									   UFGPipeConnectionFactory* PipeInputComp)
{
	int32 PullAmount = 400;

	// check is connection set
	if (PipeInputComp && Inventory && AllowedItem)
	{
		// check is connected
		if (PipeInputComp->IsConnected())
		{
			FInventoryStack Stack;

			// check can inventory store the item
			UFGPipeConnectionComponent* Con = Cast<UFGPipeConnectionComponent>(PipeInputComp->GetConnection());
			// cast success
			if (Con)
			{
				TSubclassOf<UFGItemDescriptor> FluidItem = Con->GetFluidDescriptor();
				// if same fluid, has Fluid etc
				if (Con->HasFluidIntegrant() && Con->GetFluidIntegrant()->GetFluidBox() && FluidItem == AllowedItem)
				{
					// Pull on Max
					FInventoryStack StackControl;
					Inventory->GetStackFromIndex(InventoryIndex, StackControl);
					int SlotSize = Inventory->GetSlotSize(InventoryIndex, FluidItem);

					if (StackControl.HasItems())
					{
						PullAmount = FMath::Min(PullAmount, SlotSize - StackControl.NumItems);
					}
					else
					{
						PullAmount = FMath::Min(PullAmount, SlotSize);
					}

					if (CanStoreItem(Inventory, InventoryIndex, FluidItem, PullAmount))
					{
						// Pull success
						if (PipeInputComp->Factory_PullPipeInput(dt, Stack, AllowedItem, PullAmount))
						{
							// add pulled item into Inventory
							Inventory->AddStackToIndex(InventoryIndex, Stack, false);
						}
					}
				}
			}
		}
	}
}

void UKBFLCppInventoryHelper::PullPipe(UFGInventoryComponent* Inventory, int InventoryIndex, float dt,
									   TArray<TSubclassOf<UFGItemDescriptor>> AllowedItem,
									   UFGPipeConnectionFactory* PipeInputComp)
{
	// look if the inventory has already an item on index and pull only this.
	FInventoryStack Stack;
	Inventory->GetStackFromIndex(InventoryIndex, Stack);
	if (Stack.HasItems())
	{
		if (AllowedItem.Contains(Stack.Item.GetItemClass()))
		{
			PullPipe(Inventory, InventoryIndex, dt, Stack.Item.GetItemClass(), PipeInputComp);
		}
		return;
	}

	for (const auto ItemClass : AllowedItem)
	{
		PullPipe(Inventory, InventoryIndex, dt, ItemClass, PipeInputComp);
	}
}

void UKBFLCppInventoryHelper::PullBelt(UFGInventoryComponent* Inventory, int InventoryIndex, float dt,
									   TSubclassOf<UFGItemDescriptor> AllowedItem,
									   UFGFactoryConnectionComponent* BeltInput)
{
	if (BeltInput)
	{
		if (BeltInput->IsConnected())
		{
			if (CanStoreItem(Inventory, InventoryIndex, AllowedItem, 1))
			{
				FInventoryItem Item;
				float offset = 0.0f;
				uint8 maxGrab = FMath::Min(static_cast<uint8>(1), BeltInput->MaxNumGrab(dt));
				for (uint8 i = 0; i < maxGrab; i++)
				{
					bool pulledItem = BeltInput->Factory_GrabOutput(Item, offset, AllowedItem);
					if (pulledItem)
					{
						StoreItemAmountInInventory(Inventory, InventoryIndex, AllowedItem);
					}
				}
			}
		}
	}
}

void UKBFLCppInventoryHelper::PullBelt(UFGInventoryComponent* Inventory, int InventoryIndex, float dt,
									   TArray<TSubclassOf<UFGItemDescriptor>> AllowedItem,
									   UFGFactoryConnectionComponent* BeltInput)
{
	// look if the inventory has already an item on index and pull only this.
	FInventoryStack Stack;
	Inventory->GetStackFromIndex(InventoryIndex, Stack);
	if (Stack.HasItems())
	{
		if (AllowedItem.Contains(Stack.Item.GetItemClass()))
		{
			PullBelt(Inventory, InventoryIndex, dt, Stack.Item.GetItemClass(), BeltInput);
		}
		return;
	}

	for (auto Item : AllowedItem)
	{
		PullBelt(Inventory, InventoryIndex, dt, Item, BeltInput);
	}
}

void UKBFLCppInventoryHelper::PullBeltChildClass(UFGInventoryComponent* Inventory, int InventoryIndex,
												 TSubclassOf<UFGItemDescriptor> AllowedItemClass,
												 UFGFactoryConnectionComponent* BeltInput)
{
	if (BeltInput && Inventory && AllowedItemClass)
	{
		if (BeltInput->IsConnected())
		{
			TArray<FInventoryItem> Items;
			if (BeltInput->Factory_PeekOutput(Items))
			{
				for (FInventoryItem InventoryItem : Items)
				{
					if (InventoryItem.GetItemClass())
					{
						if (InventoryItem.GetItemClass()->IsChildOf(AllowedItemClass) &&
							CanStoreItem(Inventory, InventoryIndex, InventoryItem.GetItemClass()))
						{
							FInventoryItem Item;
							float offset;
							if (BeltInput->Factory_GrabOutput(Item, offset, InventoryItem.GetItemClass()))
							{
								StoreItemAmountInInventory(Inventory, InventoryIndex, InventoryItem.GetItemClass());
							}
						}
					}
				}
			}
		}
	}
}

void UKBFLCppInventoryHelper::PullBeltChildClass(UFGInventoryComponent* Inventory,
												 TSubclassOf<UFGItemDescriptor> AllowedItemClass,
												 UFGFactoryConnectionComponent* BeltInput)
{
	if (!IsValid(BeltInput) || !IsValid(Inventory) || !IsValid(AllowedItemClass))
	{
		return;
	}
	if (!BeltInput->IsConnected())
	{
		return;
	}

	TArray<FInventoryItem> Items;
	if (BeltInput->Factory_PeekOutput(Items))
	{
		for (FInventoryItem InventoryItem : Items)
		{
			if (!IsValid(InventoryItem.GetItemClass()))
			{
				return;
			}

			if (InventoryItem.GetItemClass()->IsChildOf(AllowedItemClass) &&
				Inventory->HasEnoughSpaceForItem(InventoryItem))
			{
				FInventoryItem Item;
				float offset;
				if (BeltInput->Factory_GrabOutput(Item, offset, InventoryItem.GetItemClass()))
				{
					AddItemsInInventory(Inventory, InventoryItem.GetItemClass());
				}
			}
		}
	}
}

void UKBFLCppInventoryHelper::PullBeltChildClass(UFGInventoryComponent* Inventory, int InventoryIndex, float dt,
												 TSubclassOf<UFGItemDescriptor> AllowedItemClass,
												 UFGFactoryConnectionComponent* BeltInput)
{
	if (BeltInput && Inventory && AllowedItemClass)
	{
		if (BeltInput->IsConnected())
		{
			TArray<FInventoryItem> Items;
			if (BeltInput->Factory_PeekOutput(Items))
			{
				for (FInventoryItem InventoryItem : Items)
				{
					if (InventoryItem.GetItemClass())
					{
						if (InventoryItem.GetItemClass()->IsChildOf(AllowedItemClass) &&
							CanStoreItem(Inventory, InventoryIndex, InventoryItem.GetItemClass()))
						{
							FInventoryItem Item;
							float offset;
							uint8 maxGrab = FMath::Min(static_cast<uint8>(1), BeltInput->MaxNumGrab(dt));
							for (uint8 i = 0; i < maxGrab; i++)
							{
								if (BeltInput->Factory_GrabOutput(Item, offset, InventoryItem.GetItemClass()))
								{
									StoreItemAmountInInventory(Inventory, InventoryIndex, InventoryItem.GetItemClass());
								}
							}
						}
					}
				}
			}
		}
	}
}

void UKBFLCppInventoryHelper::PullBeltChildClass(UFGInventoryComponent* Inventory, float dt,
												 TSubclassOf<UFGItemDescriptor> AllowedItemClass,
												 UFGFactoryConnectionComponent* BeltInput)
{
	if (!IsValid(BeltInput) || !IsValid(Inventory) || !IsValid(AllowedItemClass))
	{
		return;
	}
	if (!BeltInput->IsConnected())
	{
		return;
	}

	TArray<FInventoryItem> Items;
	if (BeltInput->Factory_PeekOutput(Items))
	{
		for (FInventoryItem InventoryItem : Items)
		{
			if (!IsValid(InventoryItem.GetItemClass()))
			{
				return;
			}

			if (InventoryItem.GetItemClass()->IsChildOf(AllowedItemClass) &&
				Inventory->HasEnoughSpaceForItem(InventoryItem))
			{
				FInventoryItem Item;
				float offset;
				uint8 maxGrab = FMath::Min(static_cast<uint8>(1), BeltInput->MaxNumGrab(dt));
				for (uint8 i = 0; i < maxGrab; i++)
				{
					if (BeltInput->Factory_GrabOutput(Item, offset, InventoryItem.GetItemClass()))
					{
						AddItemsInInventory(Inventory, InventoryItem.GetItemClass());
					}
				}
			}
		}
	}
}

void UKBFLCppInventoryHelper::PullAllFromPipe(UFGInventoryComponent* Inventory, int InventoryIndex, float dt,
											  UFGPipeConnectionFactory* PipeInputComp)
{
	bool Debug = false;
	int32 PullAmount = 400;
	// check is connection set
	if (PipeInputComp && Inventory)
	{
		// check is connected
		if (PipeInputComp->IsConnected())
		{
			if (UFGPipeConnectionComponent* Con = Cast<UFGPipeConnectionComponent>(PipeInputComp->GetConnection()))
			{
				TSubclassOf<UFGItemDescriptor> FluidItem = Con->GetFluidDescriptor();
				if (!FluidItem)
				{
					return;
				}

				FInventoryStack Stack;

				// Pull on Max
				FInventoryStack StackControl;
				Inventory->GetStackFromIndex(InventoryIndex, StackControl);
				int SlotSize = Inventory->GetSlotSize(InventoryIndex, FluidItem);

				if (StackControl.HasItems())
				{
					PullAmount = FMath::Min(PullAmount, SlotSize - StackControl.NumItems);
				}
				else
				{
					PullAmount = FMath::Min(PullAmount, SlotSize);
				}

				if (PullAmount == 200)
				{
					PullAmount = 199;
				}

				if (CanStoreItem(Inventory, InventoryIndex, FluidItem, PullAmount))
				{
					// Pull
					const bool PulledItem = PipeInputComp->Factory_PullPipeInput(dt, Stack, FluidItem, PullAmount);

					// Pull pull success
					if (PulledItem)
					{
						// add pulled item into Inventory
						Inventory->AddStackToIndex(InventoryIndex, Stack, false);
					}
				}
			}
			else if (Debug)
			{
				UE_LOG(LogTemp, Error, TEXT("PullAllFromPipe, Connection is not a UFGPipeConnectionComponent"));
			}
		}
		else if (Debug)
		{
			UE_LOG(LogTemp, Error, TEXT("PullAllFromPipe, Pipe is not connected!"));
		}
	}
	else if (Debug)
	{
		UE_LOG(LogTemp, Error, TEXT("PullAllFromPipe, invalid connection (%d) or inventory (%d)!"),
			   PipeInputComp != nullptr, Inventory != nullptr);
	}
}
