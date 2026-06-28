// ILikeBanas

#include "Buildable/Modular/KPCLModularBuildingBase.h"

#include "FGPlayerController.h"
#include "FGPowerConnectionComponent.h"
#include "Net/UnrealNetwork.h"

AKPCLModularBuildingBase::AKPCLModularBuildingBase() : mModularHandler(nullptr) {}

FText AKPCLModularBuildingBase::GetLookAtDecription_Implementation(AFGCharacterPlayer* byCharacter,
																   const FUseState& state) const
{
	return mShouldUseUiFromMaster && GetMasterBuildable() != this && IsValid(GetMasterBuildable())
		? Execute_GetLookAtDecription(GetMasterBuildable(), byCharacter, state)
		: Super::GetLookAtDecription_Implementation(byCharacter, state);
}

bool AKPCLModularBuildingBase::IsUseable_Implementation() const
{
	return mShouldUseUiFromMaster && GetMasterBuildable() != this && IsValid(GetMasterBuildable())
		? Execute_IsUseable(GetMasterBuildable())
		: Super::IsUseable_Implementation();
}

void AKPCLModularBuildingBase::OnUseStop_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& State)
{
	if (GetMasterBuildable() != this && mShouldUseUiFromMaster && IsValid(GetMasterBuildable()))
	{
		Execute_OnUseStop(mMasterBuilding.Get(), byCharacter, State);
		return;
	}
	Super::OnUseStop_Implementation(byCharacter, State);
}

void AKPCLModularBuildingBase::OnUse_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& State)
{
	if (GetMasterBuildable() != this && mShouldUseUiFromMaster && IsValid(GetMasterBuildable()))
	{
		Execute_OnUse(mMasterBuilding.Get(), byCharacter, State);
		return;
	}
	Super::OnUse_Implementation(byCharacter, State);
}

UFGFactoryClipboardSettings* AKPCLModularBuildingBase::CopySettings_Implementation()
{
	if (mShouldUseUiFromMaster && GetMasterBuildable() != this && IsValid(GetMasterBuildable()))
	{
		return Execute_CopySettings(GetMasterBuildable());
	}

	return Super::CopySettings_Implementation();
}

bool AKPCLModularBuildingBase::PasteSettings_Implementation(UFGFactoryClipboardSettings* factoryClipboard,
															class AFGPlayerController* player)
{
	if (mShouldUseUiFromMaster && GetMasterBuildable() != this && IsValid(GetMasterBuildable()))
	{
		return Execute_PasteSettings(GetMasterBuildable(), factoryClipboard, player);
	}

	return Super::PasteSettings_Implementation(factoryClipboard, player);
}

TSubclassOf<UKAPIModularAttachmentDescriptor> AKPCLModularBuildingBase::GetModularAttachmentClass_Implementation()
{
	return mUpgradeClass;
}

int32 AKPCLModularBuildingBase::GetModularIndex_Implementation() { return mModularIndex; }

void AKPCLModularBuildingBase::ApplyModularIndex_Implementation(int32 Index) { mModularIndex = Index; }

void AKPCLModularBuildingBase::SetAttachedActor_Implementation(AFGBuildable* Actor)
{
	if (Actor)
	{
		mMasterBuilding = Actor;
		OnMasterBuildingReceived(Actor);
	}
}

void AKPCLModularBuildingBase::RemoveAttachedActor_Implementation(AFGBuildable* Actor)
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

bool AKPCLModularBuildingBase::AttachedActor_Implementation(AFGBuildable* Actor,
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

void AKPCLModularBuildingBase::OnModulesUpdated_Implementation()
{
	OnMeshUpdate();
	Event_OnMeshUpdate();
}

void AKPCLModularBuildingBase::ReadyForVisuelUpdate()
{
	Super::ReadyForVisuelUpdate();
	OnMeshUpdate();
	Event_OnMeshUpdate();
}

void AKPCLModularBuildingBase::GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const
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

void AKPCLModularBuildingBase::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		HandlePowerInit();
	}

	if (IsValid(GetHandler()))
	{
		GetHandler()->OnHandlerTriggerUpdate.AddUniqueDynamic(this, &AKPCLModularBuildingBase::OnMeshUpdate);
		GetHandler()->SetIsReplicated(true);
		OnMeshUpdate();
	}
}

void AKPCLModularBuildingBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

void AKPCLModularBuildingBase::OnModulesWasUpdated_Implementation() {}

void AKPCLModularBuildingBase::OnRep_MasterBuilding()
{
	if (mMasterBuilding.IsValid())
	{
		OnMasterBuildingReceived(mMasterBuilding.Get());
	}
}

void AKPCLModularBuildingBase::OnRep_ModularIndex() { ReadyForVisuelUpdate(); }

void AKPCLModularBuildingBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLModularBuildingBase, mModularHandler);
	DOREPLIFETIME(AKPCLModularBuildingBase, mMasterBuilding);
	DOREPLIFETIME(AKPCLModularBuildingBase, mModularIndex);
}

void AKPCLModularBuildingBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	mModularHandler = FindComponentByClass<UKPCLModularBuildingHandlerBase>();
}
