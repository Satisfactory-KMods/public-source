// ILikeBanas

#include "Buildable/Modular/KPCLModularExtractorBase.h"

#include "AkAcousticTextureSetComponent.h"
#include "FGFactoryConnectionComponent.h"
#include "FGPlayerController.h"
#include "FGPowerConnectionComponent.h"
#include "FGPowerInfoComponent.h"
#include "Logging.h"

#include "Net/UnrealNetwork.h"

AKPCLModularExtractorBase::AKPCLModularExtractorBase() : mModularHandler(nullptr) {}

FText AKPCLModularExtractorBase::GetLookAtDecription_Implementation(AFGCharacterPlayer* byCharacter,
																	const FUseState& state) const
{
	return mShouldUseUiFromMaster && GetMasterBuildable() != this && IsValid(GetMasterBuildable())
		? Execute_GetLookAtDecription(GetMasterBuildable(), byCharacter, state)
		: Super::GetLookAtDecription_Implementation(byCharacter, state);
}

bool AKPCLModularExtractorBase::IsUseable_Implementation() const
{
	return mShouldUseUiFromMaster && GetMasterBuildable() != this && IsValid(GetMasterBuildable())
		? Execute_IsUseable(GetMasterBuildable())
		: Super::IsUseable_Implementation();
}

void AKPCLModularExtractorBase::OnUseStop_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& State)
{
	if (GetMasterBuildable() != this && mShouldUseUiFromMaster && mMasterBuilding.IsValid())
	{
		Execute_OnUseStop(mMasterBuilding.Get(), byCharacter, State);
		return;
	}
	Super::OnUseStop_Implementation(byCharacter, State);
}

void AKPCLModularExtractorBase::OnUse_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& State)
{
	if (GetMasterBuildable() != this && mShouldUseUiFromMaster && mMasterBuilding.IsValid())
	{
		Execute_OnUse(mMasterBuilding.Get(), byCharacter, State);
		return;
	}
	Super::OnUse_Implementation(byCharacter, State);
}

void AKPCLModularExtractorBase::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	Super::PreSaveGame_Implementation(saveVersion, gameVersion);
	mCanHaveBuildabled = true;
}

void AKPCLModularExtractorBase::OnBuildEffectFinished()
{
	Super::OnBuildEffectFinished();
	mCanHaveBuildabled = true;
}

bool AKPCLModularExtractorBase::GetCanHaveModules_Implementation()
{
	return (mCanHaveBuildabled || !IsPlayingBuildEffect()) && IsValid(mModularHandler);
}

AFGBuildable* AKPCLModularExtractorBase::GetMasterBuilding_Implementation()
{
	if (mMasterBuilding.IsValid())
	{
		return mMasterBuilding.Get();
	}
	return nullptr;
}

FPowerOptions AKPCLModularExtractorBase::GetPowerOptions_Implementation() { return mPowerOptions; }

UKPCLModularBuildingHandlerBase* AKPCLModularExtractorBase::GetModularHandler_Implementation()
{
	return mModularHandler;
}

void AKPCLModularExtractorBase::SetMasterBuilding_Implementation(AFGBuildable* Actor)
{
	mMasterBuilding = Actor;
	OnMasterBuildingReceived(Actor);
}

int32 AKPCLModularExtractorBase::GetModularIndex_Implementation() { return mModularIndex; }

void AKPCLModularExtractorBase::ApplyModularIndex_Implementation(int32 Index) { mModularIndex = Index; }

void AKPCLModularExtractorBase::SetAttachedActor_Implementation(AFGBuildable* Actor)
{
	if (Actor)
	{
		mMasterBuilding = Actor;
		OnMasterBuildingReceived(Actor);
	}
}

void AKPCLModularExtractorBase::RemoveAttachedActor_Implementation(AFGBuildable* Actor)
{
	if (mModularHandler)
	{
		mModularHandler->AttachedActorRemoved(Actor);
		OnModulesWasUpdated();
		if (OnModulesUpdated.IsBound())
		{
			OnModulesUpdated.Broadcast();
		}
	}
	else
	{
		if (mMasterBuilding.IsValid())
		{
			Execute_RemoveAttachedActor(mMasterBuilding.Get(), Actor);
		}
	}
}

bool AKPCLModularExtractorBase::AttachedActor_Implementation(AFGBuildable* Actor,
															 TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment,
															 FTransform Location, float Distance)
{
	if (mModularHandler)
	{
		if (mModularHandler->AddNewActorToAttachment(Actor, Attachment, Location, Distance))
		{
			OnModulesWasUpdated();
			if (OnModulesUpdated.IsBound())
			{
				OnModulesUpdated.Broadcast();
			}
			return true;
		}
	}
	return false;
}

void AKPCLModularExtractorBase::OnModulesUpdated_Implementation()
{
	OnMeshUpdate();
	Event_OnMeshUpdate();
}

void AKPCLModularExtractorBase::ReadyForVisuelUpdate()
{
	Super::ReadyForVisuelUpdate();
	OnMeshUpdate();
	Event_OnMeshUpdate();
}

void AKPCLModularExtractorBase::GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const
{
	Super::GetChildDismantleActors_Implementation(out_ChildDismantleActors);

	UKPCLModularBuildingHandlerBase* Base = GetHandler<UKPCLModularBuildingHandlerBase>();
	if (IsValid(Base) && mDismantleAllChilds)
	{
		TArray<AFGBuildable*> Buildables;
		Base->GetAttachedActors(Buildables);
		for (AFGBuildable* Buildable : Buildables)
		{
			if (IsValid(Buildable))
			{
				out_ChildDismantleActors.AddUnique(Buildable);
				Execute_GetChildDismantleActors(Buildable, out_ChildDismantleActors);
			}
		}
	}
}

void AKPCLModularExtractorBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		HandlePowerInit();
	}

	if (IsValid(GetHandler()))
	{
		GetHandler()->OnHandlerTriggerUpdate.AddUniqueDynamic(this, &AKPCLModularExtractorBase::OnMeshUpdate);
		GetHandler()->SetIsReplicated(true);
		OnMeshUpdate();
	}
}

void AKPCLModularExtractorBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (!mModularHandler)
	{
		if (mMasterBuilding.IsValid())
		{
			Execute_RemoveAttachedActor(mMasterBuilding.Get(), this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AKPCLModularExtractorBase::OnMasterBuildingReceived_Implementation(AActor* Actor) {}

void AKPCLModularExtractorBase::OnMeshUpdate() { Event_OnMeshUpdate(); }

UKPCLModularBuildingHandlerBase* AKPCLModularExtractorBase::GetHandler() const { return mModularHandler; }

AFGBuildable* AKPCLModularExtractorBase::GetMasterBuildable() const
{
	if (mMasterBuilding.IsValid())
	{
		return mMasterBuilding.Get();
	}
	return nullptr;
}

void AKPCLModularExtractorBase::OnModulesWasUpdated_Implementation() {}

void AKPCLModularExtractorBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLModularExtractorBase, mModularHandler);
	DOREPLIFETIME(AKPCLModularExtractorBase, mMasterBuilding);
	DOREPLIFETIME(AKPCLModularExtractorBase, mModularIndex);
}

void AKPCLModularExtractorBase::OnRep_MasterBuilding()
{
	if (mMasterBuilding.IsValid())
	{
		OnMasterBuildingReceived(mMasterBuilding.Get());
	}
}

void AKPCLModularExtractorBase::OnRep_ModularIndex() { ReadyForVisuelUpdate(); }

void AKPCLModularExtractorBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	mModularHandler = FindComponentByClass<UKPCLModularBuildingHandlerBase>();
}
