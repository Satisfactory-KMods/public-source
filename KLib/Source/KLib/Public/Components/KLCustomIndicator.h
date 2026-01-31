// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "FactoryGame.h"
#include "FGColoredInstanceMeshProxy.h"
#include "Components/StaticMeshComponent.h"
#include "KLCustomIndicator.generated.h"


/**
* Proxy placed in buildings to be replaced with an instance on creation, supports coloring.
*/
UCLASS(Blueprintable, meta = ( BlueprintSpawnableComponent ))
class KLIB_API UKLCustomIndicator : public UFGColoredInstanceMeshProxy
{
	GENERATED_BODY()

public:
	UKLCustomIndicator();

	// Begin AActorComponent interface
	virtual void BeginPlay() override;
	// End AActorComponent interface

	UFUNCTION(BlueprintCallable, Category="KMods Indicator")
	void UpdateColors(const TArray<FLinearColor>& ColorArray);

	UFUNCTION(BlueprintCallable, Category="KMods Indicator")
	void UpdateColorIndex(int NewIndex);

	UFUNCTION(BlueprintNativeEvent, Category="KMods Indicator")
	void onUpdatedColorIndex(int NewIndex);

	UPROPERTY(BlueprintReadOnly, Category="KMods Indicator")
	UMaterialInstanceDynamic* mDynamicMaterial;

	UPROPERTY(EditDefaultsOnly, Category="KMods Indicator")
	int mMaterialIndex = 0;

	UPROPERTY(BlueprintReadWrite, Category="KMods Indicator")
	int mActiveColorIndex = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="KMods Indicator")
	TArray<FLinearColor> mColorArray;
};
