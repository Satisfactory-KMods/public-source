// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "KAPIDataAssetBase.generated.h"

UCLASS(BlueprintType)
class KAPI_API UKAPIDataAssetBase : public UPrimaryDataAsset
{
	GENERATED_BODY()

protected:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FPrimaryAssetType("KAPIDataAsset"), GetFName());
	}

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
	FText mName = FText::FromString("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI", meta=(MultiLine=true))
	FText mDescription = FText::FromString("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
	UTexture* mIcon;

	/**
	 * Can be overridden to disable the asset or make some other checks
	 */
	virtual bool IsEnabled_Internal(UObject* WorldContextObject) const
	{
		return !mIsDisabled;
	}

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
