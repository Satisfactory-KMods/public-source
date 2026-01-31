// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Subsystem/KPCLSwatchSystem.h"

#include "BFL/KBFL_Player.h"
#include "BFL/KBFL_Util.h"
#include "Net/UnrealNetwork.h"
#include "Replication/KPCLDefaultRCO.h"


void AKPCLSwatchSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLSwatchSystem, mCustomSwatchData);
}

// Sets default values
AKPCLSwatchSystem::AKPCLSwatchSystem()
{
	mShouldSave = true;
	PrimaryActorTick.bCanEverTick = true;
}

AKPCLSwatchSystem* AKPCLSwatchSystem::Get(UObject* worldContext)
{
	return UKBFL_Util::GetSubsystem<AKPCLSwatchSystem>(worldContext);
}

void AKPCLSwatchSystem::AddCustomSwatchData(FCustomSwatchData Data)
{
	if (HasAuthority())
	{
		mCustomSwatchData.Add(Data);
		OnRep_CustomSwatchData();
	}
	else if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld()))
	{
		RCO->Server_AddCustomSwatchData(this, Data);
	}
}

void AKPCLSwatchSystem::RemoveCustomSwatchData(int32 Idx)
{
	if (HasAuthority())
	{
		if (mCustomSwatchData.IsValidIndex(Idx))
		{
			const FCustomSwatchData OldCustomSwatchData = mCustomSwatchData[Idx];
			TArray<FCustomSwatchData> NewArray;

			bool WasRemove = false;
			for (FCustomSwatchData CustomSwatchData : mCustomSwatchData)
			{
				if (CustomSwatchData != OldCustomSwatchData || WasRemove)
				{
					NewArray.Add(CustomSwatchData);
				}
				else
				{
					WasRemove = true;
				}
			}

			mCustomSwatchData = NewArray;
			OnRep_CustomSwatchData();
		}
	}
	else if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld()))
	{
		RCO->Server_RemoveCustomSwatchData(this, Idx);
	}
}

void AKPCLSwatchSystem::UseCustomSwatchData(int32 Idx)
{
	AFGPlayerState* State = UKBFL_Player::GetFgPlayerState(GetWorld());
	if (State && mCustomSwatchData.IsValidIndex(Idx))
	{
		FFactoryCustomizationColorSlot Slot = State->GetCustomColorData();
		Slot.PrimaryColor = mCustomSwatchData[Idx].mPrimaryColor;
		Slot.SecondaryColor = mCustomSwatchData[Idx].mSecondaryColor;
		State->SetPlayerCustomizationSlotData(Slot);
	}
}

void AKPCLSwatchSystem::UpdateCustomSwatchData(FCustomSwatchData Data, int32 Idx)
{
	if (HasAuthority())
	{
		if (mCustomSwatchData.IsValidIndex(Idx))
		{
			mCustomSwatchData[Idx] = Data;
			OnRep_CustomSwatchData();
		}
	}
	else if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld()))
	{
		RCO->Server_UpdateCustomSwatchData(this, Data, Idx);
	}
}

TArray<FCustomSwatchData> AKPCLSwatchSystem::GetCustomSwatchData() const { return mCustomSwatchData; }

void AKPCLSwatchSystem::OnRep_CustomSwatchData()
{
	if (mOnSwatchDataUpdated.IsBound())
	{
		mOnSwatchDataUpdated.Broadcast();
	}
}
