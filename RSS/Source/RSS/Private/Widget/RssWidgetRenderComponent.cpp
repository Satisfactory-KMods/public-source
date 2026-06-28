// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/RssWidgetRenderComponent.h"

#include "BFL/KBFL_Player.h"
#include "Engine/TextureRenderTarget2D.h"
#include "FGColoredInstanceMeshProxy.h"
#include "GameFramework/PlayerController.h"
#include "Interface/RssSignInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RssBlueprintFunctionLibrary.h"
#include "Subsystem/RSSDataManagerSubsystem.h"
#include "Widget/RssSignWidget.h"

URssWidgetRenderComponent::URssWidgetRenderComponent()
{
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bCanEverTick = false; // Disabled - Subsystem handles ticking
	bAutoActivate = true;
}

void URssWidgetRenderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	WipeWidgetAndRenderTarget();
	Super::EndPlay(EndPlayReason);
}

void URssWidgetRenderComponent::BeginPlay()
{
	Super::BeginPlay();
	InitComponent();
}

void URssWidgetRenderComponent::RequestRedraw()
{
	mRenderWasRequested = true;
	mSomeWasRendered = true; // Force visibility check to pass
}

void URssWidgetRenderComponent::SetDrawSize(FVector2D NewSize)
{
	if (NewSize == mDrawSize && GetRenderTarget())
	{
		return;
	}
	mDrawSize = NewSize;
	if (GetRenderTarget())
	{
		mRenderTarget->ResizeTarget(NewSize.IntPoint().X, NewSize.IntPoint().Y);
	}
}
void URssWidgetRenderComponent::PoolTick(float DeltaTime)
{
	// Check if the owner is still significant - if not, we shouldn't render
	// But always allow explicitly requested redraws
	if (GetOwner() && !mRenderWasRequested)
	{
		if (bOwnerImplementsInterface)
		{
			bool bIsSignificant = IRssSignInterface::Execute_IsBuildingSignificance(GetOwner());
			if (!bIsSignificant)
			{
				// Not significant - don't render
				return;
			}
		}
	}

	if (mRedrawTime.Tick(DeltaTime) || mRenderWasRequested)
	{
		// First time initialization - always render at least once
		if (!mInit)
		{
			mSomeWasRendered = true; // Force render on first tick
			mInit = true;

			// Do initial render
			DoRender(DeltaTime);
			mFramesSinceLastRender = 0;
			return;
		}

		// Use async frustum check from subsystem for ALL signs
		// This is calculated in background thread with ParallelFor for maximum performance
		if (!mRenderWasRequested)
		{
			mSomeWasRendered = IsInCameraFrustum();
		}
		else
		{
			// Explicit redraw request - always render
			mSomeWasRendered = true;
		}

		// Check if we should render this frame (respects FPS limits)
		if (mSomeWasRendered && ShouldRenderThisFrame(DeltaTime))
		{
			DoRender(DeltaTime);
			mFramesSinceLastRender = 0;
		}
		else
		{
			mFramesSinceLastRender++;
		}
	}
}

bool URssWidgetRenderComponent::ShouldRenderThisFrame(float DeltaTime)
{
	// If a redraw was explicitly requested, render immediately
	if (mRenderWasRequested)
	{
		mRenderWasRequested = false;
		mRenderAccumulator = 0.0f;
		return true;
	}

	// Check render frequency
	float TargetFPS = mMaxRenderFPS;
	float TargetInterval = 1.0f / FMath::Max(TargetFPS, 1.0f);
	mRenderAccumulator += DeltaTime;

	if (mRenderAccumulator >= TargetInterval || mFramesSinceLastRender > 10)
	{
		mRenderAccumulator = FMath::Fmod(mRenderAccumulator, TargetInterval);
		return true;
	}

	return false;
}


void URssWidgetRenderComponent::SetMaxRenderFrequency(float MaxFPS)
{
	mMaxRenderFPS = FMath::Clamp(MaxFPS, 1.0f, 60.0f);
}

bool URssWidgetRenderComponent::IsInCameraFrustum() const
{
	// Ask subsystem if we're in frustum (calculated async in background thread).
	if (GetOwner() && GetWorld() && bShouldCheckInView)
	{
		if (!mCachedDataManager.IsValid())
		{
			mCachedDataManager = ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(GetWorld());
		}
		if (mCachedDataManager.IsValid())
		{
			return mCachedDataManager->IsComponentInFrustum(const_cast<URssWidgetRenderComponent*>(this));
		}
	}

	// Fallback: if subsystem not available, assume visible
	return true;
}

void URssWidgetRenderComponent::ResetForPooling()
{
	// reallocation on every significance cycle. Only the mesh binding and widget state are reset here.
	// WipeWidgetAndRenderTarget() is still called on genuine EndPlay teardown.

	// Reset materials on the old sign's mesh to the empty material so it no longer shows the render target.
	for (FRssMeshInfo& MeshInfo : mMeshInfo)
	{
		MeshInfo.ResetMaterial(mEmptyMaterial);
	}
	mMeshInfo.Empty();

	// Release the Slate widget shared ref so DoRender will re-acquire it from mRssWidget->TakeWidget() for the
	// new sign. The UMG URssSignWidget object itself is kept alive (mRssWidget stays non-null).
	if (mSlateWidget)
	{
		mSlateWidget.Reset();
	}

	// Reset all state variables
	mInit = false;
	mRenderWasRequested = false;
	mSomeWasRendered = false;
	mFramesSinceLastRender = 0;
	mRenderAccumulator = 0.0f;
	bOwnerImplementsInterface = false;
	mCachedDataManager.Reset();

	// Reset timers
	mRedrawTime.Reset();

	UE_LOG(LogTemp, Verbose, TEXT("RssWidgetRenderComponent: Reset for pooling (keeping render target + renderer)"));
}

void URssWidgetRenderComponent::DoRender(float dt)
{
	// Ensure we have all required components
	if (!mRssWidget || !mRenderTarget || !mSlateWidget.IsValid())
	{
		InitWidgetAndRenderTarget();
		InitMeshAndScreen();

		if (mRssWidget && !mSlateWidget.IsValid())
		{
			mSlateWidget = mRssWidget->TakeWidget();
		}

		// Still missing components - skip this frame
		if (!mRssWidget || !mRenderTarget || !mSlateWidget.IsValid())
		{
			UE_LOG(LogTemp, Warning,
				   TEXT("RssWidgetRenderComponent: Missing components - Widget: %d, RenderTarget: %d, SlateWidget: %d"),
				   mRssWidget != nullptr, mRenderTarget != nullptr, mSlateWidget.IsValid());
			return;
		}
	}

	if (FSlateApplication::IsInitialized() && mRssWidget && mWidgetRenderer && mRenderTarget && mSlateWidget.IsValid())
	{
		// Use full draw size
		FVector2D RenderSize = mDrawSize;

		// Ensure render size is valid
		if (RenderSize.X > 0 && RenderSize.Y > 0)
		{
			// during InitWidgetAndRenderTarget/InitMeshAndScreen and does not need to be rebound every frame.
			mWidgetRenderer->DrawWidget(mRenderTarget, mSlateWidget.ToSharedRef(), RenderSize, dt, false);
		}
	}
}

void URssWidgetRenderComponent::InitComponent()
{
	if (IsValid(Cast<ARssDataManagerSubsystem>(GetOwner())))
	{
		return;
	}

	// Reset state
	mInit = false;
	mSomeWasRendered = true; // Assume visible until proven otherwise
	mRenderAccumulator = 0.0f;
	mFramesSinceLastRender = 0;

	InitComponentData();

	// Force an initial render
	if (mRssWidget && mRenderTarget)
	{
		DoRender(0.0f);
	}

	URssBlueprintFunctionLibrary::UpdateSignGeneralFunction(GetOwner());
}
void URssWidgetRenderComponent::InitComponentData()
{
	if (!GetOwner())
	{
		UE_LOG(LogTemp, Warning, TEXT("RssWidgetRenderComponent: No owner!"));
		return;
	}

	bOwnerImplementsInterface =
		UKismetSystemLibrary::DoesImplementInterface(GetOwner(), URssSignInterface::StaticClass());
	if (!bOwnerImplementsInterface)
	{
		UE_LOG(LogTemp, Warning, TEXT("RssWidgetRenderComponent: Owner does not implement IRssSignInterface!"));
		return;
	}

	if (mUseDrawSizeFromInterface)
	{
		FRssSignData SignData = IRssSignInterface::Execute_GetSignData(GetOwner());
		FVector2D NewSize = URssBlueprintFunctionLibrary::GetScreenSize(SignData.mSignTypeSize);
		if (NewSize.X > 0 && NewSize.Y > 0)
		{
			SetDrawSize(NewSize);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("RssWidgetRenderComponent: Invalid draw size from interface: %s"),
				   *NewSize.ToString());
		}
	}

	TArray<struct FRssMeshInfo> NewMeshInfo = IRssSignInterface::Execute_GetScreenMeshInfos(GetOwner());
	if (NewMeshInfo.Num() > 0)
	{
		mMeshInfo = NewMeshInfo;
		for (FRssMeshInfo& MeshInfo : mMeshInfo)
		{
			MeshInfo.TryToMakeDynMaterial();
		}

		// Initialize widget and render target
		InitWidgetAndRenderTarget();
		InitMeshAndScreen();
		RequestUpdateComponent();

		UE_LOG(LogTemp, Verbose, TEXT("RssWidgetRenderComponent: Initialized component for %s"),
			   *GetOwner()->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("RssWidgetRenderComponent: No mesh info from interface!"));
	}
}

void URssWidgetRenderComponent::InitWidgetAndRenderTarget()
{
	if (mRssWidgetClass && GetOwner())
	{
		FRssSignData SignData = IRssSignInterface::Execute_GetSignData(GetOwner());

		if (!mRssWidget)
		{
			mRssWidget = CreateWidget<URssSignWidget>(GetWorld(), mRssWidgetClass);
		}

		if (mRssWidget)
		{
			mRssWidget->SetBuildable(GetOwner());
			RequestUpdateWidget();
		}

		if (mRenderTarget == nullptr)
		{
			mRenderTarget = NewObject<UTextureRenderTarget2D>(this);
			mRenderTarget->ClearColor = FColor(0);

			if (mDrawSize.X <= 0 || mDrawSize.Y <= 0)
			{
				SetDrawSize(URssBlueprintFunctionLibrary::GetScreenSize(SignData.mSignTypeSize));
			}
			// Ensure
			if (mDrawSize.X <= 0 || mDrawSize.Y <= 0)
			{
				return;
			}

			mRenderTarget->InitCustomFormat(mDrawSize.IntPoint().X, mDrawSize.IntPoint().Y,
											FSlateApplication::Get().GetRenderer()->GetSlateRecommendedColorFormat(),
											false);

			if (mMeshInfo.Num() > 0)
			{
				for (FRssMeshInfo& MeshInfo : mMeshInfo)
				{
					MeshInfo.TryToMakeDynMaterial();

					if (MeshInfo.mDynamicMaterial)
					{
						MeshInfo.mDynamicMaterial->SetTextureParameterValue("SlateUI", GetRenderTarget());
					}
				}
				RequestUpdateComponent();
			}
		}

		if (!mWidgetRenderer)
		{
			mWidgetRenderer = new FWidgetRenderer(mUseGammaCorecture);
		}
	}
}

void URssWidgetRenderComponent::WipeWidgetAndRenderTarget()
{
	if (mMeshInfo.Num() > 0)
	{
		for (FRssMeshInfo& MeshInfo : mMeshInfo)
		{
			MeshInfo.ResetMaterial(mEmptyMaterial);
		}
	}

	if (mRenderTarget)
	{
		mRenderTarget->RemoveFromRoot();
		mRenderTarget->MarkAsGarbage();

		// Guard against null resource (can be null if RT was never fully initialized).
		if (FTextureRenderTargetResource* RTResource = mRenderTarget->GameThread_GetRenderTargetResource())
		{
			BeginReleaseResource(RTResource->GetTextureRenderTarget2DResource());
			BeginReleaseResource(RTResource);
		}
		BeginReleaseResource(mRenderTarget->GetResource());

		MarkRenderStateDirty();
		mRenderTarget = nullptr;
	}

	if (mSlateWidget)
	{
		mSlateWidget.Reset();
	}

	if (mRssWidget)
	{
		mRssWidget->ReleaseSlateResources(true);
		mRssWidget->RemoveFromRoot();
		mRssWidget->MarkAsGarbage();
		mRssWidget->ConditionalBeginDestroy();
		mRssWidget = nullptr;
	}

	if (mWidgetRenderer)
	{
		BeginCleanup(mWidgetRenderer);
		mWidgetRenderer = nullptr;
	}
}

void URssWidgetRenderComponent::InitMeshAndScreen()
{
	if (!mRenderTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("RssWidgetRenderComponent::InitMeshAndScreen - No render target!"));
		return;
	}

	if (mMeshInfo.Num() == 0)
	{
		// Try to get mesh info from interface
		if (GetOwner() && UKismetSystemLibrary::DoesImplementInterface(GetOwner(), URssSignInterface::StaticClass()))
		{
			mMeshInfo = IRssSignInterface::Execute_GetScreenMeshInfos(GetOwner());
		}
	}

	if (mMeshInfo.Num() > 0)
	{
		for (FRssMeshInfo& MeshInfo : mMeshInfo)
		{
			if (!MeshInfo.mMeshComponent)
			{
				continue;
			}

			// Ensure dynamic material is created
			if (!MeshInfo.mDynamicMaterial)
			{
				MeshInfo.TryToMakeDynMaterial();
			}

			if (MeshInfo.mDynamicMaterial)
			{
				MeshInfo.mDynamicMaterial->SetTextureParameterValue("SlateUI", mRenderTarget);
			}
			else
			{
				UE_LOG(
					LogTemp, Warning,
					TEXT("RssWidgetRenderComponent::InitMeshAndScreen - Failed to create dynamic material for mesh!"));
			}
		}
	}
}

void URssWidgetRenderComponent::RequestUpdateWidget()
{
	if (GetWidget())
	{
		GetWidget()->SetBuildable(GetOwner());
		RequestRedraw();
	}
}

void URssWidgetRenderComponent::RequestUpdateComponent()
{
	if (GetOwner())
	{
		InitWidgetAndRenderTarget();
		InitMeshAndScreen();
		RequestUpdateWidget();
	}
}

FVector2D URssWidgetRenderComponent::GetDrawSize() const
{
	return URssBlueprintFunctionLibrary::GetScreenSize(
		IRssSignInterface::Execute_GetSignData(GetOwner()).mSignTypeSize);
}

bool URssWidgetRenderComponent::HasEffect(FRssSignData SignData)
{
	bool hasElement = false;
	for (auto Element : SignData.mElements)
	{
		if (Element.IsTypeOf(ESignElementType::Effect))
		{
			hasElement = true;
			break;
		}
	}
	return hasElement && !SignData.mFlatData.mUseArrowMaterial;
}
