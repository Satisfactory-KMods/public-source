// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "KAPIGameplayTags.h"

#include "KAPIDataAssetBase.generated.h"

UCLASS(BlueprintType)
class KAPI_API UKAPIDataAssetBase : public UPrimaryDataAsset, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

protected:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FPrimaryAssetType("KAPIDataAsset"), GetFName());
	}

public:

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override
	{
		TagContainer.AppendTags(mTags);
	}

	UFUNCTION(BlueprintPure)
	static bool IsEnabled(UKAPIDataAssetBase* Asset, UObject* WorldContextObject)
	{
		if (!Asset)
		{
			return false;
		}
		return Asset->IsEnabled_Internal(WorldContextObject);
	};

	virtual bool IsEnabled_Internal(UObject* WorldContextObject) const
	{
		return !(mIsDisabled || HasTag(TAG_KMods_Disabled));
	}

	UFUNCTION(BlueprintPure, Category = "Asset|Tags")
	FGameplayTagContainer GetTags() const { return mTags; }

	UFUNCTION(BlueprintPure, Category = "Asset|Tags")
	bool HasTag(FGameplayTag Tag, bool bExactMatch = false) const
	{
		return bExactMatch ? mTags.HasTagExact(Tag) : mTags.HasTag(Tag);
	}

	UFUNCTION(BlueprintPure, Category = "Asset|Tags")
	bool HasAnyTags(const FGameplayTagContainer& Tags, bool bExactMatch = false) const
	{
		return bExactMatch ? mTags.HasAnyExact(Tags) : mTags.HasAny(Tags);
	}

	UFUNCTION(BlueprintPure, Category = "Asset|Tags")
	bool HasAllTags(const FGameplayTagContainer& Tags, bool bExactMatch = false) const
	{
		return bExactMatch ? mTags.HasAllExact(Tags) : mTags.HasAll(Tags);
	}

	UFUNCTION(BlueprintPure, Category = "Asset|Tags")
	FGameplayTagContainer FilterTags(const FGameplayTagContainer& Filter, bool bExactMatch = false) const
	{
		return bExactMatch ? mTags.FilterExact(Filter) : mTags.Filter(Filter);
	}

	UFUNCTION(BlueprintCallable, Category = "Asset|Tags")
	bool AddTag(FGameplayTag Tag)
	{
		if (!Tag.IsValid() || mTags.HasTagExact(Tag))
		{
			return false;
		}
		mTags.AddTag(Tag);
		return true;
	}

	UFUNCTION(BlueprintCallable, Category = "Asset|Tags")
	void AppendTags(const FGameplayTagContainer& Other) { mTags.AppendTags(Other); }

	UFUNCTION(BlueprintCallable, Category = "Asset|Tags")
	bool RemoveTag(FGameplayTag Tag) { return mTags.RemoveTag(Tag); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FText mName = FText::FromString("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI", meta = (MultiLine = true))
	FText mDescription = FText::FromString("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TObjectPtr<UTexture2D> mIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset")
	bool mIsDisabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset")
	int mPriority = -MAX_int32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset")
	FGameplayTagContainer mTags;
};
