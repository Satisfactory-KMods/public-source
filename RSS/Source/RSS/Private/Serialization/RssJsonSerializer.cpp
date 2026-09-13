// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Serialization/RssJsonSerializer.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/Font.h"
#include "Engine/Texture.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/SoftObjectPath.h"

#include "Buildable/RSSSignRCO.h"
#include "Interface/RssSignInterface.h"
#include "RSSModule.h"
#include "RssBlueprintFunctionLibrary.h"
#include "Widget/RssDownloadImage.h"

static constexpr int32 RSS_MAX_URL_LENGTH = 2048;
static constexpr int32 RSS_MAX_OBJECT_PATH_LENGTH = 2048;
static constexpr double RSS_JSON_SCHEMA_VERSION = 1.0;

namespace
{
	template <typename NumberType>
	bool GetNumberFieldOrDefault(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName,
								 NumberType DefaultValue, NumberType& OutValue)
	{
		OutValue = DefaultValue;
		if (!JsonObject->HasField(FieldName))
		{
			return true;
		}
		return JsonObject->HasTypedField<EJson::Number>(FieldName) &&
			JsonObject->TryGetNumberField(FieldName, OutValue);
	}

	template <typename NumberType>
	bool GetRequiredNumberField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName,
								NumberType& OutValue)
	{
		return JsonObject->HasTypedField<EJson::Number>(FieldName) &&
			JsonObject->TryGetNumberField(FieldName, OutValue);
	}

	bool GetIntegerFieldOrDefault(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName,
								  int32 DefaultValue, int32& OutValue)
	{
		double Value = DefaultValue;
		if (!GetNumberFieldOrDefault(JsonObject, FieldName, static_cast<double>(DefaultValue), Value) ||
			!FMath::IsFinite(Value) || FMath::Floor(Value) != Value || Value < MIN_int32 || Value > MAX_int32)
		{
			return false;
		}
		OutValue = static_cast<int32>(Value);
		return true;
	}

	bool GetBoolFieldOrDefault(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName, bool DefaultValue,
								   bool& OutValue)
	{
		OutValue = DefaultValue;
		if (!JsonObject->HasField(FieldName))
		{
			return true;
		}
		return JsonObject->HasTypedField<EJson::Boolean>(FieldName) &&
			JsonObject->TryGetBoolField(FieldName, OutValue);
	}

	bool GetStringFieldOrDefault(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName,
								 const TCHAR* DefaultValue, FString& OutValue)
	{
		OutValue = DefaultValue;
		if (!JsonObject->HasField(FieldName))
		{
			return true;
		}
		return JsonObject->HasTypedField<EJson::String>(FieldName) &&
			JsonObject->TryGetStringField(FieldName, OutValue);
	}

	bool GetOptionalObjectPath(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName, FString& OutPath)
	{
		OutPath.Empty();
		const TSharedPtr<FJsonValue>* Value = JsonObject->Values.Find(FString(FieldName));
		if (!Value)
		{
			return true;
		}
		if (!Value->IsValid() || (*Value)->Type == EJson::Null)
		{
			return Value->IsValid();
		}
		return JsonObject->TryGetStringField(FieldName, OutPath) &&
			OutPath.Len() <= RSS_MAX_OBJECT_PATH_LENGTH;
	}

	bool GetOptionalObject(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName,
						   TSharedPtr<FJsonObject>& OutObject)
	{
		OutObject.Reset();
		if (!JsonObject->HasField(FieldName))
		{
			return true;
		}

		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!JsonObject->TryGetObjectField(FieldName, Object) || !Object || !Object->IsValid())
		{
			return false;
		}
		OutObject = *Object;
		return true;
	}

	bool GetOptionalArray(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName,
						  const TArray<TSharedPtr<FJsonValue>>*& OutArray)
	{
		OutArray = nullptr;
		if (!JsonObject->HasField(FieldName))
		{
			return true;
		}
		return JsonObject->TryGetArrayField(FieldName, OutArray) && OutArray;
	}

	bool IsPlaceableSignTypeAndSize(ESignType SignType, ESignSize SignSize)
	{
		if (SignType == ESignType::RSS_Normal)
		{
			switch (SignSize)
			{
			case ESignSize::RSS_05x05:
			case ESignSize::RSS_1x1:
			case ESignSize::RSS_1x2:
			case ESignSize::RSS_1x7:
			case ESignSize::RSS_2x05:
			case ESignSize::RSS_2x1:
			case ESignSize::RSS_2x2:
			case ESignSize::RSS_2x3:
			case ESignSize::RSS_3x05:
			case ESignSize::RSS_4x05:
			case ESignSize::RSS_7x1:
			case ESignSize::RSS_8x4:
			case ESignSize::RSS_16x8:
			case ESignSize::RSS_25x2:
				return true;
			default:
				return false;
			}
		}

		if (SignType == ESignType::RSS_Hologram)
		{
			switch (SignSize)
			{
			case ESignSize::RSS_1x1:
			case ESignSize::RSS_1x2:
			case ESignSize::RSS_1x7:
			case ESignSize::RSS_2x1:
			case ESignSize::RSS_7x1:
			case ESignSize::RSS_25x2:
				return true;
			default:
				return false;
			}
		}

		return false;
	}

	bool HasSupportedSchemaVersion(const TSharedPtr<FJsonObject>& RootObject, FText* OutError)
	{
		double SchemaVersion = 0.0;
		if (!RootObject.IsValid() || !RootObject->TryGetNumberField(TEXT("schemaVersion"), SchemaVersion))
		{
			if (OutError)
			{
				*OutError = FI18N_NS("RSSJson_InvalidSchemaVersion",
					"RSS JSON schemaVersion is missing or invalid; expected 1.");
			}
			return false;
		}

		if (!FMath::IsFinite(SchemaVersion) || SchemaVersion != RSS_JSON_SCHEMA_VERSION)
		{
			if (OutError)
			{
				FFormatNamedArguments Arguments;
				Arguments.Add(TEXT("SchemaVersion"), FText::AsNumber(SchemaVersion));
				*OutError = FText::Format(
					FI18N_NS("RSSJson_UnsupportedSchemaVersion",
						"Unsupported RSS JSON schemaVersion {SchemaVersion}; only version 1 is supported."),
					Arguments);
			}
			return false;
		}

		return true;
	}

	FString GetElementNameOrGenerated(const FText& ElementName)
	{
		const FString ExistingName = ElementName.ToString();
		if (!ExistingName.TrimStartAndEnd().IsEmpty())
		{
			return ExistingName;
		}

		return FString::Printf(TEXT("Element %s"),
			*FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens));
	}

	FString GetLegacyWebTextureRedirect(const FString& Path)
	{
		if (!Path.StartsWith(TEXT("/RSS/Assets/Images/")))
		{
			return FString();
		}

		FString PackagePath;
		FString ObjectName;
		if (!Path.Split(TEXT("."), &PackagePath, &ObjectName, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
		{
			return FString();
		}

		FString Directory;
		FString AssetId;
		if (!PackagePath.Split(TEXT("/"), &Directory, &AssetId, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
		{
			return FString();
		}
		if (ObjectName != AssetId)
		{
			return FString();
		}

		if (AssetId.RemoveFromStart(TEXT("masks_")))
		{
			return FString::Printf(TEXT("%s/Masks/%s.%s"), *Directory, *AssetId, *AssetId);
		}

		if (Directory.EndsWith(TEXT("/Preview")) && AssetId.RemoveFromStart(TEXT("bg_")))
		{
			return FString::Printf(TEXT("%s/BG_%s.BG_%s"), *Directory, *AssetId, *AssetId);
		}

		if (!Directory.EndsWith(TEXT("/UI")) || !AssetId.RemoveFromStart(TEXT("shapes_")))
		{
			return FString();
		}

		FString Folder;
		FString AssetName;
		if (!AssetId.Split(TEXT("_"), &Folder, &AssetName))
		{
			return FString();
		}

		if (Folder.Equals(TEXT("custom"), ESearchCase::IgnoreCase))
		{
			Folder = TEXT("Custom");
		}
		else if (Folder != TEXT("1x1") && Folder != TEXT("1x2") && Folder != TEXT("2x1") &&
			Folder != TEXT("7x1"))
		{
			return FString();
		}

		return FString::Printf(TEXT("%s/Shapes/%s/%s.%s"), *Directory, *Folder, *AssetName, *AssetName);
	}

	template <typename EnumType>
	FText GetEnumDisplayName(EnumType Value)
	{
		if (const UEnum* Enum = StaticEnum<EnumType>())
		{
			return Enum->GetDisplayNameTextByValue(static_cast<int64>(Value));
		}
		return FText::AsNumber(static_cast<int64>(Value));
	}
}

FString URssJsonSerializer::SignDataToJson(const FRssSignData& SignData)
{
	return FRssJsonSerializer::SignDataToJson(SignData);
}

bool URssJsonSerializer::JsonToSignData(const FString& JsonString, FRssSignData& OutSignData)
{
	FRssSignData ParsedSignData;
	if (!FRssJsonSerializer::JsonToSignData(JsonString, ParsedSignData))
	{
		OutSignData = FRssSignData();
		return false;
	}

	OutSignData = MoveTemp(ParsedSignData);
	return true;
}

bool URssJsonSerializer::ApplyJsonToSign(AActor* SignActor, const FString& JsonString, FText& OutErrors,
										 FText& OutWarnings)
{
	TArray<FText> Errors;
	TArray<FText> Warnings;
	bool bIsRssSign = false;

	if (!IsValid(SignActor))
	{
		Errors.Add(FI18N_NS("RSSJsonApply_InvalidTarget", "Target actor is invalid."));
	}
	else if (!SignActor->GetClass()->ImplementsInterface(URssSignInterface::StaticClass()))
	{
		Errors.Add(FI18N_NS("RSSJsonApply_NotRssSign", "Target actor is not an RSS sign."));
	}
	else
	{
		bIsRssSign = true;
	}

	FRssSignData ParsedSignData;
	bool bJsonValid = false;
	if (JsonString.TrimStartAndEnd().IsEmpty())
	{
		Errors.Add(FI18N_NS("RSSJsonApply_EmptyJson", "JSON is empty."));
	}
	else
	{
		FText JsonError;
		bJsonValid = FRssJsonSerializer::JsonToSignData(JsonString, ParsedSignData, &JsonError, &Warnings);
		if (!bJsonValid)
		{
			Errors.Add(JsonError.IsEmpty()
				? FI18N_NS("RSSJsonApply_InvalidJson", "JSON could not be parsed as valid RSS sign data.")
				: JsonError);
		}
	}
	OutWarnings = Warnings.Num() > 0 ? FText::Join(FText::FromString(LINE_TERMINATOR), Warnings) : FText::GetEmpty();

	bool bSignIdentityMatches = false;
	if (bIsRssSign && bJsonValid)
	{
		const FRssSignData CurrentSignData = IRssSignInterface::Execute_GetSignData(SignActor);
		const bool bSignTypeMatches = CurrentSignData.mSignType == ParsedSignData.mSignType;
		const bool bSignSizeMatches = CurrentSignData.mSignTypeSize == ParsedSignData.mSignTypeSize;
		bSignIdentityMatches = bSignTypeMatches && bSignSizeMatches;

		if (!bSignTypeMatches)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("SignType"), GetEnumDisplayName(CurrentSignData.mSignType));
			Arguments.Add(TEXT("JsonType"), GetEnumDisplayName(ParsedSignData.mSignType));
			Errors.Add(FText::Format(
				FI18N_NS("RSSJsonApply_SignTypeMismatch",
					"Sign type mismatch: target is {SignType}; JSON is {JsonType}."),
				Arguments));
		}
		if (!bSignSizeMatches)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("SignSize"), GetEnumDisplayName(CurrentSignData.mSignTypeSize));
			Arguments.Add(TEXT("JsonSize"), GetEnumDisplayName(ParsedSignData.mSignTypeSize));
			Errors.Add(FText::Format(
				FI18N_NS("RSSJsonApply_SignSizeMismatch",
					"Sign size mismatch: target is {SignSize}; JSON is {JsonSize}."),
				Arguments));
		}
	}

	if (bIsRssSign && bJsonValid && bSignIdentityMatches && !SignActor->HasAuthority() &&
		!URSSSignRCO::Get(SignActor))
	{
		Errors.Add(FI18N_NS("RSSJsonApply_RemoteCallUnavailable",
			"RSS remote call object is unavailable; sign update cannot be sent to server."));
	}

	if (Errors.Num() > 0)
	{
		OutErrors = FText::Join(FText::FromString(LINE_TERMINATOR), Errors);
		return false;
	}

	IRssSignInterface::Execute_UpdateSignData(SignActor, ParsedSignData);
	OutErrors = FText::GetEmpty();
	return true;
}

FString FRssJsonSerializer::SignDataToJson(const FRssSignData& SignData)
{
	TSharedPtr<FJsonObject> RootObject = BuildRootDocument(SignData);
	if (!RootObject.IsValid())
	{
		return FString();
	}

	FString OutputStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputStr);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	return OutputStr;
}

bool FRssJsonSerializer::JsonToSignData(const FString& JsonString, FRssSignData& OutSignData)
{
	return JsonToSignData(JsonString, OutSignData, nullptr, nullptr);
}

bool FRssJsonSerializer::JsonToSignData(const FString& JsonString, FRssSignData& OutSignData, FText* OutError,
										TArray<FText>* OutWarnings)
{
	if (OutError)
	{
		*OutError = FText::GetEmpty();
	}
	if (OutWarnings)
	{
		OutWarnings->Reset();
	}

	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		if (OutError)
		{
			*OutError = FI18N_NS("RSSJson_InvalidSyntax", "RSS JSON syntax is invalid.");
		}
		return false;
	}

	if (!ParseRootDocument(RootObject, OutSignData, OutError, OutWarnings))
	{
		if (OutError && OutError->IsEmpty())
		{
			*OutError = FI18N_NS("RSSJson_InvalidSignData",
				"RSS JSON signData has an invalid structure or asset reference.");
		}
		return false;
	}

	if (!URssBlueprintFunctionLibrary::IsSignDataSafe(OutSignData))
	{
		if (OutError)
		{
			*OutError = FI18N_NS("RSSJson_UnsafeSignData", "RSS JSON signData contains unsafe or unsupported values.");
		}
		return false;
	}
	return true;
}

FString FRssJsonSerializer::TemplatesToJson(const TArray<FRssSignData>& Templates)
{
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetNumberField(TEXT("schemaVersion"), RSS_JSON_SCHEMA_VERSION);

	TArray<TSharedPtr<FJsonValue>> TemplateArray;
	for (const FRssSignData& Template : Templates)
	{
		TSharedPtr<FJsonObject> TemplateJson = StructToJson(Template);
		if (TemplateJson.IsValid())
		{
			TemplateArray.Add(MakeShared<FJsonValueObject>(TemplateJson));
		}
	}

	RootObject->SetArrayField(TEXT("templates"), TemplateArray);

	FString OutputStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputStr);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	return OutputStr;
}

bool FRssJsonSerializer::JsonToTemplates(const FString& JsonString, TArray<FRssSignData>& OutTemplates)
{
	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		return false;
	}
	if (!HasSupportedSchemaVersion(RootObject, nullptr))
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* TemplateArray = nullptr;
	if (!GetOptionalArray(RootObject, TEXT("templates"), TemplateArray) || !TemplateArray)
	{
		return false;
	}

	OutTemplates.Empty();
	for (const TSharedPtr<FJsonValue>& Value : *TemplateArray)
	{
		if (!Value.IsValid() || Value->Type != EJson::Object)
		{
			return false;
		}

		TSharedPtr<FJsonObject> TemplateObj = Value->AsObject();
		if (!TemplateObj.IsValid())
		{
			return false;
		}

		FRssSignData Template;
		if (!JsonToStruct(TemplateObj, Template) || !URssBlueprintFunctionLibrary::IsSignDataSafe(Template))
		{
			return false;
		}
		OutTemplates.Add(Template);
	}

	return true;
}

FString FRssJsonSerializer::CustomDataToJson(const FRssCustomDatas& CustomData)
{
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetNumberField(TEXT("schemaVersion"), RSS_JSON_SCHEMA_VERSION);

	TArray<TSharedPtr<FJsonValue>> InfoArray;
	for (const FRssCustomSignUrlData& Info : CustomData.mInformations)
	{
		TSharedPtr<FJsonObject> InfoJson = CustomSignUrlDataToJson(Info);
		if (InfoJson.IsValid())
		{
			InfoArray.Add(MakeShared<FJsonValueObject>(InfoJson));
		}
	}

	RootObject->SetArrayField(TEXT("informations"), InfoArray);

	FString OutputStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputStr);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	return OutputStr;
}

bool FRssJsonSerializer::JsonToCustomData(const FString& JsonString, FRssCustomDatas& OutCustomData)
{
	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		return false;
	}
	if (!HasSupportedSchemaVersion(RootObject, nullptr))
	{
		return false;
	}

	OutCustomData.mInformations.Empty();

	const TArray<TSharedPtr<FJsonValue>>* InfoArray = nullptr;
	if (!GetOptionalArray(RootObject, TEXT("informations"), InfoArray))
	{
		return false;
	}
	if (!InfoArray)
	{
		return true;
	}

	for (const TSharedPtr<FJsonValue>& Value : *InfoArray)
	{
		if (!Value.IsValid() || Value->Type != EJson::Object)
		{
			return false;
		}

		TSharedPtr<FJsonObject> InfoObj = Value->AsObject();
		if (!InfoObj.IsValid())
		{
			return false;
		}

		FRssCustomSignUrlData Info;
		if (!JsonToCustomSignUrlData(InfoObj, Info))
		{
			return false;
		}

		OutCustomData.mInformations.Add(Info);
	}

	return true;
}

TSharedPtr<FJsonObject> FRssJsonSerializer::BuildRootDocument(const FRssSignData& SignData)
{
	if (!IsPlaceableSignTypeAndSize(SignData.mSignType, SignData.mSignTypeSize))
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetNumberField(TEXT("schemaVersion"), RSS_JSON_SCHEMA_VERSION);

	TSharedPtr<FJsonObject> SignDataJson = StructToJson(SignData);
	if (!SignDataJson.IsValid())
	{
		return nullptr;
	}

	RootObject->SetObjectField(TEXT("signData"), SignDataJson);
	return RootObject;
}

bool FRssJsonSerializer::ParseRootDocument(const TSharedPtr<FJsonObject>& RootObject, FRssSignData& OutSignData,
											FText* OutError, TArray<FText>* OutWarnings)
{
	if (!HasSupportedSchemaVersion(RootObject, OutError))
	{
		return false;
	}

	TSharedPtr<FJsonObject> SignDataObj;
	if (!GetOptionalObject(RootObject, TEXT("signData"), SignDataObj) || !SignDataObj.IsValid())
	{
		if (OutError)
		{
			*OutError = FI18N_NS("RSSJson_MissingSignData", "RSS JSON signData is missing or invalid.");
		}
		return false;
	}
	FString DocumentName;
	if (RootObject->HasField(TEXT("name")) &&
		(!RootObject->TryGetStringField(TEXT("name"), DocumentName) || DocumentName.Len() > 128))
	{
		if (OutError)
		{
			*OutError = FI18N_NS("RSSJson_NameTooLong", "RSS JSON document name cannot exceed 128 characters.");
		}
		return false;
	}

	if (!JsonToStruct(SignDataObj, OutSignData, OutWarnings))
	{
		return false;
	}

	if (!IsPlaceableSignTypeAndSize(OutSignData.mSignType, OutSignData.mSignTypeSize))
	{
		if (OutError)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("SignType"), GetEnumDisplayName(OutSignData.mSignType));
			Arguments.Add(TEXT("SignSize"), GetEnumDisplayName(OutSignData.mSignTypeSize));
			*OutError = FText::Format(
				FI18N_NS("RSSJson_UnsupportedPlaceableSign",
					"RSS JSON sign type {SignType} does not support size {SignSize} for a placeable sign."),
				Arguments);
		}
		return false;
	}

	return true;
}

TSharedPtr<FJsonObject> FRssJsonSerializer::StructToJson(const FRssSignData& SignData)
{
	if (!URssBlueprintFunctionLibrary::IsSignDataSafe(SignData))
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();

	JsonObj->SetStringField(TEXT("signType"), EnumToString(SignData.mSignType));
	JsonObj->SetStringField(TEXT("signTypeSize"), EnumToString(SignData.mSignTypeSize));

	JsonObj->SetObjectField(TEXT("hologramData"), HologramDataToJson(SignData.mHologramData));
	JsonObj->SetObjectField(TEXT("flatData"), FlatDataToJson(SignData.mFlatData));
	JsonObj->SetObjectField(TEXT("roundedData"), RoundedDataToJson(SignData.mRoundedData));

	TArray<TSharedPtr<FJsonValue>> ElementArray;
	for (const FRssElement& Element : SignData.mElements)
	{
		TSharedPtr<FJsonObject> ElementJson = ElementToJson(Element);
		if (ElementJson.IsValid())
		{
			ElementArray.Add(MakeShared<FJsonValueObject>(ElementJson));
		}
	}
	JsonObj->SetArrayField(TEXT("elements"), ElementArray);

	JsonObj->SetObjectField(TEXT("materialData"), SignMaterialDataToJson(SignData.mMaterialData));
	JsonObj->SetObjectField(TEXT("templateData"), TemplateDataToJson(SignData.mTemplateData));

	return JsonObj;
}

bool FRssJsonSerializer::JsonToStruct(const TSharedPtr<FJsonObject>& JsonObj, FRssSignData& OutSignData,
										 TArray<FText>* OutWarnings)
{
	if (!JsonObj.IsValid())
	{
		return false;
	}

	OutSignData = FRssSignData();

	FString SignType;
	if (JsonObj->HasField(TEXT("signType")))
	{
		if (!JsonObj->TryGetStringField(TEXT("signType"), SignType))
		{
			return false;
		}
		const ESignType ParsedSignType = StringToESignType(SignType);
		if (ParsedSignType == ESignType::RSS_InValid && SignType != TEXT("RSS_InValid"))
		{
			return false;
		}
		OutSignData.mSignType = ParsedSignType;
	}

	FString SignSize;
	if (JsonObj->HasField(TEXT("signTypeSize")))
	{
		if (!JsonObj->TryGetStringField(TEXT("signTypeSize"), SignSize))
		{
			return false;
		}
		const ESignSize ParsedSignSize = StringToESignSize(SignSize);
		if (ParsedSignSize == ESignSize::RSS_InValid && SignSize != TEXT("RSS_InValid"))
		{
			return false;
		}
		OutSignData.mSignTypeSize = ParsedSignSize;
	}

	TSharedPtr<FJsonObject> ChildObject;
	if (!GetOptionalObject(JsonObj, TEXT("hologramData"), ChildObject) ||
		(ChildObject.IsValid() && !JsonToHologramData(ChildObject, OutSignData.mHologramData)))
	{
		return false;
	}
	if (!GetOptionalObject(JsonObj, TEXT("flatData"), ChildObject) ||
		(ChildObject.IsValid() && !JsonToFlatData(ChildObject, OutSignData.mFlatData)))
	{
		return false;
	}
	if (!GetOptionalObject(JsonObj, TEXT("roundedData"), ChildObject) ||
		(ChildObject.IsValid() && !JsonToRoundedData(ChildObject, OutSignData.mRoundedData)))
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* ElementArray = nullptr;
	if (!GetOptionalArray(JsonObj, TEXT("elements"), ElementArray))
	{
		return false;
	}
	if (ElementArray)
	{
		if (ElementArray->Num() > 128)
		{
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *ElementArray)
		{
			if (!Value.IsValid() || Value->Type != EJson::Object)
			{
				return false;
			}

			TSharedPtr<FJsonObject> ElementObj = Value->AsObject();
			if (!ElementObj.IsValid())
			{
				return false;
			}

			FRssElement Element;
			if (!JsonToElement(ElementObj, Element, OutWarnings))
			{
				return false;
			}

			OutSignData.mElements.Add(Element);
		}
	}

	if (!GetOptionalObject(JsonObj, TEXT("materialData"), ChildObject) ||
		(ChildObject.IsValid() && !JsonToSignMaterialData(ChildObject, OutSignData.mMaterialData)))
	{
		return false;
	}
	if (!GetOptionalObject(JsonObj, TEXT("templateData"), ChildObject) ||
		(ChildObject.IsValid() && !JsonToTemplateData(ChildObject, OutSignData.mTemplateData)))
	{
		return false;
	}

	return true;
}

TSharedPtr<FJsonObject> FRssJsonSerializer::HologramDataToJson(const FRssHologramData& Data)
{
	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetBoolField(TEXT("enable"), Data.mEnable);
	JsonObj->SetNumberField(TEXT("distortionIntensity"), Data.mDistortionIntensity);
	JsonObj->SetNumberField(TEXT("scanlineIntensity"), Data.mScanlineIntensity);
	JsonObj->SetNumberField(TEXT("borderIntensity"), Data.mBorderIntensity);
	JsonObj->SetBoolField(TEXT("useScanlines"), Data.mUseScanlines);
	return JsonObj;
}

bool FRssJsonSerializer::JsonToHologramData(const TSharedPtr<FJsonObject>& JsonObj, FRssHologramData& OutData)
{
	if (!JsonObj.IsValid())
	{
		return false;
	}

	if (!JsonObj->TryGetBoolField(TEXT("enable"), OutData.mEnable))
	{
		return false;
	}
	return GetNumberFieldOrDefault(JsonObj, TEXT("distortionIntensity"), 0.01f,
			OutData.mDistortionIntensity) &&
		GetNumberFieldOrDefault(JsonObj, TEXT("scanlineIntensity"), 1.0f, OutData.mScanlineIntensity) &&
		GetNumberFieldOrDefault(JsonObj, TEXT("borderIntensity"), 10.0f, OutData.mBorderIntensity) &&
		GetBoolFieldOrDefault(JsonObj, TEXT("useScanlines"), true, OutData.mUseScanlines);
}

TSharedPtr<FJsonObject> FRssJsonSerializer::FlatDataToJson(const FRssFlatData& Data)
{
	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetBoolField(TEXT("enable"), Data.mEnable);
	JsonObj->SetBoolField(TEXT("useParallax"), Data.mUseParallax);
	JsonObj->SetBoolField(TEXT("useArrowMaterial"), Data.mUseArrowMaterial);
	JsonObj->SetNumberField(TEXT("overwriteParalaxVerticalRatio"),
		Data.mOverwriteParalaxVerticalRatio < 0.0f ? -1.0f : Data.mOverwriteParalaxVerticalRatio);
	JsonObj->SetNumberField(TEXT("overwriteParalaxHorizontalRatio"),
		Data.mOverwriteParalaxHorizontalRatio < 0.0f ? -1.0f : Data.mOverwriteParalaxHorizontalRatio);
	return JsonObj;
}

bool FRssJsonSerializer::JsonToFlatData(const TSharedPtr<FJsonObject>& JsonObj, FRssFlatData& OutData)
{
	if (!JsonObj.IsValid())
	{
		return false;
	}

	if (!JsonObj->TryGetBoolField(TEXT("enable"), OutData.mEnable))
	{
		return false;
	}
	if (!GetBoolFieldOrDefault(JsonObj, TEXT("useParallax"), false, OutData.mUseParallax) ||
		!GetBoolFieldOrDefault(JsonObj, TEXT("useArrowMaterial"), false, OutData.mUseArrowMaterial) ||
		!GetNumberFieldOrDefault(JsonObj, TEXT("overwriteParalaxVerticalRatio"), -1.0f,
			OutData.mOverwriteParalaxVerticalRatio) ||
		!GetNumberFieldOrDefault(JsonObj, TEXT("overwriteParalaxHorizontalRatio"), -1.0f,
			OutData.mOverwriteParalaxHorizontalRatio))
	{
		return false;
	}
	if (OutData.mOverwriteParalaxVerticalRatio >= -1.0f && OutData.mOverwriteParalaxVerticalRatio < 0.0f)
	{
		OutData.mOverwriteParalaxVerticalRatio = -1.0f;
	}
	if (OutData.mOverwriteParalaxHorizontalRatio >= -1.0f && OutData.mOverwriteParalaxHorizontalRatio < 0.0f)
	{
		OutData.mOverwriteParalaxHorizontalRatio = -1.0f;
	}
	return true;
}

TSharedPtr<FJsonObject> FRssJsonSerializer::RoundedDataToJson(const FRssRoundedData& Data)
{
	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetBoolField(TEXT("enable"), Data.mEnable);
	JsonObj->SetNumberField(TEXT("roationSpeed"), Data.mRoationSpeed);
	return JsonObj;
}

bool FRssJsonSerializer::JsonToRoundedData(const TSharedPtr<FJsonObject>& JsonObj, FRssRoundedData& OutData)
{
	if (!JsonObj.IsValid())
	{
		return false;
	}

	if (!JsonObj->TryGetBoolField(TEXT("enable"), OutData.mEnable))
	{
		return false;
	}
	return GetNumberFieldOrDefault(JsonObj, TEXT("roationSpeed"), 3.0f, OutData.mRoationSpeed);
}

TSharedPtr<FJsonObject> FRssJsonSerializer::TemplateDataToJson(const FRssTemplateData& Data)
{
	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetBoolField(TEXT("enable"), Data.mEnable);
	JsonObj->SetStringField(TEXT("templateName"), TextToString(Data.mTemplateName));
	return JsonObj;
}

bool FRssJsonSerializer::JsonToTemplateData(const TSharedPtr<FJsonObject>& JsonObj, FRssTemplateData& OutData)
{
	if (!JsonObj.IsValid())
	{
		return false;
	}

	if (!JsonObj->TryGetBoolField(TEXT("enable"), OutData.mEnable))
	{
		return false;
	}
	FString TemplateName;
	if (!GetStringFieldOrDefault(JsonObj, TEXT("templateName"), TEXT(""), TemplateName))
	{
		return false;
	}
	OutData.mTemplateName = StringToText(TemplateName);
	return true;
}

TSharedPtr<FJsonObject> FRssJsonSerializer::ElementToJson(const FRssElement& Element)
{
	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(TEXT("elementType"), EnumToString(Element.mElementType));
	JsonObj->SetObjectField(TEXT("textData"), ElementTextDataToJson(Element.mTextData));
	JsonObj->SetObjectField(TEXT("imageData"), ElementImageDataToJson(Element.mImageData));
	JsonObj->SetObjectField(TEXT("effectData"), ElementEffectDataToJson(Element.mEffectData));
	JsonObj->SetObjectField(TEXT("sharedData"), ElementSharedDataToJson(Element.mSharedData));
	return JsonObj;
}

bool FRssJsonSerializer::JsonToElement(const TSharedPtr<FJsonObject>& JsonObj, FRssElement& OutElement,
									  TArray<FText>* OutWarnings)
{
	if (!JsonObj.IsValid())
	{
		return false;
	}

	OutElement = FRssElement();
	FString ElementType;
	if (!JsonObj->TryGetStringField(TEXT("elementType"), ElementType) ||
		(ElementType != TEXT("Text") && ElementType != TEXT("Effect") && ElementType != TEXT("Image")))
	{
		return false;
	}
	OutElement.mElementType = StringToESignElementType(ElementType);

	if (JsonObj->HasField(TEXT("textData")))
	{
		TSharedPtr<FJsonObject> TextData;
		if (!GetOptionalObject(JsonObj, TEXT("textData"), TextData) ||
			!JsonToElementTextData(TextData, OutElement.mTextData))
		{
			return false;
		}
	}

	if (JsonObj->HasField(TEXT("imageData")))
	{
		TSharedPtr<FJsonObject> ImageData;
		if (!GetOptionalObject(JsonObj, TEXT("imageData"), ImageData) ||
			!JsonToElementImageData(ImageData, OutElement.mImageData))
		{
			return false;
		}
	}

	if (JsonObj->HasField(TEXT("effectData")))
	{
		TSharedPtr<FJsonObject> EffectData;
		if (!GetOptionalObject(JsonObj, TEXT("effectData"), EffectData) ||
			!JsonToElementEffectData(EffectData, OutElement.mEffectData))
		{
			return false;
		}
	}

	if (JsonObj->HasField(TEXT("sharedData")))
	{
		TSharedPtr<FJsonObject> SharedData;
		if (!GetOptionalObject(JsonObj, TEXT("sharedData"), SharedData) ||
			!JsonToElementSharedData(SharedData, OutElement.mSharedData, OutWarnings))
		{
			return false;
		}
	}

	OutElement.mSharedData.mElementName =
		FText::FromString(GetElementNameOrGenerated(OutElement.mSharedData.mElementName));

	return true;
}

TSharedPtr<FJsonObject> FRssJsonSerializer::ElementTextDataToJson(const FRssElementTextData& Data)
{
	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(TEXT("text"), TextToString(Data.mText));
	JsonObj->SetStringField(TEXT("textType"), EnumToString(Data.mTextType));
	JsonObj->SetObjectField(TEXT("backgroundColour"), LinearColorToJson(Data.mBackgroundColour));
	JsonObj->SetBoolField(TEXT("isBold"), Data.mIsBold);
	JsonObj->SetBoolField(TEXT("isUppercase"), Data.mIsUppercase);
	JsonObj->SetNumberField(TEXT("textSize"), Data.mTextSize);
	JsonObj->SetNumberField(TEXT("letterSpacing"), Data.mLetterSpacing);
	JsonObj->SetNumberField(TEXT("lineHeight"), Data.mLineHeight);
	JsonObj->SetObjectField(TEXT("padding"), Vector4ToJson(Data.mPadding));
	JsonObj->SetStringField(TEXT("textJustify"), EnumToString(Data.mTextJustify));
	JsonObj->SetStringField(TEXT("font"), ObjectToPath(Data.mFont));
	return JsonObj;
}

bool FRssJsonSerializer::JsonToElementTextData(const TSharedPtr<FJsonObject>& JsonObj, FRssElementTextData& OutData)
{
	if (!JsonObj.IsValid())
	{
		return false;
	}

	FString Text;
	FString TextType;
	if (!GetStringFieldOrDefault(JsonObj, TEXT("text"), TEXT(""), Text) ||
		!GetStringFieldOrDefault(JsonObj, TEXT("textType"), TEXT("NormalText"), TextType) ||
		(TextType != TEXT("NormalText") && TextType != TEXT("BackgroundText")))
	{
		return false;
	}
	OutData.mText = StringToText(Text);
	OutData.mTextType = StringToERssTextType(TextType);
	TSharedPtr<FJsonObject> ChildObject;
	if (!GetOptionalObject(JsonObj, TEXT("backgroundColour"), ChildObject))
	{
		return false;
	}
	if (!JsonToLinearColor(ChildObject, OutData.mBackgroundColour, FLinearColor::Black) ||
		!GetBoolFieldOrDefault(JsonObj, TEXT("isBold"), false, OutData.mIsBold) ||
		!GetBoolFieldOrDefault(JsonObj, TEXT("isUppercase"), false, OutData.mIsUppercase) ||
		!GetIntegerFieldOrDefault(JsonObj, TEXT("textSize"), 20, OutData.mTextSize) ||
		!GetIntegerFieldOrDefault(JsonObj, TEXT("letterSpacing"), 0, OutData.mLetterSpacing) ||
		!GetNumberFieldOrDefault(JsonObj, TEXT("lineHeight"), 1.0f, OutData.mLineHeight))
	{
		return false;
	}
	if (!GetOptionalObject(JsonObj, TEXT("padding"), ChildObject))
	{
		return false;
	}
	FString TextJustify;
	if (!JsonToVector4(ChildObject, OutData.mPadding, FVector4(20, 10, 20, 10)) ||
		!GetStringFieldOrDefault(JsonObj, TEXT("textJustify"), TEXT("RSS_Right"), TextJustify) ||
		(TextJustify != TEXT("RSS_Left") && TextJustify != TEXT("RSS_Middle") && TextJustify != TEXT("RSS_Right")))
	{
		return false;
	}
	OutData.mTextJustify = StringToEJust(TextJustify);

	FString FontPath;
	if (!GetOptionalObjectPath(JsonObj, TEXT("font"), FontPath))
	{
		return false;
	}
	OutData.mFont = PathToObject(FontPath, UFont::StaticClass());
	return FontPath.IsEmpty() || IsValid(OutData.mFont);
}

TSharedPtr<FJsonObject> FRssJsonSerializer::ElementImageDataToJson(const FRssElementImageData& Data)
{
	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetObjectField(TEXT("imageSize"), Vector2DToJson(Data.mImageSize));
	JsonObj->SetObjectField(TEXT("scaleMirrow"), Vector2DToJson(Data.mScaleMirrow));
	JsonObj->SetObjectField(TEXT("overwriteImageSize"), Vector2DToJson(Data.mOverwriteImageSize));
	JsonObj->SetObjectField(TEXT("use9SliceMode"), Vector2DToJson(Data.mUse9SliceMode));
	return JsonObj;
}

bool FRssJsonSerializer::JsonToElementImageData(const TSharedPtr<FJsonObject>& JsonObj, FRssElementImageData& OutData)
{
	if (!JsonObj.IsValid())
	{
		return false;
	}

	TSharedPtr<FJsonObject> ChildObject;
	if (!GetOptionalObject(JsonObj, TEXT("imageSize"), ChildObject))
	{
		return false;
	}

	if (!JsonToVector2D(ChildObject, OutData.mImageSize, FVector2D(1, 0)))
	{
		return false;
	}
	if (!GetOptionalObject(JsonObj, TEXT("scaleMirrow"), ChildObject))
	{
		return false;
	}
	if (!JsonToVector2D(ChildObject, OutData.mScaleMirrow, FVector2D(1, 1)))
	{
		return false;
	}
	if (!GetOptionalObject(JsonObj, TEXT("overwriteImageSize"), ChildObject))
	{
		return false;
	}
	if (!JsonToVector2D(ChildObject, OutData.mOverwriteImageSize))
	{
		return false;
	}
	if (!GetOptionalObject(JsonObj, TEXT("use9SliceMode"), ChildObject))
	{
		return false;
	}
	return JsonToVector2D(ChildObject, OutData.mUse9SliceMode, FVector2D(0, 0.25f));
}

TSharedPtr<FJsonObject> FRssJsonSerializer::ElementEffectDataToJson(const FRssElementEffectData& Data)
{
	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(TEXT("pannerType"), EnumToString(Data.mPannerType));
	JsonObj->SetNumberField(TEXT("stepFrequenz"), Data.mStepFrequenz);
	JsonObj->SetObjectField(TEXT("scaleAndSpeed"), LinearColorToJson(Data.mScaleAndSpeed));
	return JsonObj;
}

bool FRssJsonSerializer::JsonToElementEffectData(const TSharedPtr<FJsonObject>& JsonObj, FRssElementEffectData& OutData)
{
	if (!JsonObj.IsValid())
	{
		return false;
	}

	FString PannerType;
	if (!GetStringFieldOrDefault(JsonObj, TEXT("pannerType"), TEXT("RSS_Linear"), PannerType) ||
		(PannerType != TEXT("RSS_Linear") && PannerType != TEXT("RSS_Step")))
	{
		return false;
	}
	OutData.mPannerType = StringToERssPannerType(PannerType);
	if (!GetNumberFieldOrDefault(JsonObj, TEXT("stepFrequenz"), 5.0f, OutData.mStepFrequenz))
	{
		return false;
	}
	TSharedPtr<FJsonObject> ScaleAndSpeed;
	if (!GetOptionalObject(JsonObj, TEXT("scaleAndSpeed"), ScaleAndSpeed))
	{
		return false;
	}
	return JsonToLinearColor(ScaleAndSpeed, OutData.mScaleAndSpeed,
		FLinearColor(0.1f, 0.1f, 5.0f, 5.0f));
}

TSharedPtr<FJsonObject> FRssJsonSerializer::ElementSharedDataToJson(const FRssElementSharedData& Data)
{
	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(TEXT("elementName"), GetElementNameOrGenerated(Data.mElementName));
	JsonObj->SetObjectField(TEXT("position"), Vector2DToJson(Data.mPosition));
	JsonObj->SetObjectField(TEXT("colourOverwrite"), LinearColorToJson(Data.mColourOverwrite));
	JsonObj->SetNumberField(TEXT("zIndex"), Data.mZIndex);
	JsonObj->SetNumberField(TEXT("opacity"), Data.mOpacity);
	JsonObj->SetNumberField(TEXT("rotation"), Data.mRotation);
	JsonObj->SetStringField(TEXT("texture"), TextureToPath(Data.mTexture));
	JsonObj->SetStringField(TEXT("url"), Data.mUrl);
	JsonObj->SetBoolField(TEXT("isUsingCustom"), Data.mIsUsingCustom);
	return JsonObj;
}

bool FRssJsonSerializer::JsonToElementSharedData(const TSharedPtr<FJsonObject>& JsonObj, FRssElementSharedData& OutData,
													 TArray<FText>* OutWarnings)
{
	if (!JsonObj.IsValid())
	{
		return false;
	}

	FString ElementName;
	if (!GetStringFieldOrDefault(JsonObj, TEXT("elementName"), TEXT(""), ElementName))
	{
		return false;
	}
	OutData.mElementName = StringToText(ElementName);
	TSharedPtr<FJsonObject> ChildObject;
	if (!GetOptionalObject(JsonObj, TEXT("position"), ChildObject))
	{
		return false;
	}
	if (!JsonToVector2D(ChildObject, OutData.mPosition))
	{
		return false;
	}
	if (!GetOptionalObject(JsonObj, TEXT("colourOverwrite"), ChildObject))
	{
		return false;
	}
	if (!JsonToLinearColor(ChildObject, OutData.mColourOverwrite, FLinearColor::White) ||
		!GetIntegerFieldOrDefault(JsonObj, TEXT("zIndex"), 0, OutData.mZIndex) ||
		!GetNumberFieldOrDefault(JsonObj, TEXT("opacity"), 1.0f, OutData.mOpacity) ||
		!GetNumberFieldOrDefault(JsonObj, TEXT("rotation"), 0.0f, OutData.mRotation))
	{
		return false;
	}
	FString TexturePath;
	FString CustomTexturePath;
	if (!GetOptionalObjectPath(JsonObj, TEXT("texture"), TexturePath) ||
		!GetOptionalObjectPath(JsonObj, TEXT("customTexture"), CustomTexturePath))
	{
		return false;
	}
	OutData.mTexture = PathToTexture(TexturePath);
	if (!TexturePath.IsEmpty() && !IsValid(OutData.mTexture))
	{
		OutData.mTexture = nullptr;
		if (OutWarnings)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("TexturePath"), FText::FromString(TexturePath));
			OutWarnings->Add(FText::Format(
				FI18N_NS("RSSJson_MissingTexture",
					"Texture {TexturePath} could not be loaded and was replaced with an empty texture."),
				Arguments));
		}
	}

	if (!GetStringFieldOrDefault(JsonObj, TEXT("url"), TEXT(""), OutData.mUrl))
	{
		return false;
	}

	OutData.mCustomTexture = nullptr;
	if (!GetBoolFieldOrDefault(JsonObj, TEXT("isUsingCustom"), false, OutData.mIsUsingCustom))
	{
		return false;
	}

	if (OutData.mIsUsingCustom && !URssDownloadImage::IsStructurallySafeRemoteImageUrl(OutData.mUrl))
	{
		return false;
	}

	return true;
}

TSharedPtr<FJsonObject> FRssJsonSerializer::SignMaterialDataToJson(const FRssSignMaterialData& Data)
{
	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetNumberField(TEXT("emissiveIntensity"), Data.mEmissiveIntensity);
	JsonObj->SetObjectField(TEXT("backgroundColor"), LinearColorToJson(Data.mBackgroundColor));
	JsonObj->SetObjectField(TEXT("rotation"), Vector4ToJson(Data.mRotation));
	return JsonObj;
}

bool FRssJsonSerializer::JsonToSignMaterialData(const TSharedPtr<FJsonObject>& JsonObj, FRssSignMaterialData& OutData)
{
	if (!JsonObj.IsValid())
	{
		return false;
	}

	if (!GetNumberFieldOrDefault(JsonObj, TEXT("emissiveIntensity"), 0.3f, OutData.mEmissiveIntensity))
	{
		return false;
	}
	TSharedPtr<FJsonObject> ChildObject;
	if (!GetOptionalObject(JsonObj, TEXT("backgroundColor"), ChildObject))
	{
		return false;
	}
	if (!JsonToLinearColor(ChildObject, OutData.mBackgroundColor,
			FLinearColor(0.039216f, 0.039216f, 0.039216f, 1.0f)))
	{
		return false;
	}
	if (!GetOptionalObject(JsonObj, TEXT("rotation"), ChildObject))
	{
		return false;
	}
	return JsonToVector4(ChildObject, OutData.mRotation, FVector4(0, 0, 0, 1));
}

TSharedPtr<FJsonObject> FRssJsonSerializer::CustomSignUrlDataToJson(const FRssCustomSignUrlData& Data)
{
	const FString TexturePath = TextureToPath(Data.mTexture);
	if (Data.mUrl.Len() > RSS_MAX_URL_LENGTH || TexturePath.Len() > RSS_MAX_OBJECT_PATH_LENGTH)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(TEXT("url"), Data.mUrl);
	JsonObj->SetStringField(TEXT("texture"), TexturePath);

	TArray<TSharedPtr<FJsonValue>> SizeArray;
	for (ESignSize Size : Data.mAllowedInSigns)
	{
		const FString SerializedSize = EnumToString(Size);
		if (SerializedSize == TEXT("RSS_InValid"))
		{
			return nullptr;
		}
		SizeArray.Add(MakeShared<FJsonValueString>(SerializedSize));
	}
	JsonObj->SetArrayField(TEXT("allowedInSigns"), SizeArray);

	return JsonObj;
}

bool FRssJsonSerializer::JsonToCustomSignUrlData(const TSharedPtr<FJsonObject>& JsonObj, FRssCustomSignUrlData& OutData)
{
	if (!JsonObj.IsValid())
	{
		return false;
	}

	if (!GetStringFieldOrDefault(JsonObj, TEXT("url"), TEXT(""), OutData.mUrl))
	{
		return false;
	}
	FString TexturePath;
	if (OutData.mUrl.Len() > RSS_MAX_URL_LENGTH ||
		!GetOptionalObjectPath(JsonObj, TEXT("texture"), TexturePath))
	{
		return false;
	}
	OutData.mTexture = PathToTexture(TexturePath);

	const TArray<TSharedPtr<FJsonValue>>* SizeArray = nullptr;
	if (!GetOptionalArray(JsonObj, TEXT("allowedInSigns"), SizeArray))
	{
		return false;
	}
	if (SizeArray)
	{
		for (const TSharedPtr<FJsonValue>& Value : *SizeArray)
		{
			if (!Value.IsValid() || Value->Type != EJson::String)
			{
				return false;
			}
			const ESignSize Size = StringToESignSize(Value->AsString());
			if (Size == ESignSize::RSS_InValid)
			{
				return false;
			}
			OutData.mAllowedInSigns.Add(Size);
		}
	}

	return true;
}

TSharedPtr<FJsonObject> FRssJsonSerializer::Vector2DToJson(const FVector2D& Vec)
{
	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetNumberField(TEXT("x"), Vec.X);
	JsonObj->SetNumberField(TEXT("y"), Vec.Y);
	return JsonObj;
}

bool FRssJsonSerializer::JsonToVector2D(const TSharedPtr<FJsonObject>& JsonObj, FVector2D& OutValue,
										const FVector2D& DefaultValue)
{
	OutValue = DefaultValue;
	if (!JsonObj.IsValid())
	{
		return true;
	}

	return GetRequiredNumberField(JsonObj, TEXT("x"), OutValue.X) &&
		GetRequiredNumberField(JsonObj, TEXT("y"), OutValue.Y);
}

TSharedPtr<FJsonObject> FRssJsonSerializer::Vector4ToJson(const FVector4& Vec)
{
	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetNumberField(TEXT("x"), Vec.X);
	JsonObj->SetNumberField(TEXT("y"), Vec.Y);
	JsonObj->SetNumberField(TEXT("z"), Vec.Z);
	JsonObj->SetNumberField(TEXT("w"), Vec.W);
	return JsonObj;
}

bool FRssJsonSerializer::JsonToVector4(const TSharedPtr<FJsonObject>& JsonObj, FVector4& OutValue,
									  const FVector4& DefaultValue)
{
	OutValue = DefaultValue;
	if (!JsonObj.IsValid())
	{
		return true;
	}

	return GetRequiredNumberField(JsonObj, TEXT("x"), OutValue.X) &&
		GetRequiredNumberField(JsonObj, TEXT("y"), OutValue.Y) &&
		GetRequiredNumberField(JsonObj, TEXT("z"), OutValue.Z) &&
		GetRequiredNumberField(JsonObj, TEXT("w"), OutValue.W);
}

TSharedPtr<FJsonObject> FRssJsonSerializer::LinearColorToJson(const FLinearColor& Color)
{
	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetNumberField(TEXT("r"), Color.R);
	JsonObj->SetNumberField(TEXT("g"), Color.G);
	JsonObj->SetNumberField(TEXT("b"), Color.B);
	JsonObj->SetNumberField(TEXT("a"), Color.A);
	return JsonObj;
}

bool FRssJsonSerializer::JsonToLinearColor(const TSharedPtr<FJsonObject>& JsonObj, FLinearColor& OutValue,
										 const FLinearColor& DefaultValue)
{
	OutValue = DefaultValue;
	if (!JsonObj.IsValid())
	{
		return true;
	}

	return GetRequiredNumberField(JsonObj, TEXT("r"), OutValue.R) &&
		GetRequiredNumberField(JsonObj, TEXT("g"), OutValue.G) &&
		GetRequiredNumberField(JsonObj, TEXT("b"), OutValue.B) &&
		GetRequiredNumberField(JsonObj, TEXT("a"), OutValue.A);
}

FString FRssJsonSerializer::EnumToString(ESignType Value)
{
	switch (Value)
	{
	case ESignType::RSS_InValid:
		return TEXT("RSS_InValid");
	case ESignType::RSS_Normal:
		return TEXT("RSS_Normal");
	case ESignType::RSS_Hologram:
		return TEXT("RSS_Hologram");
	case ESignType::RSS_Secret:
		return TEXT("RSS_Secret");
	case ESignType::RSS_Decal:
		return TEXT("RSS_Decal");
	default:
		return TEXT("RSS_InValid");
	}
}

ESignType FRssJsonSerializer::StringToESignType(const FString& Str)
{
	if (Str == TEXT("RSS_Normal"))
		return ESignType::RSS_Normal;
	if (Str == TEXT("RSS_Hologram"))
		return ESignType::RSS_Hologram;
	if (Str == TEXT("RSS_Secret"))
		return ESignType::RSS_Secret;
	if (Str == TEXT("RSS_Decal"))
		return ESignType::RSS_Decal;
	return ESignType::RSS_InValid;
}

FString FRssJsonSerializer::EnumToString(ESignSize Value)
{
	switch (Value)
	{
	case ESignSize::RSS_InValid:
		return TEXT("RSS_InValid");
	case ESignSize::RSS_05x05:
		return TEXT("RSS_05x05");
	case ESignSize::RSS_1x1:
		return TEXT("RSS_1x1");
	case ESignSize::RSS_1x2:
		return TEXT("RSS_1x2");
	case ESignSize::RSS_1x7:
		return TEXT("RSS_1x7");
	case ESignSize::RSS_2x05:
		return TEXT("RSS_2x05");
	case ESignSize::RSS_2x1:
		return TEXT("RSS_2x1");
	case ESignSize::RSS_2x2:
		return TEXT("RSS_2x2");
	case ESignSize::RSS_2x3:
		return TEXT("RSS_2x3");
	case ESignSize::RSS_3x05:
		return TEXT("RSS_3x05");
	case ESignSize::RSS_3x1:
		return TEXT("RSS_3x1");
	case ESignSize::RSS_4x05:
		return TEXT("RSS_4x05");
	case ESignSize::RSS_4x1:
		return TEXT("RSS_4x1");
	case ESignSize::RSS_7x1:
		return TEXT("RSS_7x1");
	case ESignSize::RSS_8x4:
		return TEXT("RSS_8x4");
	case ESignSize::RSS_16x8:
		return TEXT("RSS_16x8");
	case ESignSize::RSS_25x2:
		return TEXT("RSS_25x2");
	case ESignSize::RSS_Custom:
		return TEXT("RSS_Custom");
	case ESignSize::RSS_Secret:
		return TEXT("RSS_Secret");
	default:
		return TEXT("RSS_InValid");
	}
}

ESignSize FRssJsonSerializer::StringToESignSize(const FString& Str)
{
	if (Str == TEXT("RSS_05x05"))
		return ESignSize::RSS_05x05;
	if (Str == TEXT("RSS_1x1"))
		return ESignSize::RSS_1x1;
	if (Str == TEXT("RSS_1x2"))
		return ESignSize::RSS_1x2;
	if (Str == TEXT("RSS_1x7"))
		return ESignSize::RSS_1x7;
	if (Str == TEXT("RSS_2x05"))
		return ESignSize::RSS_2x05;
	if (Str == TEXT("RSS_2x1"))
		return ESignSize::RSS_2x1;
	if (Str == TEXT("RSS_2x2"))
		return ESignSize::RSS_2x2;
	if (Str == TEXT("RSS_2x3"))
		return ESignSize::RSS_2x3;
	if (Str == TEXT("RSS_3x05"))
		return ESignSize::RSS_3x05;
	if (Str == TEXT("RSS_3x1"))
		return ESignSize::RSS_3x1;
	if (Str == TEXT("RSS_4x05"))
		return ESignSize::RSS_4x05;
	if (Str == TEXT("RSS_4x1"))
		return ESignSize::RSS_4x1;
	if (Str == TEXT("RSS_7x1"))
		return ESignSize::RSS_7x1;
	if (Str == TEXT("RSS_8x4"))
		return ESignSize::RSS_8x4;
	if (Str == TEXT("RSS_16x8"))
		return ESignSize::RSS_16x8;
	if (Str == TEXT("RSS_25x2"))
		return ESignSize::RSS_25x2;
	if (Str == TEXT("RSS_Custom"))
		return ESignSize::RSS_Custom;
	if (Str == TEXT("RSS_Secret"))
		return ESignSize::RSS_Secret;
	return ESignSize::RSS_InValid;
}

FString FRssJsonSerializer::EnumToString(ESignElementType Value)
{
	switch (Value)
	{
	case ESignElementType::Text:
		return TEXT("Text");
	case ESignElementType::Effect:
		return TEXT("Effect");
	case ESignElementType::Image:
		return TEXT("Image");
	default:
		return TEXT("Text");
	}
}

ESignElementType FRssJsonSerializer::StringToESignElementType(const FString& Str)
{
	if (Str == TEXT("Effect"))
		return ESignElementType::Effect;
	if (Str == TEXT("Image"))
		return ESignElementType::Image;
	return ESignElementType::Text;
}

FString FRssJsonSerializer::EnumToString(ERssTextType Value)
{
	switch (Value)
	{
	case ERssTextType::NormalText:
		return TEXT("NormalText");
	case ERssTextType::BackgroundText:
		return TEXT("BackgroundText");
	default:
		return TEXT("NormalText");
	}
}

ERssTextType FRssJsonSerializer::StringToERssTextType(const FString& Str)
{
	if (Str == TEXT("BackgroundText"))
		return ERssTextType::BackgroundText;
	return ERssTextType::NormalText;
}

FString FRssJsonSerializer::EnumToString(ERssPannerType Value)
{
	switch (Value)
	{
	case ERssPannerType::RSS_Linear:
		return TEXT("RSS_Linear");
	case ERssPannerType::RSS_Step:
		return TEXT("RSS_Step");
	default:
		return TEXT("RSS_Linear");
	}
}

ERssPannerType FRssJsonSerializer::StringToERssPannerType(const FString& Str)
{
	if (Str == TEXT("RSS_Step"))
		return ERssPannerType::RSS_Step;
	return ERssPannerType::RSS_Linear;
}

FString FRssJsonSerializer::EnumToString(EJust Value)
{
	switch (Value)
	{
	case EJust::RSS_Left:
		return TEXT("RSS_Left");
	case EJust::RSS_Middle:
		return TEXT("RSS_Middle");
	case EJust::RSS_Right:
		return TEXT("RSS_Right");
	default:
		return TEXT("RSS_Right");
	}
}

EJust FRssJsonSerializer::StringToEJust(const FString& Str)
{
	if (Str == TEXT("RSS_Left"))
		return EJust::RSS_Left;
	if (Str == TEXT("RSS_Middle"))
		return EJust::RSS_Middle;
	return EJust::RSS_Right;
}

FString FRssJsonSerializer::ObjectToPath(const UObject* Object)
{
	if (!IsValid(Object))
	{
		return FString();
	}

	return Object->GetPathName();
}

UObject* FRssJsonSerializer::PathToObject(const FString& Path, const UClass* ExpectedClass)
{
	if (Path.IsEmpty() || !ExpectedClass)
	{
		return nullptr;
	}

	const FSoftObjectPath ObjectPath(Path);
	if (!ObjectPath.IsValid())
	{
		return nullptr;
	}

	UObject* Object = ObjectPath.ResolveObject();
	if (!IsValid(Object))
	{
		Object = ObjectPath.TryLoad();
	}

	return IsValid(Object) && Object->IsA(ExpectedClass) ? Object : nullptr;
}

FString FRssJsonSerializer::TextureToPath(UTexture* Texture)
{
	return ObjectToPath(Texture);
}

UTexture* FRssJsonSerializer::PathToTexture(const FString& Path)
{
	if (UTexture* Texture = Cast<UTexture>(PathToObject(Path, UTexture::StaticClass())))
	{
		return Texture;
	}

	const FString RedirectedPath = GetLegacyWebTextureRedirect(Path);
	return RedirectedPath.IsEmpty() ? nullptr : Cast<UTexture>(PathToObject(RedirectedPath, UTexture::StaticClass()));
}

FString FRssJsonSerializer::TextToString(const FText& Text) { return Text.ToString(); }

FText FRssJsonSerializer::StringToText(const FString& Str) { return FText::FromString(Str); }
