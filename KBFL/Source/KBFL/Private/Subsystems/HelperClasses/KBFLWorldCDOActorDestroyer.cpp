#include "Subsystems/HelperClasses/KBFLWorldCDOActorDestroyer.h"

#include "Engine/World.h"
#include "FGDismantleInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/StructuredLog.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogKBFLActorDestroyer, VeryVerbose, All);

void UKBFLWorldCDOActorDestroyer::ApplyToActorsInWorld()
{
	if (!GetWorld())
	{
		UE_LOGFMT(LogKBFLActorDestroyer, Warning, "ApplyToActorsInWorld: World is null | Asset: {AssetPath}",
				  GetPathName());
		return;
	}

	UE_LOGFMT(LogKBFLActorDestroyer, Log,
			  "ApplyToActorsInWorld: Starting actor destruction - ClassCount: {Count} | Asset: {AssetPath}",
			  mActorClassesToDestroy.Num(), GetPathName());

	DestoryAll();
}
void UKBFLWorldCDOActorDestroyer::DestoryAll()
{
	int32 TotalDestroyed = 0;

	for (TSubclassOf<AActor> ActorClass : mActorClassesToDestroy)
	{
		if (!ActorClass)
		{
			UE_LOGFMT(LogKBFLActorDestroyer, Warning, "DestoryAll: Invalid actor class in list | Asset: {AssetPath}",
					  GetPathName());
			continue;
		}

		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ActorClass, FoundActors);

		UE_LOGFMT(LogKBFLActorDestroyer, Log,
				  "DestoryAll: Processing class {ClassName} - Found {Count} actors | Asset: {AssetPath}",
				  ActorClass->GetName(), FoundActors.Num(), GetPathName());

		int32 DestroyedForClass = 0;
		for (AActor* Actor : FoundActors)
		{
			if (!IsValid(Actor))
			{
				continue;
			}

			HandleDestroyActor(Actor);
			DestroyedForClass++;
		}

		if (DestroyedForClass > 0)
		{
			UE_LOGFMT(LogKBFLActorDestroyer, Log,
					  "DestoryAll: Destroyed {Count} actors of class {ClassName} | Asset: {AssetPath}",
					  DestroyedForClass, ActorClass->GetName(), GetPathName());
		}

		TotalDestroyed += DestroyedForClass;
	}

	if (TotalDestroyed > 0)
	{
		UE_LOGFMT(LogKBFLActorDestroyer, Log, "DestoryAll: Total actors destroyed: {Count} | Asset: {AssetPath}",
				  TotalDestroyed, GetPathName());
	}
	else
	{
		UE_LOGFMT(LogKBFLActorDestroyer, Verbose, "DestoryAll: No actors destroyed | Asset: {AssetPath}",
				  GetPathName());
	}
}
void UKBFLWorldCDOActorDestroyer::Start()
{
	Super::Start();

	if (!bShouldDestroySpawnedActors)
	{
		UE_LOGFMT(LogKBFLActorDestroyer, Verbose,
				  "Start: Spawn listener disabled (bShouldDestroySpawnedActors=false) | Asset: {AssetPath}",
				  GetPathName());
		return;
	}

	TWeakObjectPtr<UKBFLWorldCDOActorDestroyer> WeakThis(this);
	mActorHandle = GetWorld()->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda(
		[WeakThis](AActor* Actor)
		{
			if (!IsValid(Actor))
			{
				return;
			}
			Actor->GetWorldTimerManager().SetTimerForNextTick(
				[WeakThis, Actor]()
				{
					UKBFLWorldCDOActorDestroyer* StrongThis = WeakThis.Get();
					if (!IsValid(StrongThis) || !IsValid(Actor))
					{
						return;
					}
					StrongThis->OnActorEvent(Actor);
				});
		}));

	UE_LOGFMT(LogKBFLActorDestroyer, Log,
			  "Start: Registered spawn listener for {Count} actor classes | Asset: {AssetPath}",
			  mActorClassesToDestroy.Num(), GetPathName());
}
void UKBFLWorldCDOActorDestroyer::Clear()
{
	if (GetWorld() && mActorHandle.IsValid())
	{
		GetWorld()->RemoveOnActorSpawnedHandler(mActorHandle);
		UE_LOGFMT(LogKBFLActorDestroyer, Verbose, "Clear: Unregistered spawn listener | Asset: {AssetPath}",
				  GetPathName());
	}

	Super::Clear();
}

void UKBFLWorldCDOActorDestroyer::OnActorEvent(AActor* Actor)
{
	if (!bShouldDestroySpawnedActors || !IsValid(Actor))
	{
		return;
	}

	HandleDestroyActor(Actor);
}

void UKBFLWorldCDOActorDestroyer::HandleDestroyActor(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	// Check if the spawned actor is of a relevant class
	for (TSubclassOf<AActor> ActorClass : mActorClassesToDestroy)
	{
		bool bMatchSubclass = ActorClass && Actor->IsA(ActorClass);
		bool bMatchExactClass = ActorClass && Actor->GetClass() == ActorClass;

		if ((bMatchSubclass && bUseSubclassCheck) || bMatchExactClass)
		{
			if (Requirements_IsMet(Actor))
			{
				UE_LOGFMT(LogKBFLActorDestroyer, Verbose,
						  "HandleDestroyActor: Destroying actor {ActorName} of class {ClassName} | Asset: {AssetPath}",
						  Actor->GetName(), Actor->GetClass()->GetName(), GetPathName());

				UWorld* World = Actor->GetWorld();
				if (!IsValid(World))
				{
					break;
				}

				TWeakObjectPtr<UKBFLWorldCDOActorDestroyer> WeakThis(this);
				TWeakObjectPtr<AActor> WeakActor(Actor);
				World->GetTimerManager().SetTimerForNextTick(
					[WeakThis, WeakActor]()
					{
						UKBFLWorldCDOActorDestroyer* StrongThis = WeakThis.Get();
						AActor* StrongActor = WeakActor.Get();
						if (!IsValid(StrongThis) || !IsValid(StrongActor))
						{
							return;
						}

						StrongThis->Requirements_NotifyOnModify(StrongActor);

						if (StrongActor->Implements<UFGDismantleInterface>())
						{
							TArray<AActor*> ActorsToDestroy;
							IFGDismantleInterface::Execute_GetChildDismantleActors(StrongActor, ActorsToDestroy);
							for (AActor* ToDestroy : ActorsToDestroy)
							{
								if (!IsValid(ToDestroy))
								{
									continue;
								}
								if (ToDestroy->Implements<UFGDismantleInterface>())
								{
									IFGDismantleInterface::Execute_Dismantle(ToDestroy);
								}
								else
								{
									ToDestroy->Destroy();
								}
							}
							IFGDismantleInterface::Execute_Dismantle(StrongActor);
						}
						else
						{
							StrongActor->Destroy();
						}
					});
			}
			else
			{
				UE_LOGFMT(LogKBFLActorDestroyer, Verbose,
						  "HandleDestroyActor: Requirements not met for actor {ActorName} | Asset: {AssetPath}",
						  Actor->GetName(), GetPathName());
			}
			break;
		}
	}
}
