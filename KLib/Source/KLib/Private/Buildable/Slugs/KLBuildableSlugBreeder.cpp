// ILikeBanas


#include "Buildable/Slugs/KLBuildableSlugBreeder.h"

#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Cpp/KBFLCppInventoryHelper.h"
#include "Description/KPCLNetworkDrive.h"

#include "Net/UnrealNetwork.h"

#include "Resources/FGNoneDescriptor.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

AKLBuildableSlugBreeder::AKLBuildableSlugBreeder()
{
	mInputInventory->SetDefaultSize(3);
	mOutputInventory->SetDefaultSize(2);
	bEnableCustomOverclocking = true;
}

void AKLBuildableSlugBreeder::UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots)
{
	Super::UI_ApplyRelevantItems_Implementation(OutSlots);

	OutSlots.Append(mPossibleSlugs.Array());
	OutSlots.Add(GetCurrentFood());
	UKAPISugHatchingData* Slot1 = GetDataForSlot(ESlugSlot::Slot1);
	UKAPISugHatchingData* Slot2 = GetDataForSlot(ESlugSlot::Slot2);
	OutSlots.Add(Slot1->mEgg);
	OutSlots.Add(Slot2->mEgg);
}

void AKLBuildableSlugBreeder::Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo)
{
	Super::Overclocking_GetInfo_Implementation(OutProductionInfo);

	UKAPISugHatchingData* Active = GetActiveData();

	OutProductionInfo.mItemClass = Active->mEgg;
	OutProductionInfo.mAmount = Active->mProductionCountEggs;
	OutProductionInfo.mDefaultProductionTime = Active->mBreedingTime;
}

void AKLBuildableSlugBreeder::Overclocking_GetProductionResults_Implementation(
	TArray<FKPCLOverclockingProductionResults>& OutIngredients, TArray<FKPCLOverclockingProductionResults>& OutProducts)
{
	Super::Overclocking_GetProductionResults_Implementation(OutIngredients, OutProducts);

	UKAPISugHatchingData* Slot1 = GetDataForSlot(ESlugSlot::Slot1);
	UKAPISugHatchingData* Slot2 = GetDataForSlot(ESlugSlot::Slot2);
	UKAPISugHatchingData* Active = GetActiveData();

	OutIngredients.Add(FKPCLOverclockingProductionResults(
		Slot1->mSlug,
		1,
		Slot1->mDieTime
	));
	OutIngredients.Add(FKPCLOverclockingProductionResults(
		Slot2->mSlug,
		1,
		Slot2->mDieTime
	));

	OutIngredients.Add(FKPCLOverclockingProductionResults(
		Active->mRequiredFood,
		GetNumOfFoodConsume(),
		mFoodProductionHandle.mProductionTime
	));

	OutProducts.Add(FKPCLOverclockingProductionResults(
		Slot1->mEgg,
		Slot1->mProductionCountEggs,
		Slot1->mBreedingTime
	));
	OutProducts.Add(FKPCLOverclockingProductionResults(
		Slot2->mEgg,
		Slot2->mProductionCountEggs,
		Slot2->mBreedingTime
	));
}

void AKLBuildableSlugBreeder::ApplySlugData()
{
	if (GetInventory())
	{
		GetInventory()->AddArbitrarySlotSize(FIRST_SLUG_IDX, mMaxSlugPerSlot);
		GetInventory()->AddArbitrarySlotSize(SECOND_SLUG_IDX, mMaxSlugPerSlot);
		GetInventory()->AddArbitrarySlotSize(FOOD_IDX, mMaxFoodStack);
		UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), FIRST_SLUG_IDX, nullptr);
		UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), SECOND_SLUG_IDX, nullptr);
	}

	// make sure that the index set to -1 (pull from all)
	if (GetConv(0, KPCLOutput))
	{
		GetConv(0, KPCLOutput)->SetInventoryAccessIndex(-1);
		GetConv(0, KPCLOutput)->SetInventory(GetOutputInventory());
	}
}

void AKLBuildableSlugBreeder::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);

	FG_DOREPCONDITIONAL(ThisClass, mFoodProductionHandle);
	FG_DOREPCONDITIONAL(ThisClass, mCurrentHatchingData);
	FG_DOREPCONDITIONAL(ThisClass, mDeadTimer);
	FG_DOREPCONDITIONAL(ThisClass, mTemperature);
	FG_DOREPCONDITIONAL(ThisClass, mHumidity);
}

void AKLBuildableSlugBreeder::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool AKLBuildableSlugBreeder::CanUseFactoryClipboard_Implementation()
{
	return true;
}

UFGFactoryClipboardSettings* AKLBuildableSlugBreeder::CopySettings_Implementation()
{
	UKLSlugBreederClipboardSettings* Settings = NewObject<UKLSlugBreederClipboardSettings>();
	WriteClipboardSettings(Settings);
	Settings->mTemperature = mTemperature.mTargetValue;
	Settings->mHumidity = mHumidity.mTargetValue;
	return Settings;
}

bool AKLBuildableSlugBreeder::PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
                                                           class AFGPlayerController* player)
{
	bool Result = Super::PasteSettings_Implementation(factoryClipboard, player);
	if (UKLSlugBreederClipboardSettings* Settings = Cast<UKLSlugBreederClipboardSettings>(factoryClipboard))
	{
		ApplyNewTemperature(Settings->mTemperature);
		ApplyNewHumidity(Settings->mHumidity);
		SetIsProductionPaused(Settings->bIsPaused);
		if (!Result)
		{
			Result = true;
		}
	}

	return Result;
}

void AKLBuildableSlugBreeder::SetPendingPotential(float NewPendingPotential)
{
	NewPendingPotential = FMath::Clamp(NewPendingPotential, GetCurrentMinPotential(), GetCurrentMaxPotential());
	Super::SetPendingPotential(NewPendingPotential);

	mDeadTimer[0].mPendingPotential = NewPendingPotential;
	mDeadTimer[1].mPendingPotential = NewPendingPotential;
	mFoodProductionHandle.mPendingPotential = NewPendingPotential;
}


void AKLBuildableSlugBreeder::BeginPlay()
{
	Super::BeginPlay();

	mDataSubsystem = UKAPIDataAssetSubsystem::Get(this);
	fgcheckf(IsValid(mDataSubsystem), TEXT("KLBuildableSlugBreeder: DataSubsystem is invalid!"));

	for (UKAPISugHatchingData* HatchingData : mDataSubsystem->Slug_GetAll())
	{
		mPossibleSlugs.Add(HatchingData->mSlug);
	}

	if (HasAuthority())
	{
		CheckSlugs();
		OnSlugInformationHasChanged();
	}
}

void AKLBuildableSlugBreeder::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);

	if (HasAuthority())
	{
		CheckSlugs();

		mFoodProductionHandle.TickHandle(dt, IsProducing(), [&]()
		{
			OnFoodFinial();
		});

		mDeadTimer[0].TickHandle(dt, IsProducing(), [&]()
		{
			OnSlugDied(0);
		});

		mDeadTimer[1].TickHandle(dt, IsProducing(), [&]()
		{
			OnSlugDied(1);
		});

		if (HasPower())
		{
			mTemperature.Tick(dt);
			mHumidity.Tick(dt);
		}
	}
}

void AKLBuildableSlugBreeder::CollectBelts()
{
	Super::CollectBelts();

	UKBFLCppInventoryHelper::PullBeltChildClass(GetInventory(), FIRST_SLUG_IDX, 0.f, UFGItemDescriptor::StaticClass(),
	                                            GetConv(0));
	UKBFLCppInventoryHelper::PullBeltChildClass(GetInventory(), SECOND_SLUG_IDX, 0.f, UFGItemDescriptor::StaticClass(),
	                                            GetConv(1));
	UKBFLCppInventoryHelper::PullBeltChildClass(GetInventory(), FOOD_IDX, 0.f, UFGItemDescriptor::StaticClass(),
	                                            GetConv(2));
}

bool AKLBuildableSlugBreeder::CanProduce_Implementation() const
{
	if (IsPlayingBuildEffect() || !HasPower() || IsProductionPaused())
	{
		return false;
	}

	if (!GetInventory() || !GetOutputInventory() || !GetStoredFoodClass())
	{
		return false;
	}

	return SlugFeeling() && CanConsume() && CanStorage() && IsFoodCorrect();
}

void AKLBuildableSlugBreeder::OnSlugInformationHasChanged_Implementation()
{
	if (GetInventory())
	{
		UKAPISugHatchingData* Slot1 = GetDataForSlot(ESlugSlot::Slot1);
		UKAPISugHatchingData* Slot2 = GetDataForSlot(ESlugSlot::Slot2);
		UKAPISugHatchingData* ActiveSlot = GetActiveData();

		FFullProductionHandle& DeadTimer1 = mDeadTimer[static_cast<uint8>(ESlugSlot::Slot1)];
		FFullProductionHandle& DeadTimer2 = mDeadTimer[static_cast<uint8>(ESlugSlot::Slot2)];

		mFoodProductionHandle.mPendingPotential = GetPendingPotential();
		DeadTimer1.mPendingPotential = GetPendingPotential();
		DeadTimer2.mPendingPotential = GetPendingPotential();

		mFoodProductionHandle.mCurrentPotential = GetCurrentPotential();
		DeadTimer1.mCurrentPotential = GetCurrentPotential();
		DeadTimer2.mCurrentPotential = GetCurrentPotential();

		mFoodProductionHandle.SetNewTime(ActiveSlot->mFoodDuration, false);
		DeadTimer1.SetNewTime(Slot1->mDieTime, false);
		DeadTimer2.SetNewTime(Slot2->mDieTime, false);

		SetProductionTime(ActiveSlot->mBreedingTime);

		UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), FOOD_IDX,
		                                                      GetCurrentFood());
		UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetOutputInventory(), FIRST_SLUG_IDX,
		                                                      Slot1
		                                                      ->mEgg);
		UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetOutputInventory(), SECOND_SLUG_IDX,
		                                                      Slot2
		                                                      ->mEgg);

		ApplySlugData();
		OnBreedingInformationHasChanged.Broadcast();
	}
}

bool AKLBuildableSlugBreeder::SlugFeeling() const
{
	UKAPISugHatchingData* Active = GetActiveData();
	FKAPISlugFeeling Feeling = Active->GetFeeling();
	return Feeling.IsHumidityInRange(mHumidity.mValue) && Feeling.IsTempInRange(mTemperature.mValue);
}

bool AKLBuildableSlugBreeder::CanConsume() const
{
	FInventoryStack Stack = GetFoodStack();
	return IsFoodCorrect() && Stack.NumItems >= GetNumOfFoodConsume();
}

bool AKLBuildableSlugBreeder::CanStorage() const
{
	UKAPISugHatchingData* Slot1 = GetDataForSlot(ESlugSlot::Slot1);
	UKAPISugHatchingData* Slot2 = GetDataForSlot(ESlugSlot::Slot2);

	bool Slot1CanStore = UKBFLCppInventoryHelper::CanStoreItem(GetOutputInventory(), FIRST_SLUG_IDX,
	                                                           Slot1->mEgg,
	                                                           Slot1->mProductionCountEggs);

	bool Slot2CanStore = UKBFLCppInventoryHelper::CanStoreItem(GetOutputInventory(), SECOND_SLUG_IDX,
	                                                           Slot2->mEgg,
	                                                           Slot2->mProductionCountEggs);


	return Slot1CanStore && Slot2CanStore;
}

void AKLBuildableSlugBreeder::CheckSlugs()
{
	if (GetInventory())
	{
		FInventoryStack Slot1;
		FInventoryStack Slot2;

		GetInventory()->GetStackFromIndex(FIRST_SLUG_IDX, Slot1);
		GetInventory()->GetStackFromIndex(SECOND_SLUG_IDX, Slot2);

		UKAPISugHatchingData* Slot1CurrentData = GetDataForSlot(ESlugSlot::Slot1);
		UKAPISugHatchingData* Slot2CurrentData = GetDataForSlot(ESlugSlot::Slot2);

		UKAPISugHatchingData* Slot1NewData;
		UKAPISugHatchingData* Slot2NewData;

		bool bDirty = false;
		if (Slot1.HasItems() && mDataSubsystem->Slug_GetForItem(Slot1.Item.GetItemClass(), Slot1NewData) && Slot1NewData
			!=
			Slot1CurrentData)
		{
			mActiveHatchingDatas[static_cast<uint8>(ESlugSlot::Slot1)] = Slot1NewData;
			bDirty = true;
		}

		if (Slot2.HasItems() && mDataSubsystem->Slug_GetForItem(Slot2.Item.GetItemClass(), Slot2NewData) && Slot2NewData
			!=
			Slot2CurrentData)
		{
			mActiveHatchingDatas[static_cast<uint8>(ESlugSlot::Slot2)] = Slot2NewData;
			bDirty = true;
		}

		if (bDirty)
		{
			OnSlugInformationHasChanged();
		}
	}
}

void AKLBuildableSlugBreeder::OnFoodFinial_Implementation()
{
	if (GetInventory())
	{
		GetInventory()->RemoveFromIndex(FOOD_IDX, GetNumOfFoodConsume());
	}
}

void AKLBuildableSlugBreeder::OnSlugDied_Implementation(uint8 slot)
{
	if (GetInventory())
	{
		GetInventory()->RemoveFromIndex(slot, 1);
	}
}

void AKLBuildableSlugBreeder::onProducingFinal_Implementation()
{
	if (GetOutputInventory())
	{
		UKAPISugHatchingData* Slot1 = GetDataForSlot(ESlugSlot::Slot1);
		UKAPISugHatchingData* Slot2 = GetDataForSlot(ESlugSlot::Slot2);

		UKBFLCppInventoryHelper::StoreItemAmountInInventory(GetOutputInventory(), FIRST_SLUG_IDX,
		                                                    Slot1->mEgg,
		                                                    Slot1->mProductionCountEggs);
		UKBFLCppInventoryHelper::StoreItemAmountInInventory(GetOutputInventory(), SECOND_SLUG_IDX,
		                                                    Slot2->mEgg,
		                                                    Slot2->mProductionCountEggs);
	}
}

void AKLBuildableSlugBreeder::EndProductionTime()
{
	Super::EndProductionTime();

	ApplySlugData();
}

FInventoryStack AKLBuildableSlugBreeder::GetFoodStack() const
{
	FInventoryStack Stack = GetStackFromIndex(FOOD_IDX);

	if (Stack.HasItems())
	{
		return Stack;
	}

	return Stack;
}

bool AKLBuildableSlugBreeder::IsFoodCorrect() const
{
	return GetStoredFoodClass() == GetCurrentFood();
}

int32 AKLBuildableSlugBreeder::GetNumOfFood() const
{
	FInventoryStack Stack = GetFoodStack();

	if (Stack.HasItems())
	{
		return Stack.NumItems;
	}
	return 0;
}

void AKLBuildableSlugBreeder::ApplyNewHumidity(float Humidity)
{
	if (HasAuthority())
	{
		mHumidity.SetNewTarget(Humidity);
	}
	else if (UKLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKLDefaultRCO>(GetWorld()))
	{
		RCO->Server_SetTargetHumidity(this, Humidity);
	}
}

void AKLBuildableSlugBreeder::ApplyNewTemperature(float Temperature)
{
	if (HasAuthority())
	{
		mTemperature.SetNewTarget(Temperature);
	}
	else if (UKLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKLDefaultRCO>(GetWorld()))
	{
		RCO->Server_SetTargetTemperature(this, Temperature);
	}
}

int32 AKLBuildableSlugBreeder::GetNumOfFoodConsume() const
{
	int TotalCount = 0;

	UKAPISugHatchingData* Slot1 = GetDataForSlot(ESlugSlot::Slot1);
	UKAPISugHatchingData* Slot2 = GetDataForSlot(ESlugSlot::Slot2);

	TotalCount += Slot1->mFoodConsumePerCycle;
	TotalCount += Slot2->mFoodConsumePerCycle;

	return TotalCount;
}

UKAPISugHatchingData* AKLBuildableSlugBreeder::GetDataForSlot(ESlugSlot Slot) const
{
	if (!IsValid(mActiveHatchingDatas[static_cast<uint8>(Slot)]))
	{
		return mDefaultHatchingData;
	}
	return mActiveHatchingDatas[static_cast<uint8>(Slot)];
}

UKAPISugHatchingData* AKLBuildableSlugBreeder::GetActiveData() const
{
	return UKAPISugHatchingData::GetHighterSlugStatic(GetDataForSlot(ESlugSlot::Slot1),
	                                                  GetDataForSlot(ESlugSlot::Slot2));
}

TSubclassOf<UFGItemDescriptor> AKLBuildableSlugBreeder::GetCurrentFood() const
{
	return GetActiveData()->mRequiredFood;
}

TSubclassOf<UFGItemDescriptor> AKLBuildableSlugBreeder::GetStoredFoodClass() const
{
	FInventoryStack Stack = GetFoodStack();

	if (Stack.HasItems())
	{
		return TSubclassOf<UFGItemDescriptor>(Stack.Item.GetItemClass());
	}
	return nullptr;
}

bool AKLBuildableSlugBreeder::AreSlugsComfortable() const
{
	return GetDataForSlot(ESlugSlot::Slot1)->IsComfortableWith(GetDataForSlot(ESlugSlot::Slot2));
}

FFullProductionHandle AKLBuildableSlugBreeder::GetDeadTimer(ESlugSlot Slot) const
{
	return mDeadTimer[static_cast<uint8>(Slot)];
}

bool AKLBuildableSlugBreeder::FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const
{
	if (!IsValid(object))
	{
		return false;
	}

	if (idx == FOOD_IDX)
	{
		return object->IsChildOf(GetCurrentFood());
	}

	if (TSubclassOf<UFGItemDescriptor> AsItem = TSubclassOf<UFGItemDescriptor>(object))
	{
		return mPossibleSlugs.Contains(AsItem);
	}

	return false;
}
