// ILikeBanas


#include "Buildable/Slugs/KLBuildableSlugHatchingModule.h"

#include "BFL/KBFL_Player.h"
#include "Buildable/Modular/KPCLModularBuildingHandlerStacker.h"
#include "Buildable/Slugs/KLBuildableSlugHatching.h"
#include "FGPowerInfoComponent.h"

#include "Net/UnrealNetwork.h"

#include "Replication/KLDefaultRCO.h"

void AKLBuildableSlugHatchingModule::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AKLBuildableSlugHatchingModule::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);

	FG_DOREPCONDITIONAL(ThisClass, mTemperature);
	FG_DOREPCONDITIONAL(ThisClass, mHumidity);
	FG_DOREPCONDITIONAL(ThisClass, mOverwriteTime);
}

void AKLBuildableSlugHatchingModule::CommitTemperature()
{
	mPropertyReplicator.MarkPropertyDirty(FName("mTemperature"));
}

void AKLBuildableSlugHatchingModule::CommitHumidity() { mPropertyReplicator.MarkPropertyDirty(FName("mHumidity")); }

void AKLBuildableSlugHatchingModule::CommitOverwriteTime()
{
	mPropertyReplicator.MarkPropertyDirty(FName("mOverwriteTime"));
}

bool AKLBuildableSlugHatchingModule::CanUseFactoryClipboard_Implementation() { return true; }

void AKLBuildableSlugHatchingModule::BeginPlay()
{
	Super::BeginPlay();

	if (mModuleType == EHatchingModuleType::Incubator)
	{
		UpdateIncubatorVisuals();
	}

	if (mModuleType == EHatchingModuleType::Temperature)
	{
		UpdateTemperatureVisuals();
	}
}

bool AKLBuildableSlugHatchingModule::CanProduce_Implementation() const
{
	if (IsPlayingBuildEffect() || !HasPower())
	{
		return false;
	}

	if (GetMasterBuildable<AKLBuildableSlugHatching>())
	{
		return GetMasterBuildable<AKLBuildableSlugHatching>()->IsProducing();
	}
	return Super::CanProduce_Implementation();
}

bool AKLBuildableSlugHatchingModule::IsModuleTypeOf_Implementation(uint8 Type)
{
	if (mModuleType == EHatchingModuleType::None)
	{
		return false;
	}
	return static_cast<uint8>(mModuleType) == Type;
}

void AKLBuildableSlugHatchingModule::Stacker_AddBuildingHeight_Implementation(float& Height)
{
	Height += mBuildingHeight;
}

void AKLBuildableSlugHatchingModule::GetChildDismantleActors_Implementation(
	TArray<AActor*>& Out_ChildDismantleActors) const
{
	Super::GetChildDismantleActors_Implementation(Out_ChildDismantleActors);
	if (mMasterBuilding.IsValid())
	{
		UKPCLModularBuildingHandlerStacker* Handler =
			Cast<UKPCLModularBuildingHandlerStacker>(Execute_GetModularHandler(mMasterBuilding.Get()));
		if (Handler)
		{
			TArray<AFGBuildable*> Modules = Handler->mAttachmentDatas[0].GetActorsUpperIndex(mModularIndex);
			if (Modules.Num() > 0)
			{
				for (AFGBuildable* Module : Modules)
				{
					if (Module)
					{
						Out_ChildDismantleActors.AddUnique(Module);
					}
				}
			}
		}
	}
}

void AKLBuildableSlugHatchingModule::HandlePower(float dt)
{
	mPowerOptions.bHasPower = Factory_HasPower();
	if (UFGPowerInfoComponent* PowerInfo = GetPowerInfo())
	{
		PowerInfo->SetTargetConsumption(0.0f);
		PowerInfo->SetMaximumTargetConsumption(0.0f);
	}
}

bool AKLBuildableSlugHatchingModule::Factory_HasPower() const
{
	if (AKLBuildableSlugHatching* Hatching = GetMasterBuildable<AKLBuildableSlugHatching>())
	{
		return Hatching->HasPower();
	}

	return false;
}

bool AKLBuildableSlugHatchingModule::GetHasPower() const { return mPowerOptions.bHasPower; }

void AKLBuildableSlugHatchingModule::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);

	if (mModuleType == EHatchingModuleType::Temperature)
	{
		HandleTemperatureModule(dt);
	}
	else if (mModuleType == EHatchingModuleType::Tank)
	{
		HandleTankModule(dt);
	}
	else if (mModuleType == EHatchingModuleType::Incubator)
	{
		HandleIncubatorModule(dt);
	}
	else if (mModuleType == EHatchingModuleType::Humidity)
	{
		HandleHumidityModule(dt);
	}
	else if (mModuleType == EHatchingModuleType::Time)
	{
		HandleTimeModule(dt);
	}
}

void AKLBuildableSlugHatchingModule::HandleTemperatureModule(float dt)
{
	if (mTemperature.Tick(dt))
	{
		if (AKLBuildableSlugHatching* Master = Cast<AKLBuildableSlugHatching>(GetMasterBuildable()))
		{
			if (HasAuthority())
			{
				CommitTemperature();
				SetPowerMultiplier(mTemperature.mValue >= 0.f ? mTemperature.mValue / mTemperature.mMax
															  : mTemperature.mValue / mTemperature.mMin);
				Master->DoGatherStats();
			}

			UpdateTemperatureVisuals();
		}
	}
}

void AKLBuildableSlugHatchingModule::UpdateTemperatureVisuals()
{
	if (IsValid(Cast<AKLBuildableSlugHatching>(GetMasterBuildable())))
	{
		if (UKPCLColoredStaticMesh* ColoredMesh = FindComponentByClass<UKPCLColoredStaticMesh>())
		{
			ColoredMesh->ApplyNewColorData(FKPCLColorData(0,
														  mTemperature.mValue >= 0.f
															  ? mTemperature.mValue / mTemperature.mMax
															  : mTemperature.mValue / mTemperature.mMin));
			ColoredMesh->ApplyNewColorData(FKPCLColorData(1, mTemperature.mValue < 0.f));
		}
		AIO_UpdateCustomFloat(20,
							  mTemperature.mValue >= 0.f ? mTemperature.mValue / mTemperature.mMax
														 : mTemperature.mValue / mTemperature.mMin,
							  0, false);
		AIO_UpdateCustomFloat(21, mTemperature.mValue < 0.f, 0, true);
	}
}

void AKLBuildableSlugHatchingModule::HandleTankModule(float dt) {}

void AKLBuildableSlugHatchingModule::HandleIncubatorModule(float dt)
{
	if (mIncubatorTimer.Tick(dt))
	{
		UpdateIncubatorVisuals();
	}
}

void AKLBuildableSlugHatchingModule::UpdateIncubatorVisuals()
{
	if (AKLBuildableSlugHatching* Hatchery = GetMasterBuildable<AKLBuildableSlugHatching>())
	{
		UKAPISugHatchingData* HatchingData = Hatchery->GetHatchingData();
		if (UKPCLColoredStaticMesh* ColoredMesh = FindComponentByClass<UKPCLColoredStaticMesh>())
		{
			ColoredMesh->ApplyNewLinearColorData(FKPCLLinearColorData(0, HatchingData->mEggColor));
			ColoredMesh->ApplyNewColorData(FKPCLColorData(3, true));
		}

		AIO_UpdateCustomFloatAsColor(20, HatchingData->mEggColor, 0, false);
		AIO_UpdateCustomFloat(23, true, 0, true);
	}
}

void AKLBuildableSlugHatchingModule::HandleHumidityModule(float dt)
{
	if (mHumidity.Tick(dt) && HasAuthority())
	{
		if (AKLBuildableSlugHatching* Master = Cast<AKLBuildableSlugHatching>(GetMasterBuildable()))
		{
			CommitHumidity();
			Master->DoGatherStats();
			SetPowerMultiplier(mHumidity.mValue >= 0.f ? mHumidity.mValue / mHumidity.mMax
													   : mHumidity.mValue / mHumidity.mMin);
		}
	}
}

void AKLBuildableSlugHatchingModule::HandleTimeModule(float dt) {}

void AKLBuildableSlugHatchingModule::ApplyNewHumidity(float Humidity)
{
	if (HasAuthority())
	{
		mHumidity.SetNewTarget(Humidity);
		CommitHumidity();
	}
	else if (UKLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKLDefaultRCO>(this))
	{
		RCO->Server_SetTargetHumidity(this, Humidity);
	}
}

void AKLBuildableSlugHatchingModule::ApplyNewTemperature(float Temperature)
{
	if (HasAuthority())
	{
		mTemperature.SetNewTarget(Temperature);
		CommitTemperature();
	}
	else if (UKLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKLDefaultRCO>(this))
	{
		RCO->Server_SetTargetTemperature(this, Temperature);
	}
}

void AKLBuildableSlugHatchingModule::ApplyNewOverwriteTime(EKAPISlugTime OverwriteTime)
{
	if (AKLBuildableSlugHatching* Master = GetMasterBuildable<AKLBuildableSlugHatching>())
	{
		if (HasAuthority())
		{
			mOverwriteTime = OverwriteTime;
			CommitOverwriteTime();
			Master->DoGatherStats();
		}
		else if (UKLDefaultRCO* RCO = UKPCLDefaultRCO::GetRCO<UKLDefaultRCO>(this))
		{
			RCO->Server_SetNewModuleTime(this, OverwriteTime);
		}
	}
}
