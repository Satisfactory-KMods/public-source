#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "KDFActorPatchSubsystem.generated.h"

class AActor;

/**
 * Applies registered runtime actor patches (FKDFActorPatch) to actor instances:
 *  - to every already-spawned matching actor when the world begins play,
 *  - to every matching actor spawned afterwards (spawn delegate).
 *
 * This covers values that per-instance state would otherwise override after spawn (e.g. archetype
 * or save-restored values), guaranteeing YAML-defined values win for the patched properties.
 * Patches are registered by cdo documents with `applyToSpawnedActors: true`.
 */
UCLASS()
class KDATAFORGE_API UKDFActorPatchSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Applies all matching patches to one actor. Returns the number of applied ops. */
	int32 ApplyPatchesToActor(AActor* Actor);

private:
	void OnActorSpawned(AActor* Actor);

	FDelegateHandle mSpawnHandle;
};
