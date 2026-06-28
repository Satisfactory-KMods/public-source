// ILikeBanas


#include "Buildable/ModularMiner/KLMMBuildableBase.h"

#include "BFL/KBFL_Player.h"
#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Buildable/ModularMiner/KLMMBuildableModule.h"
#include "FGPowerInfoComponent.h"
#include "Logging.h"

#include "Net/UnrealNetwork.h"

#include "Replication/KLDefaultRCO.h"

void AKLMMBuildableBase::SetUIProductionBuff(float NewBuff)
{
	mUIProductionBuff = NewBuff;
	CommitUIProductionBuff();
}

void AKLMMBuildableBase::SetResourceToProduceInternal(TSubclassOf<UFGItemDescriptor> NewResource)
{
	mResourceToProduce = NewResource;
	mPropertyReplicator.MarkPropertyDirty(FName("mResourceToProduce"));
}

void AKLMMBuildableBase::CommitMinerTask() { mPropertyReplicator.MarkPropertyDirty(FName("mMinerTask")); }

void AKLMMBuildableBase::CommitModuleProductionTask()
{
	mPropertyReplicator.MarkPropertyDirty(FName("mModuleProductionTask"));
}

void AKLMMBuildableBase::CommitFluidConsumeTask() { mPropertyReplicator.MarkPropertyDirty(FName("mFluidConsumeTask")); }

void AKLMMBuildableBase::CommitUIProductionBuff() { mPropertyReplicator.MarkPropertyDirty(FName("mUIProductionBuff")); }

AKLMMBuildableBase::AKLMMBuildableBase()
{
	mInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::InputName);
	mInventory->SetDefaultSize(3);
	mBoosterInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::BoosterName);
	mBoosterInventory->SetDefaultSize(1);
}

void AKLMMBuildableBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		HandlePowerInit();
		SetResourceToProduce();
		SetupInventory();
	}
}

void AKLMMBuildableBase::Factory_TickAuthOnly(float dt)
{
	Super::Factory_TickAuthOnly(dt);
	HandlePower(dt);
	HandleState();
	SetResourceToProduce();

	mMinerTask.mProductionTime = GetNodeProductionTime();
	mMinerTask.TickHandle(dt, IsProducing(), [&]() { OnMainProductionFinial(); });
	CommitMinerTask();

	AKLMMBuildableModule* Module = GetFirstModule(mWasteProducerAttachment);
	bool ModuleIsProducing = IsProducing() && IsValid(Module);

	mModuleProductionTask.TickHandle(dt, ModuleIsProducing,
									 [&]()
									 {
										 OnModuleProductionFinial();
										 Module->OnModuleProductionCompleted();
									 });
	CommitModuleProductionTask();

	float NewUiBuff = GetProductionCycleBuff();
	if (NewUiBuff != mUIProductionBuff)
	{
		SetUIProductionBuff(NewUiBuff);
	}
}

void AKLMMBuildableBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool AKLMMBuildableBase::CanProduce_Implementation() const
{
	if (IsPlayingBuildEffect() || !HasPower() || IsProductionPaused())
	{
		return false;
	}

	return CheckProduction();
}

void AKLMMBuildableBase::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);

	FG_DOREPCONDITIONAL(ThisClass, mMinerTask);
	FG_DOREPCONDITIONAL(ThisClass, mFluidConsumeTask);
	FG_DOREPCONDITIONAL(ThisClass, mModuleProductionTask);
	FG_DOREPCONDITIONAL(ThisClass, mResourceToProduce);
	FG_DOREPCONDITIONAL(ThisClass, mUIProductionBuff);
}

void AKLMMBuildableBase::GetChildDismantleActors_Implementation(TArray<AActor*>& Out_ChildDismantleActors) const
{
	Super::GetChildDismantleActors_Implementation(Out_ChildDismantleActors);

	for (const auto Module : GetAllModules())
	{
		const auto CastModule = Cast<AKLMMBuildableModule>(Module);
		if (IsValid(CastModule))
		{
			Out_ChildDismantleActors.AddUnique(CastModule);
		}
	}
}

bool AKLMMBuildableBase::Factory_HasPower() const { return Super::Factory_HasPower(); }

void AKLMMBuildableBase::SetupInventory() {}

float AKLMMBuildableBase::GetNodeProductionTime() const
{
	switch (GetPurity())
	{
	case RP_Inpure:
		return 4.f;
	case RP_Normal:
		return 2.f;
	case RP_Pure:
		return 1.f;
	default:
		return 2.f;
	}
}

void AKLMMBuildableBase::Factory_ProductionCycleCompleted(float overProductionRate) {}

void AKLMMBuildableBase::EndProductionTime()
{
	Super::EndProductionTime();

	mMinerTask.mCurrentPotential = GetCurrentPotential();
	mMinerTask.mPendingPotential = GetPendingPotential();
	mModuleProductionTask.mCurrentPotential = GetCurrentPotential();
	mModuleProductionTask.mPendingPotential = GetPendingPotential();

	CommitMinerTask();
	CommitModuleProductionTask();
	CommitUIProductionBuff();
}

void AKLMMBuildableBase::Factory_TickProducing(float dt) {}

void AKLMMBuildableBase::OnMainProductionFinial_Implementation() {}

float AKLMMBuildableBase::GetProductionsPerMin() const { return 60.00f / GetProductionCycleTime(); }

void AKLMMBuildableBase::FlushFluids()
{
	Super::FlushFluids();
	Server_Flush();
}

void AKLMMBuildableBase::Server_Flush()
{
	if (GetInventory())
	{
		if (!GetInventory()->IsIndexEmpty(2))
		{
			GetInventory()->RemoveAllFromIndex(2);
		}
	}
}

void AKLMMBuildableBase::SetResourceToProduce()
{
	bool bChanged = false;
	if (!mResourceToProduce)
	{
		SetResourceToProduceInternal(GetResourceClass());
		OnResourceToProduceChanged();
		bChanged = true;
	}

	if (mResourceToProduce != GetResourceToProduce())
	{
		SetResourceToProduceInternal(GetResourceToProduce());
		OnResourceToProduceChanged();
		bChanged = true;
	}

	if (bChanged)
	{
		CommitUIProductionBuff();
	}
}

void AKLMMBuildableBase::OnResourceToProduceChanged()
{
	if (GetInventory())
	{
		UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), 0, GetResourceToProduce());
		UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(GetInventory(), 1, GetResourceToProduce());

		if (!GetInventory()->IsIndexEmpty(0))
		{
			GetInventory()->RemoveAllFromIndex(0);
		}

		if (!GetInventory()->IsIndexEmpty(1))
		{
			GetInventory()->RemoveAllFromIndex(1);
		}

		// Set also the Task
		// mMinerTask.Reset();
		return;
	}

	GetWorldTimerManager().SetTimerForNextTick(this, &AKLMMBuildableBase::OnResourceToProduceChanged);
}

float AKLMMBuildableBase::GetMaxPowerConsume() const
{
	float Consume = mPowerOptions.GetMaxPowerConsume();

	TArray<AKLMMBuildableModule*> Modules = GetAllModules();
	for (AKLMMBuildableModule* Module : Modules)
	{
		if (Module)
		{
			Consume += Module->GetPowerOption().GetMaxPowerConsume();
		}
	}

	return Consume;
}

float AKLMMBuildableBase::GetPowerConsume() const
{
	float Consume = mPowerOptions.GetPowerConsume();

	TArray<AKLMMBuildableModule*> Modules = GetAllModules();
	for (AKLMMBuildableModule* Module : Modules)
	{
		if (Module)
		{
			Consume += Module->GetPowerOption().GetPowerConsume();
		}
	}

	return Consume;
}

float AKLMMBuildableBase::GetProductionTime() const { return GetMinerProductionHandle().GetProductionTime(); }

float AKLMMBuildableBase::GetPendingProductionTime() const
{
	return GetMinerProductionHandle().GetPendingProductionTime();
}

float AKLMMBuildableBase::GetPurityMulti() const
{
	if (GetPurity() == RP_Inpure)
	{
		return 0.5f;
	}
	if (GetPurity() == RP_Normal)
	{
		return 1.0f;
	}
	if (GetPurity() == RP_Pure)
	{
		return 2.0f;
	}
	return 0.0f;
}

TSubclassOf<UFGResourceDescriptor> AKLMMBuildableBase::GetResourceClass() const
{
	auto Interface = GetExtractableResource();
	if (Interface)
	{
		return Interface->GetResourceClass();
	}
	return nullptr;
}

EResourceForm AKLMMBuildableBase::GetResourceForm() const { return UFGItemDescriptor::GetForm(GetResourceClass()); }

EResourcePurity AKLMMBuildableBase::GetPurity() const
{
	if (Cast<AFGResourceNode>(GetExtractableResourceActor()))
	{
		return Cast<AFGResourceNode>(GetExtractableResourceActor())->GetResourcePurity();
	}
	return RP_MAX;
}

void AKLMMBuildableBase::OnModulesUpdated_Implementation()
{
	Super::OnModulesUpdated_Implementation(); // calls OnMeshUpdate() + Event_OnMeshUpdate()
	RecalculateMinerCaches();
}

void AKLMMBuildableBase::RecalculateMinerCaches()
{
	mCachedModules.Empty();
	if (UKPCLModularBuildingHandlerBase* Handler = GetHandler<UKPCLModularBuildingHandlerBase>())
	{
		TArray<AFGBuildable*> BuildableModules;
		Handler->GetAttachedActors(BuildableModules);
		for (AFGBuildable* Module : BuildableModules)
		{
			if (AKLMMBuildableModule* MMModule = Cast<AKLMMBuildableModule>(Module))
			{
				mCachedModules.Add(MMModule);
			}
		}
	}
}

int AKLMMBuildableBase::GetOccupiedPoints(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const
{
	int32 Count = 0;
	for (const auto& Module : mCachedModules)
	{
		if (IsValid(Module) && Module->GetType() == AttachmentClass)
		{
			++Count;
		}
	}
	return Count;
}

bool AKLMMBuildableBase::HasModule(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const
{
	for (const auto& Module : mCachedModules)
	{
		if (IsValid(Module) && Module->GetType() == AttachmentClass)
		{
			return true;
		}
	}
	return false;
}

bool AKLMMBuildableBase::HasEnergyModuleSlot(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass,
											 int idx) const
{
	for (const auto& Module : mCachedModules)
	{
		if (IsValid(Module) && Module->GetType() == AttachmentClass &&
			IKPCLModularBuildingInterface::Execute_GetModularIndex(Module) == idx)
		{
			return true;
		}
	}
	return false;
}

TArray<bool>
AKLMMBuildableBase::GetBoolArrayOfEnergySlots(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const
{
	TArray<bool> lReturn = {false, false, false};
	int32 SlotIdx = 0;
	for (const auto& Module : mCachedModules)
	{
		if (IsValid(Module) && Module->GetType() == AttachmentClass)
		{
			if (lReturn.IsValidIndex(SlotIdx))
			{
				lReturn[SlotIdx++] = true;
			}
		}
	}
	return lReturn;
}

AKLMMBuildableModule*
AKLMMBuildableBase::GetFirstModule(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const
{
	for (const auto& Module : mCachedModules)
	{
		if (IsValid(Module) && Module->GetType() == AttachmentClass)
		{
			return Module;
		}
	}
	return nullptr;
}

TArray<class AKLMMBuildableModule*> AKLMMBuildableBase::GetAllModules() const
{
	TArray<AKLMMBuildableModule*> AllModules;
	AllModules.Reserve(mCachedModules.Num());
	for (const auto& Module : mCachedModules)
	{
		if (IsValid(Module))
		{
			AllModules.Add(Module);
		}
	}
	return AllModules;
}

TArray<class AKLMMBuildableModule*>
AKLMMBuildableBase::GetModule(TSubclassOf<UKAPIModularAttachmentDescriptor> AttachmentClass) const
{
	TArray<AKLMMBuildableModule*> Result;
	for (const auto& Module : mCachedModules)
	{
		if (IsValid(Module) && Module->GetType() == AttachmentClass)
		{
			Result.Add(Module);
		}
	}
	return Result;
}

void AKLMMBuildableBase::HandleState() {}

void AKLMMBuildableBase::HandlePower(float dt)
{
	GetPowerInfo()->SetTargetConsumption(GetPowerConsume());
	GetPowerInfo()->SetMaximumTargetConsumption(GetMaxPowerConsume());
	mPowerOptions.bHasPower = HasPower();
	mPowerOptions.StructureTick(dt, IsProducing());
}

void AKLMMBuildableBase::HandlePowerInit() { mPowerOptions.Init(); }

void AKLMMBuildableBase::OnModuleProductionFinial_Implementation() { ApplyPotentialToModule(); }

void AKLMMBuildableBase::ApplyPotentialToModule()
{
	mModuleProductionTask.mCurrentPotential = GetCurrentPotential();
	mModuleProductionTask.mPendingPotential = GetPendingPotential();
	mModuleProductionTask.mProductionTime = GetModuleProductionCycleTime();

	CommitModuleProductionTask();
	CommitUIProductionBuff();
}

void AKLMMBuildableBase::SetPendingPotential(float newPendingPotential)
{
	newPendingPotential = FMath::Clamp(newPendingPotential, GetCurrentMinPotential(), GetCurrentMaxPotential());
	Super::SetPendingPotential(newPendingPotential);

	mMinerTask.mPendingPotential = newPendingPotential;
	mModuleProductionTask.mPendingPotential = newPendingPotential;

	CommitMinerTask();
	CommitModuleProductionTask();
	CommitUIProductionBuff();
}

void AKLMMBuildableBase::OnFluidProductionFinial_Implementation() {}
