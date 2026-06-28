#include "BFL/KBFL_Util.h"

#include "BFL/KBFL_Player.h"
#include "Buildables/FGBuildableResourceExtractorBase.h"
#include "FGPlayerController.h"
#include "FGUnlockSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Unlocks/FGUnlockScannableResource.h"

#pragma warning(disable : 4702) // intentional early-return dead code

void UKBFL_Util::SortItemArray(TArray<TSubclassOf<UFGItemDescriptor>>& Out_Items,
							   const TArray<TSubclassOf<UFGItemDescriptor>>& In_Items,
							   const TArray<TSubclassOf<UFGItemDescriptor>>& ForceFirstItems, bool Reverse)
{
	Out_Items.Empty();
	Out_Items.Append(In_Items);
	if (Out_Items.Num() > 1)
	{
		Out_Items.Sort(
			[Reverse](const TSubclassOf<UFGItemDescriptor> A, const TSubclassOf<UFGItemDescriptor> B)
			{
				if (!Reverse)
				{
					return UFGItemDescriptor::GetItemName(A).ToString() < UFGItemDescriptor::GetItemName(B).ToString();
				}
				return UFGItemDescriptor::GetItemName(A).ToString() > UFGItemDescriptor::GetItemName(B).ToString();
			});

		if (ForceFirstItems.Num() > 0)
		{
			TArray<TSubclassOf<UFGItemDescriptor>> ForceReturn;
			for (TSubclassOf<UFGItemDescriptor> Out_Item : ForceFirstItems)
			{
				if (Out_Items.Contains(Out_Item))
				{
					Out_Items.Remove(Out_Item);
					ForceReturn.Add(Out_Item);
				}
			}
			ForceReturn.Append(Out_Items);
			Out_Items.Empty();
			Out_Items.Append(ForceReturn);
		}
	}
}

AModSubsystem* UKBFL_Util::GetSubsystem(UObject* WorldContext, TSubclassOf<AModSubsystem> Subsystem)
{
	return GetSubsystemFromChild(WorldContext, Subsystem);
}

bool UKBFL_Util::DoPlayerViewLineTrace(UObject* WorldContext, FHitResult& Hit, float Distance,
									   TArray<AActor*> ActorsToIgnore, ETraceTypeQuery TraceChannel, bool TraceComplex)
{
	if (AFGCharacterPlayer* Char = UKBFL_Player::GetFGCharacter(WorldContext))
	{
		FVector Start = Char->GetCameraComponentWorldLocation();
		FVector End = Start + Distance * Char->GetCameraComponentForwardVector();

		// Add Player self that he dont trace!
		ActorsToIgnore.AddUnique(Char);

		UKismetSystemLibrary::LineTraceSingle(WorldContext, Start, End, TraceChannel, TraceComplex, ActorsToIgnore,
											  EDrawDebugTrace::None, Hit, true);
		return Hit.IsValidBlockingHit();
	}
	return false;
}

bool UKBFL_Util::DoPlayerViewLineTraceSphere(UObject* WorldContext, TArray<AActor*>& OutActors, float Distance,
											 TArray<AActor*> ActorsToIgnore, ETraceTypeQuery TraceChannel,
											 TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes,
											 TSubclassOf<AActor> ActorClass, float SphereSize, bool TraceComplex)
{
	if (AFGCharacterPlayer* Char = UKBFL_Player::GetFGCharacter(WorldContext))
	{
		FHitResult LineHit;
		if (DoPlayerViewLineTrace(WorldContext, LineHit, Distance, ActorsToIgnore, TraceChannel, TraceComplex))
		{
			ActorsToIgnore.AddUnique(Char);
			return UKismetSystemLibrary::SphereOverlapActors(WorldContext, LineHit.ImpactPoint, SphereSize, ObjTypes,
															 ActorClass, ActorsToIgnore, OutActors);
		}
	}
	return false;
}

AModSubsystem* UKBFL_Util::GetSubsystemFromChild(UObject* WorldContext, TSubclassOf<AModSubsystem> SubsystemClass)
{
#if WITH_EDITOR
	return SubsystemClass.GetDefaultObject();
#endif

	if (WorldContext)
	{
		const UWorld* WorldObject = GEngine->GetWorldFromContextObjectChecked(WorldContext);
		USubsystemActorManager* SubsystemActorManager = WorldObject->GetSubsystem<USubsystemActorManager>();
		check(SubsystemActorManager);

		for (auto Subsystem : SubsystemActorManager->SubsystemActors)
		{
			if (Subsystem.Key->IsChildOf(SubsystemClass))
			{
				return Subsystem.Value;
			}
		}
	}
	return nullptr;
}

void UKBFL_Util::GetAllSubsystemsFromChild(UObject* WorldContext, TSubclassOf<AModSubsystem> SubsystemClass,
										   TArray<AModSubsystem*>& Subsystems)
{
#if WITH_EDITOR
	return;
#endif

	if (WorldContext)
	{
		const UWorld* WorldObject = GEngine->GetWorldFromContextObjectChecked(WorldContext);
		USubsystemActorManager* SubsystemActorManager = WorldObject->GetSubsystem<USubsystemActorManager>();
		check(SubsystemActorManager);

		for (auto Subsystem : SubsystemActorManager->SubsystemActors)
		{
			if (Subsystem.Key->IsChildOf(SubsystemClass))
			{
				Subsystems.AddUnique(Subsystem.Value);
			}
		}
	}
}
