// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "SBSBlueprintMigrationLibrary.h"

#include "Async/TaskGraphInterfaces.h"
#include "Containers/Ticker.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Http/SBSHttp.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include "Interfaces/IPluginManager.h"
#include "Internationalization/Text.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Sharing/SBSSharingProtocol.h"
#include "Sharing/SBSPermissions.h"
#include "Sharing/SBSCredentialStore.h"
#include "Structures/ApiStatics.h"
#include "Subsystem/SBSSharingSubsystem.h"

namespace
{
	void PumpSharingHttp()
	{
		FHttpModule::Get().GetHttpManager().Tick(0.01f);
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
		FTSTicker::GetCoreTicker().Tick(0.01f);
		FPlatformProcess::SleepNoStats(0.01f);
	}
}

FString USBSBlueprintMigrationLibrary::ValidateSharingData()
{
	const TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Failures;
	int32 Checks = 0;
	const auto Verify = [&Failures, &Checks](bool bPassed, const TCHAR* Message)
	{
		++Checks;
		if (!bPassed)
		{
			Failures.Add(MakeShared<FJsonValueString>(Message));
		}
	};
	const FString Base = FSBSSharingProtocol::ProductionBase();
	Verify(FSBSSharingProtocol::IsUserKey(FSBSSharingProtocol::FixtureKey()), TEXT("Dummy key shape rejected"));
	Verify(!FSBSSharingProtocol::IsUserKey(TEXT("old-public-key")), TEXT("Legacy key accepted as user key"));
	Verify(!FSBSSharingProtocol::IsUserKey(FString(FSBSSharingProtocol::FixtureKey()) + TEXT("\r\n")),
		   TEXT("Header injection key accepted"));
	Verify(FSBSSharingProtocol::IsCredentialUrl(Base + TEXT("auth/me"), Base), TEXT("Production auth URL rejected"));
	Verify(!FSBSSharingProtocol::IsCredentialUrl(TEXT("https://k-mods.com.evil.test/api/v1/sbs/auth/me"), Base),
		   TEXT("Lookalike credential origin accepted"));
	Verify(!FSBSSharingProtocol::IsCredentialUrl(TEXT("http://k-mods.com/api/v1/sbs/auth/me"), Base),
		   TEXT("Credential downgrade accepted"));
	Verify(!FSBSSharingProtocol::IsCredentialUrl(Base + TEXT("../other"), Base),
		   TEXT("Credential path escape accepted"));
	Verify(!FSBSSharingProtocol::IsCredentialUrl(TEXT("https://dev-sbs.kmods.space/api/v1/auth/me"),
												 TEXT("https://dev-sbs.kmods.space/api/v1/")),
		   TEXT("Legacy dev origin accepted a user key"));
	FString Secret = FSBSSharingProtocol::FixtureKey();
	FSBSSharingProtocol::ClearSecret(Secret);
	Verify(Secret.IsEmpty(), TEXT("Secret clearing failed"));
	Verify(FSBSCredentialStore::ValidateIsolatedStore(),
		   TEXT("Isolated credential write/read/delete round trip failed"));

	FSBSBlueprintUpload Upload;
	Upload.LocalBlueprintName = TEXT("Copper Factory");
	Upload.Name = TEXT("Copper \"Factory\" \\ line");
	Upload.Description = TEXT("First line\nSecond line\tend");
	Upload.TagIds = {TEXT("factory"), TEXT("copper")};
	Upload.RequestId = FGuid::NewGuid();
	Verify(!FSBSSharingProtocol::ValidateUpload(Upload).bSuccess, TEXT("Unconfirmed publication accepted"));
	Upload.bPublishPublicly = true;
	const FSBSOperationResult Valid = FSBSSharingProtocol::ValidateUpload(Upload);
	Verify(Valid.bSuccess, TEXT("Valid upload metadata rejected"));
	const TOptional<FString> Namespace = FTextInspector::GetNamespace(Valid.Message);
	const TOptional<FString> Key = FTextInspector::GetKey(Valid.Message);
	Verify(Namespace.IsSet() && Namespace.GetValue() == TEXT("SBS") && Key.IsSet() &&
			   Key.GetValue().StartsWith(TEXT("Sharing.")),
		   TEXT("Operation text lost localization identity"));
	FSBSBlueprintUpload Invalid = Upload;
	Invalid.LocalBlueprintName = TEXT("../escape");
	Verify(!FSBSSharingProtocol::ValidateUpload(Invalid).bSuccess, TEXT("Upload path traversal accepted"));
	Invalid = Upload;
	Invalid.LocalBlueprintName = TEXT("NUL.txt");
	Verify(!FSBSSharingProtocol::ValidateUpload(Invalid).bSuccess, TEXT("Upload device filename accepted"));
	Invalid = Upload;
	Invalid.Name = FString::ChrN(121, TEXT('x'));
	Verify(!FSBSSharingProtocol::ValidateUpload(Invalid).bSuccess, TEXT("Oversized name accepted"));
	Invalid.Name = TEXT("line\nline");
	Verify(!FSBSSharingProtocol::ValidateUpload(Invalid).bSuccess, TEXT("Name control character accepted"));
	Invalid = Upload;
	Invalid.Description = FString::ChrN(10001, TEXT('x'));
	Verify(!FSBSSharingProtocol::ValidateUpload(Invalid).bSuccess, TEXT("Oversized description accepted"));
	Invalid = Upload;
	Invalid.TagIds.Add(TEXT("factory"));
	Verify(!FSBSSharingProtocol::ValidateUpload(Invalid).bSuccess, TEXT("Duplicate tag accepted"));
	Invalid = Upload;
	Invalid.TagIds = {TEXT("../tag")};
	Verify(!FSBSSharingProtocol::ValidateUpload(Invalid).bSuccess, TEXT("Unsafe tag accepted"));

	TArray<uint8> Sbp, Config, Body;
	for (int32 Index = 0; Index < 256; ++Index)
	{
		Sbp.Add(static_cast<uint8>(Index));
		Config.Add(static_cast<uint8>(255 - Index));
	}
	const FString Boundary = TEXT("SBS_native_fixture_boundary");
	Verify(FSBSSharingProtocol::BuildMultipart(Upload, Sbp, Config, Boundary, Body).bSuccess,
		   TEXT("Binary multipart construction failed"));
	const FString Directory =
		FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("SBS"))->GetBaseDir(), TEXT(".validation/sharing"));
	IFileManager::Get().MakeDirectory(*Directory, true);
	Verify(FFileHelper::SaveArrayToFile(Body, *FPaths::Combine(Directory, TEXT("multipart.bin"))),
		   TEXT("Could not save wire fixture"));
	TArray<uint8> Rejected;
	Verify(!FSBSSharingProtocol::BuildMultipart(Upload, {}, Config, Boundary, Rejected).bSuccess && Rejected.IsEmpty(),
		   TEXT("Empty binary accepted"));
	Verify(!FSBSSharingProtocol::BuildMultipart(Upload, Sbp, Config, TEXT("bad\r\nheader"), Rejected).bSuccess,
		   TEXT("Multipart boundary injection accepted"));
	TArray<uint8> TooLarge;
	TooLarge.SetNumZeroed(FSBSSharingProtocol::MaxConfigBytes + 1);
	Verify(!FSBSSharingProtocol::BuildMultipart(Upload, Sbp, TooLarge, Boundary, Rejected).bSuccess,
		   TEXT("Oversized config accepted"));
	const FString Stem = TEXT("files-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	Verify(FFileHelper::SaveArrayToFile(Sbp, *FPaths::Combine(Directory, Stem + TEXT(".sbp"))) &&
			   FFileHelper::SaveArrayToFile(Config, *FPaths::Combine(Directory, Stem + TEXT(".sbpcfg"))),
		   TEXT("Could not create local upload pair"));
	TArray<uint8> ReadSbp, ReadConfig;
	Verify(FSBSSharingProtocol::ReadLocalFiles(Directory, Stem, ReadSbp, ReadConfig).bSuccess && Sbp == ReadSbp &&
			   Config == ReadConfig,
		   TEXT("Local upload read corrupted binary data"));
	Verify(!FSBSSharingProtocol::ReadLocalFiles(Directory, TEXT("../escape"), ReadSbp, ReadConfig).bSuccess,
		   TEXT("Read outside blueprint directory accepted"));
	Verify(!FSBSSharingProtocol::ReadLocalFiles(Directory, Stem + TEXT("_missing"), ReadSbp, ReadConfig).bSuccess &&
			   ReadSbp.IsEmpty() && ReadConfig.IsEmpty(),
		   TEXT("Missing pair retained previous bytes"));

	const auto Parse = [](const FString& Text)
	{
		TSharedPtr<FJsonObject> Json;
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json);
		return Json;
	};
	TSharedPtr<FJsonObject> SessionJson = Parse(TEXT(
		"{\"schemaVersion\":1,\"user\":{\"id\":\"user_123\",\"username\":\"Builder\",\"permissions\":[]},\"scopes\":[\"sbs:publish\"]}"));
	FSBSSession Session;
	Verify(FSBSSharingProtocol::ParseSession(SessionJson, Session) && Session.bCanPublish &&
			   Session.User.ID == TEXT("user_123"),
		   TEXT("Session DTO rejected"));
	Verify(FSBSPermissions::CanEditContent(Session.User, TEXT("user_123")) &&
			   !FSBSPermissions::CanEditContent(Session.User, TEXT("other_user")),
		   TEXT("Ownership policy needs special flags or grants foreign editing"));
	SessionJson->GetObjectField(TEXT("user"))->SetNumberField(TEXT("permissions"), 10);
	Verify(!FSBSSharingProtocol::ParseSession(SessionJson, Session) && Session.State == ESBSLoginState::SignedOut &&
			   !Session.bCanPublish,
		   TEXT("Malformed login retained authorization"));
	SessionJson->GetObjectField(TEXT("user"))
		->SetArrayField(TEXT("permissions"), {MakeShared<FJsonValueString>(TEXT("moderate:sbs"))});
	Verify(FSBSSharingProtocol::ParseSession(SessionJson, Session) &&
			   FSBSPermissions::CanEditContent(Session.User, TEXT("other_user")) &&
			   !FSBSPermissions::HasPermission(Session.User, TEXT("manage_tags"), TEXT("sbs")),
		   TEXT("SBS moderation permission did not remain action-specific"));
	Session.User.Permissions = {TEXT("moderate:wiki")};
	Verify(!FSBSPermissions::CanEditContent(Session.User, TEXT("other_user")),
		   TEXT("Other service permission grants SBS moderation"));
	Session.User.Permissions = {TEXT("developer:all")};
	Verify(FSBSPermissions::CanEditContent(Session.User, TEXT("other_user")) &&
			   FSBSPermissions::HasPermission(Session.User, TEXT("sync"), TEXT("sbs")),
		   TEXT("Shared developer override rejected"));
	Session.User.ID.Reset();
	Verify(!FSBSPermissions::CanEditContent(Session.User, TEXT("other_user")), TEXT("Signed-out user can moderate"));
	SessionJson->GetObjectField(TEXT("user"))
		->SetArrayField(TEXT("permissions"), {MakeShared<FJsonValueString>(TEXT("sbs:moderate"))});
	Verify(FSBSSharingProtocol::ParseSession(SessionJson, Session) &&
			   !FSBSPermissions::CanEditContent(Session.User, TEXT("other_user")),
		   TEXT("Reversed permission key grants moderation"));
	SessionJson->GetObjectField(TEXT("user"))->SetArrayField(TEXT("permissions"), {});
	SessionJson->GetObjectField(TEXT("user"))->SetNumberField(TEXT("role"), 2147483647);
	Verify(FSBSSharingProtocol::ParseSession(SessionJson, Session) && Session.bCanPublish &&
			   !FSBSPermissions::CanEditContent(Session.User, TEXT("other_user")),
		   TEXT("Legacy role grants moderation or ordinary upload requires a flag"));
	SessionJson->SetArrayField(TEXT("scopes"), {});
	Verify(FSBSSharingProtocol::ParseSession(SessionJson, Session) && !Session.bCanPublish,
		   TEXT("Unscoped login can publish"));
	const FString Id = Upload.RequestId.ToString(EGuidFormats::DigitsWithHyphensLower);
	TSharedPtr<FJsonObject> UploadJson = Parse(
		TEXT("{\"schemaVersion\":1,\"requestId\":\"") + Id +
		TEXT(
			"\",\"blueprint\":{\"id\":\"bp_123\",\"url\":\"https://k-mods.com/sbs/blueprints/bp_123\",\"visibility\":\"public\"}}"));
	FSBSBlueprintUploadResult Published;
	Verify(FSBSSharingProtocol::ParseUpload(UploadJson, Upload.RequestId, Published) &&
			   Published.BlueprintId == TEXT("bp_123"),
		   TEXT("Publication DTO rejected"));
	Verify(!FSBSSharingProtocol::ParseUpload(UploadJson, FGuid::NewGuid(), Published) &&
			   Published.BlueprintId.IsEmpty(),
		   TEXT("Mismatched publication request accepted"));
	UploadJson->GetObjectField(TEXT("blueprint"))->SetStringField(TEXT("url"), TEXT("https://evil.test/phishing"));
	Verify(!FSBSSharingProtocol::ParseUpload(UploadJson, Upload.RequestId, Published),
		   TEXT("Untrusted publication URL accepted"));

	const int32 Port = FSBSStatics::GetLocalTestPort();
	if (Port)
	{
		const FString TestBase = FString::Printf(TEXT("http://127.0.0.1:%d/api/v1/sbs/"), Port);
		const auto Send = [&TestBase](const FString& Route, const FString& Method, TArray<uint8> Payload,
									  const FString& ContentType, const FString& RequestId, int32& Status,
									  TSharedPtr<FJsonObject>& Json)
		{
			const TSharedPtr<FSBSRequest> Request =
				FSBSRequest::Create(TestBase + Route, Method, TEXT(""), nullptr, 65536, 5.0f, false);
			Request->mRequest->SetHeader(TEXT("Authorization"),
										 FString(TEXT("Bearer ")) + FSBSSharingProtocol::FixtureKey());
			Request->mRequest->SetHeader(TEXT("Content-Type"), ContentType);
			Request->mRequest->SetHeader(TEXT("Idempotency-Key"), RequestId);
			Request->mRequest->SetContent(MoveTemp(Payload));
			bool bDone = false;
			bool bParsed = false;
			Request->mRequest->OnProcessRequestComplete().BindLambda(
				[&](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
				{
					Status = Response ? Response->GetResponseCode() : 0;
					bParsed = Request->ReadJson(Response, bSuccess, Json);
					bDone = true;
				});
			const bool bStarted = Request->Start();
			const double Deadline = FPlatformTime::Seconds() + 6.0;
			while (bStarted && !bDone && FPlatformTime::Seconds() < Deadline)
			{
				PumpSharingHttp();
			}
			Request->Cancel();
			return bDone && bParsed;
		};
		int32 Status = 0;
		TSharedPtr<FJsonObject> Json;
		Verify(Send(TEXT("auth/me"), TEXT("GET"), {}, TEXT("application/json"), TEXT(""), Status, Json) &&
				   Status == 200 && FSBSSharingProtocol::ParseSession(Json, Session),
			   TEXT("Native HTTP login fixture failed"));
		const FString ContentType = TEXT("multipart/form-data; boundary=") + Boundary;
		Verify(Send(TEXT("blueprints"), TEXT("POST"), Body, ContentType, Id, Status, Json) && Status == 201 &&
				   FSBSSharingProtocol::ParseUpload(Json, Upload.RequestId, Published),
			   TEXT("Native HTTP upload fixture failed"));
		Verify(Send(TEXT("blueprints"), TEXT("POST"), Body, ContentType, Id, Status, Json) && Status == 200 &&
				   FSBSSharingProtocol::ParseUpload(Json, Upload.RequestId, Published),
			   TEXT("Native upload retry duplicated publication"));
		Upload.Description += TEXT(" changed");
		FSBSSharingProtocol::BuildMultipart(Upload, Sbp, Config, Boundary, Body);
		Send(TEXT("blueprints"), TEXT("POST"), Body, ContentType, Id, Status, Json);
		Verify(Status == 409, TEXT("Changed native retry did not conflict"));

		UGameInstance* Instance = NewObject<UGameInstance>();
		Instance->AddToRoot();
		USBSSharingSubsystem* Sharing = NewObject<USBSSharingSubsystem>(Instance);
		Sharing->AddToRoot();
		FSubsystemCollection<UGameInstanceSubsystem> Collection;
		Sharing->Initialize(Collection);
		Verify(!Sharing->CanRememberLogin(), TEXT("Fixture mode can access production credential storage"));
		Sharing->LoginWithKey(FSBSSharingProtocol::FixtureKey(), false);
		const double Deadline = FPlatformTime::Seconds() + 6.0;
		while (Sharing->GetSession().State == ESBSLoginState::SigningIn && FPlatformTime::Seconds() < Deadline)
		{
			PumpSharingHttp();
		}
		Verify(Sharing->GetSession().State == ESBSLoginState::SignedIn && Sharing->GetSession().bCanPublish,
			   TEXT("Account subsystem login failed"));
		TMap<FString, FString> Headers;
		Verify(Sharing->ApplyAuthorization(TestBase + TEXT("blueprints"), Headers) && Headers.Num() == 1 &&
				   Headers.Contains(TEXT("Authorization")),
			   TEXT("Session bearer header missing"));
		Headers.Reset();
		Verify(!Sharing->ApplyAuthorization(Base + TEXT("blueprints"), Headers) && Headers.IsEmpty(),
			   TEXT("Fixture session leaked credentials to production"));
		Sharing->Logout();
		Verify(Sharing->GetSession().State == ESBSLoginState::SignedOut &&
				   !Sharing->ApplyAuthorization(TestBase + TEXT("blueprints"), Headers),
			   TEXT("Logout retained usable credentials"));
		Sharing->LoginWithKey(FSBSSharingProtocol::FixtureKey(), false);
		Sharing->Logout();
		for (int32 Index = 0; Index < 30; ++Index)
		{
			PumpSharingHttp();
		}
		Verify(Sharing->GetSession().State == ESBSLoginState::SignedOut,
			   TEXT("Cancelled login resurrected the session"));
		Sharing->Deinitialize();
		Sharing->RemoveFromRoot();
		Instance->RemoveFromRoot();
	}
	Report->SetBoolField(TEXT("loopbackHttpTested"), Port != 0);
	Report->SetNumberField(TEXT("checks"), Checks);
	Report->SetArrayField(TEXT("failures"), Failures);
	FString Text;
	FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Text));
	return Text;
}
