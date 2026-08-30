#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGCentralStorageContainer.h"
#include "FGResourceSinkSubsystem.h"

#include "KLDepotSinkStorage.generated.h"

UCLASS()
class KLIB_API AKLDepotSinkStorage : public AFGCentralStorageContainer
{
	GENERATED_BODY()

public:
	AKLDepotSinkStorage();

	virtual bool CanProduce_Implementation() const override;

	virtual void BeginPlay() override;

	void CacheSubsystem();

	void TrySinkOverflow();

	void OnInventoryItemAdded(TSubclassOf<UFGItemDescriptor> ItemClass, int32 NumAdded,
							  UFGInventoryComponent* SourceInventory);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|SinkStorage",
			  meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float mKeepStackFraction = 0.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|SinkStorage")
	int32 mMaxItemsToSinkPerCycle = 0;

	UPROPERTY()
	TObjectPtr<AFGResourceSinkSubsystem> mResourceSinkSubsystem;
};
