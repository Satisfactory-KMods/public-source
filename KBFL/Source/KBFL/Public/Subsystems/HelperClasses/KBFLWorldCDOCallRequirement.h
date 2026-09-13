// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/HelperClasses/KBFLCDOCallRequirement.h"

#include "KBFLWorldCDOCallRequirement.generated.h"

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
