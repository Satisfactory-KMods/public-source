#pragma once

#include "CoreMinimal.h"

#include "FGFactoryConnectionComponent.h"
#include "FGInventoryComponent.h"
#include "FGPipeConnectionFactory.h"

#include "KBFLCppInventoryHelper.generated.h"

UCLASS()
class KBFL_API UKBFLCppInventoryHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** This function check if can Store an Item in a Inventory */
	UFUNCTION(BlueprintPure, Category = "KMods|CppInventoryHelper")
	static bool CanStoreItem(UFGInventoryComponent* Inventory, int InvIndex, TSubclassOf<UFGItemDescriptor> ItemClass,
							 int Amount = 1);

	/** This function check if can Store an Item in a Inventory */
	UFUNCTION(BlueprintPure, Category = "KMods|CppInventoryHelper")
	static bool CanStoreItemStackOnIndex(UFGInventoryComponent* Inventory, int InvIndex, FInventoryStack Stack);

	/** This function check if can Store an Item in a Inventory */
	UFUNCTION(BlueprintPure, Category = "KMods|CppInventoryHelper")
	static bool CanStoreItemStacksOnIndex(UFGInventoryComponent* Inventory, int InvIndex,
										  TArray<FInventoryStack> Stacks);

	/** This function check if can Store an Item in a Inventory */
	UFUNCTION(BlueprintPure, Category = "KMods|CppInventoryHelper")
	static bool CanStoreItemStacks(UFGInventoryComponent* Inventory, TArray<FInventoryStack> Stacks);

	/** This function check if can Store an Item in a Inventory */
	UFUNCTION(BlueprintPure, Category = "KMods|CppInventoryHelper")
	static bool CanStoreItemStack(UFGInventoryComponent* Inventory, FInventoryStack Stack);

	/** This function can Store an Item in a Inventory */
	UFUNCTION(BlueprintCallable, Category = "KMods|CppInventoryHelper")
	static void StoreItemAmountInInventory(UFGInventoryComponent* inventory, int InvIndex,
										   TSubclassOf<UFGItemDescriptor> itemClass, int amount = 1);

	/** This function can Store an Item in a Inventory */
	UFUNCTION(BlueprintCallable, Category = "KMods|CppInventoryHelper")
	static void StoreItemStackInInventory(UFGInventoryComponent* inventory, int InvIndex, FInventoryStack ItemStack);

	/** This function can Store an Item in a Inventory */
	UFUNCTION(BlueprintCallable, Category = "KMods|CppInventoryHelper")
	static void AddItemsInInventory(UFGInventoryComponent* inventory, TSubclassOf<UFGItemDescriptor> itemClass,
									int Amount = 1);

	/** This function can Store an Item in a Inventory */
	UFUNCTION(BlueprintCallable, Category = "KMods|CppInventoryHelper")
	static void AddStackInInventory(UFGInventoryComponent* inventory, FInventoryStack Stack);

	/** This function can Store an Item in a Inventory */
	UFUNCTION(BlueprintCallable, Category = "KMods|CppInventoryHelper")
	static void AddStacksInInventory(UFGInventoryComponent* inventory, TArray<FInventoryStack> Stacks);

	/** This function can Store an Item in a Inventory */
	UFUNCTION(BlueprintPure, Category = "KMods|CppInventoryHelper")
	static bool HasItems(UFGInventoryComponent* inventory, TSubclassOf<UFGItemDescriptor> itemClass, int Amount = 1);

	static void GatherRefundFromInventory(UFGInventoryComponent* inventory, TArray<FInventoryStack>& out_refund);

	static void PushPipe(UFGInventoryComponent* Inventory, int InventoryIndex, float dt,
						 UFGPipeConnectionFactory* PipeOutput);
	static void PullPipe(UFGInventoryComponent* Inventory, int InventoryIndex, float dt,
						 TSubclassOf<UFGItemDescriptor> AllowedItem, UFGPipeConnectionFactory* PipeInputComp);
	static void PullPipe(UFGInventoryComponent* Inventory, int InventoryIndex, float dt,
						 TArray<TSubclassOf<UFGItemDescriptor>> AllowedItem, UFGPipeConnectionFactory* PipeInputComp);
	static void PullBelt(UFGInventoryComponent* Inventory, int InventoryIndex, float dt,
						 TSubclassOf<UFGItemDescriptor> AllowedItem, UFGFactoryConnectionComponent* BeltInput);
	static void PullBelt(UFGInventoryComponent* Inventory, int InventoryIndex, float dt,
						 TArray<TSubclassOf<UFGItemDescriptor>> AllowedItem, UFGFactoryConnectionComponent* BeltInput);
	void PullBeltChildClass(UFGInventoryComponent* Inventory, int InventoryIndex,
							TSubclassOf<UFGItemDescriptor> AllowedItemClass, UFGFactoryConnectionComponent* BeltInput);

	void PullBeltChildClass(UFGInventoryComponent* Inventory, TSubclassOf<UFGItemDescriptor> AllowedItemClass,
							UFGFactoryConnectionComponent* BeltInput);
	/** Pull Items with ChildClass Ref */
	static void PullBeltChildClass(UFGInventoryComponent* Inventory, int InventoryIndex, float dt,
								   TSubclassOf<UFGItemDescriptor> AllowedItemClass,
								   UFGFactoryConnectionComponent* BeltInput);
	static void PullBeltChildClass(UFGInventoryComponent* Inventory, float dt,
								   TSubclassOf<UFGItemDescriptor> AllowedItemClass,
								   UFGFactoryConnectionComponent* BeltInput);
	static void PullBelt(UFGInventoryComponent* Inventory, UFGFactoryConnectionComponent* BeltInput);

	/** Pull Items with ChildClass Ref */
	static void PullAllFromPipe(UFGInventoryComponent* Inventory, int InventoryIndex, float dt,
								UFGPipeConnectionFactory* PipeInputComp);
};
