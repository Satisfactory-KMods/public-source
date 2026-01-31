// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "KBFLWorldModule.h"
#include "KPCLWorldModule.generated.h"


/**
 * This is a full native version for UKBFLWorldModule
 */
UCLASS(Blueprintable)
class KPRIVATECODELIB_API UKPCLWorldModule : public UKBFLWorldModule
{
	GENERATED_BODY()

public:
	UKPCLWorldModule();

	// Start: UKBFLWorldModule
	virtual void InitPhase_Implementation() override;
	// End: UKBFLWorldModule

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Options")
	bool IsEasyNodesEnabled();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Options")
	void ApplyEasyNodes(const TArray<TSubclassOf<UFGResearchTree>>& Nodes);

	UPROPERTY(EditDefaultsOnly, Category="KMods|ADA")
	bool mUseEasyNodes = true;
};
