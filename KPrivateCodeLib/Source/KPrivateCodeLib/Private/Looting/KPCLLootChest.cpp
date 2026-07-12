// Copyright Coffee Stain Studios. All Rights Reserved.

#include "Looting/KPCLLootChest.h"

#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "EngineUtils.h"
#include "FGBlueprintFunctionLibrary.h"
#include "FGCharacterPlayer.h"
#include "Internationalization/StringTableRegistry.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Replication/KPCLDefaultRCO.h"
#include "Resources/FGNoneDescriptor.h"
#include "Subsystem/KPCLUnlockSubsystem.h"

// Sets default values
AKPCLLootChest::AKPCLLootChest() : Super()
{
	PrimaryActorTick.bCanEverTick = 1;
	PrimaryActorTick.bStartWithTickEnabled = 1;

	mInventory = CreateDefaultSubobject<UFGInventoryComponent>(FKPCLInventoryStructure::InputName);
}

FScannableActorDetails
UKPCLScannableDetailsLootChest::FindClosestRelevantActor(class UWorld* world, const FVector& scanLocation,
														 const float maxRangeSquare,
														 TSubclassOf<AActor> actorClassToScanFor) const
{
	if (!IsValid(world))
	{
		return FScannableActorDetails();
	}

	AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(world);
	if (IsValid(UnlockSubsystem))
	{
		TArray<AKPCLLootChest*> Chests = UnlockSubsystem->GetRegisteredLootChests();
		AActor* ClosestActor = UKPCLBlueprintFunctionLib::GetClostestActor<AKPCLLootChest>(
			Chests, scanLocation, maxRangeSquare,
			[](const AKPCLLootChest* LootChest) { return !LootChest->WasLooted(); });

		if (IsValid(ClosestActor))
		{
			return FScannableActorDetails(ClosestActor);
		}
	}

	return FScannableActorDetails();
}

void AKPCLLootChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bIsEmpty);
	DOREPLIFETIME(ThisClass, mInventory);
}

bool AKPCLLootChest::ShouldSave_Implementation() const { return true; }

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

bool AKPCLLootChest::CanDismantle_Implementation() const { return !bIsDismantled; }

void AKPCLLootChest::GetDismantleRefund_Implementation(TArray<FInventoryStack>& out_refund,
													   bool noBuildCostEnabled) const
{
	if (!noBuildCostEnabled) // Only add these additional refunds if NoBuildCost is disabled
	{
		// Loop though all the inventories and get their items, merge them if possible.
		TInlineComponentArray<UFGInventoryComponent*> inventories;
		GetComponents(inventories);
		for (const auto inventory : inventories)
		{
			if (inventory->GetSizeLinear() > 0)
			{
				FInventoryStack stack;

				for (int32 i = 0; inventory->GetStackFromIndex(i, stack); ++i)
				{
					if (stack.HasItems())
					{
						const EResourceForm form = UFGItemDescriptor::GetForm(stack.Item.GetItemClass());

						// Only add items that are of form SOLID. We don't want to return liquids to the player since
						// they can't hold them
						if (form == EResourceForm::RF_SOLID)
						{
							UFGInventoryLibrary::MergeInventoryItem(out_refund, stack);
						}
					}
				}
			}
		}
	}

	UFGInventoryLibrary::ConsolidateInventoryItems(out_refund);
}

FVector AKPCLLootChest::GetRefundSpawnLocationAndArea_Implementation(const FVector& aimHitLocation,
																	 float& out_radius) const
{
	FVector spawnLocation = GetActorLocation();

	FVector origin, extent;
	GetActorBounds(true, origin, extent);
	out_radius = FMath::Min(extent.X, extent.Y);

	return spawnLocation;
}

void AKPCLLootChest::PreUpgrade_Implementation() {}

void AKPCLLootChest::Upgrade_Implementation(AActor* newActor) {}

void AKPCLLootChest::Dismantle_Implementation()
{
	// Fixes duplication bug when dismantling
	if (bIsDismantled)
	{
		return;
	}

	bIsDismantled = true;

	if (APlayerController* pc = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (AFGCharacterPlayer* byCharacter = Cast<AFGCharacterPlayer>(pc->GetPawn()))
		{
			Execute_StopIsLookedAtForDismantle(this, byCharacter);
		}
	}

	// Empty all inventory components to prevent Race Condition Duplicate item exploits
	// NOTE: Prior to this call All Dismantled objects should have return their inventories to the dismantling player or
	// to crates
	TInlineComponentArray<UFGInventoryComponent*> inventoryComponents(this);
	for (UFGInventoryComponent* component : inventoryComponents)
	{
		// Mark inventory that its buildable is dismantling to prevent players adding items into the inventory
		// (effectively destroying the item)
		component->SetLocked(true);
		component->Empty();
	}

	ForceNetUpdate();
}

void AKPCLLootChest::StartIsLookedAtForDismantle_Implementation(class AFGCharacterPlayer* byCharacter)
{
	if (bIsDismantled)
	{
		return;
	}
	if (UFGOutlineComponent* outline = byCharacter->GetOutline())
	{
		if (bIsDismantled)
		{
			outline->HideOutline(this);
		}
		else
		{
			outline->ShowOutline(this, EOutlineColor::OC_DISMANTLE);
		}
	}

	MarkComponentsRenderStateDirty();
}

void AKPCLLootChest::StopIsLookedAtForDismantle_Implementation(class AFGCharacterPlayer* byCharacter)
{
	if (UFGOutlineComponent* outline = byCharacter->GetOutline())
	{
		outline->HideOutline(this);
	}
}

void AKPCLLootChest::GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const {}

FText AKPCLLootChest::GetDismantleDisplayName_Implementation(AFGCharacterPlayer* byCharacter) const
{
	return mDisplayName;
}

void AKPCLLootChest::GetDismantleDependencies_Implementation(TArray<AActor*>& out_dismantleDependencies) const {}
void AKPCLLootChest::SetEmpty(bool NewIsEmpty)
{
	if (!HasAuthority() || bIsEmpty == NewIsEmpty)
	{
		return;
	}

	bIsEmpty = NewIsEmpty;
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLLootChest, bIsEmpty, this);
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
	return FText::Format(LOCTABLE("Buildables_UI", "Buildables/LookAtDescription/DefaultConfigure/Pattern"), keyText,
						 mDisplayName);
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

	GetInventory()->SetIsReplicated(true);

	GenerateLoot();
	SetEmpty(GetInventory()->IsEmpty());

	GetInventory()->mItemFilter.BindUObject(this, &AKPCLLootChest::FilterItemClasses);

	{
		bool bEmptyCapture = bIsEmpty;
		TWeakObjectPtr<AKPCLLootChest> WeakThis(this);
		GetWorldTimerManager().SetTimerForNextTick(
			[WeakThis, bEmptyCapture]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->UpdateActorState(bEmptyCapture);
				}
			});
	}

	if (HasAuthority())
	{
		GetInventory()->OnItemRemovedDelegate_Native.BindUObject(this, &AKPCLLootChest::OnInputItemRemoved);
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
	if (!HasAuthority() || GetDestructibleActorState() == EDestructibleActorState::DSS_Destroyed || bIsEmpty)
	{
		return;
	}

	if (mInventory->IsEmpty() && !mRandomData.IsEmpty())
	{
		int32 Trys = mRandomTrys.GetRandom();

		mInventory->Resize(Trys);
		for (int32 Idx = 0; Idx < Trys; ++Idx)
		{
			FKPCLLootChestRandomData Data =
				mRandomData[UKismetMathLibrary::RandomIntegerInRange(0, mRandomData.Num() - 1)];
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

	GetInventory()->MarkInventoryContentsDirty();
	ForceNetUpdate();
}

void AKPCLLootChest::UpdateActorState(bool IsEmpty)
{
	if (IsValid(mStaticMeshProxy))
	{
		mStaticMeshProxy->SetHiddenInGame(IsEmpty);
	}
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
	SetActorTickEnabled(!IsEmpty);

	if (WasLooted())
	{
		AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld());
		if (IsValid(UnlockSubsystem))
		{
			UnlockSubsystem->UnregisterLootChest(this);
		}
	}

	MarkComponentsRenderStateDirty();
}

bool AKPCLLootChest::WasLooted() const
{
	return mInventory->IsEmpty() || GetDestructibleActorState() == EDestructibleActorState::DSS_Destroyed;
}

UFGInventoryComponent* AKPCLLootChest::GetInventory() const { return mInventory; }

void AKPCLLootChest::AnnounceActorStateUpdate_Implementation(bool IsEmpty)
{
	bIsEmpty = IsEmpty;
	if (HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLLootChest, bIsEmpty, this);
	}
	UpdateActorState(IsEmpty);
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
		const bool bNowEmpty = GetInventory()->IsEmpty();
		UpdateActorState(bNowEmpty);
		AnnounceActorStateUpdate(bNowEmpty);
		ForceNetUpdate();
	}
	else if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(GetWorld()))
	{
		RCO->Server_LootChest(this, Player);
	}
}

void AKPCLLootChest::OnRep_IsEmpty() { UpdateActorState(bIsEmpty); }

bool AKPCLLootChest::FilterItemClasses(TSubclassOf<UObject> object, int32 idx) const { return false; }

void AKPCLLootChest::OnInputItemRemoved(TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved,
										UFGInventoryComponent* sourceInventory)
{
	const bool bNowEmpty = GetInventory()->IsEmpty();

	SetEmpty(bNowEmpty);
	UpdateActorState(bIsEmpty);
	ForceNetUpdate();
	AnnounceActorStateUpdate(bNowEmpty);
}
