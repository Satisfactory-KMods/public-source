//

#pragma once

#include "CoreMinimal.h"
#include "FGSwatchGroup.h"
#include "Runtime/CoreUObject/Public/UObject/Object.h"

#include "KBFLCustomizationProvider.generated.h"

USTRUCT(BlueprintType)
struct FKBFLSwatchInformation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FColor mPrimaryColour = FColor(250, 149, 73, 255);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FColor mSecondaryColour = FColor(95, 102, 140, 255);
};


USTRUCT(BlueprintType)
struct FKBFLMaterialDescriptorInformation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<class UFGFactoryCustomizationDescriptor_Material>> mApplyThisInformationTo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<class AFGBuildable>> mValidBuildables;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TSubclassOf<class AFGBuildable>, TSubclassOf<class UFGRecipe>> mBuildableMap;
};

/**
 *
 */
UCLASS()
class KBFL_API UKBFLCustomizationProvider : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customizer")
	TArray<FKBFLMaterialDescriptorInformation> mMaterialInformation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customizer")
	TMap<TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>, FKBFLSwatchInformation> mSwatchInformation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customizer")
	TMap<TSubclassOf<UFGSwatchGroup>, TSubclassOf<UFGFactoryCustomizationDescriptor_Swatch>> mSwatchGroups;
};
