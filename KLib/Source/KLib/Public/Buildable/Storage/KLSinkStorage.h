// ILikeBanas

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

	//~ Begin AFGBuildableFactory Interface
	virtual bool CanProduce_Implementation() const override;
	//~ End AFGBuildableFactory Interface

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

	void CacheSubsystem();

	/** Drains the last inventory slot if it holds a sinkable item. Only the last slot acts as the sink slot. */
	void TrySinkOverflow();

	void OnInventoryItemAdded(TSubclassOf<UFGItemDescriptor> ItemClass, int32 NumAdded,
							  UFGInventoryComponent* SourceInventory);

	UFUNCTION()
	void OnPowerStateChanged(bool bNewHasPower);

	/** Max items to sink per item-added event. 0 = drain the entire eligible slot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|SinkStorage")
	int32 mMaxItemsToSinkPerCycle = 0;

	UPROPERTY()
	TObjectPtr<UFGPowerConnectionComponent> mPowerConnection;

	UPROPERTY()
	TObjectPtr<AFGResourceSinkSubsystem> mResourceSinkSubsystem;
};
