#pragma once

#include "CoreMinimal.h"
#include "Subsystems/HelperClasses/KBFLCDOCallRequirement.h"

#include "KBFLWorldCDOCallRequirement.generated.h"

/**
 * WIP: A specialized requirement for world-based CDO overwrites
 * This class provides additional context for requirements that need world access
 */
UCLASS(BlueprintType, Blueprintable, HideCategories = (Object))
class KBFL_API UKBFLWorldCDOCallRequirement : public UKBFLCDOCallRequirement
{
	GENERATED_BODY()

public:
	/** Per-frame tick driven by the owning world-based overwrite. Default: no-op (override to add per-frame logic). */
	UFUNCTION(BlueprintNativeEvent)
	void Tick(float dt, class UKBFLCDOOverwriteWorldBasedBase* From);

	/** Returns the cached world (mWorld) this requirement was given, rather than resolving via the subsystem. */
	virtual class UWorld* GetWorld() const override;

	UPROPERTY()
	TObjectPtr<UWorld> mWorld;
};
