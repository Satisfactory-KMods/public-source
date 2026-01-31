// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KAPIDataAssetBase.h"
#include "Buildables/FGBuildableResourceExtractorBase.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"

#include "KAPTooltipWidgetInjector.generated.h"


UCLASS(BlueprintType)
class KAPI_API UKAPTooltipWidgetInjector : public UKAPIDataAssetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	bool ShouldInjectForItem(TSubclassOf<UFGItemDescriptor> ItemClass) const;

	static void SetPadding(UPanelSlot* Slot, FMargin Padding);
	static FMargin GetPadding(UPanelSlot* Slot);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	TSubclassOf<UUserWidget> mWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	TSet<TSubclassOf<UFGItemDescriptor>> mClassFilter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	int32 mInjectAtIndex = -1;
};
