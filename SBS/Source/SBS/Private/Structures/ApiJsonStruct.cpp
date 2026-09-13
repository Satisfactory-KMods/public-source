// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Structures/ApiJsonStruct.h"

#include "Structures/ApiStatics.h"

namespace
{
	constexpr int32 MaxEntries = 512;

	int32 ReadInteger(const TSharedPtr<FJsonObject>& Json, const TCHAR* Key, int32 Default = 0)
	{
		double Value = Default;
		if (!Json->TryGetNumberField(Key, Value) || !FMath::IsFinite(Value) || Value < MIN_int32 || Value > MAX_int32)
		{
			return Default;
		}
		return static_cast<int32>(Value);
	}

	float ReadChannel(const TSharedPtr<FJsonObject>& Json, const TCHAR* Key)
	{
		double Value = 1.0;
		Json->TryGetNumberField(Key, Value);
		return FMath::IsFinite(Value) ? static_cast<float>(FMath::Clamp(Value, 0.0, 1.0)) : 1.0f;
	}

	void ReadStrings(const TSharedPtr<FJsonObject>& Json, const TCHAR* Key, TArray<FString>& Values)
	{
		const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
		if (Json->TryGetArrayField(Key, Items))
		{
			Values.Reserve(FMath::Min(Items->Num(), MaxEntries));
			for (const TSharedPtr<FJsonValue>& Item : *Items)
			{
				FString Value;
				if (Item && Item->TryGetString(Value))
				{
					Values.Add(MoveTemp(Value));
				}
				if (Values.Num() == MaxEntries)
				{
					break;
				}
			}
		}
	}

	template <typename T>
	void ReadObjects(const TSharedPtr<FJsonObject>& Json, const TCHAR* Key, TArray<T>& Values, UObject* WorldContext)
	{
		const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
		if (Json->TryGetArrayField(Key, Items))
		{
			Values.Reserve(FMath::Min(Items->Num(), MaxEntries));
			for (const TSharedPtr<FJsonValue>& Item : *Items)
			{
				const TSharedPtr<FJsonObject>* Object = nullptr;
				if (Item && Item->TryGetObject(Object) && Object->IsValid())
				{
					T Value;
					Value.setJsonObject(*Object);
					Value.parse(WorldContext);
					if (!Value.ID.IsEmpty())
					{
						Values.Add(MoveTemp(Value));
					}
				}
				if (Values.Num() == MaxEntries)
				{
					break;
				}
			}
		}
	}

	void ReadIcon(const TSharedPtr<FJsonObject>& Json, FBlueprintJsonColorStructure& Icon, UObject* WorldContext)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (Json->TryGetObjectField(TEXT("iconData"), Object) && Object->IsValid())
		{
			Icon.setJsonObject(*Object);
			Icon.parse(WorldContext);
		}
	}
}

void FBlueprintJsonStructure::parse(UObject* WorldContext)
{
	const TSharedPtr<FJsonObject> Json = MoveTemp(mJsonObject);
	*this = FBlueprintJsonStructure();
	if (!Json)
	{
		return;
	}
	Json->TryGetStringField(TEXT("_id"), ID);
	Json->TryGetStringField(TEXT("owner"), Owner);
	Json->TryGetStringField(TEXT("DesignerSize"), DesignerSize);
	Json->TryGetStringField(TEXT("name"), Name);
	Json->TryGetStringField(TEXT("createdAt"), CreatedAt);
	Json->TryGetStringField(TEXT("updatedAt"), UpdatedAt);
	Json->TryGetStringField(TEXT("originalName"), OriginalName);
	Json->TryGetStringField(TEXT("SCIM"), SCIM);
	ReadStrings(Json, TEXT("mods"), Mods);
	ReadStrings(Json, TEXT("images"), Images);
	TotalRating = ReadInteger(Json, TEXT("totalRating"));
	TotalRatingCount = FMath::Max(0, ReadInteger(Json, TEXT("totalRatingCount")));
	Downloads = FMath::Max(0, ReadInteger(Json, TEXT("downloads")));
	ReadObjects(Json, TEXT("tags"), Tags, WorldContext);
	ReadIcon(Json, IconData, WorldContext);
}

void FBlueprintInPackJsonStructure::parse(UObject* WorldContext)
{
	const TSharedPtr<FJsonObject> Json = MoveTemp(mJsonObject);
	*this = FBlueprintInPackJsonStructure();
	if (!Json)
	{
		return;
	}
	Json->TryGetStringField(TEXT("_id"), ID);
	Json->TryGetStringField(TEXT("originalName"), OriginalName);
	Json->TryGetStringField(TEXT("name"), Name);
	ReadIcon(Json, IconData, WorldContext);
}

void FBlueprintPackJsonStructure::parse(UObject* WorldContext)
{
	const TSharedPtr<FJsonObject> Json = MoveTemp(mJsonObject);
	*this = FBlueprintPackJsonStructure();
	if (!Json)
	{
		return;
	}
	Json->TryGetStringField(TEXT("_id"), ID);
	Json->TryGetStringField(TEXT("owner"), Owner);
	Json->TryGetStringField(TEXT("name"), Name);
	Json->TryGetStringField(TEXT("createdAt"), CreatedAt);
	Json->TryGetStringField(TEXT("updatedAt"), UpdatedAt);
	ReadStrings(Json, TEXT("mods"), Mods);
	TotalRating = ReadInteger(Json, TEXT("totalRating"));
	TotalRatingCount = FMath::Max(0, ReadInteger(Json, TEXT("totalRatingCount")));
	ReadObjects(Json, TEXT("blueprints"), Blueprints, WorldContext);
	ReadObjects(Json, TEXT("tags"), Tags, WorldContext);
	if (!Blueprints.IsEmpty())
	{
		FirstIconData = Blueprints[0].IconData;
		FString Image;
		if (Json->TryGetStringField(TEXT("image"), Image) && !Image.IsEmpty())
		{
			ImageUrl = FSBSStatics::MakeUrl(TEXT("image/") + FSBSStatics::EncodePathSegment(Blueprints[0].ID) +
												TEXT("/") + FSBSStatics::EncodePathSegment(Image),
											WorldContext);
		}
	}
}

void FBlueprintJsonTagStructure::parse(UObject* WorldContext)
{
	const TSharedPtr<FJsonObject> Json = MoveTemp(mJsonObject);
	*this = FBlueprintJsonTagStructure();
	if (Json)
	{
		Json->TryGetStringField(TEXT("_id"), ID);
		Json->TryGetStringField(TEXT("DisplayName"), DisplayName);
	}
}

void FBlueprintJsonColorStructure::parse(UObject* WorldContext)
{
	const TSharedPtr<FJsonObject> Json = MoveTemp(mJsonObject);
	*this = FBlueprintJsonColorStructure();
	if (!Json)
	{
		return;
	}
	IconID = ReadInteger(Json, TEXT("iconID"));
	const TSharedPtr<FJsonObject>* Object = nullptr;
	if (Json->TryGetObjectField(TEXT("color"), Object) && Object->IsValid())
	{
		Color = FLinearColor(ReadChannel(*Object, TEXT("r")), ReadChannel(*Object, TEXT("g")),
							 ReadChannel(*Object, TEXT("b")), ReadChannel(*Object, TEXT("a")));
	}
}
