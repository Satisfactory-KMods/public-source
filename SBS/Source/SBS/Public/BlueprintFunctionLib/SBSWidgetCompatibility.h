// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SBSWidgetCompatibility.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnWidgetCreated, UUserWidget*, Widget);

UCLASS()
class SBS_API USBSWidgetCompatibility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KMods|WidgetUtils")
	static void BindOnWidget(const TSubclassOf<UUserWidget> WidgetClass, FOnWidgetCreated Binding);

	UFUNCTION(BlueprintCallable, Category = "KMods|WidgetUtils")
	static void BindOnPreWidget(const TSubclassOf<UUserWidget> WidgetClass, FOnWidgetCreated Binding);

	static void ShutdownHooks();
};
