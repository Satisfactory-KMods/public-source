// ILikeBanas


#include "Buildable/Slugs/KLBuildableSlugHatching.h"

#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Buildable/Modular/KPCLModularBuildingHandlerStacker.h"
#include "Buildable/Slugs/KLBuildableSlugHatchingModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "Cpp/KBFLCppInventoryHelper.h"
#include "FGColoredInstanceMeshProxy.h"
#include "Kismet/KismetMathLibrary.h"
#include "Logging.h"
#include "TimerManager.h"

#include "Net/UnrealNetwork.h"

#include "Resources/FGNoneDescriptor.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

class UKBFLAssetDataSubsystem;

void AKLBuildableSlugHatching::SetHasIncubatorModule(bool bNewValue)
{
	bHasIncubatorModule = bNewValue;
	mPropertyReplicator.MarkPropertyDirty(FName("bHasIncubatorModule"));
}

void AKLBuildableSlugHatching::SetHasTankModule(bool bNewValue)
{
	bHasTankModule = bNewValue;
	mPropertyReplicator.MarkPropertyDirty(FName("bHasTankModule"));
}

void AKLBuildableSlugHatching::SetHasTimeModule(bool bNewValue)
{
	bHasTimeModule = bNewValue;
	mPropertyReplicator.MarkPropertyDirty(FName("bHasTimeModule"));
}

void AKLBuildableSlugHatching::SetOverwriteTime(EKAPISlugTime NewTime)
{
	mOverwriteTime = NewTime;
	mPropertyReplicator.MarkPropertyDirty(FName("mOverwriteTime"));
}

void AKLBuildableSlugHatching::SetSlugTime(EKAPISlugTime NewTime)
{
	mTime = NewTime;
	mPropertyReplicator.MarkPropertyDirty(FName("mTime"));
}

void AKLBuildableSlugHatching::SetHatching_Humidity(float NewHumidity)
{
	mHumidity = NewHumidity;
	mPropertyReplicator.MarkPropertyDirty(FName("mHumidity"));
}

void AKLBuildableSlugHatching::SetHatching_Temperature(float NewTemperature)
{
	mTemperature = NewTemperature;
	mPropertyReplicator.MarkPropertyDirty(FName("mTemperature"));
}

void AKLBuildableSlugHatching::CommitFluidProductionHandle()
{
	mPropertyReplicator.MarkPropertyDirty(FName("mFluidProductionHandle"));
}

void AKLBuildableSlugHatching::CommitFixedChanceList()
{
	mPropertyReplicator.MarkPropertyDirty(FName("mFixedChanceList"));
}

void AKLBuildableSlugHatching::SetCurrentHatchingData(UKAPISugHatchingData* NewData)
{
	mCurrentHatchingData = NewData;
	ForceNetUpdate();
}

void AKLBuildableSlugHatching::OnRep_CurrentHatchingData() { OnNewEgg(mCurrentHatchingData, false); }

AKLBuildableSlugHatching::AKLBuildableSlugHatching() : mTimeSub(nullptr)
{
	mInputInventory->SetDefaultSize(2);
	mOutputInventory->SetDefaultSize(8);
	bEnableCustomOverclocking = true;

	mModuleTypeLimit.Add(EHatchingModuleType::Humidity, 8);
	mModuleTypeLimit.Add(EHatchingModuleType::Time, 1);
	mModuleTypeLimit.Add(EHatchingModuleType::Incubator, 1);
	mModuleTypeLimit.Add(EHatchingModuleType::Tank, 1);
	mModuleTypeLimit.Add(EHatchingModuleType::Temperature, 8);
	mModuleTypeLimit.Add(EHatchingModuleType::None, 0);
}

bool AKLBuildableSlugHatching::Overclocking_IsConsumer_Implementation() { return true; }

void AKLBuildableSlugHatching::UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots)
{
	Super::UI_ApplyRelevantItems_Implementation(OutSlots);

	UKAPISugHatchingData* HatchingData = GetHatchingData();
	if (!IsValid(HatchingData))
	{
		return;
	}
	OutSlots.Add(HatchingData->mEgg);
	OutSlots.Append(HatchingData->GetPossibleSlugs());
}

void AKLBuildableSlugHatching::Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo)
{
	Super::Overclocking_GetInfo_Implementation(OutProductionInfo);

	UKAPISugHatchingData* HatchingData = GetHatchingData();
	if (!IsValid(HatchingData))
	{
		return;
	}
	OutProductionInfo.mItemClass = HatchingData->mEgg;
	OutProductionInfo.mAmount = mEggsToConsume;
}

void AKLBuildableSlugHatching::Overclocking_GetProductionResults_Implementation(
	TArray<FKPCLOverclockingProductionResults>& OutIngredients, TArray<FKPCLOverclockingProductionResults>& OutProducts)
{
	Super::Overclocking_GetProductionResults_Implementation(OutIngredients, OutProducts);

	UKAPISugHatchingData* HatchingData = GetHatchingData();
	if (!IsValid(HatchingData))
	{
		return;
	}

	OutIngredients.Add(FKPCLOverclockingProductionResults(HatchingData->mEgg, 1, mProductionHandle.mProductionTime));

	if (HatchingData->IncubationFluidRequired())
	{
		OutIngredients.Add(FKPCLOverclockingProductionResults(HatchingData->mFluidClass, HatchingData->mFluidConsume,
															  HatchingData->mFluidConsumeTime));
	}

	for (FKAPISlugIncubation Slug : HatchingData->mPossibleSlugs)
	{
		OutProducts.Add(
			FKPCLOverclockingProductionResults(Slug.mSlug, Slug.mProductionCount, HatchingData->mHatchDuration));
	}
}

void AKLBuildableSlugHatching::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKLBuildableSlugHatching, mCurrentHatchingData);
}

void AKLBuildableSlugHatching::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);

	FG_DOREPCONDITIONAL(ThisClass, mTemperature);
	FG_DOREPCONDITIONAL(ThisClass, mHumidity);
	FG_DOREPCONDITIONAL(ThisClass, bHasTankModule);
	FG_DOREPCONDITIONAL(ThisClass, bHasIncubatorModule);
	FG_DOREPCONDITIONAL(ThisClass, bHasTimeModule);
	FG_DOREPCONDITIONAL(ThisClass, mTime);
	FG_DOREPCONDITIONAL(ThisClass, mOverwriteTime);
	FG_DOREPCONDITIONAL(ThisClass, mFluidProductionHandle);
	FG_DOREPCONDITIONAL(ThisClass, mFixedChanceList);
}

UFGFactoryClipboardSettings* AKLBuildableSlugHatching::CopySettings_Implementation()
{
	UKLSlugHatcherClipboardSettings* Settings = NewObject<UKLSlugHatcherClipboardSettings>();
	WriteClipboardSettings(Settings);
	Settings->mModuleSettings.Empty();

	UKPCLModularBuildingHandlerStacker* Handler = GetHandler<UKPCLModularBuildingHandlerStacker>();
	if (IsValid(Handler))
	{
		for (AFGBuildable* Buildable : Handler->mAttachmentDatas[0].mSnappedActors)
		{
			if (!IsValid(Buildable))
			{
				continue;
			}
			if (AKLBuildableSlugHatchingModule* Module = Cast<AKLBuildableSlugHatchingModule>(Buildable))
			{
				FKLSlugHatcherModuleSetting ModuleSetting;
				ModuleSetting.mModuleType = Module->mModuleType;
				ModuleSetting.mModuleTier = Module->mTier;
				ModuleSetting.mTemperature = Module->mTemperature.mTargetValue;
				ModuleSetting.mHumidity = Module->mHumidity.mTargetValue;
				ModuleSetting.mOverwriteTime = Module->mOverwriteTime;
				Settings->mModuleSettings.Add(ModuleSetting);
			}
		}
	}

	return Settings;
}

bool AKLBuildableSlugHatching::PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
															class AFGPlayerController* player)
{
	bool bResult = Super::PasteSettings_Implementation(factoryClipboard, player);
	if (UKLSlugHatcherClipboardSettings* Settings = Cast<UKLSlugHatcherClipboardSettings>(factoryClipboard))
	{
		SetIsProductionPaused(Settings->bIsPaused);

		UKPCLModularBuildingHandlerStacker* Handler = GetHandler<UKPCLModularBuildingHandlerStacker>();
		if (IsValid(Handler))
		{
			TArray<AKLBuildableSlugHatchingModule*> Modules;
			for (AFGBuildable* Buildable : Handler->mAttachmentDatas[0].mSnappedActors)
			{
				if (!IsValid(Buildable))
				{
					continue;
				}
				if (AKLBuildableSlugHatchingModule* Module = Cast<AKLBuildableSlugHatchingModule>(Buildable))
				{
					Modules.Add(Module);
				}
			}

			for (FKLSlugHatcherModuleSetting Setting : Settings->mModuleSettings)
			{
				AKLBuildableSlugHatchingModule* FoundModule = nullptr;
				for (AKLBuildableSlugHatchingModule* Module : Modules)
				{
					if (Module->mModuleType == Setting.mModuleType && Module->mTier == Setting.mModuleTier)
					{
						FoundModule = Module;
						break;
					}
				}

				if (IsValid(FoundModule))
				{
					Modules.Remove(FoundModule);
					FoundModule->ApplyNewHumidity(Setting.mHumidity);
					FoundModule->ApplyNewTemperature(Setting.mTemperature);
					FoundModule->ApplyNewOverwriteTime(Setting.mOverwriteTime);
				}
			}
		}

		DoGatherStats();
		if (!bResult)
		{
			bResult = true;
		}
	}

	return bResult;
}

bool AKLBuildableSlugHatching::CanUseFactoryClipboard_Implementation() { return true; }

void AKLBuildableSlugHatching::SetPendingPotential(float NewPendingPotential)
{
	NewPendingPotential = FMath::Clamp(NewPendingPotential, GetCurrentMinPotential(), GetCurrentMaxPotential());
	Super::SetPendingPotential(NewPendingPotential);
	mFluidProductionHandle.mPendingPotential = NewPendingPotential;
	CommitFluidProductionHandle();
}

void AKLBuildableSlugHatching::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		mTimeSub = AFGTimeOfDaySubsystem::Get(this);
		if (mTime == EKAPISlugTime::NONE)
		{
			mTime = EKAPISlugTime::Any;
		}
		if (mTimeSub)
		{
			mTimeSub->mOnDayStateDelegate.AddDynamic(this, &AKLBuildableSlugHatching::OnDayTimeUpdated);
			mTime = mTimeSub->IsDay() ? EKAPISlugTime::Day : EKAPISlugTime::Night;
		}
		SetSlugTime(mTime);

		DoGatherStats();
		CheckEgg();
		OnNewEgg_Native(GetHatchingData(), false);
	}
}

void AKLBuildableSlugHatching::Factory_Tick(float dt)
{
	if (HasAuthority())
	{
		CheckEgg();

		mFluidProductionHandle.TickHandle(dt, bHasTankModule && IsProducing(), [&]() { OnFluidFinial(); });
		CommitFluidProductionHandle();
	}

	Super::Factory_Tick(dt);
}

void AKLBuildableSlugHatching::CollectBelts()
{
	Super::CollectBelts();
	UKBFLCppInventoryHelper::PullBeltChildClass(GetInventory(), 0, 0.f, UFGItemDescriptor::StaticClass(), GetConv(0));
}

void AKLBuildableSlugHatching::CollectAndPushPipes(float dt, bool IsPush)
{
	Super::CollectAndPushPipes(dt, IsPush);

	if (bHasTankModule && !IsPush)
	{
		UKBFLCppInventoryHelper::PullPipe(GetInventory(), FLUID_IDX, dt, GetFluidClass(), GetPipe(0));
	}
}

void AKLBuildableSlugHatching::HandlePower(float dt)
{
	UKAPISugHatchingData* HatchingData = GetHatchingData();
	if (IsProducing() && IsValid(HatchingData))
	{
		FPowerOptions& PowerOptions = GetPowerOptionRef();

		float EggPowerConumption = HatchingData->mPowerConsume;
		PowerOptions.mNormalPowerConsume = EggPowerConumption + mPowerConsumption;

		UKPCLModularBuildingHandlerStacker* Handler = GetHandler<UKPCLModularBuildingHandlerStacker>();
		if (IsValid(Handler))
		{
			for (AFGBuildable* Buildable : Handler->mAttachmentDatas[0].mSnappedActors)
			{
				if (AKLBuildableSlugHatchingModule* Module = Cast<AKLBuildableSlugHatchingModule>(Buildable))
				{
					PowerOptions.mNormalPowerConsume +=
						FMath::Max(Module->GetPowerOption().mNormalPowerConsume, Module->mPowerConsumption);
				}
			}
		}
	}

	Super::HandlePower(dt);
}

bool AKLBuildableSlugHatching::IsAllowedToSnap_Implementation(AFGBuildable* Actor)
{
	if (AKLBuildableSlugHatchingModule* Module = Cast<AKLBuildableSlugHatchingModule>(Actor))
	{
		int32 Limit = mModuleTypeLimit.FindRef(Module->mModuleType);
		int32 CurrentCount = 0;
		UKPCLModularBuildingHandlerStacker* Handler = GetHandler<UKPCLModularBuildingHandlerStacker>();
		if (IsValid(Handler))
		{
			for (AFGBuildable* Buildable : Handler->mAttachmentDatas[0].mSnappedActors)
			{
				if (!IsValid(Buildable))
				{
					continue;
				}
				if (AKLBuildableSlugHatchingModule* ExistingModule = Cast<AKLBuildableSlugHatchingModule>(Buildable))
				{
					if (ExistingModule->mModuleType == Module->mModuleType)
					{
						CurrentCount++;
					}
				}
			}
		}

		return CurrentCount < Limit;
	}
	return false;
}

void AKLBuildableSlugHatching::POST_RemoveAttachedActor_Implementation() { DoGatherStats(); }

void AKLBuildableSlugHatching::OnModulesWasUpdated_Implementation()
{
	Super::OnModulesWasUpdated_Implementation();
	DoGatherStats();
}

bool AKLBuildableSlugHatching::AttachedActor_Implementation(AFGBuildable* Actor,
															TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment,
															FTransform Location, float Distance)
{
	bool success = Super::AttachedActor_Implementation(Actor, Attachment, Location, Distance);
	DoGatherStats();
	return success;
}

void AKLBuildableSlugHatching::onProducingFinal_Implementation()
{
	if (!mSlugSubsystem)
	{
		mSlugSubsystem = AKLSlugSubsystem::Get(GetWorld());
	}

	UKAPISugHatchingData* HatchingData = GetHatchingData();
	if (!IsValid(HatchingData))
	{
		return;
	}
	TArray<FItemAmount> ProductedSlugs;

	const bool bRolled = HatchingData->RollSlugs(ProductedSlugs, mFixedChanceList);
	CommitFixedChanceList();
	if (bRolled)
	{
		for (FItemAmount Amount : ProductedSlugs)
		{
			FInventoryStack Stack = FInventoryStack(Amount.Amount, Amount.ItemClass);
			UKBFLCppInventoryHelper::AddStackInInventory(GetOutputInventory(), Stack);
			if (IsValid(mSlugSubsystem) && !mSlugSubsystem->WasSlugBreeded(Amount.ItemClass))
			{
				AsyncTask(ENamedThreads::GameThread,
						  [this, Amount]()
						  {
							  if (IsValid(mSlugSubsystem))
							  {
								  mSlugSubsystem->AddBreededSlugs(Amount.ItemClass);
							  }
						  });
			}
		}
	}

	TSubclassOf<UFGItemDescriptor> EggClass = GetInventoryEgg();
	if (IsValid(mSlugSubsystem) && !mSlugSubsystem->WasEggBreeded(EggClass))
	{
		AsyncTask(ENamedThreads::GameThread,
				  [this, EggClass]()
				  {
					  if (IsValid(mSlugSubsystem))
					  {
						  mSlugSubsystem->AddBreededEgg(EggClass);
					  }
				  });
	}

	GetInventory()->RemoveFromIndex(EGG_IDX, mEggsToConsume);
	DoGatherStats();
}

void AKLBuildableSlugHatching::HandleIndicator()
{
	if (!GetHasPower())
	{
		mCurrentState = ENewProductionState::NoPower;
		return;
	}

	if (IsProductionPaused())
	{
		mCurrentState = ENewProductionState::Paused;
		return;
	}

	if (!CheckModules())
	{
		mCurrentState = ENewProductionState::ExtraState_1;
		return;
	}

	if (!CheckFeelingOfEgg())
	{
		mCurrentState = ENewProductionState::ExtraState_2;
		return;
	}

	Super::HandleIndicator();
}

void AKLBuildableSlugHatching::Server_DoFlush()
{
	Super::Server_DoFlush();

	if (GetInventory())
	{
		if (!GetInventory()->IsIndexEmpty(FLUID_IDX))
		{
			GetInventory()->RemoveAllFromIndex(FLUID_IDX);
		}
	}
}

void AKLBuildableSlugHatching::GetChildDismantleActors_Implementation(TArray<AActor*>& Out_ChildDismantleActors) const
{
	Super::GetChildDismantleActors_Implementation(Out_ChildDismantleActors);
	if (mModularHandler)
	{
		TArray<AFGBuildable*> Modules;
		mModularHandler->GetAttachedActors(Modules);
		for (AFGBuildable* Module : Modules)
		{
			if (Module)
			{
				Out_ChildDismantleActors.AddUnique(Module);
			}
		}
	}
}

bool AKLBuildableSlugHatching::CanProduce_Implementation() const
{
	if (IsPlayingBuildEffect() || !HasPower())
	{
		return false;
	}

	if (IsProductionPaused())
	{
		return false;
	}

	if (!GetInventory() || !GetOutputInventory() || !GetInventoryEgg())
	{
		return false;
	}

	if (!CanStoreSlugs() || !IsHatchingValid() || !CheckFeelingOfEgg() || !CheckModules())
	{
		return false;
	}

	return true;
}

bool AKLBuildableSlugHatching::FilterInputInventory(TSubclassOf<UObject> object, int32 idx) const
{
	if (idx == FLUID_IDX)
	{
		if (GetFluidClass() == object)
		{
			return true;
		}
	}

	if (TSubclassOf<UFGItemDescriptor> Item = TSubclassOf<UFGItemDescriptor>(object))
	{
		if (!IsValid(mAssetSubsystem))
		{
			return false;
		}

		UKAPISugHatchingData* HatchingData;
		mAssetSubsystem->Slug_GetForItem(Item, HatchingData);

		return IsValid(HatchingData) && HatchingData->mEgg == Item;
	}

	return false;
}

bool AKLBuildableSlugHatching::FilterOutputInventory(TSubclassOf<UObject> object, int32 idx) const
{
	// if (TSubclassOf<UFGItemDescriptor> Item = TSubclassOf<UFGItemDescriptor>(object))
	//{
	//	UKAPISugHatchingData* HatchingData;
	//	mAssetSubsystem->Slug_GetForItem(Item, HatchingData);
	//	TSubclassOf<UFGItemDescriptor> SlotItem = nullptr;
	//
	//	if (IsValid(HatchingData) && HatchingData->mPossibleSlugs.IsValidIndex(idx))
	//	{
	//		const FKAPISlugIncubation& Incubation = HatchingData->mPossibleSlugs[idx];
	//		SlotItem = Incubation.mSlug;
	//	}
	//
	//	return IsValid(SlotItem) && SlotItem == Item;
	//}

	return true;
}

void AKLBuildableSlugHatching::CheckEgg()
{
	if (TSubclassOf<UFGItemDescriptor> Egg = GetInventoryEgg())
	{
		UKAPISugHatchingData* NewData;
		UKAPIDataAssetSubsystem* AssetSubsystem = UKAPIDataAssetSubsystem::Get(this);
		if (IsValid(AssetSubsystem) && AssetSubsystem->Slug_GetForItem(Egg, NewData) && NewData != mCurrentHatchingData)
		{
			OnNewEgg_Native(NewData, true);
		}
	}
}

void AKLBuildableSlugHatching::OnMeshUpdate()
{
	UKPCLModularBuildingHandlerStacker* Handler = GetHandler<UKPCLModularBuildingHandlerStacker>();
	if (!IsValid(Handler))
	{
		UE_LOGFMT(LogKLIB, Error, "OnMeshUpdate called without Handler on {0}", GetName());
		return;
	}

	if (IsValid(mCustomProductionStateIndicator))
	{
		if (AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(this))
		{
			UFGColoredInstanceManager* Manager =
				BuildableSubsystem->GetColoredInstanceManager(mCustomProductionStateIndicator);
			if (IsValid(Manager))
			{
				FTransform Transform = Handler->mAttachmentDatas[0].GetSnapLocation();
				FVector Location = Transform.GetLocation();
				Location.Z += mToppingZOffset;
				Transform.SetLocation(Location);
				if (mCustomProductionStateIndicator->mInstanceHandle.IsInstanced())
				{
					mCustomProductionStateIndicator->UpdateWorldTransform(Transform);
				}
				else
				{
					GetWorldTimerManager().SetTimerForNextTick(this, &AKLBuildableSlugHatching::OnMeshUpdate);
				}
			}
		}
	}
	else if (mCustomIndicatorHandleIndexes.Num() > 0)
	{
		const FInstanceOwnerHandlePtr newHandle;
		const FInstanceOwnerHandlePtr Handle = mInstanceHandles.IsValidIndex(mCustomIndicatorHandleIndexes[0])
			? mInstanceHandles[mCustomIndicatorHandleIndexes[0]]
			: newHandle;
		if (Handle.IsValid() && Handle->IsInstanced())
		{
			FTransform Transform = Handler->mAttachmentDatas[0].GetSnapLocation();
			FVector Location = Transform.GetLocation();
			Location.Z += mToppingZOffset;
			Transform.SetLocation(Location);

			mInstanceHandles[mCustomIndicatorHandleIndexes[0]]->SetLocalTransform(Transform);
			if (USkeletalMeshComponent* SkeletalMesh = FindComponentByClass<USkeletalMeshComponent>())
			{
				SkeletalMesh->SetWorldLocation(Location);
			}
		}
		else
		{
			UE_LOGFMT(LogKLIB, Error, "Handle->IsInstanced is false on {0}", *GetName());
		}
	}
}

void AKLBuildableSlugHatching::OnModulesUpdated_Implementation()
{
	Super::OnModulesUpdated_Implementation();

	DoGatherStats();
}

TSubclassOf<UFGItemDescriptor> AKLBuildableSlugHatching::GetFluidClass() const
{
	UKAPISugHatchingData* HatchingData = GetHatchingData();
	if (IsValid(HatchingData))
	{
		if (HatchingData->IncubationFluidRequired())
		{
			return HatchingData->mFluidClass;
		}
	}
	return nullptr;
}

TSubclassOf<UFGItemDescriptor> AKLBuildableSlugHatching::GetInventoryEgg() const
{
	if (GetInventory())
	{
		FInventoryStack Stack;
		GetInventory()->GetStackFromIndex(EGG_IDX, Stack);
		if (Stack.HasItems())
		{
			return Stack.Item.GetItemClass();
		}
	}
	return nullptr;
}

TArray<TSubclassOf<UFGItemDescriptor>> AKLBuildableSlugHatching::GetAllPossibleSlugs() const
{
	UKAPISugHatchingData* HatchingData = GetHatchingData();
	if (IsValid(HatchingData))
	{
		return HatchingData->GetPossibleSlugs();
	}
	return {};
}

bool AKLBuildableSlugHatching::CheckFeelingOfEgg() const
{
	UKAPISugHatchingData* HatchingData = GetHatchingData();
	if (!IsValid(HatchingData))
	{
		return false;
	}

	FKAPISlugFeeling Feeling = HatchingData->GetFeeling();
	if (Feeling.IsTempInRange(mTemperature))
	{
		if (Feeling.IsHumidityInRange(mHumidity))
		{
			return Feeling.IsDayTimeValid(GetCurrentSlugTime());
		}
	}
	return false;
}

bool AKLBuildableSlugHatching::CheckModules() const
{
	UKAPISugHatchingData* HatchingData = GetHatchingData();

	if (!bHasIncubatorModule || !IsValid(HatchingData))
	{
		return false;
	}

	if (HatchingData->IncubationFluidRequired() && !bHasTankModule)
	{
		return false;
	}

	if (HatchingData->IncubationFluidRequired())
	{
		const FInventoryStack Stack = GetStackFromIndex(FLUID_IDX);
		return Stack.HasItems() && Stack.Item.GetItemClass() == HatchingData->mFluidClass &&
			Stack.NumItems >= HatchingData->mFluidConsume;
	}

	return true;
}

bool AKLBuildableSlugHatching::CanStoreSlugs() const
{
	UFGInventoryComponent* Inventory = GetOutputInventory();
	if (IsValid(Inventory))
	{
		TArray<TSubclassOf<UFGItemDescriptor>> AllSlugs = GetAllPossibleSlugs();
		for (int32 i = 0; i < AllSlugs.Num(); ++i)
		{
			if (!UKBFLCppInventoryHelper::CanStoreItem(Inventory, i, AllSlugs[i]))
			{
				return false;
			}
		}
	}
	return IsValid(Inventory);
}

bool AKLBuildableSlugHatching::IsHatchingValid() const
{
	return IsValid(GetHatchingData()) && !GetHatchingData()->mPossibleSlugs.IsEmpty();
}

float AKLBuildableSlugHatching::GetDefaultProductionCycleTime() const
{
	UKAPISugHatchingData* HatchingData = GetHatchingData();
	if (!IsValid(HatchingData))
	{
		return 0.f;
	}
	return HatchingData->mHatchDuration;
}

float AKLBuildableSlugHatching::GetPendingProductionTime() const
{
	UKAPISugHatchingData* HatchingData = GetHatchingData();
	if (!IsValid(HatchingData))
	{
		return 0.f;
	}
	return UKismetMathLibrary::SafeDivide(HatchingData->mHatchDuration, mProductionHandle.mPendingPotential);
}

float AKLBuildableSlugHatching::GetRawProductionTime() const { return GetDefaultProductionCycleTime(); }

float AKLBuildableSlugHatching::GetProductionTime() const { return mProductionHandle.GetProductionTime(); }

void AKLBuildableSlugHatching::OnDayTimeUpdated(bool isDayTime)
{
	SetSlugTime(isDayTime ? EKAPISlugTime::Day : EKAPISlugTime::Night);
}

void AKLBuildableSlugHatching::DoGatherStats()
{
	if (!HasAuthority() || bGatherStatsPending)
	{
		return;
	}
	bGatherStatsPending = true;

	EKAPISlugTime NewTime = EKAPISlugTime::NONE;
	float Temperature = 25.0f;
	float Humidity = .5f;

	bool bNewHasTankModule = false;
	bool bNewHasIncubatorModule = false;
	bool bNewHasTimeModule = false;

	UKPCLModularBuildingHandlerStacker* Handler = GetHandler<UKPCLModularBuildingHandlerStacker>();
	if (IsValid(Handler))
	{
		for (AFGBuildable* Buildable : Handler->mAttachmentDatas[0].mSnappedActors)
		{
			if (!IsValid(Buildable))
			{
				continue;
			}
			if (AKLBuildableSlugHatchingModule* Module = Cast<AKLBuildableSlugHatchingModule>(Buildable))
			{
				if (Module->mModuleType == EHatchingModuleType::Time)
				{
					bNewHasTimeModule = true;
					if (Module->mOverwriteTime != EKAPISlugTime::NONE)
					{
						NewTime = Module->mOverwriteTime;
					}
				}

				if (Module->mModuleType == EHatchingModuleType::Humidity)
				{
					Humidity += Module->mHumidity.mValue;
				}

				if (Module->mModuleType == EHatchingModuleType::Temperature)
				{
					Temperature += Module->mTemperature.mValue;
				}

				if (Module->mModuleType == EHatchingModuleType::Tank)
				{
					bNewHasTankModule = true;
				}

				if (Module->mModuleType == EHatchingModuleType::Incubator)
				{
					bNewHasIncubatorModule = true;
				}
			}
		}
	}

	SetOverwriteTime(NewTime);
	SetHatching_Temperature(Temperature);
	SetHatching_Humidity(FMath::Clamp(Humidity, .0f, 1.f));

	if (bHasIncubatorModule != bNewHasIncubatorModule)
	{
		SetHasIncubatorModule(bNewHasIncubatorModule);
		OnIncubatorChanged(bNewHasIncubatorModule);
	}

	if (bHasTankModule != bNewHasTankModule)
	{
		SetHasTankModule(bNewHasTankModule);
		OnTankChanged(bNewHasTankModule);
	}

	if (bHasTimeModule != bNewHasTimeModule)
	{
		SetHasTimeModule(bNewHasTimeModule);
		OnTimeModuleChanged(bNewHasTimeModule);
	}

	if (GetInventory())
	{
		GetInventory()->AddArbitrarySlotSize(FLUID_IDX, 50000);
	}

	mProductionHandle.SetNewTime(GetRawProductionTime(), false);
	bGatherStatsPending = false;
}

void AKLBuildableSlugHatching::OnIncubatorChanged_Implementation(bool bHas) {}

void AKLBuildableSlugHatching::OnTankChanged_Implementation(bool bHas) {}

void AKLBuildableSlugHatching::OnFluidFinial_Implementation()
{
	UKAPISugHatchingData* HatchingData = GetHatchingData();
	if (IsValid(HatchingData) && HatchingData->IncubationFluidRequired())
	{
		GetInventory()->RemoveFromIndex(FLUID_IDX, HatchingData->mFluidConsume);
	}
}

void AKLBuildableSlugHatching::OnTimeModuleChanged_Implementation(bool bHas) {}

UKAPISugHatchingData* AKLBuildableSlugHatching::GetHatchingData() const
{
	if (!IsValid(mCurrentHatchingData))
	{
		return mDefaultHatchingData;
	}
	return mCurrentHatchingData;
}

FFullProductionHandle AKLBuildableSlugHatching::GetFluidHandle() const { return mFluidProductionHandle; }

float AKLBuildableSlugHatching::GetFluidRawProductionTime() const { return mFluidProductionHandle.mProductionTime; }

float AKLBuildableSlugHatching::GetFluidProductionTime() const { return mFluidProductionHandle.GetProductionTime(); }

EKAPISlugTime AKLBuildableSlugHatching::GetCurrentSlugTime() const
{
	if (mOverwriteTime != EKAPISlugTime::NONE)
	{
		return mOverwriteTime;
	}
	return mTime;
}

float AKLBuildableSlugHatching::GetChanceForSlug(TSubclassOf<UFGItemDescriptor> SlugClass) const
{
	UKAPISugHatchingData* HatchingData = GetHatchingData();
	if (!IsValid(HatchingData))
	{
		return 0.f;
	}

	bool bFound = false;
	FKAPISlugIncubation Incubation;
	for (const FKAPISlugIncubation& PossibleSlug : HatchingData->mPossibleSlugs)
	{
		if (PossibleSlug.mSlug == SlugClass)
		{
			Incubation = PossibleSlug;
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		return 0.f;
	}

	for (const FKAPISlugFixedChance& Fixed : mFixedChanceList)
	{
		if (Fixed.mSlug == SlugClass)
		{
			return Fixed.mChance;
		}
	}
	return Incubation.mProbability;
}

void AKLBuildableSlugHatching::OnNewEgg_Native_Implementation(UKAPISugHatchingData* NewEgg, bool bSlugTypeChanged)
{
	if (HasAuthority() && IsValid(GetOutputInventory()) && IsValid(NewEgg))
	{
		SetCurrentHatchingData(NewEgg);
		if (!IsValid(mCurrentHatchingData))
		{
			SetCurrentHatchingData(mDefaultHatchingData);
		}
		TArray<FKAPISlugIncubation> SlugHatchingInformation = mCurrentHatchingData->mPossibleSlugs;

		if (bSlugTypeChanged)
		{
			mFixedChanceList.Reset(SlugHatchingInformation.Num());
			for (const FKAPISlugIncubation& Incubation : SlugHatchingInformation)
			{
				if (Incubation.Valid() && Incubation.bShouldUseFixedChance)
				{
					mFixedChanceList.Emplace(Incubation.mSlug, Incubation.mProbability);
				}
			}
			CommitFixedChanceList();
		}
		TArray<TSubclassOf<UFGItemDescriptor>> Slugs = GetAllPossibleSlugs();

		if (SlugHatchingInformation.Num() > 0)
		{
			// Set Allowed on indexes
			for (int i = 0; i < GetOutputInventory()->GetSizeLinear(); ++i)
			{
				if (GetOutputInventory()->IsValidIndex(i))
				{
					TSubclassOf<UFGItemDescriptor> ItemClass = Slugs.IsValidIndex(i)
						? TSubclassOf<UFGItemDescriptor>{Slugs[i]}
						: TSubclassOf<UFGItemDescriptor>{UFGNoneDescriptor::StaticClass()};
					if (!ItemClass)
					{
						ItemClass = UFGNoneDescriptor::StaticClass();
					}
					UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetOutputInventory(), i, ItemClass);
				}
			}

			// Set Time & Make sure that the Hatching starting again from 0 if a new Hatching is beginning...
			mProductionHandle.mProductionTime = mCurrentHatchingData->mHatchDuration;
			if (bSlugTypeChanged)
			{
				mProductionHandle.Reset();
			}

			// Flush Fluids
			if (mCurrentHatchingData->IncubationFluidRequired())
			{
				FInventoryStack Stack = GetStackFromIndex(FLUID_IDX);
				if (Stack.HasItems())
				{
					if (Stack.Item.GetItemClass() != mCurrentHatchingData->mFluidClass)
					{
						GetInventory()->RemoveAllFromIndex(FLUID_IDX);
					}
				}

				UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), FLUID_IDX,
																	  mCurrentHatchingData->mFluidClass);
				mFluidProductionHandle.SetNewTime(mCurrentHatchingData->mFluidConsumeTime);
			}
			else
			{
				UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), FLUID_IDX,
																	  UFGNoneDescriptor::StaticClass());
				GetInventory()->RemoveAllFromIndex(FLUID_IDX);
				mFluidProductionHandle.SetNewTime(0);
			}
		}
		DoGatherStats();
	}

	SetCurrentHatchingData(NewEgg);
	if (!IsValid(mCurrentHatchingData))
	{
		SetCurrentHatchingData(mDefaultHatchingData);
	}
	CommitFluidProductionHandle();

	if (!IsValid(mCurrentHatchingData))
	{
		UE_LOGFMT(LogKLIB, Warning,
				  "OnNewEgg_Native called with invalid data on {0} default hatching data should be: {1}", GetName(),
				  mDefaultHatchingData ? *mDefaultHatchingData->GetName() : TEXT("NULLPETER!!!"));
		return;
	}

	TArray<FKAPISlugIncubation> SlugHatchingInformation = mCurrentHatchingData->mPossibleSlugs;

	if (SlugHatchingInformation.Num() > 0)
	{
		// Called BP Event for UI or so
		OnNewEgg(NewEgg, bSlugTypeChanged);
	}
}
