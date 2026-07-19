#include "RssBlueprintFunctionLibrary.h"

#include "Algo/AllOf.h"
#include "Buildables/FGBuildable.h"
#include "Cache/RssImageCache.h"
#include "Components/WidgetComponent.h"
#include "FGBoundedTextRenderComponent.h"
#include "FGSignificanceInterface.h"
#include "Interface/RssSignInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystem/RSSDataManagerSubsystem.h"
#include "Widget/RssSignWidget.h"
#include "Widget/RssWidgetRenderComponent.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

void URssBlueprintFunctionLibrary::HexToLinearColor(const FString& InString, FLinearColor& OutConvertedColor,
													bool& OutIsValid)
{
	FString Hex = InString.TrimStartAndEnd();
	Hex.RemoveFromStart(TEXT("#"));
	OutIsValid = (Hex.Len() == 6 || Hex.Len() == 8) &&
		Algo::AllOf(Hex, [](TCHAR Character) { return FChar::IsHexDigit(Character); });
	OutConvertedColor = OutIsValid ? FLinearColor(FColor::FromHex(Hex)) : FLinearColor::Black;
}

void URssBlueprintFunctionLibrary::ValidateCustomData(AActor* Building)
{
	if (!IsValid(Building) || !UKismetSystemLibrary::DoesImplementInterface(Building, URssSignInterface::StaticClass()))
	{
		return;
	}

	FRssSignRequestData Request;
	Request.mBuildable = Building;
	FRssSignData ModifyData = IRssSignInterface::Execute_GetSignData(Building);

	ARSSImageSubsystem* Subsystem = GetImageSubsystem(Building);
	URssImageCacheManager* CacheManager = Subsystem ? Subsystem->GetCacheManager() : nullptr;

	if (Subsystem)
	{
		for (int i = 0; i < ModifyData.mElements.Num(); ++i)
		{
			if (ModifyData.mElements.IsValidIndex(i))
			{
				FRssElement Data = ModifyData.mElements[i];

				if (Data.mSharedData.mIsUsingCustom)
				{
					if (ARSSImageSubsystem::IsUrlValid(Data.mSharedData.mUrl))
					{
						// Try cache first for faster loading
						UTexture2DDynamic* CachedTexture = nullptr;
						bool bFoundInCache =
							CacheManager && CacheManager->GetCachedImage(Data.mSharedData.mUrl, CachedTexture);

						if (bFoundInCache && CachedTexture)
						{
							// Already in cache - use it directly
							if (CachedTexture != ModifyData.mElements[i].mSharedData.mCustomTexture)
							{
								ModifyData.mElements[i].mSharedData.mCustomTexture = CachedTexture;
								CacheManager->AddReference(Data.mSharedData.mUrl);
								IRssSignInterface::Execute_UpdateSignData(Building, ModifyData);
							}
						}
						else
						{
							// Not in cache - request from subsystem
							FRssCustomSignUrlData CData = Subsystem->GetDataFromUrl(Data.mSharedData.mUrl);

							if (CData.mTexture != ModifyData.mElements[i].mSharedData.mCustomTexture ||
								(CData.mTexture == ModifyData.mElements[i].mSharedData.mCustomTexture &&
								 CData.mTexture == nullptr))
							{
								Request.mRequestOnImageIndex = i;
								Request.mRequestUrl = Data.mSharedData.mUrl;

								UE_LOG(LogTemp, Warning, TEXT("ValidateCustom > SendRequest to Subsystem, %ls"),
									   *Request.mRequestUrl);
								Subsystem->onRequestImages(Request);
							}
						}
					}
					else
					{
						ModifyData.mElements[i].mSharedData.mIsUsingCustom = false;
						IRssSignInterface::Execute_UpdateSignData(Building, ModifyData);
					}
					UE_LOG(LogTemp, Warning, TEXT("Sign Is using Custom : %s"), *Data.mSharedData.mUrl);
				}
			}
		}
	}
}

void URssBlueprintFunctionLibrary::RequestUpgradeSignToCustom(AActor* Building, FRssSignRequestData Request)
{
	if (!IsValid(Building) || !UKismetSystemLibrary::DoesImplementInterface(Building, URssSignInterface::StaticClass()))
	{
		return;
	}

	FRssSignData ModifyData = IRssSignInterface::Execute_GetSignData(Building);
	if (Request.mWasSuccess && Request.mSuccessTexture && !Request.mOnlyAddToUI)
	{
		if (Request.mRequestOnImageIndex > -1)
		{
			if (ModifyData.mElements.IsValidIndex(Request.mRequestOnImageIndex))
			{
				ModifyData.mElements[Request.mRequestOnImageIndex].mSharedData.mCustomTexture = Request.mSuccessTexture;
				ModifyData.mElements[Request.mRequestOnImageIndex].mSharedData.mUrl = Request.mRequestUrl;
				ModifyData.mElements[Request.mRequestOnImageIndex].mSharedData.mIsUsingCustom = true;

				ValidateCustomData(Building);
				IRssSignInterface::Execute_ApplySignData(Building, ModifyData);
			}
		}
	}
	else if (!Request.mOnlyAddToUI)
	{
		if (Request.mRequestOnImageIndex > -1)
		{
			if (ModifyData.mElements.IsValidIndex(Request.mRequestOnImageIndex))
			{
				ModifyData.mElements[Request.mRequestOnImageIndex].mSharedData.mIsUsingCustom = false;
				IRssSignInterface::Execute_ApplySignData(Building, ModifyData);
			}
		}
	}
}

void URssBlueprintFunctionLibrary::UpdateSignGeneralFunction(AActor* Building)
{
	if (IsValid(Building))
	{
		if (UKismetSystemLibrary::DoesImplementInterface(Building, URssSignInterface::StaticClass()))
		{
			const FRssSignData SignData = IRssSignInterface::Execute_GetSignData(Building);
			URssWidgetRenderComponent* RenderComponent =
				Cast<URssWidgetRenderComponent>(IRssSignInterface::Execute_GetRenderComponent(Building));
			if (RenderComponent)
			{
				if (URssSignWidget* SignWidget = Cast<URssSignWidget>(RenderComponent->GetWidget()))
				{
					SignWidget->UpdateSignData(SignData);
				}

				// Request a redraw after updating sign data
				RenderComponent->RequestRedraw();

				TArray<struct FRssMeshInfo> MeshInfos = IRssSignInterface::Execute_GetRenderMeshInfos(Building);
				for (auto MeshInfo : MeshInfos)
				{
					MeshInfo.TryToMakeDynMaterial();
					if (UMaterialInstanceDynamic* DynamicMaterial = MeshInfo.mDynamicMaterial)
					{
						DynamicMaterial->SetVectorParameterValue("background color",
																 SignData.mMaterialData.mBackgroundColor);
						DynamicMaterial->SetScalarParameterValue("Emissive intensity",
																 SignData.mMaterialData.mEmissiveIntensity);

						if (SignData.mFlatData.mEnable)
						{
							DynamicMaterial->SetScalarParameterValue("Use parallax",
																	 SignData.mFlatData.mUseParallax ? 1 : 0);
							DynamicMaterial->SetScalarParameterValue("UseArrows",
																	 SignData.mFlatData.mUseArrowMaterial ? 1 : 0);
						}

						if (SignData.mHologramData.mEnable)
						{
							DynamicMaterial->SetScalarParameterValue("Distortion Intensity",
																	 SignData.mHologramData.mDistortionIntensity);
							DynamicMaterial->SetScalarParameterValue("use scanlines",
																	 SignData.mHologramData.mUseScanlines);
							DynamicMaterial->SetScalarParameterValue("Scanline Intensity",
																	 SignData.mHologramData.mScanlineIntensity);
							DynamicMaterial->SetScalarParameterValue("Border_mask_intensity",
																	 SignData.mHologramData.mBorderIntensity);
						}

						float VerticalRadio;
						float HorizontalRadio;
						GetParalaxRadio(Building, VerticalRadio, HorizontalRadio);
						DynamicMaterial->SetScalarParameterValue("Vertical_ratio", VerticalRadio);
						DynamicMaterial->SetScalarParameterValue("Horizontal_ratio", HorizontalRadio);
						DynamicMaterial->SetVectorParameterValue(
							"Animation",
							FLinearColor(SignData.mMaterialData.mRotation.X, SignData.mMaterialData.mRotation.Y,
										 SignData.mMaterialData.mRotation.Z, SignData.mMaterialData.mRotation.W));
					}
				}
			}
		}
	}
}

bool URssBlueprintFunctionLibrary::CreateSignComponent(AActor* SignActor)
{
	if (!UKismetSystemLibrary::DoesImplementInterface(SignActor, URssSignInterface::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSignComponent: %s (class %s) has no RSS Interface implemented!"),
			   *GetNameSafe(SignActor), SignActor ? *SignActor->GetClass()->GetName() : TEXT("null"));
		return false;
	}

	ARssDataManagerSubsystem* Subsystem = ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(SignActor);
	if (Subsystem)
	{
		Subsystem->AcquirePooledComponent(SignActor,
										  IRssSignInterface::Execute_GetWidgetRenderComponentClass(SignActor));
	}
	return true;
}

void URssBlueprintFunctionLibrary::SignActorGetSignificant(AActor* SignActor, bool bCalledFromSubsystem)
{
	if (UKismetSystemLibrary::DoesImplementInterface(SignActor, URssSignInterface::StaticClass()))
	{
		// With pooling system, always create component directly
		CreateSignComponent(SignActor);

		// Request immediate redraw when sign becomes significant again
		if (URssWidgetRenderComponent* RenderComponent = GetSignComponentFromSignActor(SignActor))
		{
			RenderComponent->RequestRedraw();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SignActorGetSignificant: %s (class %s) has no RSS Interface implemented!"),
			   *GetNameSafe(SignActor), SignActor ? *SignActor->GetClass()->GetName() : TEXT("null"));
	}
}

void URssBlueprintFunctionLibrary::SignActorLostSignificant(AActor* SignActor)
{
	if (UKismetSystemLibrary::DoesImplementInterface(SignActor, URssSignInterface::StaticClass()))
	{
		// Return components to pool
		DestorySignComponent(SignActor);

		// Remove from pending queue if actor was waiting
		ARssDataManagerSubsystem* Subsystem = ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(SignActor);
		if (Subsystem)
		{
			Subsystem->RemoveFromPendingQueue(SignActor);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SignActorLostSignificant: %s (class %s) has no RSS Interface implemented!"),
			   *GetNameSafe(SignActor), SignActor ? *SignActor->GetClass()->GetName() : TEXT("null"));
	}
}

bool URssBlueprintFunctionLibrary::DestorySignComponent(AActor* SignActor)
{
	if (!UKismetSystemLibrary::DoesImplementInterface(SignActor, URssSignInterface::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("DestorySignComponent: %s (class %s) has no RSS Interface implemented!"),
			   *GetNameSafe(SignActor), SignActor ? *SignActor->GetClass()->GetName() : TEXT("null"));
		return false;
	}

	TArray<URssWidgetRenderComponent*> Components;
	SignActor->GetComponents<URssWidgetRenderComponent>(Components);

	// Try to return components to pool if subsystem is available
	ARssDataManagerSubsystem* Subsystem = ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(SignActor);

	for (URssWidgetRenderComponent* Component : Components)
	{
		if (Component)
		{
			if (Subsystem)
			{
				// Return to pool
				Subsystem->ReturnPooledComponent(Component);
			}
			else
			{
				// Fallback to direct destruction
				Component->DestroyComponent();
			}
		}
	}
	return true;
}

URssWidgetRenderComponent* URssBlueprintFunctionLibrary::GetSignComponentFromSignActor(AActor* SignActor)
{
	if (ensureAlways(SignActor))
	{
		return SignActor->FindComponentByClass<URssWidgetRenderComponent>();
	}
	return nullptr;
}

void URssBlueprintFunctionLibrary::GetParalaxRadio(AActor* SignBuilding, float& VerticalRadio, float& HorizontalRadio)
{
	if (SignBuilding)
	{
		if (UKismetSystemLibrary::DoesImplementInterface(SignBuilding, URssSignInterface::StaticClass()))
		{
			FRssSignData SignData = IRssSignInterface::Execute_GetSignData(SignBuilding);

			if (SignData.mFlatData.mOverwriteParalaxHorizontalRatio > 0.0f ||
				SignData.mFlatData.mOverwriteParalaxVerticalRatio > 0.0f)
			{
				VerticalRadio = SignData.mFlatData.mOverwriteParalaxVerticalRatio;
				HorizontalRadio = SignData.mFlatData.mOverwriteParalaxHorizontalRatio;
				return;
			}

			if (!SignData.mFlatData.mUseArrowMaterial)
			{
				FVector2D Size = GetScreenSize(SignData.mSignTypeSize);
				VerticalRadio = Size.Y / 1000 * 3;
				HorizontalRadio = Size.X / 1000 * 3;
				return;
			}

			GetArrowRadio(SignData.mSignTypeSize, VerticalRadio, HorizontalRadio);
			return;
		}
	}

	VerticalRadio = 0.0f;
	HorizontalRadio = 0.0f;
}

void URssBlueprintFunctionLibrary::GetArrowRadio(ESignSize SignSize, float& VerticalRadio, float& HorizontalRadio)
{
	switch (SignSize)
	{
	case ESignSize::RSS_05x05:
		HorizontalRadio = .5;
		VerticalRadio = .5;
		return;
	case ESignSize::RSS_1x1:
		HorizontalRadio = 1;
		VerticalRadio = 1;
		return;
	case ESignSize::RSS_1x2:
		HorizontalRadio = 1;
		VerticalRadio = 2;
		return;
	case ESignSize::RSS_1x7:
		HorizontalRadio = 1;
		VerticalRadio = 7;
		return;
	case ESignSize::RSS_2x05:
		HorizontalRadio = 2;
		VerticalRadio = .5;
		return;
	case ESignSize::RSS_2x1:
		HorizontalRadio = 2;
		VerticalRadio = 1;
		return;
	case ESignSize::RSS_2x2:
		HorizontalRadio = 2;
		VerticalRadio = 2;
		return;
	case ESignSize::RSS_2x3:
		HorizontalRadio = 2;
		VerticalRadio = 3;
		return;
	case ESignSize::RSS_3x05:
		HorizontalRadio = 3;
		VerticalRadio = .5;
		return;
	case ESignSize::RSS_3x1:
		HorizontalRadio = 3;
		VerticalRadio = 1;
		return;
	case ESignSize::RSS_4x05:
		HorizontalRadio = 4;
		VerticalRadio = .5;
		return;
	case ESignSize::RSS_4x1:
		HorizontalRadio = 4;
		VerticalRadio = 1;
		return;
	case ESignSize::RSS_7x1:
		HorizontalRadio = 7;
		VerticalRadio = 1;
		return;
	case ESignSize::RSS_8x4:
		HorizontalRadio = 8;
		VerticalRadio = 4;
		return;
	case ESignSize::RSS_16x8:
		HorizontalRadio = 16;
		VerticalRadio = 8;
		return;
	case ESignSize::RSS_25x2:
		HorizontalRadio = 25;
		VerticalRadio = 2;
		return;
	case ESignSize::RSS_Custom:
		return;
	case ESignSize::RSS_Secret:
		return;
	default:
		return;
	}
}

void URssBlueprintFunctionLibrary::GetParalaxRadioFromCustomData(FRssSignData SignData, float& VerticalRadio,
																 float& HorizontalRadio)
{
	if (SignData.mFlatData.mOverwriteParalaxHorizontalRatio > 0.0f ||
		SignData.mFlatData.mOverwriteParalaxVerticalRatio > 0.0f)
	{
		VerticalRadio = SignData.mFlatData.mOverwriteParalaxVerticalRatio;
		HorizontalRadio = SignData.mFlatData.mOverwriteParalaxHorizontalRatio;
		return;
	}

	if (!SignData.mFlatData.mUseArrowMaterial)
	{
		FVector2D Size = GetScreenSize(SignData.mSignTypeSize);
		VerticalRadio = Size.Y / 1000 * 3;
		HorizontalRadio = Size.X / 1000 * 3;
		return;
	}

	FVector2D Size = GetScreenSize(SignData.mSignTypeSize);
	VerticalRadio = 1.0f;
	HorizontalRadio = Size.X / 100 / 2.5;
}

FVector2D URssBlueprintFunctionLibrary::GetScreenSize(ESignSize SignSize)
{
	switch (SignSize)
	{
	case ESignSize::RSS_05x05:
		return FVector2D(256);
	case ESignSize::RSS_1x1:
		return FVector2D(512);
	case ESignSize::RSS_1x2:
		return FVector2D(512, 1024);
	case ESignSize::RSS_1x7:
		return FVector2D(256, 1792);
	case ESignSize::RSS_2x05:
		return FVector2D(1024, 256);
	case ESignSize::RSS_2x1:
		return FVector2D(1024, 512);
	case ESignSize::RSS_2x2:
		return FVector2D(1024);
	case ESignSize::RSS_2x3:
		return FVector2D(1024, 1536);
	case ESignSize::RSS_3x05:
		return FVector2D(768, 128);
	case ESignSize::RSS_3x1:
		return FVector2D(768, 256);
	case ESignSize::RSS_4x05:
		return FVector2D(1024, 128);
	case ESignSize::RSS_4x1:
		return FVector2D(1024, 256);
	case ESignSize::RSS_7x1:
		return FVector2D(1792, 256);
	case ESignSize::RSS_8x4:
		return FVector2D(1536, 768);
	case ESignSize::RSS_16x8:
		return FVector2D(2048, 1024);
	case ESignSize::RSS_25x2:
		return FVector2D(2500, 200);
	case ESignSize::RSS_Custom:
		return FVector2D(1024, 512);
	case ESignSize::RSS_Secret:
		return FVector2D(1024, 512);
	default:
		return FVector2D(0);
	}
}

bool URssBlueprintFunctionLibrary::IsSignDataSafe(const FRssSignData& SignData)
{
	if (SignData.mSignType == ESignType::RSS_InValid || SignData.mSignTypeSize == ESignSize::RSS_InValid ||
		SignData.mElements.Num() > 128 || SignData.mTemplateData.mTemplateName.ToString().Len() > 128 ||
		!FMath::IsFinite(SignData.mMaterialData.mEmissiveIntensity))
	{
		return false;
	}

	for (const FRssElement& Element : SignData.mElements)
	{
		const FRssElementSharedData& Shared = Element.mSharedData;
		if (Element.mTextData.mText.ToString().Len() > 2048 || Shared.mElementName.ToString().Len() > 128 ||
			Shared.mUrl.Len() > 2048 || !FMath::IsFinite(Shared.mPosition.X) || !FMath::IsFinite(Shared.mPosition.Y) ||
			!FMath::IsFinite(Shared.mOpacity) || !FMath::IsFinite(Shared.mRotation) ||
			!FMath::IsFinite(Element.mImageData.mImageSize.X) || !FMath::IsFinite(Element.mImageData.mImageSize.Y) ||
			!FMath::IsFinite(Element.mEffectData.mStepFrequenz))
		{
			return false;
		}
		if (Shared.mIsUsingCustom &&
			!(Shared.mUrl.StartsWith(TEXT("http://"), ESearchCase::IgnoreCase) ||
			  Shared.mUrl.StartsWith(TEXT("https://"), ESearchCase::IgnoreCase)))
		{
			return false;
		}
	}
	return true;
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRssHexToLinearColorTest, "RSS.Color.HexToLinearColor",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRssHexToLinearColorTest::RunTest(const FString& Parameters)
{
	FLinearColor Color;
	bool bIsValid = false;
	URssBlueprintFunctionLibrary::HexToLinearColor(TEXT("#FF0000"), Color, bIsValid);
	TestTrue(TEXT("Six-digit customizer hex input is valid"), bIsValid);
	TestEqual(TEXT("Red channel"), Color.R, 1.0f);
	TestEqual(TEXT("Six-digit input defaults alpha to opaque"), Color.A, 1.0f);

	URssBlueprintFunctionLibrary::HexToLinearColor(TEXT("00FF0080"), Color, bIsValid);
	TestTrue(TEXT("Eight-digit hex input is valid"), bIsValid);
	TestEqual(TEXT("Eight-digit input preserves alpha"), Color.A, 128.0f / 255.0f);

	URssBlueprintFunctionLibrary::HexToLinearColor(TEXT("not-a-color"), Color, bIsValid);
	TestFalse(TEXT("Malformed input is rejected"), bIsValid);
	return true;
}
#endif
