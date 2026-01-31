// ILikeBanas

#pragma once

#include "CoreMinimal.h"
#include "Buildable/KPCLProducerBase.h"
#include "DataAssets/KAPIAirCollectorData.h"

#include "KLBuildableAirCollector.generated.h"

/**
 * 
 */
UCLASS()
class KLIB_API AKLBuildableAirCollector : public AKPCLProducerBase
{
	GENERATED_BODY()

public:
	AKLBuildableAirCollector();

	virtual void Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo) override;
	virtual void Overclocking_GetProductionResults_Implementation(
		TArray<FKPCLOverclockingProductionResults>& OutIngredients,
		TArray<FKPCLOverclockingProductionResults>& OutProducts) override;

	virtual void Factory_Tick(float dt) override;
	virtual void BeginPlay() override;
	virtual bool CanProduce_Implementation() const override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual void CollectAndPushPipes(float dt, bool IsPush) override;

	virtual void onProducingFinal_Implementation() override;

	void FindNearbyCollectos();
	void CheckGasType();
	void SetInstancedMesh();

	UFUNCTION(BlueprintPure, Category = "KMods|Air Collector")
	float GetCollectorHeight() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Air Collector")
	void GetCollectorHeightBonus(float& InPercentValue, float& InFloatValue) const;

	UFUNCTION(BlueprintPure, Category = "KMods|Air Collector")
	int32 CalculateProduce() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Air Collector")
	int32 CalculateProduceWithoutMalus() const;

	int32 CalculateProductionBasedOnHits() const;
	int32 CalculateProductionBasedOnHeight() const;

	UFUNCTION(BlueprintPure, Category = "KMods|Air Collector")
	int32 CalculateProduceMalus() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|Air Collector")
	void CacheNearbyCollectors();

	UFUNCTION(BlueprintPure, Category="KMods|Air Collector")
	void GetCachedNearCollectors(TArray<AKLBuildableAirCollector*>& Collectors) const;

	UFUNCTION(BlueprintPure, Category="KMods|Air Collector")
	int32 GetNumOfCollectorsInRange() const;

	UFUNCTION(BlueprintPure, Category="KMods|Air Collector")
	UKAPIAirCollectorData* GetScannerInformation() const;

	TArray<UKAPIAirCollectorData*> GetAllScans() const;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = ( FGReplicated ), Category = "KMods|Air Collector")
	UKAPIAirCollectorData* mScannerInformation;

	UPROPERTY(SaveGame, BlueprintReadWrite, meta = ( FGReplicated ), Category = "KMods|Air Collector")
	int32 mHittedElements = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|Air Collector")
	UKAPIAirCollectorData* mScannerFallback;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KMods|Air Collector")
	int32 mRangeForFindCollectors = 2500;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KMods|Air Collector")
	int32 mCollectorMaxHeight = 45000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KMods|Air Collector")
	int32 mCollectorMinHeight = 4000;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Air Collector")
	TMap<int32, class UStaticMesh*> mFoundationMeshMapping;

	UPROPERTY(meta = ( FGReplicated ))
	TArray<TWeakObjectPtr<AKLBuildableAirCollector>> mCachedNearCollectors;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "KMods|Inventory")
	UFGInventoryComponent* mOutputInventory;

	UPROPERTY(BlueprintReadOnly, Category = "KMods|Air Collector")
	float mCollectorHeight;

	int32 PRODUCT_INV_IDX = 0;
};
