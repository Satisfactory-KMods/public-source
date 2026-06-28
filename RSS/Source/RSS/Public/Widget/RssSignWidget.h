// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"
#include "Buildables/FGBuildable.h"

#include "EnumStruc/RssStruc.h"

#include "RssSignWidget.generated.h"

UCLASS(Blueprintable, BlueprintType)
class RSS_API URssSignWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Rss Sign Widget")
	void SetBuildable(AActor* Buildable);

	UFUNCTION(BlueprintCallable, Category = "Rss Sign Widget")
	void UpdateSignData(FRssSignData SignData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Rss Sign Widget - Events")
	void OnBuildableSet(AActor* Buildable);

	UFUNCTION(BlueprintImplementableEvent, Category = "Rss Sign Widget - Events")
	void OnSignDataUpdated(FRssSignData SignData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Rss Sign Widget - Events")
	void OnRequestUpdateSign();

	UPROPERTY(BlueprintReadWrite, Category = "Rss Sign Widget")
	TObjectPtr<AActor> mBuildable;

	UPROPERTY(BlueprintReadWrite, Category = "Rss Sign Widget")
	FRssSignData mSignWidgetData;

	UPROPERTY(BlueprintReadWrite, Category = "Rss Sign Widget")
	FRssUiData mUIData;
};
