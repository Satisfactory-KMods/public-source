// Copyright Kyri123 / KMods 2026. All Rights Reserved.


#include "Subsystem/KPCLUnlockSubsystem.h"

#include "BFL/KBFL_Util.h"
#include "Engine/Engine.h"
#include "FGSchematicManager.h"
#include "KPrivateCodeLibModule.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/KAPIDataAssetSubsystem.h"
#include "Unlocks/KPCLDeliveryTaskUnlock.h"
#include "Unlocks/KPCLRepeatPurchaseUnlock.h"

class AFGSchematicManager;
DECLARE_LOG_CATEGORY_EXTERN(KPCLUnlockSubsystemLog, Log, All)

DEFINE_LOG_CATEGORY(KPCLUnlockSubsystemLog)

namespace
{
	bool IsDeliveryTaskSystemAvailable(bool bIsUnlocked, bool bHasTaskAsset) { return bIsUnlocked && bHasTaskAsset; }
}

AKPCLUnlockSubsystem::AKPCLUnlockSubsystem()
{
	PrimaryActorTick.bCanEverTick = false;
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

	DOREPLIFETIME(ThisClass, mRepeatPurchaseStates);
	DOREPLIFETIME(ThisClass, bDeliveryTaskSystemUnlocked);
	DOREPLIFETIME(ThisClass, mRegisteredLootChests);
}

void AKPCLUnlockSubsystem::BeginPlay()
{
	Super::BeginPlay();

	if (UKAPIDataAssetSubsystem* DataSubsystem = UKAPIDataAssetSubsystem::Get(GetWorld()))
	{
		DataSubsystem->mOnDeliveryTasksChanged.AddUObject(this, &AKPCLUnlockSubsystem::HandleDeliveryTasksChanged);
	}

	AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld());
	if (HasAuthority() && IsValid(SchematicManager))
	{
		SchematicManager->PurchasedSchematicDelegate.AddUniqueDynamic(this, &AKPCLUnlockSubsystem::OnSchematicUnlocked);
	}

	RefreshDeliveryTaskSystemUnlock();
	RefreshRepeatPurchaseSchematics();
	BroadcastDeliveryTaskSystemAvailabilityIfChanged();
}

void AKPCLUnlockSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UKAPIDataAssetSubsystem* DataSubsystem = UKAPIDataAssetSubsystem::Get(GetWorld()))
	{
		DataSubsystem->mOnDeliveryTasksChanged.RemoveAll(this);
	}
	if (AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld()))
	{
		SchematicManager->PurchasedSchematicDelegate.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AKPCLUnlockSubsystem::OnSchematicUnlocked(TSubclassOf<UFGSchematic> UnlockedSchematic)
{
	if (!HasAuthority())
	{
		return;
	}
	RefreshDeliveryTaskSystemUnlock();

	IKPCLRepeatPurchaseUnlock* RepeatUnlock = IKPCLRepeatPurchaseUnlock::FindOnSchematic(UnlockedSchematic);
	if (RepeatUnlock == nullptr)
	{
		return;
	}

	FKPCLRepeatPurchaseState* State = FindRepeatPurchaseState(UnlockedSchematic);
	if (State == nullptr)
	{
		State = &mRepeatPurchaseStates.AddDefaulted_GetRef();
		State->mSchematicClass = UnlockedSchematic;
	}
	State->mPurchaseCount = FMath::Max(0, State->mPurchaseCount) + 1;
	RepeatUnlock->ApplyLevelState(UnlockedSchematic, State->mPurchaseCount);
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLUnlockSubsystem, mRepeatPurchaseStates, this);
	ForceNetUpdate();
	MulticastRepeatPurchaseState(UnlockedSchematic, State->mPurchaseCount);
}

void AKPCLUnlockSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	Super::PostLoadGame_Implementation(saveVersion, gameVersion);
	RefreshDeliveryTaskSystemUnlock();
	RefreshRepeatPurchaseSchematics();
}

AKPCLUnlockSubsystem* AKPCLUnlockSubsystem::GetFromCurrentWorld()
{
	if (IsValid(GWorld) && GWorld->IsGameWorld())
	{
		if (AKPCLUnlockSubsystem* Subsystem = Get(GWorld))
		{
			return Subsystem;
		}
	}

	if (GEngine == nullptr)
	{
		return nullptr;
	}
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		UWorld* World = WorldContext.World();
		if (IsValid(World) && World->IsGameWorld())
		{
			if (AKPCLUnlockSubsystem* Subsystem = Get(World))
			{
				return Subsystem;
			}
		}
	}
	return nullptr;
}

int32 AKPCLUnlockSubsystem::GetRepeatPurchaseCount(TSubclassOf<UFGSchematic> SchematicClass) const
{
	const FKPCLRepeatPurchaseState* State = FindRepeatPurchaseState(SchematicClass);
	return State != nullptr ? FMath::Max(0, State->mPurchaseCount) : 0;
}

int32 AKPCLUnlockSubsystem::GetRemainingRepeatPurchases(TSubclassOf<UFGSchematic> SchematicClass) const
{
	const IKPCLRepeatPurchaseUnlock* RepeatUnlock = IKPCLRepeatPurchaseUnlock::FindOnSchematic(SchematicClass);
	if (RepeatUnlock == nullptr || RepeatUnlock->GetMaxPurchaseCount() <= 0)
	{
		return -1;
	}
	return FMath::Max(0, RepeatUnlock->GetMaxPurchaseCount() - GetRepeatPurchaseCount(SchematicClass));
}

bool AKPCLUnlockSubsystem::CanPurchaseRepeatSchematic(TSubclassOf<UFGSchematic> SchematicClass) const
{
	const IKPCLRepeatPurchaseUnlock* RepeatUnlock = IKPCLRepeatPurchaseUnlock::FindOnSchematic(SchematicClass);
	return RepeatUnlock == nullptr || RepeatUnlock->GetMaxPurchaseCount() <= 0 ||
		GetRepeatPurchaseCount(SchematicClass) < RepeatUnlock->GetMaxPurchaseCount();
}

float AKPCLUnlockSubsystem::GetRepeatRewardMultiplier(TSubclassOf<UFGSchematic> SchematicClass) const
{
	const IKPCLRepeatPurchaseUnlock* RepeatUnlock = IKPCLRepeatPurchaseUnlock::FindOnSchematic(SchematicClass);
	return RepeatUnlock != nullptr ? RepeatUnlock->GetRewardMultiplier(GetRepeatPurchaseCount(SchematicClass)) : 1.0f;
}

bool AKPCLUnlockSubsystem::GetIsDeliveryTaskSystemUnlocked() const
{
	const UKAPIDataAssetSubsystem* DataSubsystem = UKAPIDataAssetSubsystem::Get(GetWorld());
	const bool bHasTaskAsset = IsValid(DataSubsystem) && DataSubsystem->Delivery_HasAny();
	return ::IsDeliveryTaskSystemAvailable(bDeliveryTaskSystemUnlocked, bHasTaskAsset);
}

void AKPCLUnlockSubsystem::UnlockDeliveryTaskSystem()
{
	if (!HasAuthority() || bDeliveryTaskSystemUnlocked)
	{
		return;
	}

	bDeliveryTaskSystemUnlocked = true;
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLUnlockSubsystem, bDeliveryTaskSystemUnlocked, this);
	OnRep_DeliveryTaskSystemUnlocked();
	ForceNetUpdate();
}

void AKPCLUnlockSubsystem::OnRep_DeliveryTaskSystemUnlocked() { BroadcastDeliveryTaskSystemAvailabilityIfChanged(); }

TArray<FItemAmount> AKPCLUnlockSubsystem::GetScaledSchematicCost(TSubclassOf<UFGSchematic> SchematicClass,
																 const TArray<FItemAmount>& VanillaCost)
{
	IKPCLRepeatPurchaseUnlock* RepeatUnlock = IKPCLRepeatPurchaseUnlock::FindOnSchematic(SchematicClass);
	if (RepeatUnlock == nullptr)
	{
		return VanillaCost;
	}

	TArray<FItemAmount> ScaledCost = VanillaCost;
	const float Multiplier = RepeatUnlock->GetCostMultiplier(GetRepeatPurchaseCount(SchematicClass));
	for (FItemAmount& Cost : ScaledCost)
	{
		const double ScaledAmount = static_cast<double>(Cost.Amount) * static_cast<double>(Multiplier);
		Cost.Amount = static_cast<int32>(FMath::Clamp<int64>(FMath::RoundToInt64(ScaledAmount), 0, MAX_int32));
	}
	return ScaledCost;
}

void AKPCLUnlockSubsystem::OnRep_RepeatPurchaseStates() { RefreshRepeatPurchaseSchematics(); }

void AKPCLUnlockSubsystem::RefreshDeliveryTaskSystemUnlock()
{
	if (!HasAuthority() || bDeliveryTaskSystemUnlocked)
	{
		return;
	}

	AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld());
	if (!IsValid(SchematicManager))
	{
		return;
	}

	TArray<TSubclassOf<UFGSchematic>> PurchasedSchematics;
	SchematicManager->GetAllPurchasedSchematics(PurchasedSchematics);
	for (TSubclassOf<UFGSchematic> SchematicClass : PurchasedSchematics)
	{
		if (!IsValid(SchematicClass))
		{
			continue;
		}

		for (UFGUnlock* Unlock : UFGSchematic::GetUnlocks(SchematicClass))
		{
			if (IsValid(Cast<UKPCLDeliveryTaskUnlock>(Unlock)))
			{
				UnlockDeliveryTaskSystem();
				return;
			}
		}
	}
}

void AKPCLUnlockSubsystem::HandleDeliveryTasksChanged() { BroadcastDeliveryTaskSystemAvailabilityIfChanged(); }

void AKPCLUnlockSubsystem::BroadcastDeliveryTaskSystemAvailabilityIfChanged()
{
	const bool bIsAvailable = GetIsDeliveryTaskSystemUnlocked();
	if (bHasCachedDeliveryTaskSystemAvailability && bCachedDeliveryTaskSystemAvailability == bIsAvailable)
	{
		return;
	}

	bHasCachedDeliveryTaskSystemAvailability = true;
	bCachedDeliveryTaskSystemAvailability = bIsAvailable;
	mOnDeliveryTaskSystemUnlockChanged.Broadcast(bIsAvailable);
}

void AKPCLUnlockSubsystem::MulticastRepeatPurchaseState_Implementation(TSubclassOf<UFGSchematic> SchematicClass,
																	   int32 PurchaseCount)
{
	if (HasAuthority())
	{
		return;
	}
	ApplyRepeatPurchaseState(SchematicClass, PurchaseCount);
}

void AKPCLUnlockSubsystem::ApplyRepeatPurchaseState(TSubclassOf<UFGSchematic> SchematicClass, int32 PurchaseCount)
{
	if (!SchematicClass || PurchaseCount < 0)
	{
		return;
	}

	FKPCLRepeatPurchaseState* State = FindRepeatPurchaseState(SchematicClass);
	if (State == nullptr)
	{
		State = &mRepeatPurchaseStates.AddDefaulted_GetRef();
		State->mSchematicClass = SchematicClass;
	}
	State->mPurchaseCount = PurchaseCount;

	if (IKPCLRepeatPurchaseUnlock* RepeatUnlock = IKPCLRepeatPurchaseUnlock::FindOnSchematic(SchematicClass))
	{
		RepeatUnlock->ApplyLevelState(SchematicClass, PurchaseCount);
	}
}

void AKPCLUnlockSubsystem::RefreshRepeatPurchaseSchematics()
{
	for (const FKPCLRepeatPurchaseState& State : mRepeatPurchaseStates)
	{
		if (IKPCLRepeatPurchaseUnlock* RepeatUnlock = IKPCLRepeatPurchaseUnlock::FindOnSchematic(State.mSchematicClass))
		{
			RepeatUnlock->ApplyLevelState(State.mSchematicClass, FMath::Max(0, State.mPurchaseCount));
		}
	}

	AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld());
	if (!IsValid(SchematicManager))
	{
		return;
	}

	TArray<TSubclassOf<UFGSchematic>> Schematics;
	SchematicManager->GetAllSchematics(Schematics);
	for (TSubclassOf<UFGSchematic> SchematicClass : Schematics)
	{
		if (IKPCLRepeatPurchaseUnlock* RepeatUnlock = IKPCLRepeatPurchaseUnlock::FindOnSchematic(SchematicClass))
		{
			RepeatUnlock->ApplyLevelState(SchematicClass, GetRepeatPurchaseCount(SchematicClass));
		}
	}
}

const FKPCLRepeatPurchaseState*
AKPCLUnlockSubsystem::FindRepeatPurchaseState(TSubclassOf<UFGSchematic> SchematicClass) const
{
	return mRepeatPurchaseStates.FindByPredicate([SchematicClass](const FKPCLRepeatPurchaseState& State)
												 { return State.mSchematicClass == SchematicClass; });
}

FKPCLRepeatPurchaseState* AKPCLUnlockSubsystem::FindRepeatPurchaseState(TSubclassOf<UFGSchematic> SchematicClass)
{
	return mRepeatPurchaseStates.FindByPredicate([SchematicClass](const FKPCLRepeatPurchaseState& State)
												 { return State.mSchematicClass == SchematicClass; });
}

void AKPCLUnlockSubsystem::RegisterLootChest(AKPCLLootChest* Chest) { mRegisteredLootChests.Add(Chest); }

void AKPCLUnlockSubsystem::UnregisterLootChest(AKPCLLootChest* Chest)
{
	if (!HasAuthority())
	{
		return;
	}

	if (mRegisteredLootChests.Contains(Chest))
	{
		mRegisteredLootChests.Remove(Chest);
	}
}

const TArray<AKPCLLootChest*>& AKPCLUnlockSubsystem::GetRegisteredLootChests() const { return mRegisteredLootChests; }
