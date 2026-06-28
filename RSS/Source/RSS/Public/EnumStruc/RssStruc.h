// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildable.h"

#include "EnumStruc/RssEnum.h"

#include "RssStruc.generated.h"

// NEW

USTRUCT(BlueprintType)
struct FRssMinMax
{
	GENERATED_BODY()

	FRssMinMax() : mMin(10.0), mMax(47.5), mSteps(0.01f) {}

	FRssMinMax(float Min, float Max, float Steps = 0.01f) : mMin(Min), mMax(Max), mSteps(Steps) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mMin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mMax;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float mSteps;
};

USTRUCT(BlueprintType)
struct FRssHologramData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	bool mEnable;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Meta = (EditCondition = "mEnable"))
	float mDistortionIntensity = 0.01f;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Meta = (EditCondition = "mEnable"))
	float mScanlineIntensity = 1.0f;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Meta = (EditCondition = "mEnable"))
	float mBorderIntensity = 10.0f;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Meta = (EditCondition = "mEnable"))
	bool mUseScanlines = true;
};

USTRUCT(BlueprintType)
struct FRssFlatData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	bool mEnable;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Meta = (EditCondition = "mEnable"))
	bool mUseParallax = false;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, BlueprintReadWrite, Meta = (EditCondition = "mEnable"))
	bool mUseArrowMaterial = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintReadWrite, Meta = (EditCondition = "mEnable"))
	float mOverwriteParalaxVerticalRatio = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintReadWrite, Meta = (EditCondition = "mEnable"))
	float mOverwriteParalaxHorizontalRatio = -1;
};

USTRUCT(BlueprintType)
struct FRssTemplateData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	bool mEnable;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Meta = (EditCondition = "mEnable"))
	FText mTemplateName = FText::GetEmpty();
};

USTRUCT(BlueprintType)
struct FRssRoundedData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	bool mEnable;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Meta = (EditCondition = "mEnable"))
	float mRoationSpeed = 3.0f;
};

USTRUCT(BlueprintType)
struct FRssElementTextData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FText mText = FText::GetEmpty();

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	ERssTextType mTextType = ERssTextType::NormalText;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FLinearColor mBackgroundColour;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	bool mIsBold = false;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	bool mIsUppercase = false;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	int mTextSize = 20;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	int mLetterSpacing = 0;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	float mLineHeight = 1.0f;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FVector4 mPadding = FVector4(20, 10, 20, 10);

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	EJust mTextJustify = EJust::RSS_Right;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, meta = (AllowedClasses = "Font"))
	TObjectPtr<UObject> mFont;
};

USTRUCT(BlueprintType)
struct FRssElementImageData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FVector2D mImageSize = FVector2D();

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FVector2D mScaleMirrow = FVector2D(1);

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FVector2D mOverwriteImageSize = FVector2D();

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FVector2D mUse9SliceMode = FVector2D(0, 0.25);
};

USTRUCT(BlueprintType)
struct FRssElementEffectData
{
	GENERATED_BODY()

	// UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	// ERssEffectType mType = ERssEffectType::RSS_Panner;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	ERssPannerType mPannerType = ERssPannerType::RSS_Linear;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	float mStepFrequenz = 5.0f;

	/**
	 * B and A is Scale
	 * R and G is Speep and direction
	 */
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FLinearColor mScaleAndSpeed = FLinearColor(.1f, .1f, 5.0f, 5.0f);
};

USTRUCT(BlueprintType)
struct FRssElementSharedData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FText mElementName = FText::FromString("");

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FVector2D mPosition = FVector2D();

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FLinearColor mColourOverwrite = FLinearColor::White;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	int mZIndex = 0;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	float mOpacity = 0.5f;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	float mRotation = 0.0f;

	// Image And Effect
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	TObjectPtr<UTexture> mTexture;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FString mUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture> mCustomTexture;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	bool mIsUsingCustom;
};

USTRUCT(BlueprintType)
struct FRssElement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	ESignElementType mElementType;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FRssElementTextData mTextData;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FRssElementImageData mImageData;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FRssElementEffectData mEffectData;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FRssElementSharedData mSharedData;

	bool IsTypeOf(ESignElementType type) const { return mElementType == type; }

	bool IsTypeOf(uint8 type) const { return static_cast<uint8>(mElementType) == type; }
};

USTRUCT(BlueprintType)
struct FRssSignMaterialData
{
	GENERATED_BODY()

	// Emmisive of the Sign
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	float mEmissiveIntensity = 0.3;

	// Sign Background Color
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FLinearColor mBackgroundColor = FLinearColor(0.039216f, 0.039216f, 0.039216f, 1.0f);

	/**
	 * Assign Animation to the Signs
	 * R + G = Panner speed for the whole signs
	 * B + G is only used if the Signs use the Arrow Material
	 */
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FVector4 mRotation = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
};

USTRUCT(BlueprintType)
struct FRssSignData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	ESignType mSignType = ESignType::RSS_InValid;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	ESignSize mSignTypeSize = ESignSize::RSS_InValid;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FRssHologramData mHologramData;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FRssFlatData mFlatData;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FRssRoundedData mRoundedData;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	TArray<FRssElement> mElements;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FRssSignMaterialData mMaterialData;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FRssTemplateData mTemplateData;
};

USTRUCT(BlueprintType)
struct FRssTemplateSignData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	int mIndex;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FRssSignData mSignData;
};

USTRUCT(BlueprintType)
struct FRssCustomSignUrlData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	FString mUrl = "";

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	TObjectPtr<UTexture> mTexture = nullptr;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	TArray<ESignSize> mAllowedInSigns = {};
};

USTRUCT(BlueprintType)
struct FRssCustomDatas
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite)
	TArray<FRssCustomSignUrlData> mInformations = {};
};

USTRUCT(BlueprintType)
struct FRssUiData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRssMinMax mRoundedSpeed = FRssMinMax(0.0f, 30.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRssMinMax mEmissive = FRssMinMax(0.0f, 4.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRssMinMax mDistortionIntensity = FRssMinMax(-0.1f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRssMinMax mBorderIntensity = FRssMinMax(1.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRssMinMax mScanlineIntensity = FRssMinMax(0.1f, 5.0f);
};

USTRUCT(BlueprintType)
struct FRssSignRequestData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mOnlyAddToUI = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UUserWidget> mRequestFromWidgetObject = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> mBuildable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString mRequestUrl = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mWasSuccess = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture> mSuccessTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int mRequestOnImageIndex = -1;
};
