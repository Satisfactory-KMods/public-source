#pragma once

#include "CoreMinimal.h"

#include "Module/GameInstanceModule.h"

#include "KDFGameInstanceModule.generated.h"

/**
 * KDataForge root game instance module (auto-discovered by SML).
 *
 * CONSTRUCTION: built-in handlers exist (subsystem Initialize); third-party mods register theirs now.
 * INITIALIZATION: full YAML load — scan, parse, apply all stages (before any world/save loads).
 * POST_INITIALIZATION: load report.
 */
UCLASS()
class KDATAFORGE_API UKDFGameInstanceModule : public UGameInstanceModule
{
	GENERATED_BODY()

public:
	UKDFGameInstanceModule();

	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
};
