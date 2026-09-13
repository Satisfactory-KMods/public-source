// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Download/SBSDownloadIdProtocol.h"

#include "Serialization/JsonSerializer.h"
#include "Structures/ApiStatics.h"

namespace
{
	FSBSOperationResult InvalidInput()
	{
		return FSBSOperationResult::Make(TEXT("invalid_download_id"),
										 NSLOCTEXT("SBS", "DownloadId.Invalid", "Enter a valid blueprint or pack ID."));
	}

	bool ReadString(const TSharedPtr<FJsonObject>& Json, const TCHAR* Key, FString& Value)
	{
		return Json && Json->HasTypedField<EJson::String>(Key) && Json->TryGetStringField(Key, Value);
	}

	bool ReadName(const TSharedPtr<FJsonObject>& Json)
	{
		FString Name;
		if (!ReadString(Json, TEXT("name"), Name) || Name.IsEmpty() || Name.Len() > 256)
		{
			return false;
		}
		for (TCHAR Character : Name)
		{
			if (Character < 32 || Character == 127)
			{
				return false;
			}
		}
		return !Name.TrimStartAndEnd().IsEmpty();
	}

	bool ReadBlueprint(const TSharedPtr<FJsonObject>& Json, FString& ID, FString& FileName)
	{
		return ReadString(Json, TEXT("_id"), ID) && FSBSStatics::IsSafeIdentifier(ID) && ReadName(Json) &&
			ReadString(Json, TEXT("originalName"), FileName) && FSBSStatics::IsSafeFileName(FileName);
	}
}

FSBSOperationResult FSBSDownloadIdProtocol::Format(ESBSDownloadKind Kind, const FString& ID, FString& DownloadId)
{
	DownloadId.Reset();
	if (!FSBSStatics::IsSafeIdentifier(ID) || (Kind != ESBSDownloadKind::Blueprint && Kind != ESBSDownloadKind::Pack))
	{
		return InvalidInput();
	}
	DownloadId = (Kind == ESBSDownloadKind::Blueprint ? TEXT("SBS-BP:") : TEXT("SBS-PACK:")) + ID;
	return FSBSOperationResult::Make(TEXT("valid_download_id"), FText::GetEmpty(), true);
}

FSBSOperationResult FSBSDownloadIdProtocol::ParseInput(const FString& Input, FSBSDownloadReference& Reference)
{
	Reference = FSBSDownloadReference();
	if (Input.Len() > MaxInputCharacters)
	{
		return InvalidInput();
	}
	FString ID = Input.TrimStartAndEnd();
	ESBSDownloadKind Kind = ESBSDownloadKind::Auto;
	if (ID.StartsWith(TEXT("SBS-BP:"), ESearchCase::IgnoreCase))
	{
		Kind = ESBSDownloadKind::Blueprint;
		ID.RightChopInline(7);
	}
	else if (ID.StartsWith(TEXT("SBS-PACK:"), ESearchCase::IgnoreCase))
	{
		Kind = ESBSDownloadKind::Pack;
		ID.RightChopInline(9);
	}
	if (!FSBSStatics::IsSafeIdentifier(ID))
	{
		return InvalidInput();
	}
	Reference.Kind = Kind;
	Reference.ID = ID;
	Reference.DownloadId = ID;
	if (Kind != ESBSDownloadKind::Auto)
	{
		Format(Kind, ID, Reference.DownloadId);
	}
	return FSBSOperationResult::Make(TEXT("valid_download_id"), FText::GetEmpty(), true);
}

FString FSBSDownloadIdProtocol::MakePayload(const FSBSDownloadReference& Reference)
{
	const auto Json = MakeShared<FJsonObject>();
	Json->SetNumberField(TEXT("schemaVersion"), 1);
	Json->SetStringField(TEXT("id"), Reference.ID);
	Json->SetStringField(TEXT("kind"),
						 Reference.Kind == ESBSDownloadKind::Blueprint	? TEXT("blueprint")
							 : Reference.Kind == ESBSDownloadKind::Pack ? TEXT("pack")
																		: TEXT("auto"));
	FString Payload;
	FJsonSerializer::Serialize(Json, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Payload));
	return Payload;
}

bool FSBSDownloadIdProtocol::ParseResponse(const TSharedPtr<FJsonObject>& Json, const FSBSDownloadReference& Requested,
										   FSBSResolvedDownload& Download, UObject* WorldContext)
{
	Download = FSBSResolvedDownload();
	double Schema = 0;
	FString Kind, ID;
	if (!Json || Json->HasField(TEXT("error")) || !Json->HasTypedField<EJson::Number>(TEXT("schemaVersion")) ||
		!Json->TryGetNumberField(TEXT("schemaVersion"), Schema) || Schema != 1 ||
		!ReadString(Json, TEXT("kind"), Kind) || !ReadString(Json, TEXT("id"), ID) ||
		!FSBSStatics::IsSafeIdentifier(ID) || ID != Requested.ID)
	{
		return false;
	}
	const ESBSDownloadKind ResolvedKind = Kind == TEXT("blueprint") ? ESBSDownloadKind::Blueprint
		: Kind == TEXT("pack")										? ESBSDownloadKind::Pack
																	: ESBSDownloadKind::Auto;
	if (ResolvedKind == ESBSDownloadKind::Auto ||
		(Requested.Kind != ESBSDownloadKind::Auto && Requested.Kind != ResolvedKind))
	{
		return false;
	}
	const TSharedPtr<FJsonObject>* Object = nullptr;
	const bool bBlueprint = ResolvedKind == ESBSDownloadKind::Blueprint;
	if (Json->HasField(bBlueprint ? TEXT("pack") : TEXT("blueprint")) ||
		!Json->TryGetObjectField(bBlueprint ? TEXT("blueprint") : TEXT("pack"), Object) || !Object->IsValid())
	{
		return false;
	}
	FString ItemID, FileName;
	if (bBlueprint)
	{
		if (!ReadBlueprint(*Object, ItemID, FileName) || ItemID != ID)
		{
			return false;
		}
	}
	else
	{
		const TArray<TSharedPtr<FJsonValue>>* Members = nullptr;
		if (!ReadString(*Object, TEXT("_id"), ItemID) || ItemID != ID || !ReadName(*Object) ||
			!(*Object)->TryGetArrayField(TEXT("blueprints"), Members) || Members->IsEmpty() ||
			Members->Num() > MaxPackMembers)
		{
			return false;
		}
		TSet<FString> IDs, FileNames;
		for (const auto& Member : *Members)
		{
			const TSharedPtr<FJsonObject>* MemberObject = nullptr;
			if (!Member || !Member->TryGetObject(MemberObject) || !ReadBlueprint(*MemberObject, ItemID, FileName) ||
				IDs.Contains(ItemID) || FileNames.Contains(FileName.ToLower()))
			{
				return false;
			}
			IDs.Add(ItemID);
			FileNames.Add(FileName.ToLower());
		}
	}

	if (bBlueprint)
	{
		Download.Blueprint.setJsonObject(*Object);
		Download.Blueprint.parse(WorldContext);
	}
	else
	{
		Download.Pack.setJsonObject(*Object);
		Download.Pack.parse(WorldContext);
	}
	Download.Reference.Kind = ResolvedKind;
	Download.Reference.ID = ID;
	Format(ResolvedKind, ID, Download.Reference.DownloadId);
	Download.Result = FSBSOperationResult::Make(
		TEXT("download_id_resolved"), NSLOCTEXT("SBS", "DownloadId.Resolved", "Download ID found."), true, 200);
	return true;
}

FSBSOperationResult FSBSDownloadIdProtocol::HttpFailure(int32 Status)
{
	switch (Status)
	{
	case 0:
		return FSBSOperationResult::Make(TEXT("network_error"),
										 NSLOCTEXT("SBS", "DownloadId.Network", "Could not contact SBS. Try again."));
	case 400:
		return FSBSOperationResult::Make(TEXT("invalid_download_id"),
										 NSLOCTEXT("SBS", "DownloadId.Invalid", "Enter a valid blueprint or pack ID."),
										 false, Status);
	case 401:
		return FSBSOperationResult::Make(
			TEXT("authentication_required"),
			NSLOCTEXT("SBS", "DownloadId.Login", "Your SBS login was rejected. Sign in again."), false, Status);
	case 403:
		return FSBSOperationResult::Make(
			TEXT("download_forbidden"),
			NSLOCTEXT("SBS", "DownloadId.Forbidden", "This account cannot download this content."), false, Status);
	case 404:
	case 410:
		return FSBSOperationResult::Make(
			TEXT("download_not_found"),
			NSLOCTEXT("SBS", "DownloadId.NotFound", "No public blueprint or pack is available for this ID."), false,
			Status);
	case 409:
		return FSBSOperationResult::Make(
			TEXT("ambiguous_download_id"),
			NSLOCTEXT("SBS", "DownloadId.Ambiguous",
					  "This ID matches a blueprint and a pack. Copy its full download ID."),
			false, Status);
	case 422:
		return FSBSOperationResult::Make(
			TEXT("invalid_download_content"),
			NSLOCTEXT("SBS", "DownloadId.InvalidContent",
					  "This blueprint or pack cannot be downloaded. Its files may be missing or incompatible."),
			false, Status);
	case 429:
		return FSBSOperationResult::Make(
			TEXT("rate_limited"),
			NSLOCTEXT("SBS", "DownloadId.RateLimited", "Too many SBS requests. Wait before trying again."), false,
			Status);
	default:
		return FSBSOperationResult::Make(
			TEXT("resolve_failed"),
			NSLOCTEXT("SBS", "DownloadId.Failed", "SBS could not resolve this download ID. Try again later."), false,
			Status);
	}
}
