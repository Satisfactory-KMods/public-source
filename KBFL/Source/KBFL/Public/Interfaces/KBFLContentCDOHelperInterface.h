// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Module/ModModule.h"
#include "Subsystems/HelperClasses/KBFL_CDOHelperClass_Recipes.h"
#include "UObject/Interface.h"

#include "KBFLContentCDOHelperInterface.generated.h"

USTRUCT(BlueprintType)
struct FKBFLItemArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<UFGItemDescriptor>> mItems = {};
};

USTRUCT(BlueprintType)
struct FKBFLPhases
{
	GENERATED_BODY()

	FKBFLPhases() { mCalledPhases = {}; };

	FKBFLPhases(TArray<ELifecyclePhase> Phases) { mCalledPhases = Phases; };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<ELifecyclePhase> mCalledPhases;
};

USTRUCT(BlueprintType)
struct FKBFLCDOInformation
{
	GENERATED_BODY()

	FKBFLCDOInformation()
	{
		mItemStackSizeCDO.Add(EStackSize::SS_ONE, FKBFLItemArray());
		mItemStackSizeCDO.Add(EStackSize::SS_SMALL, FKBFLItemArray());
		mItemStackSizeCDO.Add(EStackSize::SS_MEDIUM, FKBFLItemArray());
		mItemStackSizeCDO.Add(EStackSize::SS_BIG, FKBFLItemArray());
		mItemStackSizeCDO.Add(EStackSize::SS_HUGE, FKBFLItemArray());
		mItemStackSizeCDO.Add(EStackSize::SS_FLUID, FKBFLItemArray());
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<UKBFL_CDOHelperClass_Base>> mCDOHelperClasses = {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EStackSize, FKBFLItemArray> mItemStackSizeCDO;
};


UINTERFACE()
class UKBFLContentCDOHelperInterface : public UInterface
{
	GENERATED_BODY()
};


/**
 * @deprecated will replaced with the new CDO system based on PrimaryDataAssets
 */
class KBFL_API IKBFLContentCDOHelperInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "KMods|ContentCDOHelper Interface")
	FKBFLCDOInformation GetCDOInformationFromPhase(ELifecyclePhase Phase, bool& HasPhase);
};
