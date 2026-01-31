// Fill out your copyright notice in the Description page of Project Settings.
#include "Subsystem/KPCLUnlockSubsystem.h"

#include "BFL/KBFL_Util.h"
#include "Components/KPCLNetworkPlayerComponent.h"
#include "FGSchematicManager.h"
#include "KPrivateCodeLibModule.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

class AFGSchematicManager;
DECLARE_LOG_CATEGORY_EXTERN(KPCLUnlockSubsystemLog, Log, All)

DEFINE_LOG_CATEGORY(KPCLUnlockSubsystemLog)

AKPCLUnlockSubsystem::AKPCLUnlockSubsystem()
{
	mShouldSave = true;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer_Replicate;
}

AKPCLUnlockSubsystem* AKPCLUnlockSubsystem::Get(UObject* worldContext)
{
	return UKBFL_Util::GetSubsystem<AKPCLUnlockSubsystem>(worldContext);
}

void AKPCLUnlockSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, mPlayerStates);
}

void AKPCLUnlockSubsystem::BeginPlay()
{
	Super::BeginPlay();

	UKBFLAssetDataSubsystem* Subsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

	if (HasAuthority())
	{
		AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld());
		if (IsValid(SchematicManager))
		{
			SchematicManager->PurchasedSchematicDelegate.AddUniqueDynamic(this,
																		  &AKPCLUnlockSubsystem::OnSchematicUnlocked);
		}
	}
}

void AKPCLUnlockSubsystem::OnSchematicUnlocked(TSubclassOf<UFGSchematic> UnlockedSchematic) {}

void AKPCLUnlockSubsystem::Init()
{
	Super::Init();

	SetActorTickInterval(1 / 15);
}

void AKPCLUnlockSubsystem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (AFGPlayerState* Player : mPlayerStates)
	{
		if (IsValid(Player))
		{
			UKPCLNetworkPlayerComponent* Comp = UKPCLNetworkPlayerComponent::GetOrCreateNetworkComponentToPlayerState(
				GetWorld(), Player, mStateComponentClass);
			if (IsValid(Comp))
			{
				Comp->CustomTick(DeltaSeconds);
			}
		}
	}
}

void AKPCLUnlockSubsystem::RegisterPlayerState(AFGPlayerState* State)
{
	if (IsValid(State))
	{
		mPlayerStates.AddUnique(State);
		UE_LOG(LogKPCL, Warning, TEXT("Register PlayerState by BeginPlay! %s"), *State->GetName())
		UKPCLNetworkPlayerComponent::GetOrCreateNetworkComponentToPlayerState(GetWorld(), State, mStateComponentClass);
	}
}

void AKPCLUnlockSubsystem::RegisterLootChest(AKPCLLootChest* Chest) { mRegisteredLootChests.Add(Chest); }

void AKPCLUnlockSubsystem::UnregisterLootChest(AKPCLLootChest* Chest)
{
	if (mRegisteredLootChests.Contains(Chest))
	{
		mRegisteredLootChests.Remove(Chest);
	}
}

const TArray<AKPCLLootChest*>& AKPCLUnlockSubsystem::GetRegisteredLootChests() const { return mRegisteredLootChests; }
