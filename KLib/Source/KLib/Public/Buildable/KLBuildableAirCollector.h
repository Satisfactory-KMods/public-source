// Copyright Kyri123 / KMods 2026. All Rights Reserved.

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

	virtual bool CanProduce_Implementation() const override;
	virtual void CollectAndPushPipes(float dt, bool IsPush) override;
	virtual void Factory_TickAuthOnly(float dt) override;
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void onProducingFinal_Implementation() override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	virtual void Overclocking_GetInfo_Implementation(FKPCLOverclockingProductionInfo& OutProductionInfo) override;
	virtual void
	Overclocking_GetProductionResults_Implementation(TArray<FKPCLOverclockingProductionResults>& OutIngredients,
	                                                 TArray<FKPCLOverclockingProductionResults>& OutProducts) override;

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

	int32 GetValidNearCollectorCount() const;

	float GetHeightMultiplier() const;

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

	UPROPERTY(BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Air Collector")
	int32 mCachedProduceAmount = 0;

	UPROPERTY(meta = (FGReplicated))
	int32 mCachedProduceWithoutMalus = 0;
};
