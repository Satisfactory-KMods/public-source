// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "Buildables/FGBuildableRailroadTrack.h"
#include "Buildables/FGBuildableResourceExtractorBase.h"
#include "CoreMinimal.h"
#include "FGIconLibrary.h"
#include "FGInventoryComponent.h"
#include "FGPowerConnectionComponent.h"
#include "UObject/Object.h"

#include "KPCLBlueprintFunctionLib.generated.h"
/**
 *
 */
UCLASS()
class KPRIVATECODELIB_API UKPCLBlueprintFunctionLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Cpp
	static void SetAllowOnIndex_ThreadSafe(UFGInventoryComponent* Component, int32 Index,
										   TSubclassOf<UFGItemDescriptor> ItemClass);

	UFUNCTION(BlueprintCallable, Category = "KMods|BPFL")
	UPanelSlot* InjectWidgetAt(UPanelWidget* TargetPanel, UUserWidget* WidgetToInject, FMargin Padding, int32 Index,
							   bool bRequireTFITCheck);


	// BP
	UFUNCTION(BlueprintCallable, Category = "KMods|BPFL", meta = (DeterminesOutputType = "InClass"))
	static void UpdateIconLib(UFGIconLibrary* IconLib, TArray<TSubclassOf<UObject>> InObjects, bool bRemoveList);

	UFUNCTION(BlueprintCallable, Category = "KMods|BPFL", meta = (DeterminesOutputType = "InClass"))
	static UObject* GetDefaultSilent(TSubclassOf<UObject> InClass);

	UFUNCTION(BlueprintCallable, Category = "KMods|BPFL")
	static AActor* ResolveHitResult(const FHitResult& InHitResult);
	static AActor* ResolveOverlapResult(const FOverlapResult& InOverlapResult);

	UFUNCTION(BlueprintCallable, Category = "Widget | Advanced",
			  meta = (DefaultToSelf = "CurrentWidget", DeterminesOutputType = "WidgetClass"))
	static TArray<UWidget*> FindChildWidgetsOfClassDeep(UUserWidget* CurrentWidget, TSubclassOf<UWidget> WidgetClass);
};
