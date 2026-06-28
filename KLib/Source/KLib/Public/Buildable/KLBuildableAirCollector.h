// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildable/KPCLProducerBase.h"
#include "DataAssets/KAPIAirCollectorData.h"

#include "KLBuildableAirCollector.generated.h"

UCLASS()
class KLIB_API AKLBuildableAirCollector : public AKPCLProducerBase
{
	GENERATED_BODY()

public:
	AKLBuildableAirCollector();

	//~ Begin AFGBuildableFactory Interface
	virtual bool CanProduce_Implementation() const override;
	virtual void CollectAndPushPipes(float dt, bool IsPush) override;
	virtual void Factory_TickAuthOnly(float dt) override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void onProducingFinal_Implementation() override;
	//~ End AFGBuildableFactory Interface

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	//~ End AActor Interface

	//~ Begin AKPCLProducerBase Interface
	virtual void Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo) override;
	virtual void
	Overclocking_GetProductionResults_Implementation(TArray<FKPCLOverclockingProductionResults>& OutIngredients,
	                                                 TArray<FKPCLOverclockingProductionResults>& OutProducts) override;
	//~ End AKPCLProducerBase Interface

	UFUNCTION(BlueprintPure, Category = "KMods|Air Collector")
	int32 CalculateProduce() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Air Collector")
	int32 CalculateProduceMalus() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Air Collector")
	int32 CalculateProduceWithoutMalus() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Air Collector")
	void GetCollectorHeightBonus(float& InPercentValue, float& InFloatValue) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Air Collector")
	float GetCollectorHeight() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Air Collector")
	int32 GetNumOfCollectorsInRange() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Air Collector")
	void GetCachedNearCollectors(TArray<AKLBuildableAirCollector*>& Collectors) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Air Collector")
	UKAPIAirCollectorData* GetScannerInformation() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Air Collector")
	void CacheNearbyCollectors();

	int32 CalculateProductionBasedOnHeight() const;
	int32 CalculateProductionBasedOnHits() const;
	void CheckGasType();
	void FindNearbyCollectos();
	void SetInstancedMesh();
	TArray<UKAPIAirCollectorData*> GetAllScans() const;

	void SetHittedElements(int32 NewValue);
	void SetScannerInformation(UKAPIAirCollectorData* NewInfo);
	void SetCollectorHeight(float NewHeight);

	int32 PRODUCT_INV_IDX = 0;

private:
	/** Returns the number of currently valid (non-stale) cached nearby collectors. */
	int32 GetValidNearCollectorCount() const;

	/**
	 * Height multiplier in [1,5] based on mCollectorHeight relative to
	 * mCollectorMinHeight / mCollectorMaxHeight. Used by both GetCollectorHeightBonus
	 * and CalculateProductionBasedOnHeight to avoid duplicating the formula.
	 */
	float GetHeightMultiplier() const;

	/**
	 * Recomputes mCachedProduceAmount / mCachedProduceWithoutMalus from current inputs
	 * and marks both properties dirty for replication.
	 * Server / game-thread only — never call from Factory_Tick.
	 */
	void RecalculateProduction();

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KMods|Air Collector")
	int32 mCollectorMaxHeight = 45000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KMods|Air Collector")
	int32 mCollectorMinHeight = 4000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KMods|Air Collector")
	int32 mRangeForFindCollectors = 2500;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Air Collector")
	TObjectPtr<UKAPIAirCollectorData> mScannerFallback;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Air Collector")
	TMap<int32, TObjectPtr<class UStaticMesh>> mFoundationMeshMapping;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	TObjectPtr<UFGInventoryComponent> mOutputInventory;

	UPROPERTY(SaveGame, BlueprintReadWrite, meta = (FGReplicated), Category = "KMods|Air Collector")
	int32 mHittedElements = 1;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Air Collector")
	TObjectPtr<UKAPIAirCollectorData> mScannerInformation;

	UPROPERTY(BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Air Collector")
	float mCollectorHeight;

	UPROPERTY(meta = (FGReplicated))
	TArray<TWeakObjectPtr<AKLBuildableAirCollector>> mCachedNearCollectors;

	/**
	 * Cached result of CalculateProduce() — updated by RecalculateProduction() whenever an input
	 * changes. Transient (not SaveGame): recomputed in BeginPlay so save format is unchanged.
	 * Replicated so the client UI can display the value without recomputing on the wrong thread.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Air Collector")
	int32 mCachedProduceAmount = 0;

	/** Cached result of CalculateProduceWithoutMalus(). Transient, replicated (see mCachedProduceAmount). */
	UPROPERTY(meta = (FGReplicated))
	int32 mCachedProduceWithoutMalus = 0;
};
