#include "Widget/RssSignWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "Interface/RssSignInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Crc.h"
#include "RssBlueprintFunctionLibrary.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr TCHAR TextElementWidgetClassPath[] =
		TEXT("/RSS/Buildable/-Shared/BP_Bases/BP_WC_SignWidget_Text.BP_WC_SignWidget_Text_C");
	constexpr TCHAR EffectMaterialPath[] =
		TEXT("/RSS/Buildable/-Shared/BP_Bases/Materials/WidgetEffects/MI_GridEffect_Tile.MI_GridEffect_Tile");
	constexpr TCHAR PlaceholderTexturePath[] = TEXT("/RSS/Assets/Images/PlaceHolder0000.PlaceHolder0000");
	constexpr TCHAR BlankTexturePath[] = TEXT("/RSS/Assets/MaterialTextures/16x16_b.16x16_b");

	uint32 CombineHash(uint32 Seed, uint32 Value) { return HashCombine(Seed, Value); }

	uint32 HashLinearColor(const FLinearColor& Color)
	{
		uint32 Hash = GetTypeHash(Color.R);
		Hash = CombineHash(Hash, GetTypeHash(Color.G));
		Hash = CombineHash(Hash, GetTypeHash(Color.B));
		return CombineHash(Hash, GetTypeHash(Color.A));
	}

	uint32 HashVector2D(const FVector2D& Vector) { return CombineHash(GetTypeHash(Vector.X), GetTypeHash(Vector.Y)); }

	uint32 HashVector4(const FVector4& Vector)
	{
		uint32 Hash = GetTypeHash(Vector.X);
		Hash = CombineHash(Hash, GetTypeHash(Vector.Y));
		Hash = CombineHash(Hash, GetTypeHash(Vector.Z));
		return CombineHash(Hash, GetTypeHash(Vector.W));
	}

	constexpr double ResolveImageAxisSize(double OverwriteSize, double TextureSize)
	{
		return OverwriteSize > 0.0 ? OverwriteSize : TextureSize > 0.0 ? TextureSize : 1.0;
	}

	static_assert(ResolveImageAxisSize(0.0, 50.0) == 50.0);
	static_assert(ResolveImageAxisSize(150.0, 50.0) == 150.0);

	constexpr bool ResolveImageFillFlag(double FillFlag) { return FillFlag > 0.0; }

	static_assert(!ResolveImageFillFlag(0.0));
	static_assert(ResolveImageFillFlag(1.0));

	constexpr double ResolveImageMirrorAxis(double Axis) { return Axis < 0.0 ? -1.0 : 1.0; }

	static_assert(ResolveImageMirrorAxis(-1.0) == -1.0);
	static_assert(ResolveImageMirrorAxis(0.0) == 1.0);
	static_assert(ResolveImageMirrorAxis(1.0) == 1.0);

	FVector2D ResolveImageRenderScale(const FRssElementImageData& ImageData)
	{
		const double Zoom = ImageData.mImageSize.X;
		return FVector2D(ResolveImageMirrorAxis(ImageData.mScaleMirrow.X) * Zoom,
						 ResolveImageMirrorAxis(ImageData.mScaleMirrow.Y) * Zoom);
	}

	bool IsLegacyFillSignEnabled(const FRssElementImageData& ImageData, const FVector2D& DrawSize)
	{

		return DrawSize.X > 0.0 && DrawSize.Y > 0.0 && ResolveImageFillFlag(ImageData.mImageSize.Y);
	}

	UTexture* ResolveElementTexture(const FRssElementSharedData& SharedData)
	{
		UTexture* Texture = SharedData.mIsUsingCustom && IsValid(SharedData.mCustomTexture) ? SharedData.mCustomTexture
																							: SharedData.mTexture;
		if (IsValid(Texture) && Texture->GetPathName() == PlaceholderTexturePath)
		{
			return LoadObject<UTexture>(nullptr, BlankTexturePath);
		}
		return Texture;
	}

	ETextJustify::Type ResolveTextJustification(EJust Justification)
	{
		switch (Justification)
		{
		case EJust::RSS_Left:
			return ETextJustify::Left;
		case EJust::RSS_Middle:
			return ETextJustify::Center;
		case EJust::RSS_Right:
		default:
			return ETextJustify::Right;
		}
	}

	float ResolveTextCanvasAlignment(EJust Justification)
	{
		switch (Justification)
		{
		case EJust::RSS_Left:
			return 0.0f;
		case EJust::RSS_Middle:
			return 0.5f;
		case EJust::RSS_Right:
		default:
			return 1.0f;
		}
	}
}

URssSignWidget* URssSignWidget::CreateNativeSignWidget(UObject* WorldContextObject,
													   TSubclassOf<URssSignWidget> WidgetClass)
{
	if (!GEngine || !IsValid(WorldContextObject) || !WidgetClass)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	return World ? CreateWidget<URssSignWidget>(World, WidgetClass) : nullptr;
}

void URssSignWidget::SetBuildable(AActor* Buildable)
{
	if (IsValid(Buildable) && UKismetSystemLibrary::DoesImplementInterface(Buildable, URssSignInterface::StaticClass()))
	{
		mBuildable = Buildable;
		mSignWidgetData = IRssSignInterface::Execute_GetSignData(Buildable);
		mUIData = IRssSignInterface::Execute_GetSignUiIData(Buildable);
		OnBuildableSet(Buildable);
		InvalidateNativeElements();
		SynchronizeNativeElements(true);
		return;
	}

	ClearNativeElements();
}

void URssSignWidget::UpdateSignData(FRssSignData SignData)
{
	mSignWidgetData = MoveTemp(SignData);
	if (IsValid(mBuildable) &&
		UKismetSystemLibrary::DoesImplementInterface(mBuildable, URssSignInterface::StaticClass()))
	{
		OnSignDataUpdated(mSignWidgetData);
	}
	InvalidateNativeElements();
	SynchronizeNativeElements(true);
}

void URssSignWidget::NativeConstruct()
{
	Super::NativeConstruct();
	InvalidateNativeElements();
	SynchronizeNativeElements(true);
}

void URssSignWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	SynchronizeNativeElements(false);
}

void URssSignWidget::SetNativeDrawSize(FVector2D NewDrawSize)
{
	if (!FMath::IsFinite(NewDrawSize.X) || !FMath::IsFinite(NewDrawSize.Y))
	{
		return;
	}

	NewDrawSize.X = FMath::Max(0.0, NewDrawSize.X);
	NewDrawSize.Y = FMath::Max(0.0, NewDrawSize.Y);
	if (!mNativeDrawSizeOverride.Equals(NewDrawSize))
	{
		mNativeDrawSizeOverride = NewDrawSize;
		InvalidateNativeElements();
	}
}

UWidget* URssSignWidget::GetNativeElementWidget(int32 Index) const
{
	return mNativeRenderElements.IsValidIndex(Index) ? mNativeRenderElements[Index].Get() : nullptr;
}

UCanvasPanel* URssSignWidget::GetNativeElementCanvas() const { return ResolveElementCanvas(); }

void URssSignWidget::InvalidateNativeElements() { bNativeRenderDirty = true; }

void URssSignWidget::UpdateNativeElementByIndex(int32 Index, const FRssElement& Element)
{
	if (!mSignWidgetData.mElements.IsValidIndex(Index))
	{
		return;
	}

	mSignWidgetData.mElements[Index] = Element;
	InvalidateNativeElements();
	SynchronizeNativeElements(false);
}

UCanvasPanel* URssSignWidget::ResolveElementCanvas() const
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	if (UCanvasPanel* MainCanvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("mMainCanv"))))
	{
		return MainCanvas;
	}
	if (UCanvasPanel* ElementCanvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("mElementCanv"))))
	{
		return ElementCanvas;
	}

	if (const FObjectPropertyBase* ElementCanvasProperty =
			FindFProperty<FObjectPropertyBase>(GetClass(), TEXT("mElementCanv")))
	{
		return Cast<UCanvasPanel>(ElementCanvasProperty->GetObjectPropertyValue_InContainer(this));
	}
	return nullptr;
}

FVector2D URssSignWidget::ResolveDrawSize(UCanvasPanel* ElementCanvas) const
{
	if (mNativeDrawSizeOverride.X > 0.0 && mNativeDrawSizeOverride.Y > 0.0)
	{
		return mNativeDrawSizeOverride;
	}

	if (const FBoolProperty* OverwriteEnabledProperty =
			FindFProperty<FBoolProperty>(GetClass(), TEXT("mIsDrawSizeOverwrite"));
		OverwriteEnabledProperty && OverwriteEnabledProperty->GetPropertyValue_InContainer(this))
	{
		if (const FStructProperty* OverwriteSizeProperty =
				FindFProperty<FStructProperty>(GetClass(), TEXT("mOverWriteDrawSize"));
			OverwriteSizeProperty && OverwriteSizeProperty->Struct == TBaseStructure<FVector2D>::Get())
		{
			const FVector2D* OverwriteSize = OverwriteSizeProperty->ContainerPtrToValuePtr<FVector2D>(this);
			if (OverwriteSize && FMath::IsFinite(OverwriteSize->X) && FMath::IsFinite(OverwriteSize->Y) &&
				OverwriteSize->X > 0.0 && OverwriteSize->Y > 0.0)
			{
				return *OverwriteSize;
			}
		}
	}

	if (const FStructProperty* DrawSizeProperty = FindFProperty<FStructProperty>(GetClass(), TEXT("mDrawSize"));
		DrawSizeProperty && DrawSizeProperty->Struct == TBaseStructure<FVector2D>::Get())
	{
		const FVector2D* PreviewDrawSize = DrawSizeProperty->ContainerPtrToValuePtr<FVector2D>(this);
		if (PreviewDrawSize && FMath::IsFinite(PreviewDrawSize->X) && FMath::IsFinite(PreviewDrawSize->Y) &&
			PreviewDrawSize->X > 0.0 && PreviewDrawSize->Y > 0.0)
		{
			return *PreviewDrawSize;
		}
	}

	const FVector2D SignSize = URssBlueprintFunctionLibrary::GetScreenSize(mSignWidgetData.mSignTypeSize);
	if (SignSize.X > 0.0 && SignSize.Y > 0.0)
	{
		return SignSize;
	}

	if (ElementCanvas)
	{
		const FVector2D GeometrySize = ElementCanvas->GetCachedGeometry().GetLocalSize();
		if (GeometrySize.X > 0.0 && GeometrySize.Y > 0.0)
		{
			return GeometrySize;
		}
	}
	return FVector2D::ZeroVector;
}

FVector2D URssSignWidget::ResolveImageSize(const FRssElement& Element, const FVector2D& DrawSize,
										   bool& bOutFillSign) const
{
	const FRssElementImageData& ImageData = Element.mImageData;
	UTexture* Texture = ResolveElementTexture(Element.mSharedData);
	const FVector2D TextureSize =
		Texture ? FVector2D(Texture->GetSurfaceWidth(), Texture->GetSurfaceHeight()) : FVector2D(32.0, 32.0);

	bOutFillSign = IsLegacyFillSignEnabled(ImageData, DrawSize);
	if (bOutFillSign)
	{
		return FVector2D(FMath::Max(DrawSize.X, 1.0), FMath::Max(DrawSize.Y, 1.0));
	}

	return FVector2D(ResolveImageAxisSize(ImageData.mOverwriteImageSize.X, TextureSize.X),
					 ResolveImageAxisSize(ImageData.mOverwriteImageSize.Y, TextureSize.Y));
}

UWidget* URssSignWidget::CreateNativeImageElement(const FRssElement& Element, const FVector2D& DrawSize,
												  bool& bOutFillSign)
{
	UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (!Image)
	{
		return nullptr;
	}

	const FVector2D ImageSize = ResolveImageSize(Element, DrawSize, bOutFillSign);
	FSlateBrush Brush;
	Brush.SetResourceObject(ResolveElementTexture(Element.mSharedData));
	Brush.ImageSize = FVector2f(static_cast<float>(ImageSize.X), static_cast<float>(ImageSize.Y));
	if (Element.mImageData.mUse9SliceMode.X > 0.5)
	{
		const float SliceMargin = FMath::Clamp(static_cast<float>(Element.mImageData.mUse9SliceMode.Y), 0.0f, 0.5f);
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = FMargin(SliceMargin);
	}
	else
	{
		Brush.DrawAs = ESlateBrushDrawType::Image;
	}

	Image->SetBrush(Brush);
	Image->SetColorAndOpacity(Element.mSharedData.mColourOverwrite);

	Image->SetRenderScale(ResolveImageRenderScale(Element.mImageData));
	Image->SetRenderTransformPivot(FVector2D(0.5, 0.5));
	Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	return Image;
}

FName URssSignWidget::ResolveTypefaceFontName(const FRssElementTextData& TextData) const
{

	const FName FallbackName = TextData.mIsBold ? NAME_None : FName(TEXT("Default"));
	if (!IsValid(TextData.mFont))
	{
		return FallbackName;
	}

	UFunction* GetFontFunction = FindFunction(TEXT("GetFont"));
	if (!GetFontFunction)
	{
		return FallbackName;
	}

	FObjectPropertyBase* FontParameter = nullptr;
	FStructProperty* FontInfoParameter = nullptr;
	FBoolProperty* ValidParameter = nullptr;
	for (TFieldIterator<FProperty> PropertyIt(GetFontFunction); PropertyIt && PropertyIt->HasAnyPropertyFlags(CPF_Parm);
		 ++PropertyIt)
	{
		FProperty* Property = *PropertyIt;
		if (!Property->HasAnyPropertyFlags(CPF_OutParm) && Property->GetName() == TEXT("Font"))
		{
			FontParameter = CastField<FObjectPropertyBase>(Property);
		}
		else if (Property->HasAnyPropertyFlags(CPF_OutParm) && Property->GetName() == TEXT("FontInfo"))
		{
			FontInfoParameter = CastField<FStructProperty>(Property);
		}
		else if (Property->HasAnyPropertyFlags(CPF_OutParm) && Property->GetName() == TEXT("Valid"))
		{
			ValidParameter = CastField<FBoolProperty>(Property);
		}
	}
	if (!FontParameter || !FontInfoParameter || !ValidParameter)
	{
		return FallbackName;
	}

	FStructOnScope Parameters(GetFontFunction);
	uint8* ParameterMemory = Parameters.GetStructMemory();
	FontParameter->SetObjectPropertyValue_InContainer(ParameterMemory, TextData.mFont);
	const_cast<URssSignWidget*>(this)->ProcessEvent(GetFontFunction, ParameterMemory);
	if (!ValidParameter->GetPropertyValue_InContainer(ParameterMemory))
	{
		return FallbackName;
	}

	const FString TypefacePropertyPrefix = TextData.mIsBold ? TEXT("mBoldName") : TEXT("mDefaultName");
	void* FontInfoMemory = FontInfoParameter->ContainerPtrToValuePtr<void>(ParameterMemory);
	for (TFieldIterator<FNameProperty> NameIt(FontInfoParameter->Struct); NameIt; ++NameIt)
	{
		if (NameIt->GetName().StartsWith(TypefacePropertyPrefix))
		{
			return NameIt->GetPropertyValue_InContainer(FontInfoMemory);
		}
	}
	return FallbackName;
}

UWidget* URssSignWidget::CreateNativeTextElement(const FRssElement& Element)
{
	const FRssElementTextData& TextData = Element.mTextData;
	UUserWidget* TextContainer = nullptr;
	UTextBlock* TextBlock = nullptr;
	UImage* Background = nullptr;
	UImage* Outline = nullptr;

	if (UClass* TextWidgetClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, TextElementWidgetClassPath))
	{
		TextContainer = CreateWidget<UUserWidget>(this, TextWidgetClass);
		if (TextContainer && TextContainer->WidgetTree)
		{
			TextBlock = Cast<UTextBlock>(TextContainer->WidgetTree->FindWidget(TEXT("mText")));
			Background = Cast<UImage>(TextContainer->WidgetTree->FindWidget(TEXT("mBackground")));
			Outline = Cast<UImage>(TextContainer->WidgetTree->FindWidget(TEXT("mOutlineBorder")));
		}
	}

	UBorder* FallbackBorder = nullptr;
	if (!TextBlock)
	{
		FallbackBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (!FallbackBorder || !TextBlock)
		{
			return nullptr;
		}
		FallbackBorder->SetContent(TextBlock);
		FallbackBorder->SetPadding(
			FMargin(TextData.mPadding.X, TextData.mPadding.Y, TextData.mPadding.Z, TextData.mPadding.W));
		FallbackBorder->SetBrushColor(TextData.mTextType == ERssTextType::BackgroundText ? TextData.mBackgroundColour
																						 : FLinearColor::Transparent);
	}

	TextBlock->SetText(TextData.mIsUppercase ? TextData.mText.ToUpper() : TextData.mText);
	TextBlock->SetColorAndOpacity(FSlateColor(Element.mSharedData.mColourOverwrite));
	TextBlock->SetJustification(ResolveTextJustification(TextData.mTextJustify));
	TextBlock->SetLineHeightPercentage(FMath::Max(TextData.mLineHeight, 0.01f));
	TextBlock->SetAutoWrapText(false);

	FSlateFontInfo FontInfo = TextBlock->GetFont();
	FontInfo.Size = FMath::Max(TextData.mTextSize, 1);
	if (IsValid(TextData.mFont))
	{
		FontInfo.FontObject = TextData.mFont;
	}
	FontInfo.TypefaceFontName = ResolveTypefaceFontName(TextData);

	FontInfo.LetterSpacing =
		FMath::RoundToInt(static_cast<double>(TextData.mLetterSpacing) * 1000.0 / static_cast<double>(FontInfo.Size));
	TextBlock->SetFont(FontInfo);

	if (UOverlaySlot* TextSlot = Cast<UOverlaySlot>(TextBlock->Slot))
	{
		TextSlot->SetPadding(
			FMargin(TextData.mPadding.X, TextData.mPadding.Y, TextData.mPadding.Z, TextData.mPadding.W));
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
		TextSlot->SetVerticalAlignment(VAlign_Fill);
	}
	if (Background)
	{
		Background->SetColorAndOpacity(TextData.mBackgroundColour);
		Background->SetVisibility(TextData.mTextType == ERssTextType::BackgroundText
									  ? ESlateVisibility::SelfHitTestInvisible
									  : ESlateVisibility::Collapsed);
	}
	if (Outline)
	{
		Outline->SetVisibility(ESlateVisibility::Collapsed);
	}

	UWidget* Result = FallbackBorder ? static_cast<UWidget*>(FallbackBorder) : static_cast<UWidget*>(TextContainer);
	if (!Result)
	{
		return nullptr;
	}
	Result->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Result->SetRenderTransformPivot(FVector2D(0.5, 0.5));
	return Result;
}

UWidget* URssSignWidget::CreateNativeEffectElement(const FRssElement& Element, const FVector2D& DrawSize)
{
	UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	UMaterialInterface* EffectMaterial = LoadObject<UMaterialInterface>(nullptr, EffectMaterialPath);
	if (!Image || !EffectMaterial)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(EffectMaterial, this);
	if (!DynamicMaterial)
	{
		return nullptr;
	}
	mNativeEffectMaterials.Add(DynamicMaterial);

	DynamicMaterial->SetTextureParameterValue(TEXT("Texture"), ResolveElementTexture(Element.mSharedData));
	DynamicMaterial->SetScalarParameterValue(TEXT("UseMode"), static_cast<float>(Element.mEffectData.mPannerType));
	const FLinearColor& SpeedAndScale = Element.mEffectData.mScaleAndSpeed;
	DynamicMaterial->SetVectorParameterValue(TEXT("SpeedAndScale"), SpeedAndScale);
	DynamicMaterial->SetScalarParameterValue(TEXT("Frequenz"), Element.mEffectData.mStepFrequenz);
	DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), FMath::Clamp(Element.mSharedData.mOpacity, 0.0f, 1.0f));
	DynamicMaterial->SetScalarParameterValue(TEXT("Rotation"), Element.mSharedData.mRotation);

	FSlateBrush Brush;
	Brush.SetResourceObject(DynamicMaterial);
	Brush.ImageSize =
		FVector2f(static_cast<float>(FMath::Max(DrawSize.X, 1.0)), static_cast<float>(FMath::Max(DrawSize.Y, 1.0)));
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Image->SetBrush(Brush);
	Image->SetColorAndOpacity(Element.mSharedData.mColourOverwrite);
	Image->SetRenderTransformPivot(FVector2D(0.5, 0.5));
	Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	return Image;
}

bool URssSignWidget::AreNativeElementsAttached(const UCanvasPanel* ElementCanvas, const FVector2D& DrawSize) const
{
	if (!ElementCanvas || mNativeRenderElements.Num() != mSignWidgetData.mElements.Num())
	{
		return false;
	}

	int32 ChildIndex = 0;
	for (int32 ElementIndex = 0; ElementIndex < mNativeRenderElements.Num(); ++ElementIndex)
	{
		const TObjectPtr<UWidget>& ElementWidget = mNativeRenderElements[ElementIndex];
		if (!ElementWidget)
		{
			continue;
		}
		if (ChildIndex >= ElementCanvas->GetChildrenCount() || ElementCanvas->GetChildAt(ChildIndex) != ElementWidget)
		{
			return false;
		}

		const FRssElement& Element = mSignWidgetData.mElements[ElementIndex];
		if (Element.mElementType == ESignElementType::Image || Element.mElementType == ESignElementType::Effect)
		{
			const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ElementWidget->Slot);
			bool bFillSign = false;
			const FVector2D ExpectedSize = Element.mElementType == ESignElementType::Image
				? ResolveImageSize(Element, DrawSize, bFillSign)
				: DrawSize;
			if (!CanvasSlot || !CanvasSlot->GetSize().Equals(ExpectedSize, UE_KINDA_SMALL_NUMBER))
			{
				return false;
			}
		}
		++ChildIndex;
	}
	return ChildIndex == ElementCanvas->GetChildrenCount();
}

bool URssSignWidget::IsLegacyBlueprintCacheDirty() const
{
	if (const FBoolProperty* CacheUpdateProperty = FindFProperty<FBoolProperty>(GetClass(), TEXT("mChacheIsUpdate")))
	{
		return CacheUpdateProperty->GetPropertyValue_InContainer(this);
	}
	return false;
}

void URssSignWidget::SetLegacyBlueprintCacheDirty(bool bIsDirty)
{
	if (FBoolProperty* CacheUpdateProperty = FindFProperty<FBoolProperty>(GetClass(), TEXT("mChacheIsUpdate")))
	{
		CacheUpdateProperty->SetPropertyValue_InContainer(this, bIsDirty);
	}
}

void URssSignWidget::SynchronizeNativeElements(bool bForce)
{
	UCanvasPanel* ElementCanvas = ResolveElementCanvas();
	if (!ElementCanvas || !WidgetTree)
	{
		return;
	}

	const FVector2D DrawSize = ResolveDrawSize(ElementCanvas);
	const uint32 NewHash = ComputeNativeRenderHash(DrawSize);
	if (!bForce && !bNativeRenderDirty && bNativeRenderInitialized && !IsLegacyBlueprintCacheDirty() &&
		NewHash == mNativeRenderHash && AreNativeElementsAttached(ElementCanvas, DrawSize))
	{
		bNativeRenderDirty = false;
		return;
	}

	ElementCanvas->ClearChildren();
	mNativeRenderElements.Reset(mSignWidgetData.mElements.Num());
	mNativeEffectMaterials.Reset();

	for (const FRssElement& Element : mSignWidgetData.mElements)
	{
		UWidget* ElementWidget = nullptr;
		bool bFillSign = false;
		FVector2D ElementSize = FVector2D::ZeroVector;

		switch (Element.mElementType)
		{
		case ESignElementType::Text:
			ElementWidget = CreateNativeTextElement(Element);
			break;
		case ESignElementType::Image:
			ElementWidget = CreateNativeImageElement(Element, DrawSize, bFillSign);
			ElementSize = ResolveImageSize(Element, DrawSize, bFillSign);
			break;
		case ESignElementType::Effect:
			ElementWidget = CreateNativeEffectElement(Element, DrawSize);
			ElementSize = DrawSize;
			break;
		default:
			break;
		}

		if (!ElementWidget)
		{
			mNativeRenderElements.Add(nullptr);
			continue;
		}

		UCanvasPanelSlot* CanvasSlot = ElementCanvas->AddChildToCanvas(ElementWidget);
		if (!CanvasSlot)
		{
			mNativeRenderElements.Add(nullptr);
			continue;
		}

		CanvasSlot->SetPosition(bFillSign
									? FVector2D::ZeroVector
									: FVector2D(Element.mSharedData.mPosition.X, Element.mSharedData.mPosition.Y));
		CanvasSlot->SetZOrder(Element.mSharedData.mZIndex);
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(Element.mElementType == ESignElementType::Text
											   ? ResolveTextCanvasAlignment(Element.mTextData.mTextJustify)
											   : 0.5f,
										   0.5f));
		if (Element.mElementType == ESignElementType::Text)
		{
			CanvasSlot->SetAutoSize(true);
		}
		else
		{
			CanvasSlot->SetAutoSize(false);
			CanvasSlot->SetSize(FVector2D(FMath::Max(ElementSize.X, 1.0), FMath::Max(ElementSize.Y, 1.0)));
		}

		if (Element.mElementType != ESignElementType::Effect)
		{
			ElementWidget->SetRenderOpacity(FMath::Clamp(Element.mSharedData.mOpacity, 0.0f, 1.0f));
			ElementWidget->SetRenderTransformAngle(Element.mSharedData.mRotation);
		}
		mNativeRenderElements.Add(ElementWidget);
	}

	UpdateBlueprintElementCache();
	mNativeRenderHash = NewHash;
	bNativeRenderInitialized = true;
	bNativeRenderDirty = false;
}

void URssSignWidget::UpdateBlueprintElementCache()
{
	FMapProperty* CacheProperty = FindFProperty<FMapProperty>(GetClass(), TEXT("mCachedElements"));
	if (!CacheProperty || !CastField<FIntProperty>(CacheProperty->KeyProp) ||
		!CastField<FObjectPropertyBase>(CacheProperty->ValueProp))
	{
		SetLegacyBlueprintCacheDirty(false);
		return;
	}

	FScriptMapHelper CacheHelper(CacheProperty, CacheProperty->ContainerPtrToValuePtr<void>(this));
	CacheHelper.EmptyValues(mNativeRenderElements.Num());
	FIntProperty* KeyProperty = CastFieldChecked<FIntProperty>(CacheProperty->KeyProp);
	FObjectPropertyBase* ValueProperty = CastFieldChecked<FObjectPropertyBase>(CacheProperty->ValueProp);
	for (int32 Index = 0; Index < mNativeRenderElements.Num(); ++Index)
	{
		if (!mNativeRenderElements[Index])
		{
			continue;
		}
		const int32 MapIndex = CacheHelper.AddDefaultValue_Invalid_NeedsRehash();
		KeyProperty->SetPropertyValue(CacheHelper.GetKeyPtr(MapIndex), Index);
		ValueProperty->SetObjectPropertyValue(CacheHelper.GetValuePtr(MapIndex), mNativeRenderElements[Index]);
	}
	CacheHelper.Rehash();

	SetLegacyBlueprintCacheDirty(false);
}

void URssSignWidget::ClearNativeElements()
{
	mBuildable = nullptr;
	mSignWidgetData = FRssSignData();
	mUIData = FRssUiData();
	if (UCanvasPanel* ElementCanvas = ResolveElementCanvas())
	{
		ElementCanvas->ClearChildren();
	}
	mNativeRenderElements.Reset();
	mNativeEffectMaterials.Reset();
	UpdateBlueprintElementCache();
	mNativeRenderHash = 0;
	bNativeRenderInitialized = false;
	bNativeRenderDirty = true;
}

uint32 URssSignWidget::ComputeNativeRenderHash(const FVector2D& DrawSize) const
{
	uint32 Hash = HashVector2D(DrawSize);
	Hash = CombineHash(Hash, GetTypeHash(static_cast<uint8>(mSignWidgetData.mSignType)));
	Hash = CombineHash(Hash, GetTypeHash(static_cast<uint8>(mSignWidgetData.mSignTypeSize)));
	Hash = CombineHash(Hash, GetTypeHash(mSignWidgetData.mElements.Num()));

	for (const FRssElement& Element : mSignWidgetData.mElements)
	{
		const FRssElementSharedData& Shared = Element.mSharedData;
		Hash = CombineHash(Hash, GetTypeHash(static_cast<uint8>(Element.mElementType)));
		Hash = CombineHash(Hash, HashVector2D(Shared.mPosition));
		Hash = CombineHash(Hash, HashLinearColor(Shared.mColourOverwrite));
		Hash = CombineHash(Hash, GetTypeHash(Shared.mZIndex));
		Hash = CombineHash(Hash, GetTypeHash(Shared.mOpacity));
		Hash = CombineHash(Hash, GetTypeHash(Shared.mRotation));
		Hash = CombineHash(Hash, GetTypeHash(Shared.mIsUsingCustom));
		Hash = CombineHash(Hash, PointerHash(Shared.mTexture.Get()));
		Hash = CombineHash(Hash, PointerHash(Shared.mCustomTexture.Get()));

		if (UTexture* Texture = ResolveElementTexture(Shared))
		{
			Hash = CombineHash(Hash, GetTypeHash(Texture->GetSurfaceWidth()));
			Hash = CombineHash(Hash, GetTypeHash(Texture->GetSurfaceHeight()));
		}

		switch (Element.mElementType)
		{
		case ESignElementType::Text:
			{
				const FRssElementTextData& Text = Element.mTextData;
				Hash = CombineHash(Hash, FCrc::StrCrc32(*Text.mText.ToString()));
				Hash = CombineHash(Hash, GetTypeHash(static_cast<uint8>(Text.mTextType)));
				Hash = CombineHash(Hash, HashLinearColor(Text.mBackgroundColour));
				Hash = CombineHash(Hash, GetTypeHash(Text.mIsBold));
				Hash = CombineHash(Hash, GetTypeHash(Text.mIsUppercase));
				Hash = CombineHash(Hash, GetTypeHash(Text.mTextSize));
				Hash = CombineHash(Hash, GetTypeHash(Text.mLetterSpacing));
				Hash = CombineHash(Hash, GetTypeHash(Text.mLineHeight));
				Hash = CombineHash(Hash, HashVector4(Text.mPadding));
				Hash = CombineHash(Hash, GetTypeHash(static_cast<uint8>(Text.mTextJustify)));
				Hash = CombineHash(Hash, PointerHash(Text.mFont.Get()));
				break;
			}
		case ESignElementType::Image:
			Hash = CombineHash(Hash, HashVector2D(Element.mImageData.mImageSize));
			Hash = CombineHash(Hash, HashVector2D(Element.mImageData.mScaleMirrow));
			Hash = CombineHash(Hash, HashVector2D(Element.mImageData.mOverwriteImageSize));
			Hash = CombineHash(Hash, HashVector2D(Element.mImageData.mUse9SliceMode));
			break;
		case ESignElementType::Effect:
			Hash = CombineHash(Hash, GetTypeHash(static_cast<uint8>(Element.mEffectData.mPannerType)));
			Hash = CombineHash(Hash, GetTypeHash(Element.mEffectData.mStepFrequenz));
			Hash = CombineHash(Hash, HashLinearColor(Element.mEffectData.mScaleAndSpeed));
			break;
		default:
			break;
		}
	}
	return Hash;
}

void URssSignWidget::OnRequestUpdateSign_Implementation()
{
	InvalidateNativeElements();
	SynchronizeNativeElements(false);
}
