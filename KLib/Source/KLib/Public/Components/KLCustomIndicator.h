// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Components/StaticMeshComponent.h"
#include "FGColoredInstanceMeshProxy.h"
#include "FactoryGame.h"

#include "KLCustomIndicator.generated.h"

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class KLIB_API UKLCustomIndicator : public UFGColoredInstanceMeshProxy
{
	GENERATED_BODY()

public:
	UKLCustomIndicator();

	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	//~ End UActorComponent Interface

	UFUNCTION(BlueprintCallable, Category = "KMods Indicator")
	void UpdateColorIndex(int NewIndex);

	UFUNCTION(BlueprintCallable, Category = "KMods Indicator")
	void UpdateColors(const TArray<FLinearColor>& ColorArray);

	UFUNCTION(BlueprintNativeEvent, Category = "KMods Indicator")
	void onUpdatedColorIndex(int NewIndex);

	UPROPERTY(EditDefaultsOnly, Category = "KMods Indicator")
	int mMaterialIndex = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KMods Indicator")
	TArray<FLinearColor> mColorArray;

	UPROPERTY(BlueprintReadOnly, Category = "KMods Indicator")
	TObjectPtr<UMaterialInstanceDynamic> mDynamicMaterial;

	UPROPERTY(BlueprintReadWrite, Category = "KMods Indicator")
	int mActiveColorIndex = 0;
};
