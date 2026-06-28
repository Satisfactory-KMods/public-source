// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataAsset.h"

#include "KAPIDataAssetBase.generated.h"

UCLASS(BlueprintType)
class KAPI_API UKAPIDataAssetBase : public UPrimaryDataAsset
{
	GENERATED_BODY()

protected:
	//~ Begin UObject Interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FPrimaryAssetType("KAPIDataAsset"), GetFName());
	}
	//~ End UObject Interface

public:
	UFUNCTION(BlueprintPure)
	static bool IsEnabled(UKAPIDataAssetBase* Asset, UObject* WorldContextObject)
	{
		if (!Asset)
		{
			return false;
		}
		return Asset->IsEnabled_Internal(WorldContextObject);
	};

	/**
	 * Can be overridden to disable the asset or make some other checks
	 */
	virtual bool IsEnabled_Internal(UObject* WorldContextObject) const { return !mIsDisabled; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FText mName = FText::FromString("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (MultiLine = true))
	FText mDescription = FText::FromString("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TObjectPtr<UTexture> mIcon;

	/**
	 * Disable this asset from being used in the game
	 * For example if a mod is disabling some content from another mod
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset")
	bool mIsDisabled;

	/**
	 * Disable this asset from being used in the game
	 * For example if a mod is disabling some content from another mod
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset")
	int mPriority = -MAX_int32;
};
