// ILikeBanas

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

	//~ Begin AFGBuildableFactory Interface
	virtual bool CanProduce_Implementation() const override;
	//~ End AFGBuildableFactory Interface

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface

	void CacheSubsystem();

	/** Sinks the overflow of the last inventory slot above mKeepStackFraction of its stack size. */
	void TrySinkOverflow();

	void OnInventoryItemAdded(TSubclassOf<UFGItemDescriptor> ItemClass, int32 NumAdded,
							  UFGInventoryComponent* SourceInventory);

	/** Fraction of a full stack to keep in the sink slot; only the amount above this is sunk. [0,1] */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|SinkStorage",
			  meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float mKeepStackFraction = 0.9f;

	/** Max items to sink per item-added event. 0 = drain the entire overflow at once. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|SinkStorage")
	int32 mMaxItemsToSinkPerCycle = 0;

	UPROPERTY()
	TObjectPtr<AFGResourceSinkSubsystem> mResourceSinkSubsystem;
};
