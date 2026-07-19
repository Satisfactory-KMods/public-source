// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildable.h"
#include "Components/WidgetComponent.h"
#include "Slate/WidgetRenderer.h"

#include "EnumStruc/RssStruc.h"
#include "Interface/RssSignInterface.h"
#include "RssSignWidget.h"
#include "Structures/KPCLFunctionalStructure.h"

#include "RssWidgetRenderComponent.generated.h"

class ARssDataManagerSubsystem;

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RSS_API URssWidgetRenderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URssWidgetRenderComponent();

	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent Interface

	UFUNCTION(BlueprintCallable, Category = "Rss Widget Renderer")
	void RequestRedraw();

	UFUNCTION(BlueprintCallable, Category = "Rss Widget Renderer")
	void SetDrawSize(FVector2D NewSize);

	virtual void PoolTick(float DeltaTime);
	void DoRender(float dt);

	void InitComponent();
	void InitComponentData();

	UFUNCTION(BlueprintCallable, Category = "Rss Widget Renderer")
	void InitWidgetAndRenderTarget();

	UFUNCTION(BlueprintCallable, Category = "Rss Widget Renderer")
	void WipeWidgetAndRenderTarget();

	UFUNCTION(BlueprintCallable, Category = "Rss Widget Renderer")
	void RequestUpdateWidget();

	UFUNCTION(BlueprintCallable, Category = "Rss Widget Renderer")
	void RequestUpdateComponent();

	UFUNCTION(BlueprintPure, Category = "Rss Widget Renderer")
	FORCEINLINE UTextureRenderTarget2D* GetRenderTarget() const { return mRenderTarget; };

	UFUNCTION(BlueprintPure, Category = "Rss Widget Renderer")
	FORCEINLINE URssSignWidget* GetWidget() const { return mRssWidget; };

	UFUNCTION(BlueprintPure, Category = "Rss Widget Renderer")
	FVector2D GetDrawSize() const;

	UFUNCTION(BlueprintPure, Category = "Rss Widget Render Component")
	static bool HasEffect(FRssSignData SignData);

	UFUNCTION(BlueprintCallable, Category = "Rss Widget Render Component")
	void InitMeshAndScreen();

	UFUNCTION(BlueprintCallable, Category = "Rss Widget Renderer|Performance")
	void SetMaxRenderFrequency(float MaxFPS);

	UFUNCTION(BlueprintCallable, Category = "Rss Widget Renderer|Pooling")
	void ResetForPooling();

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Rss Widget Render Component")
	TArray<FRssMeshInfo> mMeshInfo;

	UPROPERTY()
	ETickMode mTickMode = ETickMode::Disabled;

	UPROPERTY()
	TObjectPtr<URssSignWidget> mRssWidget;
	TSharedPtr<SWidget> mSlateWidget;

	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<UTextureRenderTarget2D> mRenderTarget;

	FWidgetRenderer* mWidgetRenderer;

	UPROPERTY(EditDefaultsOnly, Category = "Rss Widget Renderer")
	FVector2D mDrawSize;

	UPROPERTY(EditDefaultsOnly, Category = "Rss Widget Renderer")
	bool mUseDrawSizeFromInterface = true;

	UPROPERTY(EditDefaultsOnly, Category = "Rss Widget Renderer")
	bool mUseGammaCorecture = false;

	UPROPERTY(EditDefaultsOnly, Category = "Rss Widget Renderer")
	TSubclassOf<URssSignWidget> mRssWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Rss Widget Renderer")
	FSmartTimer mRedrawTime = FSmartTimer(0.0333333f);

	UPROPERTY(EditDefaultsOnly, Category = "Rss Widget Renderer")
	float mTickInterval = 0.02f;

	UPROPERTY(EditDefaultsOnly, Category = "Rss Widget Renderer|Performance")
	float mMaxRenderFPS = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Rss Widget Renderer")
	bool bShouldCheckInView = true;

	UPROPERTY(EditDefaultsOnly, Category = "Rss Widget Render Component")
	TObjectPtr<UMaterialInterface> mEmptyMaterial;

	bool mRenderWasRequested = false;
	bool mInit = false;
	bool mSomeWasRendered = false;
	bool bHasAnimatedEffect = false;

	/** Cached result of DoesImplementInterface(URssSignInterface) for the current owner. Set once in
	 * InitComponentData. */
	bool bOwnerImplementsInterface = false;

protected:
	bool ShouldRenderThisFrame(float DeltaTime);
	bool IsInCameraFrustum() const;

	float mRenderAccumulator = 0.0f;
	int32 mFramesSinceLastRender = 0;

	/** Cached world-stable pointer to ARssDataManagerSubsystem; lazily resolved. mutable so it can be filled from
	 * const IsInCameraFrustum. */
	mutable TWeakObjectPtr<ARssDataManagerSubsystem> mCachedDataManager;
};
