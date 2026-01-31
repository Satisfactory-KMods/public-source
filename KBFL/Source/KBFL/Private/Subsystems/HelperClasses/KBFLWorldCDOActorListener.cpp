#include "Subsystems/HelperClasses/KBFLWorldCDOActorListener.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY_STATIC(LogKBFLActorListener, VeryVerbose, All);

void UKBFLWorldCDOActorListener::Start()
{
	Super::Start();

	if (!GetWorld())
	{
		UE_LOGFMT(LogKBFLActorListener, Warning, "Start: World is null | Asset: {AssetPath}", GetPathName());
		return;
	}

	// Log event type
	const FString EventTypeStr =
		mEventToListenFor == EKBFLWorldCDOActorListenerEvent::SpawnedActorEvent ? TEXT("Spawned") : TEXT("Destroyed");
	UE_LOGFMT(LogKBFLActorListener, Log,
			  "Start: Registering listener for {EventType} events - ClassCount: {Count} | Asset: {AssetPath}",
			  EventTypeStr, mActorClassesToListenFor.Num(), GetPathName());

	// Register for actor spawn events
	switch (mEventToListenFor)
	{
	case EKBFLWorldCDOActorListenerEvent::SpawnedActorEvent:
		mActorHandle = GetWorld()->AddOnActorSpawnedHandler(
			FOnActorSpawned::FDelegate::CreateUObject(this, &UKBFLWorldCDOActorListener::OnActorEvent));
		break;
	case EKBFLWorldCDOActorListenerEvent::DestroyedActorEvent:
		mActorHandle = GetWorld()->AddOnActorDestroyedHandler(
			FOnActorDestroyed::FDelegate::CreateUObject(this, &UKBFLWorldCDOActorListener::OnActorEvent));
		break;
	default:
		UE_LOGFMT(LogKBFLActorListener, Warning, "Start: Unknown event type | Asset: {AssetPath}", GetPathName());
		break;
	}
}

void UKBFLWorldCDOActorListener::ApplyToActorsInWorld()
{
	if (!GetWorld())
	{
		UE_LOGFMT(LogKBFLActorListener, Warning, "ApplyToActorsInWorld: World is null | Asset: {AssetPath}",
				  GetPathName());
		return;
	}

	UE_LOGFMT(LogKBFLActorListener, Log,
			  "ApplyToActorsInWorld: Processing existing actors for {Count} classes | Asset: {AssetPath}",
			  mActorClassesToListenFor.Num(), GetPathName());

	int32 TotalProcessed = 0;
	for (TSubclassOf<AActor> ActorClass : mActorClassesToListenFor)
	{
		if (!ActorClass)
		{
			continue;
		}

		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ActorClass, Actors);

		UE_LOGFMT(LogKBFLActorListener, Log,
				  "ApplyToActorsInWorld: Found {Count} actors of class {ClassName} | Asset: {AssetPath}", Actors.Num(),
				  ActorClass->GetName(), GetPathName());

		for (AActor* Actor : Actors)
		{
			OnActorEvent(Actor);
			TotalProcessed++;
		}
	}

	UE_LOGFMT(LogKBFLActorListener, Log,
			  "ApplyToActorsInWorld: Finished processing {Count} total actors | Asset: {AssetPath}", TotalProcessed,
			  GetPathName());
}

void UKBFLWorldCDOActorListener::Clear()
{
	if (GetWorld() && mActorHandle.IsValid())
	{
		const FString EventTypeStr = mEventToListenFor == EKBFLWorldCDOActorListenerEvent::SpawnedActorEvent
			? TEXT("Spawned")
			: TEXT("Destroyed");

		switch (mEventToListenFor)
		{
		case EKBFLWorldCDOActorListenerEvent::DestroyedActorEvent:
			GetWorld()->RemoveOnActorDestroyededHandler(mActorHandle);
			break;
		case EKBFLWorldCDOActorListenerEvent::SpawnedActorEvent:
			GetWorld()->RemoveOnActorSpawnedHandler(mActorHandle);
			break;
		default:
			break;
		}

		UE_LOGFMT(LogKBFLActorListener, Verbose, "Clear: Unregistered {EventType} listener | Asset: {AssetPath}",
				  EventTypeStr, GetPathName());
	}

	Super::Clear();
}

void UKBFLWorldCDOActorListener::OnActorEvent(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	// Check if the spawned actor is of a relevant class
	for (TSubclassOf<AActor> ActorClass : mActorClassesToListenFor)
	{
		bool bMatchSubclass = ActorClass && Actor->IsA(ActorClass);
		bool bMatchExactClass = ActorClass && Actor->GetClass() == ActorClass;

		if ((bMatchSubclass && bUseSubclassCheck) || bMatchExactClass)
		{
			if (Requirements_IsMet(Actor))
			{
				const FString EventTypeStr = mEventToListenFor == EKBFLWorldCDOActorListenerEvent::SpawnedActorEvent
					? TEXT("Spawned")
					: TEXT("Destroyed");

				UE_LOGFMT(
					LogKBFLActorListener, Log,
					"OnActorEvent: Processing {EventType} actor {ActorName} of class {ClassName} | Asset: {AssetPath}",
					EventTypeStr, Actor->GetName(), Actor->GetClass()->GetName(), GetPathName());

				Requirements_NotifyOnModify(Actor);
				Requirements_NotifyOnModified(Actor);
			}
			else
			{
				UE_LOGFMT(LogKBFLActorListener, Log,
						  "OnActorEvent: Requirements not met for actor {ActorName} | Asset: {AssetPath}",
						  Actor->GetName(), GetPathName());
			}
		}
	}
}
