#pragma once

#include "CoreMinimal.h"

#include "DataAssets/KAPIDataAssetBase.h"
#include "ItemAmount.h"

#include "KAPICleanerItemDescription.generated.h"

USTRUCT(BlueprintType)
struct FKAPICleanerInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UFGItemDescriptor> mProduceItem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float mProductionTime = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int mProduceAmount = 0;
};

UCLASS(BlueprintType)
class KAPI_API UKAPICleanerItemDescription : public UKAPIDataAssetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "KAPI|Cleaner")
	bool CleanerItemNeeded() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unlock")
	bool bSkipUnlockCheck;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = mNeedCleanItem), Category = "Main Production")
	FKAPICleanerInfo mCleanerItemInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bypass Production")
	TArray<FKAPICleanerInfo> mBypassProducts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
	float mProductionTime = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Production")
	FItemAmount mInFluid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Production")
	FItemAmount mOutFluid;

private:
	UPROPERTY(meta = (NoAutoJson = true))
	bool mNeedCleanItem = false;
};
