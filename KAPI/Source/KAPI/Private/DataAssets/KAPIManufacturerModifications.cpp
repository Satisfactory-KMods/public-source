#include "DataAssets/KAPIManufacturerModifications.h"

#include "Buildables/FGBuildableManufacturer.h"
#include "FGPipeConnectionComponent.h"
#include "KAPIModule.h"
#include "Logging/StructuredLog.h"
#include "Resources/FGNoneDescriptor.h"

bool UKAPIManufacturerModifications::ApplyInventoryChanges(AFGBuildableManufacturer* Manufacturer,
														   TSubclassOf<class UFGRecipe> recipe) const
{
	if (bDebug)
	{
		UE_LOGFMT(LogKApi, Log, "ApplyInventoryChanges: Manufacturer={0} Recipe={1}", GetNameSafe(Manufacturer),
				  GetNameSafe(recipe));
	}

	if (!recipe && IsValid(Manufacturer))
	{
		recipe = Manufacturer->GetCurrentRecipe();
	}

	if (!MatchManufacturer(Manufacturer))
	{
		if (bDebug)
		{
			UE_LOGFMT(LogKApi, Log,
					  "ApplyInventoryChanges: MatchManufacturer returned false for Manufacturer={0}, skipping.",
					  GetNameSafe(Manufacturer));
		}
		return false;
	}

	ApplyMoreInputSlots(Manufacturer, recipe);
	ApplyOutputSlots(Manufacturer, recipe);
	ApplyInputSlots(Manufacturer, recipe);

	if (bDebug)
	{
		UE_LOGFMT(LogKApi, Log, "ApplyInventoryChanges: Successfully applied changes to Manufacturer={0}.",
				  GetNameSafe(Manufacturer));
	}

	return true;
}

UTexture2D* UKAPIManufacturerModifications::GetInputSlotIcon(AFGBuildableManufacturer* Manufacturer,
															 TSubclassOf<class UFGRecipe> recipe, int32 SlotIndex,
															 bool IsSolid) const
{
	if (!IsValid(Manufacturer))
	{
		return nullptr;
	}

	UFGInventoryComponent* Inventory = Manufacturer->GetInputInventory();
	if (!IsValid(Inventory))
	{
		return nullptr;
	}
	int32 InvSize = Inventory->GetSizeLinear();

	int32 FluidCounter = 0;
	int32 SolidCounter = 0;

	for (int32 i = 0; i < FMath::Min(InvSize, SlotIndex); i++)
	{
		TSubclassOf<UFGItemDescriptor> AllowedItem = GetAllowedItemForInputSlot(recipe, Inventory, i, false);
		if (IsSolidItem(AllowedItem))
		{
			SolidCounter++;
		}
		else if (IsValid(AllowedItem))
		{
			FluidCounter++;
		}
	}

	if (IsSolid)
	{
		return mInput.mSolidSlotIcons.Contains(SolidCounter) ? mInput.mSolidSlotIcons[SolidCounter] : nullptr;
	}
	return mInput.mFluidSlotIcons.Contains(FluidCounter) ? mInput.mFluidSlotIcons[FluidCounter] : nullptr;
}

UTexture2D* UKAPIManufacturerModifications::GetOutputSlotIcon(AFGBuildableManufacturer* Manufacturer,
															  TSubclassOf<class UFGRecipe> recipe, int32 SlotIndex,
															  bool IsSolid) const
{
	if (!IsValid(Manufacturer))
	{
		return nullptr;
	}

	UFGInventoryComponent* Inventory = Manufacturer->GetOutputInventory();
	if (!IsValid(Inventory))
	{
		return nullptr;
	}
	int32 InvSize = Inventory->GetSizeLinear();

	int32 FluidCounter = 0;
	int32 SolidCounter = 0;

	for (int32 i = 0; i < FMath::Min(InvSize, SlotIndex); i++)
	{
		TSubclassOf<UFGItemDescriptor> AllowedItem = GetAllowedItemForInputSlot(recipe, Inventory, i, true);
		if (IsSolidItem(AllowedItem))
		{
			SolidCounter++;
		}
		else if (IsValid(AllowedItem))
		{
			FluidCounter++;
		}
	}

	if (IsSolid)
	{
		return mOutput.mSolidSlotIcons.Contains(SolidCounter) ? mOutput.mSolidSlotIcons[SolidCounter] : nullptr;
	}
	return mOutput.mFluidSlotIcons.Contains(FluidCounter) ? mOutput.mFluidSlotIcons[FluidCounter] : nullptr;
}

bool UKAPIManufacturerModifications::IsSolidItem(TSubclassOf<UFGItemDescriptor> Item)
{
	return IsValid(Item) ? UFGItemDescriptor::GetForm(Item) == EResourceForm::RF_SOLID : false;
}

bool UKAPIManufacturerModifications::MatchManufacturer(AFGBuildableManufacturer* Manufacturer) const
{
	if (!IsValid(mTargetClass) || !IsValid(Manufacturer))
	{
		if (bDebug)
		{
			UE_LOGFMT(LogKApi, Warning, "MatchManufacturer: Invalid TargetClass or Manufacturer={0}.",
					  GetNameSafe(Manufacturer));
		}
		return false;
	}

	UClass* Class = Manufacturer->GetClass();

	if (Class == mTargetClass)
	{
		if (bDebug)
		{
			UE_LOGFMT(LogKApi, Log, "MatchManufacturer: Exact match for Manufacturer={0} Class={1}.",
					  GetNameSafe(Manufacturer), GetNameSafe(Class));
		}
		return true;
	}

	if (bApplyOnSubclasses)
	{
		const bool bIsChild = Class->IsChildOf(mTargetClass);
		if (bDebug)
		{
			UE_LOGFMT(LogKApi, Log, "MatchManufacturer: Subclass check for Manufacturer={0} Class={1} -> {2}.",
					  GetNameSafe(Manufacturer), GetNameSafe(Class), bIsChild ? TEXT("true") : TEXT("false"));
		}
		return bIsChild;
	}

	if (bDebug)
	{
		UE_LOGFMT(LogKApi, Log, "MatchManufacturer: No match for Manufacturer={0} Class={1}.",
				  GetNameSafe(Manufacturer), GetNameSafe(Class));
	}
	return false;
}

void UKAPIManufacturerModifications::ApplyInputSlots(AFGBuildableManufacturer* Manufacturer,
													 TSubclassOf<class UFGRecipe> recipe) const
{
	if (!mInput.bEnabled)
	{
		if (bDebug)
		{
			UFGInventoryComponent* Inventory = Manufacturer->GetInputInventory();
			int32 Idx = 0;
			if (IsValid(Inventory))
			{
				for (FSimpleIngredientData MCachedIngredientData : Manufacturer->mCachedIngredientData)
				{
					UE_LOGFMT(LogKApi, Log,
							  "ApplyInputSlots: mInput.bEnabled CachedIngredientData - Size={0} Form={1} Idx={2} "
							  "SlotSize={3} AllowedItem={4}.",
							  MCachedIngredientData.StackSize, *UEnum::GetValueAsString(MCachedIngredientData.Form),
							  Idx, Inventory->GetSlotSize(Idx), GetNameSafe(Inventory->GetAllowedItemOnIndex(Idx)));
					Idx++;
				}
			}
		}
		if (bDebug)
		{
			UE_LOGFMT(LogKApi, Log, "ApplyInputSlots: Input modifications disabled, skipping Manufacturer={0}.",
					  GetNameSafe(Manufacturer));
		}
		return;
	}

	if (!IsValid(recipe))
	{
		recipe = Manufacturer->GetCurrentRecipe();
	}

	if (bDebug)
	{
		UE_LOGFMT(LogKApi, Log, "ApplyInputSlots: Manufacturer={0} Recipe={1}.", GetNameSafe(Manufacturer),
				  GetNameSafe(recipe));
	}

	UFGInventoryComponent* Inventory = Manufacturer->GetInputInventory();
	if (!IsValid(Inventory))
	{
		return;
	}
	int32 InvSize = Inventory->GetSizeLinear();

	if (bDebug)
	{
		int32 Idx = 0;
		for (FSimpleIngredientData MCachedIngredientData : Manufacturer->mCachedIngredientData)
		{
			UE_LOGFMT(LogKApi, Log,
					  "ApplyInputSlots: CachedIngredientData - Size={0} Form={1} Idx={2} SlotSize={3} AllowedItem={4}.",
					  MCachedIngredientData.StackSize, *UEnum::GetValueAsString(MCachedIngredientData.Form), Idx,
					  Inventory->GetSlotSize(Idx), GetNameSafe(Inventory->GetAllowedItemOnIndex(Idx)));
			Idx++;
		}
	}

	TArray<int32> FluidIndices;
	for (int32 i = 0; i < InvSize; i++)
	{
		TSubclassOf<UFGItemDescriptor> AllowedItem = GetAllowedItemForInputSlot(recipe, Inventory, i, false);
		int32 SlotSize = GetSlotSize(AllowedItem, mInput, Manufacturer->GetFluidInventoryStackSizeScalar());
		if (bDebug)
		{
			UE_LOGFMT(LogKApi, Log, "ApplyInputSlots: Slot={0} Item={1} Size={2}.", i, GetNameSafe(AllowedItem),
					  SlotSize);
		}
		if (SlotSize == INDEX_NONE)
		{
			Inventory->RemoveArbitrarySlotSize(i);
			continue;
		}
		Inventory->AddArbitrarySlotSize(i, SlotSize);

		if (Manufacturer->mCachedIngredientData.IsValidIndex(i))
		{
			Manufacturer->mCachedIngredientData[i].StackSize = SlotSize;
			Manufacturer->mCachedIngredientData[i].Form = UFGItemDescriptor::GetForm(AllowedItem);

			if (!IsSolidItem(AllowedItem))
			{
				FluidIndices.Add(i);
			}

			if (bDebug)
			{
				UE_LOGFMT(LogKApi, Log,
						  "ApplyInputSlots: Updated CachedIngredientData at index {0} with Size={1} Form={2}.", i,
						  Manufacturer->mCachedIngredientData[i].StackSize,
						  *UEnum::GetValueAsString(Manufacturer->mCachedIngredientData[i].Form));
			}
		}
		else
		{
			Manufacturer->mCachedIngredientData.Add(
				FSimpleIngredientData(SlotSize, UFGItemDescriptor::GetForm(AllowedItem)));
			UE_LOGFMT(LogKApi, Log, "ApplyInputSlots: Add CachedIngredientData at index {0} with Size={1} Form={2}.", i,
					  Manufacturer->mCachedIngredientData[i].StackSize,
					  *UEnum::GetValueAsString(Manufacturer->mCachedIngredientData[i].Form));
		}
	}

	for (int32 I = 0; I < FluidIndices.Num(); I++)
	{
		int32 Idx = FluidIndices[I];
		if (Manufacturer->mPipeInputConnections.IsValidIndex(I))
		{
			UFGPipeConnectionComponent* Connection = Manufacturer->mPipeInputConnections[I];
			if (!IsValid(Connection))
			{
				continue;
			}
			Connection->SetInventoryAccessIndex(Idx);
			if (bDebug)
			{
				UE_LOGFMT(
					LogKApi, Log,
					"ApplyInputSlots: Updated PipeConnectionComponent at index {0} with new InventoryAccessIndex={1}.",
					I, Idx);
			}
		}
	}

	if (bDebug)
	{
		int32 Idx = 0;
		for (FSimpleIngredientData MCachedIngredientData : Manufacturer->mCachedIngredientData)
		{
			UE_LOGFMT(
				LogKApi, Log,
				"ApplyInputSlots: CachedIngredientData AFTER - Size={0} Form={1} Idx={2} SlotSize={3} AllowedItem={4}.",
				MCachedIngredientData.StackSize, *UEnum::GetValueAsString(MCachedIngredientData.Form), Idx,
				Inventory->GetSlotSize(Idx), GetNameSafe(Inventory->GetAllowedItemOnIndex(Idx)));
			Idx++;
		}
	}
}

void UKAPIManufacturerModifications::ApplyMoreInputSlots(AFGBuildableManufacturer* Manufacturer,
														 TSubclassOf<class UFGRecipe> recipe) const
{
	if (!IsValid(recipe) || !bAllowMoreInputSlots)
	{
		if (bDebug)
		{
			UE_LOGFMT(LogKApi, Log,
					  "ApplyMoreInputSlots: Skipped for Manufacturer={0} (Recipe valid={1}, AllowMoreInputSlots={2}).",
					  GetNameSafe(Manufacturer), IsValid(recipe) ? TEXT("true") : TEXT("false"),
					  bAllowMoreInputSlots ? TEXT("true") : TEXT("false"));
		}
		return;
	}

	if (!IsValid(recipe))
	{
		recipe = Manufacturer->GetCurrentRecipe();
	}

	UFGInventoryComponent* Inventory = Manufacturer->GetInputInventory();
	if (!IsValid(Inventory))
	{
		return;
	}
	TArray<FItemAmount> Ingredients = UFGRecipe::GetIngredients(Manufacturer, recipe);

	if (bDebug)
	{
		UE_LOGFMT(LogKApi, Log, "ApplyMoreInputSlots: Manufacturer={0} Recipe={1} ResizingTo={2}.",
				  GetNameSafe(Manufacturer), GetNameSafe(recipe), FMath::Max(Ingredients.Num(), 1));
	}

	Manufacturer->AssignOutputAccessIndices(recipe);
	Manufacturer->mCurrentRecipe = recipe;
	int32 InputCount = Manufacturer->mFactoryInputConnections.Num() + Manufacturer->mPipeInputConnections.Num();
	int32 LastInventorySize = Inventory->GetSizeLinear();

	Inventory->Resize(FMath::Max(FMath::Max(Ingredients.Num(), InputCount), LastInventorySize));

	if (Ingredients.IsEmpty())
	{
		Inventory->SetAllowedItemOnIndex(0, UFGNoneDescriptor::StaticClass());
	}

	for (int32 i = 0; i < Ingredients.Num(); i++)
	{
		TSubclassOf<UFGItemDescriptor> AllowedItem = IsValid(Ingredients[i].ItemClass)
			? Ingredients[i].ItemClass
			: TSubclassOf<UFGItemDescriptor>(UFGNoneDescriptor::StaticClass());
		Inventory->SetAllowedItemOnIndex(i, AllowedItem);
	}
}

void UKAPIManufacturerModifications::ApplyOutputSlots(AFGBuildableManufacturer* Manufacturer,
													  TSubclassOf<class UFGRecipe> recipe) const
{
	if (!mOutput.bEnabled)
	{
		if (bDebug)
		{
			UE_LOGFMT(LogKApi, Log, "ApplyOutputSlots: Output modifications disabled, skipping Manufacturer={0}.",
					  GetNameSafe(Manufacturer));
		}
		return;
	}

	if (!IsValid(recipe))
	{
		recipe = Manufacturer->GetCurrentRecipe();
	}

	if (bDebug)
	{
		UE_LOGFMT(LogKApi, Log, "ApplyOutputSlots: Manufacturer={0} Recipe={1}.", GetNameSafe(Manufacturer),
				  GetNameSafe(recipe));
	}

	UFGInventoryComponent* Inventory = Manufacturer->GetOutputInventory();
	if (!IsValid(Inventory))
	{
		return;
	}
	int32 InvSize = Inventory->GetSizeLinear();

	for (int32 i = 0; i < InvSize; i++)
	{
		TSubclassOf<UFGItemDescriptor> AllowedItem = GetAllowedItemForInputSlot(recipe, Inventory, i, true);
		int32 SlotSize = GetSlotSize(AllowedItem, mOutput, Manufacturer->GetFluidInventoryStackSizeScalar());
		if (bDebug)
		{
			UE_LOGFMT(LogKApi, Log, "ApplyOutputSlots: Slot={0} Item={1} Size={2}.", i, GetNameSafe(AllowedItem),
					  SlotSize);
		}
		if (SlotSize == INDEX_NONE)
		{
			Inventory->RemoveArbitrarySlotSize(i);
			continue;
		}
		Inventory->AddArbitrarySlotSize(i, SlotSize);
	}
}

TSubclassOf<UFGItemDescriptor> UKAPIManufacturerModifications::GetAllowedItemForInputSlot(
	const TSubclassOf<UFGRecipe>& Recipe, UFGInventoryComponent* InventoryComponent, int32 SlotIndex, bool Product)
{
	if (!IsValid(Recipe) || !IsValid(InventoryComponent))
	{
		return nullptr;
	}

	TSubclassOf<UFGItemDescriptor> AllowedItem =
		InventoryComponent->IsValidIndex(SlotIndex) ? InventoryComponent->GetAllowedItemOnIndex(SlotIndex) : nullptr;

	if (!IsValid(AllowedItem))
	{
		TArray<FItemAmount> Ingredients =
			Product ? UFGRecipe::GetProducts(Recipe) : UFGRecipe::GetIngredients(InventoryComponent, Recipe);
		if (Ingredients.IsValidIndex(SlotIndex) && IsValid(Ingredients[SlotIndex].ItemClass))
		{
			AllowedItem = Ingredients[SlotIndex].ItemClass;
		}
	}

	return AllowedItem;
}

int32 UKAPIManufacturerModifications::GetSlotSize(const TSubclassOf<UFGItemDescriptor>& InItem,
												  const FKAPIManufacturerModificationData& Data,
												  int32 DefaultMultiplier)
{
	if (!IsValid(InItem) || !Data.bEnabled)
	{
		return INDEX_NONE;
	}

	int32 StackSize = UFGItemDescriptor::GetStackSize(InItem);
	float Multiplier = static_cast<float>(DefaultMultiplier);
	switch (UFGItemDescriptor::GetForm(InItem))
	{
	case EResourceForm::RF_SOLID:
		Multiplier = Data.mSolidMultiplier;
		break;
	case EResourceForm::RF_LIQUID:
		Multiplier = Data.mFluidMultiplier;
		break;
	case EResourceForm::RF_GAS:
		Multiplier = Data.mGasMultiplier;
		break;
	default:
		break;
	}

	return FMath::TruncToInt32(static_cast<float>(StackSize) * Multiplier);
}
