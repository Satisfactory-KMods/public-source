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
#include "TextureResource.h"
#include "Widget/RssSignWidget.h"

URssWidgetRenderComponent::URssWidgetRenderComponent()
{
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bCanEverTick = false;
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
	if (GetOwner() && bOwnerImplementsInterface)
	{
		bHasAnimatedEffect = HasEffect(IRssSignInterface::Execute_GetSignData(GetOwner()));
	}
	mRenderWasRequested = true;
	mSomeWasRendered = true;
}

void URssWidgetRenderComponent::SetDrawSize(FVector2D NewSize)
{
	if (!FMath::IsFinite(NewSize.X) || !FMath::IsFinite(NewSize.Y) || NewSize.X <= 0.0 || NewSize.Y <= 0.0)
	{
		UE_LOG(LogTemp, Warning, TEXT("RssWidgetRenderComponent: Rejected invalid draw size (%f, %f)"), NewSize.X,
			   NewSize.Y);
		return;
	}
	if (NewSize == mDrawSize && GetRenderTarget())
	{
		if (mRssWidget)
		{
			mRssWidget->SetNativeDrawSize(NewSize);
		}
		return;
	}
	mDrawSize = NewSize;
	if (mRssWidget)
	{
		mRssWidget->SetNativeDrawSize(mDrawSize);
	}
	if (GetRenderTarget())
	{
		mRenderTarget->ResizeTarget(NewSize.IntPoint().X, NewSize.IntPoint().Y);
	}
	RequestRedraw();
}
void URssWidgetRenderComponent::PoolTick(float DeltaTime)
{
	if (mInit && !bHasAnimatedEffect && !mRenderWasRequested)
	{
		return;
	}

	if (GetOwner() && !mRenderWasRequested)
	{
		if (bOwnerImplementsInterface)
		{
			bool bIsSignificant = IRssSignInterface::Execute_IsBuildingSignificance(GetOwner());
			if (!bIsSignificant)
			{

				return;
			}
		}
	}

	if (mRedrawTime.Tick(DeltaTime) || mRenderWasRequested)
	{

		if (!mInit)
		{
			mSomeWasRendered = true;
			mInit = true;

			DoRender(DeltaTime);
			mFramesSinceLastRender = 0;
			return;
		}

		if (!mRenderWasRequested)
		{
			mSomeWasRendered = IsInCameraFrustum();
		}
		else
		{

			mSomeWasRendered = true;
		}

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

	if (mRenderWasRequested)
	{
		mRenderWasRequested = false;
		mRenderAccumulator = 0.0f;
		return true;
	}

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

	return true;
}

void URssWidgetRenderComponent::ResetForPooling()
{

	for (FRssMeshInfo& MeshInfo : mMeshInfo)
	{
		MeshInfo.ResetMaterial(mEmptyMaterial);
	}
	mMeshInfo.Empty();

	if (mSlateWidget)
	{
		mSlateWidget.Reset();
	}

	mInit = false;
	mRenderWasRequested = false;
	mSomeWasRendered = false;
	bHasAnimatedEffect = false;
	mFramesSinceLastRender = 0;
	mRenderAccumulator = 0.0f;
	bOwnerImplementsInterface = false;
	mCachedDataManager.Reset();

	if (mRssWidget)
	{
		mRssWidget->SetBuildable(nullptr);
	}

	mRedrawTime.Reset();

	UE_LOG(LogTemp, Verbose, TEXT("RssWidgetRenderComponent: Reset for pooling (keeping render target + renderer)"));
}

void URssWidgetRenderComponent::DoRender(float dt)
{

	if (!mRssWidget || !mRenderTarget || !mSlateWidget.IsValid())
	{
		InitWidgetAndRenderTarget();
		InitMeshAndScreen();

		if (mRssWidget && !mSlateWidget.IsValid())
		{
			mSlateWidget = mRssWidget->TakeWidget();
		}

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

		FVector2D RenderSize = mDrawSize;

		if (RenderSize.X > 0 && RenderSize.Y > 0)
		{

			mRssWidget->SetNativeDrawSize(RenderSize);
			mRssWidget->SynchronizeNativeElements(false);

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

	mInit = false;
	mSomeWasRendered = true;
	mRenderAccumulator = 0.0f;
	mFramesSinceLastRender = 0;

	InitComponentData();

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

	const FRssSignData SignData = IRssSignInterface::Execute_GetSignData(GetOwner());
	bHasAnimatedEffect = HasEffect(SignData);
	if (mUseDrawSizeFromInterface)
	{
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
			mRssWidget->SetNativeDrawSize(mDrawSize);
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
		GetWidget()->SetNativeDrawSize(mDrawSize);
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
	if (mDrawSize.X > 0.0 && mDrawSize.Y > 0.0)
	{
		return mDrawSize;
	}
	if (GetOwner() && UKismetSystemLibrary::DoesImplementInterface(GetOwner(), URssSignInterface::StaticClass()))
	{
		return URssBlueprintFunctionLibrary::GetScreenSize(
			IRssSignInterface::Execute_GetSignData(GetOwner()).mSignTypeSize);
	}
	return FVector2D::ZeroVector;
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

	return hasElement && !URssBlueprintFunctionLibrary::ShouldUseArrowMaterial(SignData);
}
