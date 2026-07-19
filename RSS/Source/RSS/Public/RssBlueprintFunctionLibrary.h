// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGSignInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Subsystem/RSSImageSubsystem.h"
#include "Subsystem/RSSTemplateSubsystem.h"
#include "Widget/RssWidgetRenderComponent.h"

#include "RssBlueprintFunctionLibrary.generated.h"

UCLASS()
class RSS_API URssBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "RSS BPL|Color")
	static void HexToLinearColor(const FString& InString, FLinearColor& OutConvertedColor, bool& OutIsValid);

	UFUNCTION(BlueprintCallable, Category = "RSS BPL")
	static void ValidateCustomData(AActor* Building);

	UFUNCTION(BlueprintCallable, Category = "RSS BPL")
	static void RequestUpgradeSignToCustom(AActor* Building, FRssSignRequestData Request);

	UFUNCTION(BlueprintCallable, Category = "RSS BPL")
	static void UpdateSignGeneralFunction(AActor* Building);

	UFUNCTION(BlueprintCallable, Category = "RSS BPL")
	static void SignActorGetSignificant(AActor* SignActor, bool bCalledFromSubsystem = false);

	UFUNCTION(BlueprintCallable, Category = "RSS BPL")
	static void SignActorLostSignificant(AActor* SignActor);

	static bool CreateSignComponent(AActor* SignActor);
	static bool DestorySignComponent(AActor* SignActor);

	UFUNCTION(BlueprintCallable, Category = "RSS BPL")
	static URssWidgetRenderComponent* GetSignComponentFromSignActor(AActor* SignActor);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "RSS BPL")
	static ARSSImageSubsystem* GetImageSubsystem(UObject* WorldContext)
	{
		return UKBFL_Util::GetSubsystem<ARSSImageSubsystem>(WorldContext);
	};

	UFUNCTION(BlueprintPure, Category = "RSS BPL")
	static void GetParalaxRadio(AActor* SignBuilding, float& VerticalRadio, float& HorizontalRadio);

	UFUNCTION(BlueprintPure, Category = "RSS BPL")
	static void GetArrowRadio(ESignSize SignSize, float& VerticalRadio, float& HorizontalRadio);

	UFUNCTION(BlueprintPure, Category = "RSS BPL")
	static void GetParalaxRadioFromCustomData(FRssSignData SignData, float& VerticalRadio, float& HorizontalRadio);

	UFUNCTION(BlueprintPure, Category = "RSS BPL")
	static FVector2D GetScreenSize(ESignSize SignSize);

	static bool IsSignDataSafe(const FRssSignData& SignData);
};
