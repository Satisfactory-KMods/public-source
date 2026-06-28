// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildable.h"
#include "FGRecipe.h"
#include "FGRecipeManager.h"
#include "Resources/FGResourceDescriptor.h"

#include "DataAssets/KAPIDataAssetBase.h"
#include "Descriptors/KAPIModularAttachmentDescriptor.h"
#include "Descriptors/KAPIWasteProducerType.h"

#include "KAPIModularMinerDescription.generated.h"

class AFGRecipeManager;

USTRUCT(BlueprintType)
struct FKAPIModuleItems
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UFGItemDescriptor> mProductionItem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UFGItemDescriptor> mTrashItem = nullptr;

	/** Shouldnt be used for now */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool mTryProductionDynamic = false;
};

USTRUCT(BlueprintType)
struct FKAPIMinerInfoFluids
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	int mNormalFluidCountPerSecond;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	float ProductionTimeMulti;
};

UCLASS(BlueprintType)
class KAPI_API UKAPIModularMinerDescription : public UKAPIDataAssetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KAPI|Miner")
	FKAPIModuleItems GetItemsForModule(TSubclassOf<UKAPIWasteProducerType> Module) const;

	UFUNCTION(BlueprintCallable, Category = "KAPI|Miner")
	bool IsModuleAllowed(TSubclassOf<AFGBuildable> Module) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FText mRarityText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FText mFoundText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TObjectPtr<UTexture2D> mNodeScreenshot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	bool mIsFallbackDescriptor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Module")
	TMap<TSubclassOf<UFGItemDescriptor>, FKAPIMinerInfoFluids> mFluidInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	TArray<TSubclassOf<UKAPIModularAttachmentDescriptor>> mNeededModules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	TArray<TSubclassOf<AFGBuildable>> mPreventModules = {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modules")
	TMap<TSubclassOf<UKAPIWasteProducerType>, FKAPIModuleItems> mModuleInformation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	TSubclassOf<UFGResourceDescriptor> mResourceClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int mDrillTier = 1;

	/** Unused yet */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int mScannerTier = 1;
};
