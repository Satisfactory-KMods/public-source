#pragma once

#include "CoreMinimal.h"

#include "Module/GameWorldModule.h"

#include "KDFWorldModule.generated.h"

/**
 * KDataForge root game world module (auto-discovered by SML).
 * Registers the `/kdf` chat command and the per-save state subsystem, and pushes all queued
 * content (recipes/schematics/research/sink tables) into the world's UModContentRegistry during
 * INITIALIZATION — before the registry freezes at world begin play.
 */
UCLASS()
class KDATAFORGE_API UKDFWorldModule : public UGameWorldModule
{
	GENERATED_BODY()

public:
	UKDFWorldModule();

	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
};
