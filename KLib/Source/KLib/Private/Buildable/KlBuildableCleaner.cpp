#include "Buildable/KlBuildableCleaner.h"

#include "BFL/KBFL_Player.h"
#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Cpp/KBFLCppInventoryHelper.h"
#include "FGFactoryConnectionComponent.h"
#include "FGPlayerController.h"
#include "Logging.h"

#include "Net/UnrealNetwork.h"

#include "Resources/FGNoneDescriptor.h"
#include "Subsystem/KPCLUnlockSubsystem.h"

AKLBuildableCleaner::AKLBuildableCleaner() : Super()
{
	mBeltOutput = CreateDefaultSubobject<UFGFactoryConnectionComponent>(TEXT("mBeltOutput"));
	mBeltOutput->SetupAttachment(RootComponent);
	mBeltOutput->SetDirection(EFactoryConnectionDirection::FCD_OUTPUT);

	mBeltInput = CreateDefaultSubobject<UFGFactoryConnectionComponent>(TEXT("mBeltInput"));
	mBeltInput->SetupAttachment(RootComponent);
	mBeltInput->SetDirection(EFactoryConnectionDirection::FCD_INPUT);

	mPipeInput = CreateDefaultSubobject<UFGPipeConnectionFactory>(TEXT("mPipeInput"));
	mPipeInput->SetupAttachment(RootComponent);
	mPipeInput->SetPipeConnectionType(EPipeConnectionType::PCT_CONSUMER);

	mPipeOutput = CreateDefaultSubobject<UFGPipeConnectionFactory>(TEXT("mPipeOutput"));
	mPipeOutput->SetupAttachment(RootComponent);
	mPipeOutput->SetPipeConnectionType(EPipeConnectionType::PCT_PRODUCER);

	mInputInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::InputName);
	mInputInventory->SetIsReplicated(true);

	mOutputInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::OutputName);
	mOutputInventory->SetIsReplicated(true);

	mInputInventory->SetDefaultSize(3);
	mOutputInventory->SetDefaultSize(10);
	bEnableCustomOverclocking = true;
}

void AKLBuildableCleaner::Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo)
{
	Super::Overclocking_GetInfo_Implementation(OutProductionInfo);

	if (GetCurrentCleanerRecipe())
	{
		OutProductionInfo.mItemClass = GetCurrentCleanerRecipe()->mInFluid.ItemClass;
		OutProductionInfo.mAmount = GetCurrentCleanerRecipe()->mInFluid.Amount;
		OutProductionInfo.mDefaultProductionTime = GetCurrentCleanerRecipe()->mProductionTime;
	}
}

bool AKLBuildableCleaner::Overclocking_IsConsumer_Implementation() { return true; }

void AKLBuildableCleaner::UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots)
{
	Super::UI_ApplyRelevantItems_Implementation(OutSlots);

	if (IsValid(GetCurrentCleanerRecipe()))
	{
		if (GetCurrentCleanerRecipe()->CleanerItemNeeded())
		{
			OutSlots.AddUnique(GetCurrentCleanerRecipe()->mCleanerItemInfo.mProduceItem);
		}
		for (FKAPICleanerInfo Product : GetCurrentCleanerRecipe()->mBypassProducts)
		{
			if (!IsValid(Product.mProduceItem))
			{
				continue;
			}
			OutSlots.AddUnique(Product.mProduceItem);
		}
	}
}

bool AKLBuildableCleaner::CanUseFactoryClipboard_Implementation() { return true; }

UFGFactoryClipboardSettings* AKLBuildableCleaner::CopySettings_Implementation()
{
	UKLCleanerClipboardSettings* Settings = NewObject<UKLCleanerClipboardSettings>();
	WriteClipboardSettings(Settings);
	Settings->mCurrentCleanerRecipe = GetCurrentCleanerRecipe();
	return Settings;
}

bool AKLBuildableCleaner::PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
													   class AFGPlayerController* player)
{
	bool Result = Super::PasteSettings_Implementation(factoryClipboard, player);
	if (UKLCleanerClipboardSettings* Settings = Cast<UKLCleanerClipboardSettings>(factoryClipboard))
	{
		if (CanSetCleanerRecipe(Settings->mCurrentCleanerRecipe))
		{
			SetCleanerRecipe(Settings->mCurrentCleanerRecipe);
		}

		if (!Result)
		{
			Result = true;
		}
	}

	return Result;
}

void AKLBuildableCleaner::Overclocking_GetProductionResults_Implementation(
	TArray<FKPCLOverclockingProductionResults>& OutIngredients, TArray<FKPCLOverclockingProductionResults>& OutProducts)
{
	Super::Overclocking_GetProductionResults_Implementation(OutIngredients, OutProducts);

	UKAPICleanerItemDescription* CurrentRecipe = GetCurrentCleanerRecipe();
	if (IsValid(CurrentRecipe))
	{
		OutIngredients.Add(
			FKPCLOverclockingProductionResults(CurrentRecipe->mInFluid, mProductionHandle.mProductionTime));
		if (IsValid(CurrentRecipe->mOutFluid.ItemClass) &&
			!CurrentRecipe->mOutFluid.ItemClass->IsChildOf(UFGNoneDescriptor::StaticClass()))
		{
			OutProducts.Add(
				FKPCLOverclockingProductionResults(CurrentRecipe->mOutFluid, mProductionHandle.mProductionTime));
		}
		if (CurrentRecipe->CleanerItemNeeded())
		{
			OutIngredients.Add(FKPCLOverclockingProductionResults(CurrentRecipe->mCleanerItemInfo.mProduceItem,
																  CurrentRecipe->mCleanerItemInfo.mProduceAmount,
																  CurrentRecipe->mCleanerItemInfo.mProductionTime));
		}

		for (FKAPICleanerInfo CleanerInfo : CurrentRecipe->mBypassProducts)
		{
			OutProducts.Add(FKPCLOverclockingProductionResults(CleanerInfo.mProduceItem, CleanerInfo.mProduceAmount,
															   CleanerInfo.mProductionTime));
		}
	}
}

TArray<UKAPICleanerItemDescription*> AKLBuildableCleaner::GetAllCleanerRecipes() const
{
	AKLUnlockSubsystem* UnlockSubsystem = AKLUnlockSubsystem::Get(GetWorld());
	if (!IsValid(UnlockSubsystem))
	{
		return TArray<UKAPICleanerItemDescription*>();
	}
	return UnlockSubsystem->GetAllUnlockedCleanerDesc();
}

UKAPICleanerItemDescription* AKLBuildableCleaner::GetCurrentCleanerRecipe() const { return mCurrentCleanerRecipe; }

void AKLBuildableCleaner::SetCleanerRecipe(UKAPICleanerItemDescription* NewCleanerInfo)
{
	if (mCurrentCleanerRecipe != NewCleanerInfo && CanSetCleanerRecipe(NewCleanerInfo))
	{
		mCurrentCleanerRecipe = NewCleanerInfo;
		ApplyRecipeSettings();
	}
}

bool AKLBuildableCleaner::CanSetCleanerRecipe(UKAPICleanerItemDescription* NewCleanerInfo) const
{
	if (!IsValid(NewCleanerInfo))
	{
		return false;
	}

	return GetAllCleanerRecipes().Contains(NewCleanerInfo);
}

void AKLBuildableCleaner::SetPendingPotential(float newPendingPotential)
{
	newPendingPotential = FMath::Clamp(newPendingPotential, GetCurrentMinPotential(), GetCurrentMaxPotential());
	Super::SetPendingPotential(newPendingPotential);

	mCleanerItemHandler.mPendingPotential = newPendingPotential;
	for (FFullProductionHandle& Handle : mSlotProductionHandler)
	{
		Handle.mPendingPotential = newPendingPotential;
	}
}

bool AKLBuildableCleaner::ValidateRecipeSettings()
{
	UKAPICleanerItemDescription* CurrentInfo = GetCurrentCleanerRecipe();
	if (!IsValid(CurrentInfo) || !IsValid(GetOutputInventory()) || !IsValid(GetInventory()))
	{
		return true;
	}

	TSubclassOf<UFGItemDescriptor> AllowedItemCleaner = GetInventory()->GetAllowedItemOnIndex(CLEANER_ITEM_INDEX);
	if (CurrentInfo->CleanerItemNeeded() &&
		(!IsValid(AllowedItemCleaner) || AllowedItemCleaner != CurrentInfo->mCleanerItemInfo.mProduceItem))
	{
		return false;
	}

	TArray<FKAPICleanerInfo> CleanerInfos = CurrentInfo->mBypassProducts;
	if (GetOutputInventory()->GetSizeLinear() != MAX_BYPASS_SLOTS)
	{
		return false;
	}

	for (int i = 0; i < MAX_BYPASS_SLOTS; ++i)
	{
		TSubclassOf<UFGItemDescriptor> AllowedItem = GetInventory()->GetAllowedItemOnIndex(i);
		if (CleanerInfos.IsValidIndex(i))
		{
			if (AllowedItem != CleanerInfos[i].mProduceItem)
			{
				if (!mSlotProductionHandler.IsValidIndex(i))
				{
					return false;
				}
			}
			if (mSlotProductionHandler[i].mProductionTime != CleanerInfos[i].mProductionTime)
			{
				return false;
			}
		}
		else if (!IsValid(AllowedItem) || !AllowedItem->IsChildOf(UFGNoneDescriptor::StaticClass()))
		{
			return false;
		}
	}

	if (CurrentInfo->CleanerItemNeeded() &&
		mCleanerItemHandler.mProductionTime != CurrentInfo->mCleanerItemInfo.mProductionTime)
	{
		return false;
	}

	if (mProductionHandle.mProductionTime != GetCurrentCleanerRecipe()->mProductionTime)
	{
		return false;
	}

	return true;
}


void AKLBuildableCleaner::ApplyRecipeSettings()
{
	if (!HasAuthority())
	{
		return;
	}

	UKAPICleanerItemDescription* CurrentInfo = GetCurrentCleanerRecipe();
	if (IsValid(CurrentInfo))
	{
		// Flush Inventorys to make sure that we can start producing the new way
		GetInventory()->Empty();

		if (CurrentInfo->CleanerItemNeeded())
		{
			UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(
				GetInventory(), CLEANER_ITEM_INDEX, GetCurrentCleanerRecipe()->mCleanerItemInfo.mProduceItem);
		}
		else
		{
			UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), CLEANER_ITEM_INDEX,
																  UFGNoneDescriptor::StaticClass());
		}

		mSlotProductionHandler.Empty();
		TArray<FKAPICleanerInfo> CleanerInfos = GetCurrentCleanerRecipe()->mBypassProducts;

		GetOutputInventory()->Empty();
		GetOutputInventory()->Resize(MAX_BYPASS_SLOTS);
		for (int i = 0; i < MAX_BYPASS_SLOTS; ++i)
		{
			if (CleanerInfos.IsValidIndex(i))
			{
				UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetOutputInventory(), i,
																	  CleanerInfos[i].mProduceItem);

				FFullProductionHandle Handler;

				Handler.mPendingPotential = GetPendingPotential();
				Handler.mCurrentPotential = GetCurrentPotential();
				Handler.SetNewTime(CleanerInfos[i].mProductionTime, true);
				mSlotProductionHandler.Add(Handler);
			}
			else
			{
				UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetOutputInventory(), i,
																	  UFGNoneDescriptor::StaticClass());
			}
		}

		mCleanerItemHandler.SetNewTime(GetCurrentCleanerRecipe()->mCleanerItemInfo.mProductionTime, true);
		SetProductionTime(GetCurrentCleanerRecipe()->mProductionTime, true);
	}
}

void AKLBuildableCleaner::BeginPlay()
{
	Super::BeginPlay();

	mUnlockSubsystem = AKLUnlockSubsystem::Get(GetWorld());

	if (HasAuthority())
	{
		if (IsValid(mDefaultCleanerRecipe) && !IsValid(GetCurrentCleanerRecipe()))
		{
			SetCleanerRecipe(mDefaultCleanerRecipe);
		}

		FInventoryStack Stack;
		GetInventory()->GetStackFromIndex(FLUID_INPUT_INDEX, Stack);

		if (Stack.HasItems() && IsValid(Stack.Item.GetItemClass()))
		{
			UKAPICleanerItemDescription* OutItem;
			if (GetAssetSubsystem()->Cleaner_GetForKey(Stack.Item.GetItemClass(), OutItem))
			{
				SetCleanerRecipe(OutItem);
			}
		}

		mPipeInput->SetInventoryAccessIndex(FLUID_INPUT_INDEX);
		mPipeOutput->SetInventoryAccessIndex(FLUID_OUTPUT_INDEX);
	}
}

void AKLBuildableCleaner::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);

	if (HasAuthority())
	{
		if (HasPower())
		{
			CheckFluid(dt);
		}

		if (IsProducing())
		{
			ByPassProduceHandle(dt);
		}
	}
}

void AKLBuildableCleaner::CollectBelts()
{
	if (GetCurrentCleanerRecipe())
	{
		UKBFLCppInventoryHelper::PullBelt(GetInventory(), CLEANER_ITEM_INDEX, 0.f,
										  GetCurrentCleanerRecipe()->mCleanerItemInfo.mProduceItem, mBeltInput);
		mBeltOutput->SetInventoryAccessIndex(GetFullestStackIndex());
	}
}

void AKLBuildableCleaner::CollectAndPushPipes(float dt, bool IsPush)
{
	if (IsPush)
	{
		UKBFLCppInventoryHelper::PushPipe(GetInventory(), FLUID_OUTPUT_INDEX, dt, mPipeOutput);
		return;
	}
	UKBFLCppInventoryHelper::PullAllFromPipe(GetInventory(), FLUID_INPUT_INDEX, dt, mPipeInput);
}

void AKLBuildableCleaner::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AKLBuildableCleaner::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);

	FG_DOREPCONDITIONAL(ThisClass, mCurrentCleanerRecipe);
	FG_DOREPCONDITIONAL(ThisClass, mCleanerItemHandler);
	FG_DOREPCONDITIONAL(ThisClass, mSlotProductionHandler);
}

bool AKLBuildableCleaner::FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const
{
	if (!IsValid(GetCurrentCleanerRecipe()) || !IsValid(object))
	{
		return false;
	}

	return true;
}

UFGInventoryComponent* AKLBuildableCleaner::GetInventory() const { return mInputInventory; }

UFGInventoryComponent* AKLBuildableCleaner::GetOutputInventory() const { return mOutputInventory; }

bool AKLBuildableCleaner::CanProduce_Implementation() const
{
	if (!Super::CanProduce_Implementation() || !HasPower() || !IsValid(GetCurrentCleanerRecipe()))
	{
		return false;
	}

	return CheckOutputAndWasteInventory() && CheckConsumeAmount();
}

void AKLBuildableCleaner::onProducingFinal_Implementation()
{
	UKAPICleanerItemDescription* Default = GetCurrentCleanerRecipe();
	if (IsValid(Default))
	{
		GetInventory()->RemoveFromIndex(FLUID_INPUT_INDEX, Default->mInFluid.Amount);
		if (IsValid(Default->mOutFluid.ItemClass) && Default->mOutFluid.ItemClass != UFGNoneDescriptor::StaticClass())
		{
			UKBFLCppInventoryHelper::StoreItemAmountInInventory(
				GetInventory(), FLUID_OUTPUT_INDEX, Default->mOutFluid.ItemClass, Default->mOutFluid.Amount);
		}
	}

	Super::onProducingFinal_Implementation();
}

void AKLBuildableCleaner::CheckFluid(float dt)
{
	if (!bShouldAutoSetRecipeIfNotSet || IsValid(GetCurrentCleanerRecipe()) || !mSearchForRecipeTimer.Tick(dt))
	{
		return;
	}

	if (!IsValid(mUnlockSubsystem))
	{
		mUnlockSubsystem = AKLUnlockSubsystem::Get(GetWorld());
	}

	if (!IsValid(mUnlockSubsystem))
	{
		return;
	}

	TSubclassOf<UFGItemDescriptor> lFluid;
	if (GetInventory())
	{
		FInventoryStack Stack;
		GetInventory()->GetStackFromIndex(FLUID_INPUT_INDEX, Stack);
		lFluid = Stack.HasItems() ? Stack.Item.GetItemClass() : nullptr;
		if (!lFluid)
		{
			if (mPipeInput)
			{
				if (mPipeInput->IsConnected() && mPipeInput->HasFluidIntegrant())
				{
					if (mPipeInput->GetFluidDescriptor())
					{
						lFluid = mPipeInput->GetFluidDescriptor();
					}
					else if (const UFGPipeConnectionFactory* OtherConnection =
								 Cast<UFGPipeConnectionFactory>(mPipeInput->GetConnection()))
					{
						lFluid = OtherConnection->GetFluidDescriptor();
					}
				}
			}
		}

		if ((!GetCurrentCleanerRecipe() && lFluid) ||
			(lFluid && GetCurrentCleanerRecipe() && GetCurrentCleanerRecipe()->mInFluid.ItemClass != lFluid))
		{
			UKAPICleanerItemDescription* NewDesc = mUnlockSubsystem->GetUnlockedCleanerDesc(lFluid);
			if (IsValid(NewDesc))
			{
				SetCleanerRecipe(NewDesc);
			}
		}
	}

	if (!ValidateRecipeSettings())
	{
		ApplyRecipeSettings();
	}
}

int32 AKLBuildableCleaner::GetFullestStackIndex() const
{
	int32 Idx = -1;

	if (GetOutputInventory())
	{
		if (!GetOutputInventory()->IsEmpty())
		{
			int32 MaxIndex = GetCurrentCleanerRecipe() ? GetCurrentCleanerRecipe()->mBypassProducts.Num()
													   : GetOutputInventory()->GetSizeLinear();
			int32 Fullest = 0;
			for (int32 Index = 0; Index < MaxIndex; ++Index)
			{
				if (!GetOutputInventory()->IsIndexEmpty(Index))
				{
					FInventoryStack Stack;
					GetOutputInventory()->GetStackFromIndex(Index, Stack);
					if (Fullest <= Stack.NumItems)
					{
						Idx = Index;
						Fullest = Stack.NumItems;
					}
				}
			}
		}
	}

	return Idx;
}

void AKLBuildableCleaner::ByPassProduceHandle(float dt)
{
	UKAPICleanerItemDescription* CurrentInfo = GetCurrentCleanerRecipe();
	if (IsValid(CurrentInfo))
	{
		if (CurrentInfo->CleanerItemNeeded())
		{
			mCleanerItemHandler.TickHandle(
				dt, IsProducing(), [&]()
				{ GetInventory()->RemoveFromIndex(CLEANER_ITEM_INDEX, CurrentInfo->mCleanerItemInfo.mProduceAmount); });
		}

		for (int i = 0; i < mSlotProductionHandler.Num(); ++i)
		{
			mSlotProductionHandler[i].TickHandle(dt, IsProducing(),
												 [&]()
												 {
													 UKBFLCppInventoryHelper::StoreItemAmountInInventory(
														 GetOutputInventory(), i,
														 CurrentInfo->mBypassProducts[i].mProduceItem,
														 CurrentInfo->mBypassProducts[i].mProduceAmount);
												 });
		}
	}
}

TArray<FFullProductionHandle> AKLBuildableCleaner::GetSlotHandler() const { return mSlotProductionHandler; }

FFullProductionHandle AKLBuildableCleaner::GetCleanerItemHandler() const { return mCleanerItemHandler; }

bool AKLBuildableCleaner::CheckOutputAndWasteInventory() const
{
	UKAPICleanerItemDescription* Default = GetCurrentCleanerRecipe();
	if (Default)
	{
		for (int i = 0; i < Default->mBypassProducts.Num(); ++i)
		{
			if (!UKBFLCppInventoryHelper::CanStoreItem(GetOutputInventory(), i,
													   Default->mBypassProducts[i].mProduceItem,
													   Default->mBypassProducts[i].mProduceAmount))
			{
				return false;
			}
		}

		if (!Default->mOutFluid.ItemClass->IsChildOf(UFGNoneDescriptor::StaticClass()) &&
			!UKBFLCppInventoryHelper::CanStoreItem(GetInventory(), FLUID_OUTPUT_INDEX, Default->mOutFluid.ItemClass,
												   Default->mOutFluid.Amount))
		{
			return false;
		}
		return true;
	}

	return false;
}

void AKLBuildableCleaner::Server_DoFlush()
{
	Super::Server_DoFlush();
	if (GetInventory())
	{
		GetInventory()->RemoveAllFromIndex(FLUID_INPUT_INDEX);
		GetInventory()->RemoveAllFromIndex(FLUID_OUTPUT_INDEX);
	}
}

bool AKLBuildableCleaner::CheckConsumeAmount() const
{
	UKAPICleanerItemDescription* CurrentInfo = GetCurrentCleanerRecipe();
	if (IsValid(CurrentInfo))
	{
		FInventoryStack Fluid;
		FInventoryStack Cleaner;
		GetInventory()->GetStackFromIndex(FLUID_INPUT_INDEX, Fluid);
		GetInventory()->GetStackFromIndex(CLEANER_ITEM_INDEX, Cleaner);
		if (CurrentInfo->CleanerItemNeeded() &&
			(Cleaner.NumItems < CurrentInfo->mCleanerItemInfo.mProduceAmount ||
			 Cleaner.Item.GetItemClass() != CurrentInfo->mCleanerItemInfo.mProduceItem))
		{
			return false;
		}
		return Fluid.HasItems() && Fluid.Item.GetItemClass() == CurrentInfo->mInFluid.ItemClass &&
			Fluid.NumItems >= CurrentInfo->mInFluid.Amount;
	}
	return false;
}
