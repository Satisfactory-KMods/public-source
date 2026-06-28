// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Module/MenuWorldModule.h"

#include "KBFLMenuModule.generated.h"

/**
 *
 */
UCLASS(Blueprintable)
class KBFL_API UKBFLMenuModule : public UMenuWorldModule
{
	GENERATED_BODY()

public:
	UKBFLMenuModule();

	// BEGIN UGameWorldModule
	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
	// END UGameWorldModule

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void ConstructionPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void InitPhase();

	UFUNCTION(BlueprintNativeEvent, Category = "LifecyclePhase")
	void PostInitPhase();
};
