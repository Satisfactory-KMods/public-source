// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"
#include "Buildables/FGBuildable.h"

#include "EnumStruc/RssStruc.h"

#include "RssSignWidget.generated.h"

class UCanvasPanel;
class UMaterialInstanceDynamic;
class UWidget;

UCLASS(Blueprintable, BlueprintType)
class RSS_API URssSignWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Rss Sign Widget|Native Renderer",
			  meta = (WorldContext = "WorldContextObject"))
	static URssSignWidget* CreateNativeSignWidget(UObject* WorldContextObject, TSubclassOf<URssSignWidget> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "Rss Sign Widget")
	void SetBuildable(AActor* Buildable);

	UFUNCTION(BlueprintCallable, Category = "Rss Sign Widget")
	void UpdateSignData(FRssSignData SignData);

	UFUNCTION(BlueprintCallable, Category = "Rss Sign Widget|Native Renderer")
	void SynchronizeNativeElements(bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "Rss Sign Widget|Native Renderer")
	void UpdateNativeElementByIndex(int32 Index, const FRssElement& Element);

	UFUNCTION(BlueprintCallable, Category = "Rss Sign Widget|Native Renderer")
	void InvalidateNativeElements();

	UFUNCTION(BlueprintCallable, Category = "Rss Sign Widget|Native Renderer")
	void SetNativeDrawSize(FVector2D NewDrawSize);

	UFUNCTION(BlueprintPure, Category = "Rss Sign Widget|Native Renderer")
	int32 GetNativeElementCount() const { return mNativeRenderElements.Num(); }

	UFUNCTION(BlueprintPure, Category = "Rss Sign Widget|Native Renderer")
	UWidget* GetNativeElementWidget(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Rss Sign Widget|Native Renderer")
	UCanvasPanel* GetNativeElementCanvas() const;

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

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UCanvasPanel* ResolveElementCanvas() const;
	FVector2D ResolveDrawSize(UCanvasPanel* ElementCanvas) const;
	UWidget* CreateNativeImageElement(const FRssElement& Element, const FVector2D& DrawSize, bool& bOutFillSign);
	UWidget* CreateNativeTextElement(const FRssElement& Element);
	UWidget* CreateNativeEffectElement(const FRssElement& Element, const FVector2D& DrawSize);
	FVector2D ResolveImageSize(const FRssElement& Element, const FVector2D& DrawSize, bool& bOutFillSign) const;
	FName ResolveTypefaceFontName(const FRssElementTextData& TextData) const;
	bool AreNativeElementsAttached(const UCanvasPanel* ElementCanvas, const FVector2D& DrawSize) const;
	bool IsLegacyBlueprintCacheDirty() const;
	void SetLegacyBlueprintCacheDirty(bool bIsDirty);
	uint32 ComputeNativeRenderHash(const FVector2D& DrawSize) const;
	void ClearNativeElements();
	void UpdateBlueprintElementCache();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> mNativeRenderElements;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> mNativeEffectMaterials;

	uint32 mNativeRenderHash = 0;
	FVector2D mNativeDrawSizeOverride = FVector2D::ZeroVector;
	bool bNativeRenderInitialized = false;
	bool bNativeRenderDirty = true;
};
