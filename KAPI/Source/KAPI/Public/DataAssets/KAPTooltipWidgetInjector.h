// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Components/PanelWidget.h"

#include "DataAssets/KAPIDataAssetBase.h"

#include "KAPTooltipWidgetInjector.generated.h"

UCLASS(BlueprintType)
class KAPI_API UKAPTooltipWidgetInjector : public UKAPIDataAssetBase
{
	GENERATED_BODY()

public:
	static FMargin GetPadding(UPanelSlot* Slot);

	static void SetPadding(UPanelSlot* Slot, FMargin Padding);

	UFUNCTION(BlueprintCallable)
	bool ShouldInjectForItem(TSubclassOf<UFGItemDescriptor> ItemClass) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	TSubclassOf<UUserWidget> mWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	TSet<TSubclassOf<UFGItemDescriptor>> mClassFilter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	int32 mInjectAtIndex = -1;
};
