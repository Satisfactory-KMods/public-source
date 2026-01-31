// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "Hologram/FGFactoryBuildingHologram.h"
#include "KBFLUtilWidget.generated.h"

UCLASS()
class KBFL_API UKBFLUtilWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	void OnNewStats(const FVector& NewScale, const FVector& NewLocation, const FRotator& NewRotation);

	UPROPERTY(BlueprintReadOnly)
	FVector Scale = FVector(1);

	UPROPERTY(BlueprintReadOnly)
	FVector Location = FVector(0);

	UPROPERTY(BlueprintReadOnly)
	FRotator Rotation = FRotator(0);
};
