#pragma once

#include "CoreMinimal.h"

#include "EnumStruc/RssStruc.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "RssJsonSerializer.generated.h"

class FJsonObject;

class RSS_API FRssJsonSerializer
{
public:

	static FString SignDataToJson(const FRssSignData& SignData);

	static bool JsonToSignData(const FString& JsonString, FRssSignData& OutSignData);
	static bool JsonToSignData(const FString& JsonString, FRssSignData& OutSignData, FText* OutError,
							   TArray<FText>* OutWarnings);

	static FString TemplatesToJson(const TArray<FRssSignData>& Templates);

	static bool JsonToTemplates(const FString& JsonString, TArray<FRssSignData>& OutTemplates);

	static FString CustomDataToJson(const FRssCustomDatas& CustomData);

	static bool JsonToCustomData(const FString& JsonString, FRssCustomDatas& OutCustomData);

private:

	static TSharedPtr<FJsonObject> BuildRootDocument(const FRssSignData& SignData);

	static bool ParseRootDocument(const TSharedPtr<FJsonObject>& RootObject, FRssSignData& OutSignData,
							  FText* OutError, TArray<FText>* OutWarnings);

	static TSharedPtr<FJsonObject> StructToJson(const FRssSignData& SignData);
	static bool JsonToStruct(const TSharedPtr<FJsonObject>& JsonObj, FRssSignData& OutSignData,
							 TArray<FText>* OutWarnings = nullptr);

	static TSharedPtr<FJsonObject> HologramDataToJson(const FRssHologramData& Data);
	static bool JsonToHologramData(const TSharedPtr<FJsonObject>& JsonObj, FRssHologramData& OutData);

	static TSharedPtr<FJsonObject> FlatDataToJson(const FRssFlatData& Data);
	static bool JsonToFlatData(const TSharedPtr<FJsonObject>& JsonObj, FRssFlatData& OutData);

	static TSharedPtr<FJsonObject> RoundedDataToJson(const FRssRoundedData& Data);
	static bool JsonToRoundedData(const TSharedPtr<FJsonObject>& JsonObj, FRssRoundedData& OutData);

	static TSharedPtr<FJsonObject> TemplateDataToJson(const FRssTemplateData& Data);
	static bool JsonToTemplateData(const TSharedPtr<FJsonObject>& JsonObj, FRssTemplateData& OutData);

	static TSharedPtr<FJsonObject> ElementToJson(const FRssElement& Element);
	static bool JsonToElement(const TSharedPtr<FJsonObject>& JsonObj, FRssElement& OutElement,
						  TArray<FText>* OutWarnings = nullptr);

	static TSharedPtr<FJsonObject> ElementTextDataToJson(const FRssElementTextData& Data);
	static bool JsonToElementTextData(const TSharedPtr<FJsonObject>& JsonObj, FRssElementTextData& OutData);

	static TSharedPtr<FJsonObject> ElementImageDataToJson(const FRssElementImageData& Data);
	static bool JsonToElementImageData(const TSharedPtr<FJsonObject>& JsonObj, FRssElementImageData& OutData);

	static TSharedPtr<FJsonObject> ElementEffectDataToJson(const FRssElementEffectData& Data);
	static bool JsonToElementEffectData(const TSharedPtr<FJsonObject>& JsonObj, FRssElementEffectData& OutData);

	static TSharedPtr<FJsonObject> ElementSharedDataToJson(const FRssElementSharedData& Data);
	static bool JsonToElementSharedData(const TSharedPtr<FJsonObject>& JsonObj, FRssElementSharedData& OutData,
									TArray<FText>* OutWarnings = nullptr);

	static TSharedPtr<FJsonObject> SignMaterialDataToJson(const FRssSignMaterialData& Data);
	static bool JsonToSignMaterialData(const TSharedPtr<FJsonObject>& JsonObj, FRssSignMaterialData& OutData);

	static TSharedPtr<FJsonObject> CustomSignUrlDataToJson(const FRssCustomSignUrlData& Data);
	static bool JsonToCustomSignUrlData(const TSharedPtr<FJsonObject>& JsonObj, FRssCustomSignUrlData& OutData);

	static TSharedPtr<FJsonObject> Vector2DToJson(const FVector2D& Vec);
	static bool JsonToVector2D(const TSharedPtr<FJsonObject>& JsonObj, FVector2D& OutValue,
							   const FVector2D& DefaultValue = FVector2D::ZeroVector);

	static TSharedPtr<FJsonObject> Vector4ToJson(const FVector4& Vec);
	static bool JsonToVector4(const TSharedPtr<FJsonObject>& JsonObj, FVector4& OutValue,
							 const FVector4& DefaultValue = FVector4(0, 0, 0, 1));

	static TSharedPtr<FJsonObject> LinearColorToJson(const FLinearColor& Color);
	static bool JsonToLinearColor(const TSharedPtr<FJsonObject>& JsonObj, FLinearColor& OutValue,
								  const FLinearColor& DefaultValue = FLinearColor::White);

	static FString EnumToString(ESignType Value);
	static ESignType StringToESignType(const FString& Str);

	static FString EnumToString(ESignSize Value);
	static ESignSize StringToESignSize(const FString& Str);

	static FString EnumToString(ESignElementType Value);
	static ESignElementType StringToESignElementType(const FString& Str);

	static FString EnumToString(ERssTextType Value);
	static ERssTextType StringToERssTextType(const FString& Str);

	static FString EnumToString(ERssPannerType Value);
	static ERssPannerType StringToERssPannerType(const FString& Str);

	static FString EnumToString(EJust Value);
	static EJust StringToEJust(const FString& Str);

	static FString ObjectToPath(const UObject* Object);

	static UObject* PathToObject(const FString& Path, const UClass* ExpectedClass);

	static FString TextureToPath(UTexture* Texture);

	static UTexture* PathToTexture(const FString& Path);

	static FString TextToString(const FText& Text);

	static FText StringToText(const FString& Str);
};

UCLASS()
class RSS_API URssJsonSerializer : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "RSS|JSON", meta = (DisplayName = "Sign Data To JSON"))
	static FString SignDataToJson(const FRssSignData& SignData);

	UFUNCTION(BlueprintCallable, Category = "RSS|JSON",
		meta = (DisplayName = "JSON To Sign Data", ExpandBoolAsExecs = "ReturnValue"))
	static bool JsonToSignData(const FString& JsonString, FRssSignData& OutSignData);

	UFUNCTION(BlueprintCallable, Category = "RSS|JSON",
		meta = (DisplayName = "Apply JSON To RSS Sign", ExpandBoolAsExecs = "ReturnValue",
			Keywords = "RSS JSON sign import apply paste"))
	static bool ApplyJsonToSign(AActor* SignActor, const FString& JsonString, FText& OutErrors, FText& OutWarnings);
};
