// Copyright Kyri123 / KMods 2026. All Rights Reserved.



#pragma once

#include "CoreMinimal.h"

#include "AvailabilityDependencies/FGAvailabilityDependency.h"
#include "Curves/CurveFloat.h"
#include "DataAssets/KAPIDataAssetBase.h"
#include "FGSchematic.h"
#include "ItemAmount.h"
#include "Narrative/FGMessage.h"

#include "KAPIDeliveryTask.generated.h"

class UTexture2D;

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EKAPIDeliveryTaskParent : uint8
{
	None = 0,
	Above = 1 << 0,
	Left = 1 << 1,
	Right = 1 << 2,
	Bottom = 1 << 3,
	AboveLeft = 1 << 4,
	AboveRight = 1 << 5,
	BelowLeft = 1 << 6,
	BelowRight = 1 << 7
};
ENUM_CLASS_FLAGS(EKAPIDeliveryTaskParent);

USTRUCT(BlueprintType)
struct FKAPIItemPoolElement
{
	GENERATED_BODY()

public:
	FKAPIItemPoolElement()
	{
		mAmountRange.SetLowerBound(FInt32RangeBound::Inclusive(100));
		mAmountRange.SetUpperBound(FInt32RangeBound::Exclusive(500));
		mRequiredTas = INDEX_NONE;
	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UFGItemDescriptor> mItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInt32Range mAmountRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 mRequiredTas;

	bool Valid() const { return IsValid(mItem) && mAmountRange.GetLowerBoundValue() > 0; };
};

USTRUCT(BlueprintType)
struct FKAPITaskReward
{
	GENERATED_BODY()

public:
	FKAPITaskReward() { mRequiredTask = INDEX_NONE; };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FItemAmount> mRewards;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 mRequiredTask = INDEX_NONE;

	bool Valid() const
	{
		for (const FItemAmount& Reward : mRewards)
		{
			if (!IsValid(Reward.ItemClass) || Reward.Amount <= 0)
				return false;
		}
		return true;
	};
};

UCLASS(BlueprintType)
class KAPI_API UKAPIDeliveryTask : public UKAPIDataAssetBase
{
	GENERATED_BODY()

public:
	UKAPIDeliveryTask();

	UFUNCTION(BlueprintCallable, Category = "KAPI|DeliveryTask")
	static FItemAmount GetPoolElementAmount(const FKAPIItemPoolElement& Element, int32 MaxAmount,
											float Multiplier = 1.f);

	static bool SortByCoordinate(const UKAPIDeliveryTask& Left, const UKAPIDeliveryTask& Right)
	{
		return Left.mCoordinate.Y != Right.mCoordinate.Y ? Left.mCoordinate.Y < Right.mCoordinate.Y
														 : Left.mCoordinate.X < Right.mCoordinate.X;
	}

	void AssertTask(const TMap<FIntVector2, TObjectPtr<UKAPIDeliveryTask>>& TaskMap) const;
	void GetPoolElementsAmount(TArray<FItemAmount>& OutElements, int32 TaskCount) const;

	UFUNCTION(BlueprintPure, Category = "KAPI|DeliveryTask")
	bool IsSchematicOnlyTask() const;

	UFUNCTION(BlueprintPure, Category = "KAPI|DeliveryTask")
	bool IsGateUnlockTask() const;

	void ForwardAdaMessages(UObject* WorldContextObject) const;

	void HandleUnlock(UObject* WorldContextObject) const;

	void ApplyUnlocks(UObject* WorldContextObject) const;

	UFUNCTION(BlueprintCallable, Category = "KAPI|DeliveryTask")
	void GetAllUnlocks(TArray<class UFGUnlock*>& OutUnlocks) const;

	UFUNCTION(BlueprintCallable, Category = "KAPI|DeliveryTask")
	void GetAllDependencies(TArray<UFGAvailabilityDependency*>& OutDependencies) const;

	UFUNCTION(BlueprintPure, Category = "KAPI|DeliveryTask", meta = (WorldContext = "WorldContextObject"))
	bool AreTaskDependenciesMet(UObject* WorldContextObject) const;

	UFUNCTION(BlueprintPure, Category = "KAPI|DeliveryTask")
	TArray<FIntVector2> GetParentCoordinates() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery")
	TArray<TObjectPtr<UFGMessage>> mAdaMessages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toast")
	TObjectPtr<UTexture2D> mToastIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toast", meta = (MultiLine = true))
	FText mToastText;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Dependencies")
	TArray<TObjectPtr<UFGAvailabilityDependency>> mTaskDependencies;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly)
	TArray<TObjectPtr<class UFGUnlock>> mUnlocks;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery")
	FIntVector2 mCoordinate = FIntVector2::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery",
			  meta = (Bitmask, BitmaskEnum = "/Script/KAPI.EKAPIDeliveryTaskParent"))
	int32 mParentTask = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	bool bRequireAllParents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery", meta = (ClampMin = "1", UIMin = "1"))
	int32 mRequiredParentCompletions = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery")
	bool bIsEndless = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "One Time",
			  meta = (EditConditionHides, EditCondition = "!bIsEndless"))
	TArray<FItemAmount> mCosts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "One Time",
			  meta = (EditConditionHides, EditCondition = "!bIsEndless"))
	TArray<FItemAmount> mRewards;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "One Time",
			  meta = (EditConditionHides, EditCondition = "!bIsEndless"))
	TArray<TSubclassOf<UFGSchematic>> mSchematics;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Endless",
			  meta = (EditConditionHides, EditCondition = "bIsEndless"))
	TArray<FKAPIItemPoolElement> mPoolElements;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Endless",
			  meta = (EditConditionHides, EditCondition = "bIsEndless", ClampMin = "-1", UIMin = "-1"))
	int32 mMaxPoolItems = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Endless",
			  meta = (EditConditionHides, EditCondition = "bIsEndless"))
	TArray<FKAPITaskReward> mTaskRewards;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Endless",
			  meta = (EditConditionHides, EditCondition = "bIsEndless"))
	int32 mMaxTasks = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Endless",
			  meta = (EditConditionHides, EditCondition = "bIsEndless"))
	TObjectPtr<UCurveFloat> mAmountMultiplierCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Endless",
			  meta = (EditConditionHides, EditCondition = "bIsEndless"))
	TMap<EResourceForm, TObjectPtr<UCurveFloat>> mAbsolutMaxItemAmount;
};
