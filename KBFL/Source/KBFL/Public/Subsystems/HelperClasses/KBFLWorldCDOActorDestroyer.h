#pragma once

#include "CoreMinimal.h"
#include "Subsystems/HelperClasses/KBFLCDOOverwriteBase.h"

#include "KBFLWorldCDOActorDestroyer.generated.h"

/**
 * WIP: Destroys actors in the world based on specified criteria
 * More advanced than ContentRemover, can use custom logic for destruction
 */
UCLASS(NotBlueprintable, NotBlueprintType)
class KBFL_API UKBFLWorldCDOActorDestroyer : public UKBFLCDOOverwriteWorldBasedBase
{
	GENERATED_BODY()

public:
	/** Entry point: destroys all existing actors of the configured classes in the world (delegates to DestoryAll). */
	virtual void ApplyToActorsInWorld() override;

	/** Finds every actor of each class in mActorClassesToDestroy and routes each through HandleDestroyActor. */
	void DestoryAll();

	/**
	 * Registers an on-actor-spawned handler (when bShouldDestroySpawnedActors) so actors spawning after the initial
	 * pass are also destroyed. The spawn callback is deferred to the next tick to avoid mutating the level actor
	 * array during spawn.
	 */
	virtual void Start() override;

	/** Unregisters the spawn handler and tears down state. */
	virtual void Clear() override;

	// ===== Destroyer Settings =====
	/** If true, destroy actors that spawn after initialization */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Destroyer Settings")
	bool bShouldDestroySpawnedActors = true;

	/** If true, also destroy subclasses of the specified actor classes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Destroyer Settings")
	bool bUseSubclassCheck = true;

	// ===== Target Actors =====
	/** Specify which actor classes to destroy */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowAbstract = "true"), Category = "Target Actors")
	TArray<TSubclassOf<AActor>> mActorClassesToDestroy;

	/** Spawn-handler callback: forwards a newly spawned actor to HandleDestroyActor if destruction is enabled. */
	UFUNCTION()
	void OnActorEvent(AActor* Actor);

protected:
	FDelegateHandle mActorHandle;

	/**
	 * Destroys Actor if it matches a configured class (honoring bUseSubclassCheck) and passes the requirements.
	 * The actual destruction is deferred to the next tick (via SetTimerForNextTick) so it never mutates the level
	 * actor array during a level-add / mixin pass. On the deferred tick: dismantles via IFGDismantleInterface
	 * (including child dismantle actors) when supported, otherwise calls Destroy(). Override to customize.
	 */
	virtual void HandleDestroyActor(AActor* Actor);
};
