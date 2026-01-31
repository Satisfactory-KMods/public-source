// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "FactoryGame.h"
#include "FGColoredInstanceMeshProxy.h"
#include "Components/StaticMeshComponent.h"
#include "KLCustomColorLamp.generated.h"


/**
* Proxy placed in buildings to be replaced with an instance on creation, supports coloring.
*/
UCLASS(Blueprintable, meta = ( BlueprintSpawnableComponent ))
class KLIB_API UKLCustomColorLamp : public UFGColoredInstanceMeshProxy
{
	GENERATED_BODY()

public:
	UKLCustomColorLamp();

	UFUNCTION(BlueprintCallable, Category="KMods Indicator")
	void UpdateColor(FLinearColor Colour);

	UFUNCTION(BlueprintCallable, Category="KMods Indicator")
	void UpdateEnableByIndex(int Index, bool isEnabled);

	UFUNCTION(BlueprintImplementableEvent, Category="KMods Indicator")
	void onUpdatedColorByIndex(int Index, FLinearColor Colour, bool isEnabled);

	UPROPERTY(EditDefaultsOnly, Category="KMods Indicator")
	float mEnabledValue = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category="KMods Indicator")
	float mEnabledValueOff = 2.0f;
};
