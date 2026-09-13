// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Sharing/SBSSharingProtocol.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Sharing/SBSPermissions.h"
#include "Structures/ApiStatics.h"

FSBSOperationResult FSBSOperationResult::Make(const TCHAR* Code, const FText& Message, bool bSuccess, int32 Status)
{
	FSBSOperationResult Result;
	Result.bSuccess = bSuccess;
	Result.Code = Code;
	Result.Message = Message;
	Result.HttpStatus = Status;
	return Result;
}

const TCHAR* FSBSSharingProtocol::ProductionBase() { return TEXT("https://k-mods.com/api/v1/sbs/"); }
const TCHAR* FSBSSharingProtocol::FixtureKey() { return TEXT("sbsu_AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"); }

void FSBSSharingProtocol::ClearSecret(FString& Secret)
{
	if (!Secret.IsEmpty())
	{
		FMemory::Memzero(Secret.GetCharArray().GetData(), Secret.GetCharArray().Num() * sizeof(TCHAR));
	}
	Secret.Empty();
}

bool FSBSSharingProtocol::IsUserKey(const FString& Key)
{
	if (Key.Len() != 48 || !Key.StartsWith(TEXT("sbsu_"), ESearchCase::CaseSensitive))
	{
		return false;
	}
	return FSBSStatics::IsSafeIdentifier(Key);
}

bool FSBSSharingProtocol::IsCredentialUrl(const FString& Url, const FString& Base)
{
	const int32 TestPort = FSBSStatics::GetLocalTestPort();
	const bool bAllowedBase = Base == ProductionBase() ||
		(TestPort && Base == FString::Printf(TEXT("http://127.0.0.1:%d/api/v1/sbs/"), TestPort));
	return bAllowedBase && Url.StartsWith(Base, ESearchCase::CaseSensitive) && !Url.Contains(TEXT("..")) &&
		!Url.Contains(TEXT("\\")) && !Url.Contains(TEXT("%")) && !Url.Contains(TEXT("?")) && !Url.Contains(TEXT("#"));
}

FSBSOperationResult FSBSSharingProtocol::ValidateUpload(const FSBSBlueprintUpload& Upload)
{
	if (!Upload.bPublishPublicly)
	{
		return FSBSOperationResult::Make(
			TEXT("publication_not_confirmed"),
			NSLOCTEXT("SBS", "Sharing.PublicationNotConfirmed", "Choose public publication before uploading."));
	}
	if (!FSBSStatics::IsSafeFileName(Upload.LocalBlueprintName) || Upload.LocalBlueprintName.EndsWith(TEXT(".sbp")) ||
		Upload.LocalBlueprintName.EndsWith(TEXT(".sbpcfg")))
	{
		return FSBSOperationResult::Make(
			TEXT("invalid_filename"),
			NSLOCTEXT("SBS", "Sharing.InvalidFilename", "Select a local blueprint name without a file extension."));
	}
	if (Upload.Name.IsEmpty() || Upload.Name.Len() > 120 || Upload.Name != Upload.Name.TrimStartAndEnd() ||
		Upload.Description.Len() > 10000 || Upload.TagIds.Num() > 32)
	{
		return FSBSOperationResult::Make(
			TEXT("invalid_metadata"),
			NSLOCTEXT("SBS", "Sharing.InvalidMetadata3",
					  "Use a name of 1-120 characters, a description up to 10000 characters and at most 32 tags."));
	}
	for (const TCHAR Character : Upload.Name)
	{
		if (Character < 32 || Character == 127)
		{
			return FSBSOperationResult::Make(
				TEXT("invalid_metadata"),
				NSLOCTEXT("SBS", "Sharing.InvalidMetadata", "The public name cannot contain control characters."));
		}
	}
	for (const TCHAR Character : Upload.Description)
	{
		if ((Character < 32 && Character != '\n' && Character != '\r' && Character != '\t') || Character == 127)
		{
			return FSBSOperationResult::Make(TEXT("invalid_metadata"),
											 NSLOCTEXT("SBS", "Sharing.InvalidMetadata2",
													   "The description contains unsupported control characters."));
		}
	}
	TSet<FString> Tags;
	for (const FString& Tag : Upload.TagIds)
	{
		if (!FSBSStatics::IsSafeIdentifier(Tag) || Tags.Contains(Tag))
		{
			return FSBSOperationResult::Make(
				TEXT("invalid_tags"), NSLOCTEXT("SBS", "Sharing.InvalidTags", "Choose distinct valid SBS tag IDs."));
		}
		Tags.Add(Tag);
	}
	return FSBSOperationResult::Make(TEXT("valid"), NSLOCTEXT("SBS", "Sharing.Valid", "Upload metadata is valid."),
									 true);
}

FSBSOperationResult FSBSSharingProtocol::ReadLocalFiles(const FString& Directory, const FString& Name,
														TArray<uint8>& Sbp, TArray<uint8>& Config)
{
	Sbp.Reset();
	Config.Reset();
	if (Directory.IsEmpty() || !FSBSStatics::IsSafeFileName(Name))
	{
		return FSBSOperationResult::Make(
			TEXT("invalid_filename"),
			NSLOCTEXT("SBS", "Sharing.InvalidFilename2", "Select a blueprint from the current session."));
	}
	const auto Read = [&Directory, &Name](const TCHAR* Extension, int32 Limit, TArray<uint8>& Bytes)
	{
		const FString Path = FPaths::Combine(Directory, Name + Extension);
		if (FPlatformFileManager::Get().GetPlatformFile().IsSymlink(*Path) == ESymlinkResult::Symlink)
		{
			return false;
		}
		TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*Path, FILEREAD_Silent));
		if (!Reader || Reader->TotalSize() < 20 || Reader->TotalSize() > Limit)
		{
			return false;
		}
		const int64 Size = Reader->TotalSize();
		Bytes.SetNumUninitialized(static_cast<int32>(Size));
		Reader->Serialize(Bytes.GetData(), Size);
		return !Reader->IsError() && Reader->TotalSize() == Size && Reader->Close();
	};
	if (!Read(TEXT(".sbp"), MaxBlueprintBytes, Sbp) || !Read(TEXT(".sbpcfg"), MaxConfigBytes, Config))
	{
		Sbp.Reset();
		Config.Reset();
		return FSBSOperationResult::Make(
			TEXT("invalid_local_files"),
			NSLOCTEXT(
				"SBS", "Sharing.InvalidLocalFiles",
				"Both local blueprint files must exist and fit the 64 MiB / 1 MiB upload limits. Save the blueprint before uploading."));
	}
	return FSBSOperationResult::Make(TEXT("files_ready"),
									 NSLOCTEXT("SBS", "Sharing.FilesReady", "Local blueprint files are ready."), true);
}

FSBSOperationResult FSBSSharingProtocol::BuildMultipart(const FSBSBlueprintUpload& Upload, const TArray<uint8>& Sbp,
														const TArray<uint8>& Config, const FString& Boundary,
														TArray<uint8>& Body)
{
	Body.Reset();
	FSBSOperationResult Validation = ValidateUpload(Upload);
	if (!Validation.bSuccess)
	{
		return Validation;
	}
	if (Sbp.Num() < 20 || Sbp.Num() > MaxBlueprintBytes || Config.Num() < 20 || Config.Num() > MaxConfigBytes ||
		!FSBSStatics::IsSafeIdentifier(Boundary) || Boundary.Len() > 70)
	{
		return FSBSOperationResult::Make(
			TEXT("invalid_payload"),
			NSLOCTEXT("SBS", "Sharing.InvalidPayload", "Blueprint upload data is invalid or too large."));
	}

	const FTCHARToUTF8 BoundaryBytes(*Boundary);
	const auto ContainsBoundary = [&BoundaryBytes](const TArray<uint8>& Bytes)
	{
		for (int32 Index = 0; Index <= Bytes.Num() - BoundaryBytes.Length(); ++Index)
		{
			if (Bytes[Index] == static_cast<uint8>(BoundaryBytes.Get()[0]) &&
				FMemory::Memcmp(Bytes.GetData() + Index, BoundaryBytes.Get(), BoundaryBytes.Length()) == 0)
			{
				return true;
			}
		}
		return false;
	};
	if (ContainsBoundary(Sbp) || ContainsBoundary(Config))
	{
		return FSBSOperationResult::Make(
			TEXT("invalid_payload"),
			NSLOCTEXT("SBS", "Sharing.InvalidPayload2", "Could not encode this upload. Try again."));
	}
	const TSharedRef<FJsonObject> Metadata = MakeShared<FJsonObject>();
	Metadata->SetNumberField(TEXT("schemaVersion"), 1);
	Metadata->SetStringField(TEXT("name"), Upload.Name);
	Metadata->SetStringField(TEXT("description"), Upload.Description);
	Metadata->SetStringField(TEXT("originalName"), Upload.LocalBlueprintName);
	Metadata->SetStringField(TEXT("visibility"), TEXT("public"));
	TArray<TSharedPtr<FJsonValue>> Tags;
	for (const FString& Tag : Upload.TagIds)
	{
		Tags.Add(MakeShared<FJsonValueString>(Tag));
	}
	Metadata->SetArrayField(TEXT("tagIds"), Tags);
	const TSharedRef<FJsonObject> Files = MakeShared<FJsonObject>();
	Files->SetNumberField(TEXT("sbpBytes"), Sbp.Num());
	Files->SetNumberField(TEXT("sbpcfgBytes"), Config.Num());
	Metadata->SetObjectField(TEXT("files"), Files);
	FString Json;
	FJsonSerializer::Serialize(Metadata, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json));
	Body.Reserve(Sbp.Num() + Config.Num() + Json.Len() * 4 + 1024);
	const auto AppendText = [&Body](const FString& Text)
	{
		const FTCHARToUTF8 Utf8(*Text);
		Body.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
	};
	AppendText(TEXT("--") + Boundary +
			   TEXT("\r\nContent-Disposition: form-data; name=\"metadata\"\r\nContent-Type: application/json; "
					"charset=utf-8\r\n\r\n") +
			   Json);
	AppendText(TEXT("\r\n--") + Boundary +
			   TEXT("\r\nContent-Disposition: form-data; name=\"sbp\"; filename=\"blueprint.sbp\"\r\nContent-Type: "
					"application/octet-stream\r\n\r\n"));
	Body.Append(Sbp);
	AppendText(TEXT("\r\n--") + Boundary +
			   TEXT("\r\nContent-Disposition: form-data; name=\"sbpcfg\"; "
					"filename=\"blueprint.sbpcfg\"\r\nContent-Type: application/octet-stream\r\n\r\n"));
	Body.Append(Config);
	AppendText(TEXT("\r\n--") + Boundary + TEXT("--\r\n"));
	return FSBSOperationResult::Make(TEXT("payload_ready"),
									 NSLOCTEXT("SBS", "Sharing.PayloadReady", "Blueprint upload is ready."), true);
}

bool FSBSSharingProtocol::ParseSession(const TSharedPtr<FJsonObject>& Json, FSBSSession& Session)
{
	Session = FSBSSession();
	const TSharedPtr<FJsonObject>* User = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Scopes = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Permissions = nullptr;
	double Version = 0;
	FSBSSession Parsed;
	if (!Json || !Json->HasTypedField<EJson::Number>(TEXT("schemaVersion")) ||
		!Json->TryGetNumberField(TEXT("schemaVersion"), Version) || Version != 1 ||
		!Json->TryGetObjectField(TEXT("user"), User) || !User->IsValid() ||
		!(*User)->HasTypedField<EJson::String>(TEXT("id")) || !(*User)->TryGetStringField(TEXT("id"), Parsed.User.ID) ||
		!FSBSStatics::IsSafeIdentifier(Parsed.User.ID) || !(*User)->HasTypedField<EJson::String>(TEXT("username")) ||
		!(*User)->TryGetStringField(TEXT("username"), Parsed.User.Username) || Parsed.User.Username.IsEmpty() ||
		Parsed.User.Username.Len() > 128 || !(*User)->TryGetArrayField(TEXT("permissions"), Permissions) ||
		Permissions->Num() > 256 || !Json->TryGetArrayField(TEXT("scopes"), Scopes) || Scopes->Num() > 32)
	{
		return false;
	}
	for (const TSharedPtr<FJsonValue>& Permission : *Permissions)
	{
		FString Key;
		if (!Permission || Permission->Type != EJson::String || !Permission->TryGetString(Key) ||
			!FSBSPermissions::IsPermissionKey(Key) || Parsed.User.Permissions.Contains(Key))
		{
			return false;
		}
		Parsed.User.Permissions.Add(MoveTemp(Key));
	}
	for (const TSharedPtr<FJsonValue>& Scope : *Scopes)
	{
		FString Value;
		if (!Scope || Scope->Type != EJson::String || !Scope->TryGetString(Value) || Value.IsEmpty() ||
			Value.Len() > 64)
		{
			return false;
		}
		Parsed.bCanPublish |= Value == TEXT("sbs:publish");
	}

	Parsed.State = ESBSLoginState::SignedIn;
	Session = MoveTemp(Parsed);
	return true;
}

bool FSBSSharingProtocol::ParseUpload(const TSharedPtr<FJsonObject>& Json, const FGuid& RequestId,
									  FSBSBlueprintUploadResult& Result)
{
	Result = FSBSBlueprintUploadResult();
	Result.RequestId = RequestId;
	const TSharedPtr<FJsonObject>* Blueprint = nullptr;
	double Version = 0;
	FString ReturnedRequestId, Id, Url, Visibility;
	FGuid ParsedId;
	if (!Json || !Json->TryGetNumberField(TEXT("schemaVersion"), Version) || Version != 1 ||
		!Json->TryGetStringField(TEXT("requestId"), ReturnedRequestId) || !FGuid::Parse(ReturnedRequestId, ParsedId) ||
		ParsedId != RequestId || !Json->TryGetObjectField(TEXT("blueprint"), Blueprint) || !Blueprint->IsValid() ||
		!(*Blueprint)->TryGetStringField(TEXT("id"), Id) || !FSBSStatics::IsSafeIdentifier(Id) ||
		!(*Blueprint)->TryGetStringField(TEXT("url"), Url) || Url != TEXT("https://k-mods.com/sbs/blueprints/") + Id ||
		!(*Blueprint)->TryGetStringField(TEXT("visibility"), Visibility) || Visibility != TEXT("public"))
	{
		return false;
	}
	Result.BlueprintId = MoveTemp(Id);
	Result.PublicUrl = MoveTemp(Url);
	return true;
}
