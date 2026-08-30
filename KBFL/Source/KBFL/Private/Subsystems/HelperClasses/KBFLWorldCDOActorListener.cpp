#include "Subsystems/HelperClasses/KBFLWorldCDOActorListener.h"

#include "Engine/Level.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY_STATIC(LogKBFLActorListener, VeryVerbose, All);

void UKBFLWorldCDOActorListener::Start()
{
	Super::Start();

	if (!GetWorld())
	{

		return;
	}

	switch (mEventToListenFor)
	{
	case EKBFLWorldCDOActorListenerEvent::SpawnedActorEvent:
		mActorHandle = GetWorld()->AddOnActorSpawnedHandler(
			FOnActorSpawned::FDelegate::CreateUObject(this, &UKBFLWorldCDOActorListener::OnActorEvent));
		if (bListenStreamingLevel)
		{
			mLevelAddedHandle =
				FWorldDelegates::LevelAddedToWorld.AddUObject(this, &UKBFLWorldCDOActorListener::OnLevelAddedToWorld);
		}
		break;
	case EKBFLWorldCDOActorListenerEvent::DestroyedActorEvent:
		mActorHandle = GetWorld()->AddOnActorDestroyedHandler(
			FOnActorDestroyed::FDelegate::CreateUObject(this, &UKBFLWorldCDOActorListener::OnActorEvent));
		break;
	default:

		break;
	}
}

void UKBFLWorldCDOActorListener::ApplyToActorsInWorld()
{
	if (!GetWorld())
	{

		return;
	}

	for (TSubclassOf<AActor> ActorClass : mActorClassesToListenFor)
	{
		if (!ActorClass)
		{
			continue;
		}

		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ActorClass, Actors);

		for (AActor* Actor : Actors)
		{
			OnActorEvent(Actor);

		}
	}

}

void UKBFLWorldCDOActorListener::Clear()
{

	if (mLevelAddedHandle.IsValid())
	{
		FWorldDelegates::LevelAddedToWorld.Remove(mLevelAddedHandle);
		mLevelAddedHandle.Reset();
	}

	if (GetWorld() && mActorHandle.IsValid())
	{
		switch (mEventToListenFor)
		{
		case EKBFLWorldCDOActorListenerEvent::DestroyedActorEvent:
			GetWorld()->RemoveOnActorDestroyedHandler(mActorHandle);
			break;
		case EKBFLWorldCDOActorListenerEvent::SpawnedActorEvent:
			GetWorld()->RemoveOnActorSpawnedHandler(mActorHandle);
			break;
		default:
			break;
		}

	}

	mActorHandle.Reset();

	Super::Clear();
}

void UKBFLWorldCDOActorListener::OnLevelAddedToWorld(ULevel* Level, UWorld* World)
{
	if (!IsValid(Level) || World != GetWorld())
	{
		return;
	}

	for (AActor* Actor : Level->Actors)
	{
		OnActorEvent(Actor);
	}
}

void UKBFLWorldCDOActorListener::OnActorEvent(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	for (TSubclassOf<AActor> ActorClass : mActorClassesToListenFor)
	{
		bool bMatchSubclass = ActorClass && Actor->IsA(ActorClass);
		bool bMatchExactClass = ActorClass && Actor->GetClass() == ActorClass;

		if ((bMatchSubclass && bUseSubclassCheck) || bMatchExactClass)
		{
			if (Requirements_IsMet(Actor))
			{

				Requirements_NotifyOnModify(Actor);
				OnActorMatched(Actor);
				Requirements_NotifyOnModified(Actor);
			}
			else
			{

			}
		}
	}
}
