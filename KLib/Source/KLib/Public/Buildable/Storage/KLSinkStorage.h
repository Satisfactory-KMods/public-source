// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGPowerConnectionComponent.h"
#include "FGResourceSinkSubsystem.h"
#include "Buildables/FGBuildableStorage.h"
#include "KLSinkStorage.generated.h"

UCLASS()
class KLIB_API AKLSinkStorage : public AFGBuildableStorage
{
	GENERATED_BODY()

public:
	AKLSinkStorage();

	virtual void BeginPlay() override;
	virtual void Factory_Tick(float dt) override;
	virtual bool CanProduce_Implementation() const override;

	void HandleLastSlot();
	void CacheSubsystem();

	/** Cached resource sink subsystem */
	UPROPERTY()
	AFGResourceSinkSubsystem* mResourceSinkSubsystem;

	UPROPERTY()
	UFGPowerConnectionComponent* mPowerConnection;
};
