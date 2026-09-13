// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "KBFLCDOOverwriteBase.h"
#include "Resources/FGItemDescriptor.h"

#include "KBFLCDOItemStackSize.generated.h"

USTRUCT(BlueprintType)
struct FKBFLItemArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<UFGItemDescriptor>> mItems = {};
};

UCLASS()
class KBFL_API UKBFLCDOItemStackSize : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:

	virtual void ApplyToInstances() override;

	virtual bool ShouldCallForInstance(UClass* NewClass) override;

	virtual void ApplyToInstance(UObject* Instance) override;

	UPROPERTY(EditAnywhere, Category = "Stack Size Settings")
	TMap<EStackSize, FKBFLItemArray> mItemStackSizeCDO;
};
