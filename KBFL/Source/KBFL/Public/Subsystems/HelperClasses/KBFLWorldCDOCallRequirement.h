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
	UFUNCTION(BlueprintNativeEvent)
	void Tick(float dt, class UKBFLCDOOverwriteWorldBasedBase* From);

	virtual class UWorld* GetWorld() const override;

	UPROPERTY()
	TObjectPtr<UWorld> mWorld;
};
