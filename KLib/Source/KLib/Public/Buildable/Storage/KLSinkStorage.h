// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildableStorage.h"
#include "FGPowerConnectionComponent.h"
#include "FGResourceSinkSubsystem.h"

#include "KLSinkStorage.generated.h"

UCLASS()
class KLIB_API AKLSinkStorage : public AFGBuildableStorage
{
	GENERATED_BODY()

public:
	AKLSinkStorage();

	virtual bool CanProduce_Implementation() const override;

	virtual void BeginPlay() override;

	void CacheSubsystem();

	void TrySinkOverflow();

	void OnInventoryItemAdded(TSubclassOf<UFGItemDescriptor> ItemClass, int32 NumAdded,
							  UFGInventoryComponent* SourceInventory);

	UFUNCTION()
	void OnPowerStateChanged(bool bNewHasPower);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|SinkStorage")
	int32 mMaxItemsToSinkPerCycle = 0;

	UPROPERTY()
	TObjectPtr<UFGPowerConnectionComponent> mPowerConnection;

	UPROPERTY()
	TObjectPtr<AFGResourceSinkSubsystem> mResourceSinkSubsystem;
};
