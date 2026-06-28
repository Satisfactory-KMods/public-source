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
	virtual void Start() override;
	virtual void ApplyToActorsInWorld() override;
	virtual void Clear() override;

	// Called when a relevant actor is spawned or destroyed
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
	void OnLevelAddedToWorld(ULevel* Level, UWorld* World);

	FDelegateHandle mActorHandle;
	FDelegateHandle mLevelAddedHandle;
};
