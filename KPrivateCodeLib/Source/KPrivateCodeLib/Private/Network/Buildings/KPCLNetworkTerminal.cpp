// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Network/Buildings/KPCLNetworkTerminal.h"

#include "Description/KPCLNetworkDrive.h"
#include "Network/Buildings/KPCLNetworkCore.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

AKPCLNetworkTerminal::AKPCLNetworkTerminal() { PrimaryActorTick.bCanEverTick = false; }

void AKPCLNetworkTerminal::UI_ApplyRelevantItems_Implementation(TArray<TSubclassOf<UFGItemDescriptor>>& OutSlots)
{
	Super::UI_ApplyRelevantItems_Implementation(OutSlots);

	UKBFLAssetDataSubsystem* AssetSubsystem = UKBFLAssetDataSubsystem::Get(this);
	if (IsValid(AssetSubsystem))
	{
		TArray<TSubclassOf<UFGItemDescriptor>> Items;
		AssetSubsystem->GetItemsOfChilds({UKPCLNetworkDrive::StaticClass()}, Items, true);
		for (TSubclassOf<UFGItemDescriptor> Item : Items)
		{
			if (IsValid(Item))
			{
				OutSlots.AddUnique(Item);
			}
		}
	}
}

bool AKPCLNetworkTerminal::CanUseFactoryClipboard_Implementation() { return false; }

bool AKPCLNetworkTerminal::IsUseable_Implementation() const { return true; }

void AKPCLNetworkTerminal::OnUse_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state)
{
	if (IsProducing())
	{
		AKPCLNetworkCore* Core = Execute_GetCore(this);
		if (IsValid(Core))
		{
			Execute_OnUse(Core, byCharacter, state);
		}
	}
}

FText AKPCLNetworkTerminal::GetLookAtDecription_Implementation(AFGCharacterPlayer* byCharacter,
															   const FUseState& state) const
{
	if (IsProducing())
	{
		return Super::GetLookAtDecription_Implementation(byCharacter, state);
	}
	return mNoCoreText;
}

FString AKPCLNetworkTerminal::GetNetworkId() const
{
	if (!HasCableBoost())
	{
		return FString();
	}
	return Super::GetNetworkId();
}
