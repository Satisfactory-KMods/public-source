// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataAsset.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "KAPIGameplayTags.h"

#include "KAPIDataAssetBase.generated.h"

UCLASS(BlueprintType)
class KAPI_API UKAPIDataAssetBase : public UPrimaryDataAsset, public IGameplayTagAssetInterface
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
	//~ Begin IGameplayTagAssetInterface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override
	{
		TagContainer.AppendTags(mTags);
	}
	//~ End IGameplayTagAssetInterface

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
	virtual bool IsEnabled_Internal(UObject* WorldContextObject) const
	{
		return !(mIsDisabled || HasTag(TAG_KMods_Disabled));
	}

	/** Gameplay tags of this data asset. */
	UFUNCTION(BlueprintPure, Category = "Asset|Tags")
	FGameplayTagContainer GetTags() const { return mTags; }

	/** True when the asset carries the tag. Exact match ignores parent tags. */
	UFUNCTION(BlueprintPure, Category = "Asset|Tags")
	bool HasTag(FGameplayTag Tag, bool bExactMatch = false) const
	{
		return bExactMatch ? mTags.HasTagExact(Tag) : mTags.HasTag(Tag);
	}

	/** True when the asset carries at least one tag of the container. */
	UFUNCTION(BlueprintPure, Category = "Asset|Tags")
	bool HasAnyTags(const FGameplayTagContainer& Tags, bool bExactMatch = false) const
	{
		return bExactMatch ? mTags.HasAnyExact(Tags) : mTags.HasAny(Tags);
	}

	/** True when the asset carries every tag of the container. An empty container returns true. */
	UFUNCTION(BlueprintPure, Category = "Asset|Tags")
	bool HasAllTags(const FGameplayTagContainer& Tags, bool bExactMatch = false) const
	{
		return bExactMatch ? mTags.HasAllExact(Tags) : mTags.HasAll(Tags);
	}

	/** Intersection of this asset's tags and Filter — all tags of the asset that match any tag in Filter. */
	UFUNCTION(BlueprintPure, Category = "Asset|Tags")
	FGameplayTagContainer FilterTags(const FGameplayTagContainer& Filter, bool bExactMatch = false) const
	{
		return bExactMatch ? mTags.FilterExact(Filter) : mTags.Filter(Filter);
	}

	/** Adds a tag to the asset. Invalid or already present tags are ignored. Returns true when the tag was added. */
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

	/** Appends all tags of Other to the asset. */
	UFUNCTION(BlueprintCallable, Category = "Asset|Tags")
	void AppendTags(const FGameplayTagContainer& Other) { mTags.AppendTags(Other); }

	/** Removes a tag from the asset. Returns true when the tag was present. */
	UFUNCTION(BlueprintCallable, Category = "Asset|Tags")
	bool RemoveTag(FGameplayTag Tag) { return mTags.RemoveTag(Tag); }

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

	/**
	 * Gameplay tags of this data asset (e.g. KMods.* flags).
	 * Queryable via the HasTag/HasAnyTags/HasAllTags member helpers, IGameplayTagAssetInterface,
	 * or UKAPIGameplayTagFunctionLibrary::GetGameplayTagsForObject / ObjectHasTag.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asset")
	FGameplayTagContainer mTags;
};
