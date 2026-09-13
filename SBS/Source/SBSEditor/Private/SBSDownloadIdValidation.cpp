// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "SBSBlueprintMigrationLibrary.h"

#include "Async/TaskGraphInterfaces.h"
#include "BlueprintFunctionLib/SBSApiFunctionLibrary.h"
#include "Containers/Ticker.h"
#include "Download/SBSDownloadIdProtocol.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformProcess.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include "SBSDownloadIdValidationReceiver.h"
#include "Serialization/JsonSerializer.h"
#include "Structures/ApiStatics.h"
#include "Subsystem/SBSApiSubsystem.h"

void USBSDownloadIdValidationReceiver::Receive(FSBSResolvedDownload Download)
{
	Results.Add(Download);
	if (bResolveAgain && Download.Result.bSuccess)
	{
		bResolveAgain = false;
		Api->ResolveDownloadId(TEXT("SBS-PACK:fixture-pack"));
	}
}

namespace
{
	void PumpDownloadIdHttp()
	{
		FHttpModule::Get().GetHttpManager().Tick(0.01f);
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
		FTSTicker::GetCoreTicker().Tick(0.01f);
		FPlatformProcess::SleepNoStats(0.01f);
	}

	TSharedPtr<FJsonObject> ParseDownloadJson(const FString& Text)
	{
		TSharedPtr<FJsonObject> Json;
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json);
		return Json;
	}
}

FString USBSBlueprintMigrationLibrary::ValidateDownloadIdData()
{
	const auto Report = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Failures;
	int32 Checks = 0;
	const auto Verify = [&](bool bPassed, const TCHAR* Message)
	{
		++Checks;
		if (!bPassed)
		{
			Failures.Add(MakeShared<FJsonValueString>(Message));
		}
	};
	FSBSDownloadReference Reference;
	Verify(USBSApiFunctionLibrary::ParseDownloadId(TEXT(" \tsbs-bp:Bp_123\r\n"), Reference).bSuccess &&
			   Reference.Kind == ESBSDownloadKind::Blueprint && Reference.ID == TEXT("Bp_123") &&
			   Reference.DownloadId == TEXT("SBS-BP:Bp_123"),
		   TEXT("Blueprint ID normalization changed case-sensitive record ID"));
	Verify(USBSApiFunctionLibrary::ParseDownloadId(TEXT("sBs-PaCk:pack-1"), Reference).bSuccess &&
			   Reference.Kind == ESBSDownloadKind::Pack && Reference.DownloadId == TEXT("SBS-PACK:pack-1"),
		   TEXT("Pack ID rejected"));
	Verify(USBSApiFunctionLibrary::ParseDownloadId(TEXT("raw_id"), Reference).bSuccess &&
			   Reference.Kind == ESBSDownloadKind::Auto,
		   TEXT("Bare ID must request exact type resolution"));
	FString DownloadId;
	Verify(USBSApiFunctionLibrary::MakeDownloadId(ESBSDownloadKind::Pack, TEXT("pack-1"), DownloadId).bSuccess &&
			   DownloadId == TEXT("SBS-PACK:pack-1"),
		   TEXT("Copyable pack ID differs from website format"));
	Verify(!USBSApiFunctionLibrary::MakeDownloadId(ESBSDownloadKind::Auto, TEXT("id"), DownloadId).bSuccess &&
			   DownloadId.IsEmpty(),
		   TEXT("Ambiguous copyable ID accepted"));
	for (const FString& Invalid :
		 {FString(), FString(TEXT("SBS-BP:")), FString(TEXT("SBS-OTHER:id")), FString(TEXT("SBS-BP:../id")),
		  FString(TEXT("SBS-PACK:two ids")), FString(TEXT("SBS-BP: id")),
		  FString(TEXT("https://k-mods.com/sbs/blueprints/id")), FString(TEXT("id?query")), FString(TEXT("id%2Fpath")),
		  FString(TEXT("id\nmore")), FString(TEXT("ü")), FString::ChrN(129, 'a'), FString::ChrN(257, ' ')})
	{
		Verify(!FSBSDownloadIdProtocol::ParseInput(Invalid, Reference).bSuccess && Reference.ID.IsEmpty(),
			   TEXT("Unsafe or oversized download ID retained actionable data"));
	}
	Verify(FSBSDownloadIdProtocol::ParseInput(TEXT("SBS-BP:") + FString::ChrN(128, 'a'), Reference).bSuccess,
		   TEXT("Maximum valid record ID rejected"));
	FSBSDownloadIdProtocol::ParseInput(TEXT("SBS-BP:bp_1"), Reference);
	const auto Payload = ParseDownloadJson(FSBSDownloadIdProtocol::MakePayload(Reference));
	Verify(Payload && Payload->Values.Num() == 3 && Payload->GetStringField(TEXT("kind")) == TEXT("blueprint") &&
			   Payload->GetStringField(TEXT("id")) == TEXT("bp_1"),
		   TEXT("Resolve DTO differs from native input"));
	const FString BlueprintResponse = TEXT(
		"{\"schemaVersion\":1,\"kind\":\"blueprint\",\"id\":\"bp_1\",\"blueprint\":{\"_id\":\"bp_1\",\"name\":\"Copper\",\"originalName\":\"Copper\"}}");
	FSBSResolvedDownload Download;
	Verify(FSBSDownloadIdProtocol::ParseResponse(ParseDownloadJson(BlueprintResponse), Reference, Download, nullptr) &&
			   Download.Result.bSuccess && Download.Blueprint.ID == TEXT("bp_1") && Download.Pack.ID.IsEmpty(),
		   TEXT("Blueprint response rejected or wrong union branch populated"));
	for (const FString& Invalid :
		 {BlueprintResponse.Replace(TEXT("\"schemaVersion\":1"), TEXT("\"schemaVersion\":true")),
		  BlueprintResponse.Replace(TEXT("\"id\":\"bp_1\""), TEXT("\"id\":\"other\"")),
		  BlueprintResponse.Replace(TEXT("\"_id\":\"bp_1\""), TEXT("\"_id\":\"other\"")),
		  BlueprintResponse.Replace(TEXT("\"originalName\":\"Copper\""), TEXT("\"originalName\":\"../escape\"")),
		  BlueprintResponse.Replace(TEXT("\"name\":\"Copper\""), TEXT("\"name\":42")),
		  BlueprintResponse.Replace(TEXT("\"schemaVersion\":1"), TEXT("\"schemaVersion\":1,\"pack\":{}")),
		  BlueprintResponse.Replace(TEXT("\"schemaVersion\":1"), TEXT("\"schemaVersion\":1,\"error\":{}"))})
	{
		Verify(!FSBSDownloadIdProtocol::ParseResponse(ParseDownloadJson(Invalid), Reference, Download, nullptr) &&
				   Download.Blueprint.ID.IsEmpty() && Download.Pack.Blueprints.IsEmpty(),
			   TEXT("Malformed lookup left a partial download"));
	}
	const FString PackResponse = TEXT(
		"{\"schemaVersion\":1,\"kind\":\"pack\",\"id\":\"pack_1\",\"pack\":{\"_id\":\"pack_1\",\"name\":\"Pack\",\"blueprints\":[{\"_id\":\"bp_1\",\"name\":\"Copper\",\"originalName\":\"Copper\"},{\"_id\":\"bp_2\",\"name\":\"Iron\",\"originalName\":\"Iron\"}]}}");
	FSBSDownloadIdProtocol::ParseInput(TEXT("pack_1"), Reference);
	Verify(FSBSDownloadIdProtocol::ParseResponse(ParseDownloadJson(PackResponse), Reference, Download, nullptr) &&
			   Download.Reference.Kind == ESBSDownloadKind::Pack && Download.Pack.Blueprints.Num() == 2 &&
			   Download.Reference.DownloadId == TEXT("SBS-PACK:pack_1"),
		   TEXT("Bare pack ID failed to produce complete pack"));
	for (const FString& Invalid :
		 {PackResponse.Replace(TEXT("\"_id\":\"bp_2\""), TEXT("\"_id\":\"bp_1\"")),
		  PackResponse.Replace(TEXT("\"originalName\":\"Iron\""), TEXT("\"originalName\":\"cOpPeR\"")),
		  PackResponse.Replace(TEXT("\"originalName\":\"Iron\""), TEXT("\"originalName\":\"CON\""))})
	{
		Verify(!FSBSDownloadIdProtocol::ParseResponse(ParseDownloadJson(Invalid), Reference, Download, nullptr),
			   TEXT("Duplicate or unsafe pack member accepted"));
	}
	auto PackJson = ParseDownloadJson(PackResponse);
	PackJson->GetObjectField(TEXT("pack"))->SetArrayField(TEXT("blueprints"), {});
	Verify(!FSBSDownloadIdProtocol::ParseResponse(PackJson, Reference, Download, nullptr), TEXT("Empty pack accepted"));
	TArray<TSharedPtr<FJsonValue>> Members;
	for (int32 Index = 0; Index < 513; ++Index)
	{
		auto Item = MakeShared<FJsonObject>();
		Item->SetStringField(TEXT("_id"), FString::Printf(TEXT("bp_%d"), Index));
		Item->SetStringField(TEXT("name"), TEXT("Member"));
		Item->SetStringField(TEXT("originalName"), FString::Printf(TEXT("Member_%d"), Index));
		Members.Add(MakeShared<FJsonValueObject>(Item));
	}
	PackJson->GetObjectField(TEXT("pack"))->SetArrayField(TEXT("blueprints"), Members);
	Verify(!FSBSDownloadIdProtocol::ParseResponse(PackJson, Reference, Download, nullptr),
		   TEXT("Pack exceeds queue limit"));
	Members.Pop();
	PackJson->GetObjectField(TEXT("pack"))->SetArrayField(TEXT("blueprints"), Members);
	Verify(FSBSDownloadIdProtocol::ParseResponse(PackJson, Reference, Download, nullptr) &&
			   Download.Pack.Blueprints.Num() == 512,
		   TEXT("Maximum pack was truncated"));
	Reference.Kind = ESBSDownloadKind::Blueprint;
	Verify(!FSBSDownloadIdProtocol::ParseResponse(PackJson, Reference, Download, nullptr),
		   TEXT("Typed ID silently changed kind"));
	Verify(FSBSDownloadIdProtocol::HttpFailure(409).Code == TEXT("ambiguous_download_id") &&
			   !FSBSDownloadIdProtocol::HttpFailure(409).Message.IsEmpty(),
		   TEXT("Ambiguous ID lacks localized failure"));
	Verify(USBSApiFunctionLibrary::CopyDownloadId(ESBSDownloadKind::Blueprint, TEXT("bp_1")).Code ==
			   TEXT("clipboard_unavailable"),
		   TEXT("Commandlet unexpectedly accessed desktop clipboard"));

	const bool bLoopback = FSBSStatics::GetLocalTestPort() != 0;
	if (bLoopback)
	{
		UGameInstance* Instance = NewObject<UGameInstance>();
		Instance->AddToRoot();
		USBSApiSubsystem* Api = NewObject<USBSApiSubsystem>(Instance);
		Api->AddToRoot();

		Api->bInitialized = true;
		auto Receiver = NewObject<USBSDownloadIdValidationReceiver>();
		Receiver->AddToRoot();
		Receiver->Api = Api;
		Api->mOnDownloadIdResolved.AddDynamic(Receiver, &USBSDownloadIdValidationReceiver::Receive);
		const auto Wait = [&]()
		{
			const double Deadline = FPlatformTime::Seconds() + 6.0;
			while (Api->IsResolvingDownloadId() && FPlatformTime::Seconds() < Deadline)
			{
				PumpDownloadIdHttp();
			}
		};
		const FGuid First = Api->ResolveDownloadId(TEXT("SBS-BP:fixture-blueprint"));
		Wait();
		Verify(!Api->IsResolvingDownloadId() && Receiver->Results.Num() == 1 &&
				   Receiver->Results[0].RequestId == First && Receiver->Results[0].Result.bSuccess,
			   TEXT("Blueprint lookup HTTP/delegate failed"));
		Receiver->Results.Reset();
		Receiver->bResolveAgain = true;
		Api->ResolveDownloadId(TEXT("fixture-blueprint"));
		Wait();
		Verify(Receiver->Results.Num() == 2 && Receiver->Results[1].Result.bSuccess &&
				   Receiver->Results[1].Pack.Blueprints.Num() == 2,
			   TEXT("Reentrant callback could not resolve a pack"));
		Receiver->Results.Reset();
		const FGuid Active = Api->ResolveDownloadId(TEXT("fixture-blueprint"));
		const FGuid Busy = Api->ResolveDownloadId(TEXT("fixture-pack"));
		Verify(Receiver->Results.Num() == 1 && Receiver->Results[0].RequestId == Busy &&
				   Receiver->Results[0].Result.Code == TEXT("resolve_busy"),
			   TEXT("Busy lookup replaced the active request"));
		Api->CancelResolveDownloadId();
		for (int32 Index = 0; Index < 30; ++Index)
			PumpDownloadIdHttp();
		Verify(Receiver->Results.Num() == 2 && Receiver->Results[1].RequestId == Active &&
				   Receiver->Results[1].Result.Code == TEXT("resolve_cancelled") && !Api->IsResolvingDownloadId(),
			   TEXT("Cancelled lookup emitted a late success"));
		for (const auto& Entry : TMap<FString, FString>{{TEXT("fixture-missing"), TEXT("download_not_found")},
														{TEXT("fixture-ambiguous"), TEXT("ambiguous_download_id")},
														{TEXT("fixture-empty"), TEXT("invalid_download_content")}})
		{
			Receiver->Results.Reset();
			Api->ResolveDownloadId(Entry.Key);
			Wait();
			Verify(Receiver->Results.Num() == 1 && Receiver->Results[0].Result.Code == Entry.Value &&
					   Receiver->Results[0].Blueprint.ID.IsEmpty(),
				   TEXT("HTTP lookup failure not propagated safely"));
		}
		Receiver->Results.Reset();
		Api->ResolveDownloadId(TEXT("../unsafe"));
		Verify(!Api->IsResolvingDownloadId() && Receiver->Results.Num() == 1 &&
				   Receiver->Results[0].Result.Code == TEXT("invalid_download_id"),
			   TEXT("Invalid input started HTTP"));
		Receiver->Results.Reset();
		Api->ResolveDownloadId(TEXT("fixture-blueprint"));
		Api->OnSharingSessionChanged();
		Verify(!Api->IsResolvingDownloadId() && Receiver->Results.Num() == 1 && !Receiver->Results[0].Result.bSuccess,
			   TEXT("Account change retained pending lookup"));
		Receiver->Results.Reset();
		Api->ResolveDownloadId(TEXT("fixture-pack"));
		Api->Deinitialize();
		for (int32 Index = 0; Index < 30; ++Index)
			PumpDownloadIdHttp();
		Verify(Receiver->Results.IsEmpty() && !Api->IsResolvingDownloadId(), TEXT("Teardown notified dead UI"));
		Receiver->RemoveFromRoot();
		Api->RemoveFromRoot();
		Instance->RemoveFromRoot();
	}
	Report->SetBoolField(TEXT("loopbackHttpTested"), bLoopback);
	Report->SetNumberField(TEXT("checks"), Checks);
	Report->SetArrayField(TEXT("failures"), Failures);
	FString Text;
	FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Text));
	return Text;
}
