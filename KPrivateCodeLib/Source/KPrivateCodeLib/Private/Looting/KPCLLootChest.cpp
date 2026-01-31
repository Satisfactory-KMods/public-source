// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Looting/KPCLLootChest.h"

#include "EngineUtils.h"
#include "FGBlueprintFunctionLibrary.h"
#include "FGCharacterPlayer.h"
#include "Internationalization/StringTableRegistry.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Replication/KPCLDefaultRCO.h"
#include "Resources/FGNoneDescriptor.h"
#include "Subsystem/KPCLUnlockSubsystem.h"


// Sets default values
AKPCLLootChest::AKPCLLootChest()
	: Super()
{
	PrimaryActorTick.bCanEverTick = 1;
	PrimaryActorTick.bStartWithTickEnabled = 1;

	mInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::InputName);
}

FScannableActorDetails UKPCLScannableDetailsLootChest::FindClosestRelevantActor(class UWorld* world,
	const FVector& scanLocation, const float maxRangeSquare, TSubclassOf<AActor> actorClassToScanFor) const
{
	if (!IsValid(world))
	{
		return FScannableActorDetails();
	}

	AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(world);
	if (IsValid(UnlockSubsystem))
	{
		AActor* closestActor = nullptr;
		float closestDistanceSquare = maxRangeSquare > 0.f ? maxRangeSquare : FLT_MAX;

		for (AKPCLLootChest* lootChest : UnlockSubsystem->GetRegisteredLootChests())
		{
			if (!IsValid(lootChest) || lootChest->WasLooted())
			{
				continue;
			}

			const float distanceSquare = FVector::DistSquared(scanLocation, lootChest->GetActorLocation());
			if (distanceSquare < closestDistanceSquare)
			{
				closestDistanceSquare = distanceSquare;
				closestActor = lootChest;
			}
		}

		if (IsValid(closestActor))
		{
			return FScannableActorDetails(closestActor);
		}
	}

	return FScannableActorDetails();
}

void AKPCLLootChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLLootChest, mInventory);
}

bool AKPCLLootChest::ShouldSave_Implementation() const
{
	return true;
}

float AKPCLLootChest::TakeDamage(float DamageAmount, const struct FDamageEvent& DamageEvent,
                                 class AController* EventInstigator, AActor* DamageCauser)
{
	return 0.f;
}

void AKPCLLootChest::UpdateUseState_Implementation(class AFGCharacterPlayer* byCharacter, const FVector& atLocation,
                                                   class UPrimitiveComponent* componentHit, FUseState& out_useState)
{
	out_useState.SetUseState(UFGUseState_Valid::StaticClass());
}

void AKPCLLootChest::OnUse_Implementation(class AFGCharacterPlayer* byCharacter, const FUseState& state)
{
	fgcheck(byCharacter);

	// Only local player controllers can create widgets.
	APlayerController* controller = Cast<APlayerController>(byCharacter->GetController());
	if (controller && controller->IsLocalPlayerController())
	{
		if (IsValid(mInteractWidgetSoftClass.LoadSynchronous()))
		{
			if (auto hud = Cast<AFGHUD>(controller->GetHUD()))
			{
				hud->OpenInteractUI(mInteractWidgetSoftClass.Get(), this);
			}
		}
	}
}

void AKPCLLootChest::OnUseStop_Implementation(class AFGCharacterPlayer* byCharacter, const FUseState& state)
{
	fgcheck(byCharacter);
}

bool AKPCLLootChest::IsUseable_Implementation() const
{
	return GetDestructibleActorState() != EDestructibleActorState::DSS_Destroyed;
}

void AKPCLLootChest::StartIsLookedAt_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state)
{
	if (UFGOutlineComponent* outline = byCharacter->GetOutline())
	{
		outline->ShowOutline(this, EOutlineColor::OC_USABLE);
	}
}

void AKPCLLootChest::StopIsLookedAt_Implementation(AFGCharacterPlayer* byCharacter, const FUseState& state)
{
	if (UFGOutlineComponent* outline = byCharacter->GetOutline())
	{
		outline->HideOutline(this);
	}
}

FText AKPCLLootChest::GetLookAtDecription_Implementation(class AFGCharacterPlayer* byCharacter,
                                                         const FUseState& state) const
{
	AFGPlayerController* PC = Cast<AFGPlayerController>(byCharacter ? byCharacter->GetController() : nullptr);

	FText keyText = PC ? PC->GetKeyNameForUseAction() : FText::FromString(TEXT("N/A"));
	return FText::Format(
		LOCTABLE("Buildables_UI", "Buildables/LookAtDescription/DefaultConfigure/Pattern"), keyText, mDisplayName);
}

void AKPCLLootChest::RegisterInteractingPlayer_Implementation(class AFGCharacterPlayer* player)
{
	fgcheck(player);
	fgcheck(HasAuthority());
	ForceNetUpdate();
}

void AKPCLLootChest::UnregisterInteractingPlayer_Implementation(class AFGCharacterPlayer* player)
{
	fgcheck(player);
	fgcheck(HasAuthority());

	SetNetDormancy(DORM_Awake);
	ForceNetUpdate();
}

void AKPCLLootChest::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GenerateLoot();
	}

	GetInventory()->mItemFilter.BindUObject(this, &AKPCLLootChest::FilterItemClasses);
	GetInventory()->OnItemRemovedDelegate_Native.BindUObject(this, &AKPCLLootChest::OnInputItemRemoved);

	if (GetDestructibleActorState() == EDestructibleActorState::DSS_Destroyed)
	{
		mStaticMeshProxy->SetHiddenInGame(true);
	}

	AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld());
	if (IsValid(UnlockSubsystem) && !WasLooted())
	{
		UnlockSubsystem->RegisterLootChest(this);
	}
}

void AKPCLLootChest::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld());
	if (IsValid(UnlockSubsystem))
	{
		UnlockSubsystem->UnregisterLootChest(this);
	}
}

void AKPCLLootChest::GenerateLoot()
{
	if (!HasAuthority() || GetDestructibleActorState() == EDestructibleActorState::DSS_Destroyed)
	{
		return;
	}

	if (mInventory->IsEmpty() && !mRandomData.IsEmpty())
	{
		int32 Trys = mRandomTrys.GetRandom();

		mInventory->Resize(Trys);
		for (int32 Idx = 0; Idx < Trys; ++Idx)
		{
			FKPCLLootChestRandomData Data = mRandomData[UKismetMathLibrary::RandomIntegerInRange(
				0, mRandomData.Num() - 1)];
			if (Data.mItemClass)
			{
				int32 Amount = Data.mAmountRange.GetRandom();
				if (Amount > 0)
				{
					mInventory->AddStackToIndex(Idx, FInventoryStack(Amount, Data.mItemClass), false);
				}
			}
		}
	}

	ForceNetUpdate();
}

void AKPCLLootChest::UpdateActorState()
{
	bool IsEmpty = mInventory->IsEmpty();
	mStaticMeshProxy->SetHiddenInGame(IsEmpty);
	if (IsEmpty)
	{
		SetDestructibleActorState(EDestructibleActorState::DSS_Destroyed);
	}
	else
	{
		SetDestructibleActorState(EDestructibleActorState::DSS_Intact);
	}
	ApplyDestructibleActorState();
	OnLootedUpdated(IsEmpty);

	// If looted, unregister from the unlock subsystem so the object scanner no longer finds it
	if (WasLooted())
	{
		AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld());
		if (IsValid(UnlockSubsystem))
		{
			UnlockSubsystem->UnregisterLootChest(this);
		}
	}
}

bool AKPCLLootChest::WasLooted() const
{
	return mInventory->IsEmpty() || GetDestructibleActorState() == EDestructibleActorState::DSS_Destroyed;
}

UFGInventoryComponent* AKPCLLootChest::GetInventory() const
{
	return mInventory;
}

void AKPCLLootChest::Loot(AFGCharacterPlayer* Player)
{
	if (!IsValid(Player))
	{
		return;
	}

	if (HasAuthority())
	{
		UFGInventoryLibrary::GrabAllItemsFromInventory(GetInventory(), Player->GetInventory());
		ForceNetUpdate();
	}
	else if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld()))
	{
		RCO->Server_LootChest(this, Player);
	}
}

bool AKPCLLootChest::FilterItemClasses(TSubclassOf<UObject> object, int32 idx) const
{
	return false;
}

void AKPCLLootChest::OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
                                        UFGInventoryComponent* sourceInventory)
{
	UpdateActorState();
	ForceNetUpdate();
}
