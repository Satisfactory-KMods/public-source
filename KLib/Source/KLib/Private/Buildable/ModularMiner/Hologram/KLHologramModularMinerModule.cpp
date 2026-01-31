#include "Buildable/ModularMiner/Hologram/KLHologramModularMinerModule.h"

#include "AITypes.h"
#include "FGConstructDisqualifier.h"
#include "Buildable/ModularMiner/KLMMBuildableMiner.h"
#include "Resources/FGResourceNode.h"

AKLHologramModularMinerModule::AKLHologramModularMinerModule()
	: mCanUpdate(false)
{
	mValidHitClasses.Add(AKLMMBuildableMiner::StaticClass());
	mValidHitClasses.Add(AKLMMBuildableModule::StaticClass());
}

bool AKLHologramModularMinerModule::IsModuleAllowed(UKPCLModularBuildingHandlerBase* Handler,
                                                    AFGBuildable* TargetBuildable, const FHitResult& hitResult)
{
	TSubclassOf<AKLMMBuildableModule> ModuleClass = TSubclassOf<AKLMMBuildableModule>(mBuildClass);
	if (ModuleClass)
	{
		if (AKLMMBuildableMiner* Miner = Cast<AKLMMBuildableMiner>(TargetBuildable))
		{
			return Miner->IsModuleAllowed(ModuleClass);
		}
	}

	return false;
}


AActor* AKLHologramModularMinerModule::GetUpgradedActor() const
{
	return mCanUpdate ? mUpgradedActor : nullptr;
}

bool AKLHologramModularMinerModule::TryUpgrade(const FHitResult& hitResult)
{
	mCanUpdate = false;
	const auto Hologram = GetDefaultBuildable<AKLMMBuildableModule>();
	const auto TargetCheck = Cast<AKLMMBuildableModule>(hitResult.GetActor());
	if (Hologram && TargetCheck)
	{
		if (Hologram->GetType() == TargetCheck->GetType())
		{
			if (Hologram->GetTier() != TargetCheck->GetTier())
			{
				mCanUpdate = true;
				mUpgradedActor = TargetCheck;
				mModuleMasterHit = TargetCheck->Execute_GetMasterBuilding(TargetCheck);
				mSnapLocation.SetLocation(TargetCheck->GetActorLocation());
				this->SetActorTransform(TargetCheck->GetActorTransform());
				return true;
			}
		}
	}

	return mCanUpdate;
}

void AKLHologramModularMinerModule::ConfigureComponents(AFGBuildable* inBuildable) const
{
	if (mCanUpdate)
	{
		IKPCLModularBuildingInterface::Execute_RemoveAttachedActor(mModuleMasterHit, mUpgradedActor);
	}

	Super::ConfigureComponents(inBuildable);
}
