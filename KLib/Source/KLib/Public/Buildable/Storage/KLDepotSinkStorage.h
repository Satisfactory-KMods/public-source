// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGPowerConnectionComponent.h"
#include "FGResourceSinkSubsystem.h"
#include "Buildables/FGCentralStorageContainer.h"
#include "Structures/KPCLFunctionalStructure.h"

#include "KLDepotSinkStorage.generated.h"

UCLASS()
class KLIB_API AKLDepotSinkStorage : public AFGCentralStorageContainer
{
	GENERATED_BODY()

public:
	AKLDepotSinkStorage();

	virtual void BeginPlay() override;
	virtual void Factory_Tick(float dt) override;
	virtual bool CanProduce_Implementation() const override;

	void HandleStack();
	void CacheSubsystem();

	/** Cached resource sink subsystem */
	UPROPERTY()
	AFGResourceSinkSubsystem* mResourceSinkSubsystem;

	UPROPERTY()
	UFGPowerConnectionComponent* mPowerConnection;

	UPROPERTY(SaveGame)
	FSmartTimer mTimer;
};
