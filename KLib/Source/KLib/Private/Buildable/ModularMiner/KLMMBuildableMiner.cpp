// ILikeBanas


#include "Buildable/ModularMiner/KLMMBuildableMiner.h"

#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Buildable/ModularMiner/KLMMBuildableModule.h"
#include "Cpp/KBFLCppInventoryHelper.h"
#include "FGFactoryConnectionComponent.h"
#include "FGPlayerController.h"
#include "FGUnlockSubsystem.h"
#include "KPrivateCodeLib/Public/Logging.h"
#include "Logging.h"

#include "Net/UnrealNetwork.h"

#include "Replication/KLDefaultRCO.h"
#include "Resources/FGBuildDescriptor.h"
#include "Resources/FGNoneDescriptor.h"
#include "Subsystem/KLUnlockSubsystem.h"

void AKLMMBuildableMiner::SetExtractionInfo(UKAPIModularMinerDescription* NewInfo)
{
	mExtractionInfo = NewInfo;
	mPropertyReplicator.MarkPropertyDirty(FName("mExtractionInfo"));
}

void AKLMMBuildableMiner::SetCachedFluid(TSubclassOf<UFGItemDescriptor> NewFluid)
{
	mCachedFluid = NewFluid;
	mPropertyReplicator.MarkPropertyDirty(FName("mCachedFluid"));
}

AKLMMBuildableMiner::AKLMMBuildableMiner()
{
	mPipeInput = CreateDefaultSubobject<UFGPipeConnectionFactory>(TEXT("bPipeInput"));
	mPipeInput->SetupAttachment(RootComponent);
	mPipeInput->SetPipeConnectionType(EPipeConnectionType::PCT_CONSUMER);

	mBeltOutput_01 = CreateDefaultSubobject<UFGFactoryConnectionComponent>(TEXT("mBeltOutput_01"));
	mBeltOutput_01->SetupAttachment(RootComponent);
	mBeltOutput_01->SetDirection(EFactoryConnectionDirection::FCD_OUTPUT);

	mBeltOutput_02 = CreateDefaultSubobject<UFGFactoryConnectionComponent>(TEXT("mBeltOutput_02"));
	mBeltOutput_02->SetupAttachment(RootComponent);
	mBeltOutput_02->SetDirection(EFactoryConnectionDirection::FCD_OUTPUT);

	bEnableCustomOverclocking = true;
	mCurrentPotential = 2.f;
}

void AKLMMBuildableMiner::UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots)
{
	Super::UI_ApplyRelevantItems_Implementation(OutSlots);

	OutSlots.Add(GetTrashDescriptor());
	OutSlots.Add(GetResourceToProduce());
}

bool AKLMMBuildableMiner::Overclocking_UseInventory_Implementation(int32& UnlockedSlots)
{
	UnlockedSlots = 0;
	return false;
}

void AKLMMBuildableMiner::Overclocking_GetCostSlots_Implementation(TArray<FItemAmount>& OutSlots)
{
	OutSlots.Empty();
	/*for (AKLMMBuildableModule* Module : GetAllModules())
	{
		UClass* ResourceClass = Module->GetClass();
		TSubclassOf<UFGBuildDescriptor> BuildDesc = mOverclockingBuildingMapping.FindRef(ResourceClass);
		if (IsValid(BuildDesc))
		{
			OutSlots.Add(FItemAmount(BuildDesc, 1));
		}
	}*/
}

void AKLMMBuildableMiner::Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo)
{
	Super::Overclocking_GetInfo_Implementation(OutProductionInfo);
	OutProductionInfo.mItemClass = GetResourceToProduce();
	OutProductionInfo.mAmount = GetProductionAmount();
	OutProductionInfo.mDefaultProductionTime = mMinerTask.mProductionTime;
}

void AKLMMBuildableMiner::SetPendingPotential(float newPendingPotential)
{
	Super::SetPendingPotential(newPendingPotential);
	newPendingPotential = FMath::Clamp(newPendingPotential, GetCurrentMinPotential(), GetCurrentMaxPotential());
	mProductionHandle.mPendingPotential = newPendingPotential;
	mMinerTask.mPendingPotential = newPendingPotential;
	mModuleProductionTask.mPendingPotential = newPendingPotential;

	CommitMinerTask();
	CommitModuleProductionTask();
}

void AKLMMBuildableMiner::Overclocking_GetProductionResults_Implementation(
	TArray<FKPCLOverclockingProductionResults>& OutIngredients, TArray<FKPCLOverclockingProductionResults>& OutProducts)
{
	Super::Overclocking_GetProductionResults_Implementation(OutIngredients, OutProducts);

	FKPCLOverclockingProductionResults Output =
		FKPCLOverclockingProductionResults(GetResourceToProduce(), GetProductionAmount(), mMinerTask.mProductionTime);

	OutProducts.Add(Output);
	OutProducts.Add(Output);

	if (HasModule(mWasteProducerAttachmentClass))
	{
		if (TSubclassOf<UFGItemDescriptor> TrashItem = GetTrashDescriptor())
		{
			FKPCLOverclockingProductionResults TrashOutput = FKPCLOverclockingProductionResults(
				TrashItem, GetProductionCrusherAmount(), GetModuleProductionCycleTime());
			OutProducts.Add(TrashOutput);
		}
	}
}

void AKLMMBuildableMiner::OnBuildEffectFinished()
{
	Super::OnBuildEffectFinished();

	if (HasAuthority())
	{
		AFGUnlockSubsystem* UnlockSubsystem = AFGUnlockSubsystem::Get(GetWorld());
		if (IsValid(UnlockSubsystem) && !UnlockSubsystem->GetIsBuildingEfficiencyUnlocked())
		{
			SetPendingPotential(2.f);

			mProductionHandle.mCurrentPotential = GetPendingPotential();
			mProductionHandle.mPendingPotential = GetPendingPotential();
			mMinerTask.mCurrentPotential = GetPendingPotential();
			mMinerTask.mPendingPotential = GetPendingPotential();
			mModuleProductionTask.mCurrentPotential = GetPendingPotential();
			mModuleProductionTask.mPendingPotential = GetPendingPotential();

			CommitMinerTask();
			CommitModuleProductionTask();
			CommitUIProductionBuff();
		}
	}
}

void AKLMMBuildableMiner::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AKLMMBuildableMiner::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);

	FG_DOREPCONDITIONAL(ThisClass, mExtractionInfo);
	FG_DOREPCONDITIONAL(ThisClass, mCachedFluid);
}

void AKLMMBuildableMiner::Factory_TickAuthOnly(float dt)
{
	Super::Factory_TickAuthOnly(dt);
	mProductionHandle.mPendingPotential = GetPendingPotential();
	mModuleProductionTask.mPendingPotential = GetPendingPotential();
	mModuleProductionTask.mProductionTime = GetModuleProductionCycleTime();
	mFluidConsumeTask.TickHandle(dt, HasModule(mFluidAttachmentClass) && IsProducing(),
								 [&]() { OnFluidProductionFinial(); });
	{
		TSubclassOf<UFGItemDescriptor> CurrentFluid = GetCurrentFluid();
		if (CurrentFluid != mCachedFluid)
		{
			SetCachedFluid(CurrentFluid);
		}
	}

	CommitModuleProductionTask();
	CommitFluidConsumeTask();
	CommitUIProductionBuff();
}

void AKLMMBuildableMiner::CollectAndPushPipes(float dt, bool IsPush)
{
	Super::CollectAndPushPipes(dt, IsPush);
	if (!IsPush)
	{
		UKBFLCppInventoryHelper::PullPipe(GetInventory(), 2, dt, GetAllowedFluidDesc(), GetPipe(0));
	}
}

void AKLMMBuildableMiner::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (!GetResourceClass())
		{
			UE_LOG(LogKLIB, Error, TEXT("AKLMMBuildableMiner::BeginPlay: No ResourceClass found for %s"), *GetName());
			return;
		}

		UKAPIModularMinerDescription* TempExtractionInfo = nullptr;
		GetAssetSubsystem()->Miner_GetForKey(GetResourceClass(), TempExtractionInfo);
		SetExtractionInfo(TempExtractionInfo);
		fgcheckf(IsValid(mExtractionInfo),
				 TEXT("AKLMMBuildableMiner::BeginPlay: No MinerInfo (DataAsset) found for %s. If this Resource is not "
					  "provided from SFP please use KAPI to implement this resource. (look on our mod page for more "
					  "informations)"),
				 *GetResourceClass()->GetName());

		// GetCurrentPotential() / GetPendingPotential() return the SaveGame float from AFGBuildableFactory,
		// which is zero-initialised by UObject on first placement. Substitute 1.0 (100% speed) in that
		// case so GetProductionTime() never divides by zero and the handle can always advance.
		const float SafeCurrent = FMath::Max(GetCurrentPotential(), 1.f);
		const float SafePending = FMath::Max(GetPendingPotential(), 1.f);
		mMinerTask.mCurrentPotential = SafeCurrent;
		mMinerTask.mPendingPotential = SafePending;
		mModuleProductionTask.mCurrentPotential = SafeCurrent;
		mModuleProductionTask.mPendingPotential = SafePending;

		ApplyPotentialToModule();

		AFGUnlockSubsystem* UnlockSubsystem = AFGUnlockSubsystem::Get(GetWorld());
		if (IsValid(UnlockSubsystem) && !UnlockSubsystem->GetIsBuildingEfficiencyUnlocked())
		{
			SetPendingPotential(2.f);

			mProductionHandle.mCurrentPotential = GetPendingPotential();
			mProductionHandle.mPendingPotential = GetPendingPotential();
			mMinerTask.mCurrentPotential = GetPendingPotential();
			mMinerTask.mPendingPotential = GetPendingPotential();
			mModuleProductionTask.mCurrentPotential = GetPendingPotential();
			mModuleProductionTask.mPendingPotential = GetPendingPotential();
		}

		CommitMinerTask();
		CommitModuleProductionTask();
		CommitUIProductionBuff();
	}
}

void AKLMMBuildableMiner::Factory_ProductionCycleCompleted(float overProductionRate) {}

void AKLMMBuildableMiner::Factory_TickProducing(float dt) {}

float AKLMMBuildableMiner::GetProductionCycleBuff() const
{
	if (!IsValid(GetMinerInfoInternal()))
	{
		return 0.f;
	}

	float lBuff = 0.0f;
	float lBuffMulti = 1.0f;

	for (AKLMMBuildableModule* Module : GetAllModules())
	{
		if (Module->GetType() == mFluidAttachmentClass)
		{
			if (GetCurrentFluid())
			{
				TSubclassOf<UFGItemDescriptor> lEmtpyFiller;
				lBuff += GetCurrentFluidInfo(lEmtpyFiller).ProductionTimeMulti;
			}
		}
		lBuff += Module->GetBonus();

		if (Module->IsWasteProducer())
		{
			// Compound malus from multiple waste producers deterministically.
			const float Malus = Module->GetMalus();
			if (Malus > 0.0f)
			{
				lBuffMulti *= Malus;
			}
		}
		else
		{
			lBuff -= Module->GetMalus();
		}
	}

	if (lBuff > 0.0f)
	{
		return GetPurityMulti() + lBuff * lBuffMulti;
	}
	return GetPurityMulti();
}

float AKLMMBuildableMiner::GetCurrentMaxPotential() const { return 1.f; }

float AKLMMBuildableMiner::GetNodeProductionTime() const
{
	float Raw = 2.f;
	float Buff = FMath::Max(GetProductionCycleBuff(), 0.5f);
	return Raw / Buff;
}

TSubclassOf<UFGItemDescriptor> AKLMMBuildableMiner::GetTrashDescriptor() const
{
	if (GetMinerInfoInternal())
	{
		if (AKLMMBuildableModule* WasteModule = GetFirstModule(mWasteProducerAttachmentClass))
		{
			if (WasteModule->IsWasteProducer())
			{
				FKAPIModuleItems Item = GetMinerInfoInternal()->GetItemsForModule(WasteModule->GetWasteClass());

				return Item.mTrashItem;
			}
		}
	}
	return nullptr;
}

bool AKLMMBuildableMiner::CheckStorage() const
{
	if (!GetMinerInfoInternal())
	{
		return false;
	}

	if (GetClampedPossibleProduction() <= 0)
	{
		return false;
	}

	// Waste/crusher module: if its output storage cannot take this cycle's trash, STOP.
	if (AKLMMBuildableModule* WasteModule = GetFirstModule(mWasteProducerAttachmentClass))
	{
		if (!WasteModule->CanStorageOutput(GetTrashDescriptor(), GetProductionCrusherAmount()))
		{
			return false;
		}
	}

	// Fluid module: requires enough input fluid buffered to consume this cycle, else STOP.
	if (GetFirstModule(mFluidAttachmentClass))
	{
		TSubclassOf<UFGItemDescriptor> lEmptyFiller;
		const float NeededFluid = GetCurrentFluidInfo(lEmptyFiller).mNormalFluidCountPerSecond * GetPurityMulti();
		if (!UKBFLCppInventoryHelper::HasItems(GetInventory(), GetCurrentFluid(), NeededFluid))
		{
			return false;
		}
	}

	return true;
}

bool AKLMMBuildableMiner::CheckProduction() const
{
	if (!GetMinerInfoInternal())
	{
		return false;
	}

	if (HasModule(mDrillAttachmentClass))
	{
		if (HasAllNeededModules())
		{
			if (const auto lModuleDrill = GetFirstModule(mDrillAttachmentClass))
			{
				if (lModuleDrill->GetTier() >= GetMinerInfoInternal()->mDrillTier)
				{
					if (HasAllNeededModules() && !IsProductionPaused())
					{
						return CheckStorage();
					}
				}
			}
		}
	}
	return false;
}

void AKLMMBuildableMiner::HandleState()
{
	if (HasPower())
	{
		if (IsProducing())
		{
			mCurrentState = ENewProductionState::Producing;
			return;
		}
		if (!HasAllNeededModules())
		{
			mCurrentState = ENewProductionState::ExtraState_1;
			return;
		}
		if (IsProductionPaused())
		{
			mCurrentState = ENewProductionState::Paused;
			return;
		}
		mCurrentState = ENewProductionState::Idle;
		return;
	}
	mCurrentState = ENewProductionState::NoPower;
}

void AKLMMBuildableMiner::HandlePower(float dt) { Super::HandlePower(dt); }

TSubclassOf<UFGItemDescriptor> AKLMMBuildableMiner::GetResourceToProduce() const
{
	if (GetMinerInfoInternal())
	{
		if (const AKLMMBuildableModule* WasteModule = GetFirstModule(mWasteProducerAttachmentClass))
		{
			if (WasteModule->IsWasteProducer())
			{
				FKAPIModuleItems Item = GetMinerInfoInternal()->GetItemsForModule(WasteModule->GetWasteClass());
				return Item.mProductionItem;
			}
		}
	}

	return GetResourceClass();
}

void AKLMMBuildableMiner::SetupInventory()
{
	if (const auto Interface = GetExtractableResource())
	{
		if (GetInventory())
		{
			UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), 0, GetResourceToProduce());
			UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), 1, GetResourceToProduce());
			UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), 2, nullptr);

			SetBelts();
		}
		else
		{
			GetWorldTimerManager().SetTimerForNextTick(this, &AKLMMBuildableMiner::SetupInventory);
			return; // inventory not ready yet — belt wiring happens on the retry
		}

		GetConv(0, KPCLOutput)->SetInventory(GetInventory());
		GetConv(0, KPCLOutput)->SetInventoryAccessIndex(0);
		GetConv(1, KPCLOutput)->SetInventory(GetInventory());
		GetConv(1, KPCLOutput)->SetInventoryAccessIndex(1);
		// GetPipe(0, KPCLInput)->SetInventory(GetInventory());
		GetPipe(0, KPCLInput)->SetInventoryAccessIndex(0);
	}
	else
	{
		UE_LOG(LogKLIB, Error, TEXT("Invalid ExtractableResource!!!"));
	}
}

void AKLMMBuildableMiner::OnMainProductionFinial_Implementation()
{
	Super::OnMainProductionFinial_Implementation();

	if (!GetInventory() || !GetResourceToProduce() || GetProductionAmount() <= 0)
	{
		return;
	}

	const int32 Amount = GetProductionAmount();
	const bool bCanStoreSlot0 =
		UKBFLCppInventoryHelper::CanStoreItem(GetInventory(), 0, GetResourceToProduce(), Amount);
	const bool bCanStoreSlot1 =
		UKBFLCppInventoryHelper::CanStoreItem(GetInventory(), 1, GetResourceToProduce(), Amount);

	if (bCanStoreSlot0 && bCanStoreSlot1)
	{
		UKBFLCppInventoryHelper::StoreItemAmountInInventory(GetInventory(), 0, GetResourceToProduce(), Amount);
		UKBFLCppInventoryHelper::StoreItemAmountInInventory(GetInventory(), 1, GetResourceToProduce(), Amount);
		return;
	}

	const int32 ClampedAmount = GetClampedPossibleProduction();
	if (ClampedAmount <= 0)
	{
		return;
	}

	if (bCanStoreSlot1)
	{
		// Slot 0 full/nearly full — redirect all production to slot 1
		UKBFLCppInventoryHelper::StoreItemAmountInInventory(GetInventory(), 1, GetResourceToProduce(), ClampedAmount);
	}
	else if (bCanStoreSlot0)
	{
		// Slot 1 full/nearly full — redirect all production to slot 0
		UKBFLCppInventoryHelper::StoreItemAmountInInventory(GetInventory(), 0, GetResourceToProduce(), ClampedAmount);
	}
	else
	{
		// Both slots partially full — partial-fill each via StoreItemAmountInInventory's fallback
		UKBFLCppInventoryHelper::StoreItemAmountInInventory(GetInventory(), 0, GetResourceToProduce(), Amount);
		UKBFLCppInventoryHelper::StoreItemAmountInInventory(GetInventory(), 1, GetResourceToProduce(), Amount);
	}
}

int32 AKLMMBuildableMiner::GetSizeFromOutputSlots() const
{
	if (!IsValid(GetInventory()))
	{
		return 0;
	}

	FInventoryStack Slot1;
	FInventoryStack Slot2;

	GetInventory()->GetStackFromIndex(0, Slot1);
	GetInventory()->GetStackFromIndex(1, Slot2);

	return Slot1.NumItems + Slot2.NumItems;
}

int32 AKLMMBuildableMiner::GetClampedPossibleProduction() const
{
	if (!GetResourceToProduce())
	{
		return 0;
	}
	int32 StackSizeDiff = UFGItemDescriptor::GetStackSize(GetResourceToProduce()) * 2 - GetSizeFromOutputSlots();
	return FMath::Clamp(StackSizeDiff, 0, GetProductionAmount() * 2);
}

void AKLMMBuildableMiner::OnModuleProductionFinial_Implementation()
{
	Super::OnModuleProductionFinial_Implementation();

	if (AKLMMBuildableModule* lModule = GetFirstModule(mWasteProducerAttachmentClass))
	{
		lModule->StorageItem(GetTrashDescriptor(), GetCrusherProductionAmount());
	}
}

void AKLMMBuildableMiner::OnFluidProductionFinial_Implementation()
{
	Super::OnFluidProductionFinial_Implementation();

	if (GetInventory())
	{
		TSubclassOf<UFGItemDescriptor> ItemClass;
		GetInventory()->RemoveFromIndex(2,
										(GetCurrentFluidInfo(ItemClass).mNormalFluidCountPerSecond * GetPurityMulti()));
	}
}

bool AKLMMBuildableMiner::HasAllNeededModules() const
{
	if (!GetMinerInfoInternal())
	{
		return false;
	}

	for (const auto& NeededModule : GetMinerInfoInternal()->mNeededModules)
	{
		if (!HasModule(NeededModule))
		{
			return false;
		}
	}
	return true;
}

FKAPIMinerInfoFluids AKLMMBuildableMiner::GetCurrentFluidInfo(TSubclassOf<UFGItemDescriptor>& FluidDesc) const
{
	FluidDesc = nullptr;
	if (GetCurrentFluid() && GetMinerInfoInternal())
	{
		FluidDesc = GetCurrentFluid();
		if (GetAllowedFluidDesc().Contains(GetCurrentFluid()))
		{
			return GetMinerInfoInternal()->mFluidInfo[GetCurrentFluid()];
		}
	}
	return FKAPIMinerInfoFluids();
}

TSubclassOf<UFGItemDescriptor> AKLMMBuildableMiner::GetCurrentFluid() const
{
	if (GetInventory())
	{
		FInventoryStack lStack;
		if (GetInventory()->GetStackFromIndex(2, lStack))
		{
			if (lStack.HasItems())
			{
				return lStack.Item.GetItemClass();
			}
		}
	}
	return mCachedFluid;
}

TArray<TSubclassOf<UFGItemDescriptor>> AKLMMBuildableMiner::GetAllowedFluidDesc() const
{
	if (!IsValid(GetMinerInfoInternal()))
	{
		return TArray<TSubclassOf<UFGItemDescriptor>>();
	}
	TArray<TSubclassOf<UFGItemDescriptor>> lList;
	GetMinerInfoInternal()->mFluidInfo.GetKeys(lList);
	return lList;
}

float AKLMMBuildableMiner::GetTierMulti(int Tier) const
{
	if (Tier == 2)
	{
		return 2.0f;
	}
	if (Tier == 3)
	{
		return 4.0f;
	}
	if (Tier > 3)
	{
		return 8.0f;
	}
	return 1.0f;
}

UKAPIModularMinerDescription* AKLMMBuildableMiner::GetMinerInfo(bool& Valid) const
{
	Valid = mExtractionInfo != nullptr;
	return mExtractionInfo;
}

UKAPIModularMinerDescription* AKLMMBuildableMiner::GetMinerInfoInternal() const { return mExtractionInfo; }

bool AKLMMBuildableMiner::IsValidResource() const { return mExtractionInfo != nullptr; }

int AKLMMBuildableMiner::GetProductionCrusherAmount() const { return GetCrusherProductionAmount(); }

int AKLMMBuildableMiner::GetProductionAmount() const
{
	if (HasModule(mDrillAttachmentClass))
	{
		auto DrillModule = GetFirstModule(mDrillAttachmentClass);
		if (IsValid(DrillModule))
		{
			int lAmount = mItemsPerCycle * GetTierMulti(DrillModule->GetTier());
			return FMath::Max(lAmount, 1);
		}
	}
	return 1;
}

int AKLMMBuildableMiner::GetCrusherProductionAmount() const
{
	if (GetTrashDescriptor())
	{
		if (UFGItemDescriptor::GetForm(GetTrashDescriptor()) == EResourceForm::RF_SOLID)
		{
			return GetProductionAmount() * 2;
		}
		return GetProductionAmount() * 2000;
	}
	return 0;
}

bool AKLMMBuildableMiner::IsModuleAllowed(TSubclassOf<AKLMMBuildableModule> Module) const
{
	if (!IsValid(mExtractionInfo))
	{
		return false;
	}
	return mExtractionInfo->IsModuleAllowed(Module);
}
