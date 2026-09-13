// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Structures/ApiPostStruct.h"

#include "HAL/PlatformMisc.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystem/SBSApiSubsystem.h"
#include "Subsystem/SBSSharingSubsystem.h"

void FApiPostStruct::MakeHeader(TMap<FString, FString>& Headers, UObject* WorldContext)
{
	Headers.Add(TEXT("User-Agent"), TEXT("SBS/Satisfactory-1.2"));
	Headers.Add(TEXT("Content-Type"), TEXT("application/json"));
	Headers.Add(TEXT("Accept"), TEXT("application/json"));
	Headers.Remove(TEXT("Authorization"));
	Headers.Remove(TEXT("x-api-key"));
	Headers.Remove(TEXT("x-account-key"));
	if (const USBSSharingSubsystem* Sharing = USBSSharingSubsystem::GetSBSSharingSubsystem(WorldContext);
		Sharing && Sharing->UsesUserKeyLogin())
	{
		Sharing->ApplyAuthorization(FSBSStatics::MakeUrl(TEXT(""), WorldContext), Headers);
		return;
	}
	if (!FSBSStatics::GetLocalTestPort())
	{
		const FString AppKey = FPlatformMisc::GetEnvironmentVariable(TEXT("SBS_LEGACY_API_KEY"));
		Headers.Add(TEXT("x-api-key"), AppKey.IsEmpty() ? FSBSStatics::API_KEY : AppKey);
	}

	Headers.Add(TEXT("x-account-key"), FSBSStatics::GetAccountKey(WorldContext));
}

FString FApiPostStruct::ToString()
{
	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	ToJson(JsonObject);
	FString Result;
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), TJsonWriterFactory<>::Create(&Result));
	return Result;
}

void FDynamicApiPostStruct::OnRequestDone(FHttpRequestPtr Request, FHttpResponsePtr Response,
										  USBSApiSubsystem* Subsystem, TSharedPtr<FJsonObject> JsonObject,
										  UObject* WorldContext)
{
}

void FFilterPostStruct::ToJson(TSharedPtr<FJsonObject>& JsonObject)
{
	const TSharedRef<FJsonObject> Filter = MakeShared<FJsonObject>();
	const auto AddStrings = [&Filter](const TCHAR* Key, const TArray<FString>& Values)
	{
		if (Values.IsEmpty())
		{
			return;
		}
		TArray<TSharedPtr<FJsonValue>> Items;
		const int32 Count = FMath::Min(Values.Num(), 128);
		Items.Reserve(Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Items.Add(MakeShared<FJsonValueString>(Values[Index].Left(256)));
		}
		Filter->SetArrayField(Key, Items);
	};
	AddStrings(TEXT("tags"), Tags);
	AddStrings(TEXT("mods"), Mods);
	if (!SortByKey.IsEmpty())
	{
		const TSharedRef<FJsonObject> Sort = MakeShared<FJsonObject>();
		Sort->SetStringField(TEXT("by"), SortByKey.Left(64));
		Sort->SetBoolField(TEXT("up"), SortByUp);
		Filter->SetObjectField(TEXT("sortBy"), Sort);
	}
	if (!Name.IsEmpty())
	{
		Filter->SetStringField(TEXT("name"), Name.Left(256));
	}
	if (OnlyVanilla)
	{
		Filter->SetBoolField(TEXT("onlyVanilla"), true);
	}
	JsonObject->SetNumberField(TEXT("skip"), FMath::Max(0, Skip));
	JsonObject->SetNumberField(TEXT("limit"), FMath::Clamp(Limit, 1, 200));
	JsonObject->SetObjectField(TEXT("filterOptions"), Filter);
}

void FRatingPostStruct::ToJson(TSharedPtr<FJsonObject>& JsonObject)
{
	JsonObject->SetStringField(TEXT("blueprintId"), BlueprintID);
	JsonObject->SetNumberField(TEXT("rating"), FMath::Clamp(Rating, 1, 5));
}

void FGetTagsStruct::OnRequestDone(FHttpRequestPtr Request, FHttpResponsePtr Response, USBSApiSubsystem* Subsystem,
								   TSharedPtr<FJsonObject> JsonObject, UObject* WorldContext)
{
	if (!IsValid(Subsystem) || !JsonObject)
	{
		return;
	}
	const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
	if (!JsonObject->TryGetArrayField(TEXT("tags"), Items))
	{
		return;
	}
	TArray<FBlueprintJsonTagStructure> Tags;
	Tags.Reserve(FMath::Min(Items->Num(), 512));
	for (const TSharedPtr<FJsonValue>& Item : *Items)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (Item && Item->TryGetObject(Object) && Object->IsValid())
		{
			FBlueprintJsonTagStructure Tag;
			Tag.setJsonObject(*Object);
			Tag.parse(WorldContext);
			if (!Tag.ID.IsEmpty())
			{
				Tags.Add(MoveTemp(Tag));
			}
		}
		if (Tags.Num() == 512)
		{
			break;
		}
	}
	Subsystem->mTags = MoveTemp(Tags);
}
