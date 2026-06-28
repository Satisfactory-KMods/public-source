// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Components/StaticMeshComponent.h"
#include "FGColoredInstanceMeshProxy.h"
#include "FactoryGame.h"

#include "KLCustomColorLamp.generated.h"

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class KLIB_API UKLCustomColorLamp : public UFGColoredInstanceMeshProxy
{
	GENERATED_BODY()

public:
	UKLCustomColorLamp();

	UFUNCTION(BlueprintCallable, Category = "KMods Indicator")
	void UpdateColor(FLinearColor Colour);

	UFUNCTION(BlueprintCallable, Category = "KMods Indicator")
	void UpdateEnableByIndex(int Index, bool isEnabled);

	UFUNCTION(BlueprintImplementableEvent, Category = "KMods Indicator")
	void onUpdatedColorByIndex(int Index, FLinearColor Colour, bool isEnabled);

	UPROPERTY(EditDefaultsOnly, Category = "KMods Indicator")
	float mEnabledValue = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "KMods Indicator")
	float mEnabledValueOff = 2.0f;
};
