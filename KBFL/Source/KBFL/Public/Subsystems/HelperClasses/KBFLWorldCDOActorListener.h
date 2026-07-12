#pragma once

#include "CoreMinimal.h"
#include "Subsystems/HelperClasses/KBFLCDOOverwriteBase.h"

#include "KBFLWorldCDOActorListener.generated.h"

class ULevel;

UENUM(BlueprintType)
enum class EKBFLWorldCDOActorListenerEvent : uint8
{
	SpawnedActorEvent,
	DestroyedActorEvent,
};

/**
 * WIP: Listens for actor spawns in the world and can apply modifications
 * Can be used to modify actors as they are spawned
 */
UCLASS(NotBlueprintable, NotBlueprintType)
class KBFL_API UKBFLWorldCDOActorListener : public UKBFLCDOOverwriteWorldBasedBase
{
	GENERATED_BODY()

public:
	/**
	 * Registers the appropriate world delegate based on mEventToListenFor (spawn or destroy). For spawn events with
	 * bListenStreamingLevel, also hooks LevelAddedToWorld so actors loaded via streaming levels are processed.
	 */
	virtual void Start() override;

	/** Runs OnActorEvent against all actors of the configured classes that already exist in the world. */
	virtual void ApplyToActorsInWorld() override;

	/** Unregisters the spawn/destroy delegate (and the streaming-level handler, if registered). */
	virtual void Clear() override;

	/**
	 * Delegate callback for actor spawn/destroy. If Actor matches a configured class (honoring bUseSubclassCheck) and
	 * passes the requirements, fires the OnModify notify, calls OnActorMatched, then the OnModified notify.
	 */
	UFUNCTION()
	void OnActorEvent(AActor* Actor);

	/** Override point: a matching actor passed the class + requirement checks. Subclasses do their work here. */
	virtual void OnActorMatched(AActor* Actor) {}

	// ===== Listener Settings =====
	/** Event type to listen for (spawned or destroyed actors) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Listener Settings")
	EKBFLWorldCDOActorListenerEvent mEventToListenFor = EKBFLWorldCDOActorListenerEvent::SpawnedActorEvent;

	/** If true, also listen for subclasses of the specified actor classes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Listener Settings")
	bool bUseSubclassCheck = true;

	/** If true (and mEventToListenFor is SpawnedActorEvent), also process actors loaded via streaming levels */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Listener Settings")
	bool bListenStreamingLevel = true;

	// ===== Target Actors =====
	/** Specify which actor classes to listen for */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowAbstract = "true"), Category = "Target Actors")
	TArray<TSubclassOf<AActor>> mActorClassesToListenFor;

protected:
	/** Streaming-level callback: runs OnActorEvent for every actor in a level newly added to this listener's world. */
	void OnLevelAddedToWorld(ULevel* Level, UWorld* World);

	FDelegateHandle mActorHandle;
	FDelegateHandle mLevelAddedHandle;
};
