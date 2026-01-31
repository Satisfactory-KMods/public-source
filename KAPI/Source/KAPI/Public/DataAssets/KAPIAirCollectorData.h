// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemAmount.h"
#include "DataAssets/KAPIDataAssetBase.h"

#include "KAPIAirCollectorData.generated.h"

UCLASS(BlueprintType)

class KAPI_API UKAPIAirCollectorData : public UKAPIDataAssetBase
{
	GENERATED_BODY()

public:
	bool TestHit(AActor* InActor, int32& HitCount) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Collector World")
	int32 mRecipePrio = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Collector World")
	TArray<TSubclassOf<AActor>> mActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Collector World")
	float mRangeToScan = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Collector World")
	bool bCheckNodes = false;

	UPROPERTY(EditDefaultsOnly, Category = "Air Collector World")
	TArray<TEnumAsByte<EObjectTypeQuery>> mTraceChannel = {
		ObjectTypeQuery1, ObjectTypeQuery2
	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Collector Production")
	TSubclassOf<UFGItemDescriptor> mItemClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Collector Production")
	float mProductionTime = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Collector Production")
	bool bUseHightBasesdProduction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Collector Production")
	int32 mProduceItemCountMin = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Collector Production",
		meta = (EditCondition = "bUseHightBasesdProduction", EditConditionHides))
	int32 mProduceItemCountMax = 25000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Collector Production",
		meta = (EditCondition = "bUseHightBasesdProduction", EditConditionHides))
	int32 mProduceItemCountBase = 5000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Collector Production",
		meta = (EditCondition = "!bUseHightBasesdProduction", EditConditionHides))
	int32 mMaxHit = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Collector Production",
		meta = (EditCondition = "!bUseHightBasesdProduction", EditConditionHides))
	int32 mProductionPerHit = 2500;
};
