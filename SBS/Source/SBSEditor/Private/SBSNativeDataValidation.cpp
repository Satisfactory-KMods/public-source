// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "SBSBlueprintMigrationLibrary.h"

#include "Download/SBSBlueprintFileTransaction.h"
#include "HAL/FileManager.h"
#include "Http/SBSHttp.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Structures/ApiJsonStruct.h"
#include "Structures/ApiPostStruct.h"
#include "Structures/ApiStatics.h"

FString USBSBlueprintMigrationLibrary::ValidateNativeData()
{
	const TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Failures;
	int32 Checks = 0;
	const auto Verify = [&](bool bCondition, const TCHAR* Description)
	{
		++Checks;
		if (!bCondition)
		{
			Failures.Add(MakeShared<FJsonValueString>(Description));
		}
	};
	for (const TCHAR* Name : {TEXT("../escape"), TEXT("..\\escape"), TEXT("C:escape"), TEXT("file:stream"), TEXT("NUL"),
							  TEXT("con.sbp"), TEXT("LPT1"), TEXT("name."), TEXT("name "), TEXT(""), TEXT("a\nb")})
	{
		Verify(!FSBSStatics::IsSafeFileName(Name), TEXT("Unsafe filename was accepted"));
	}
	Verify(FSBSStatics::IsSafeFileName(TEXT("Factory Mk.2 (copper)")), TEXT("Valid filename was rejected"));
	Verify(FSBSStatics::IsSafeIdentifier(TEXT("123abc-ABC_4")), TEXT("Valid ID was rejected"));
	Verify(!FSBSStatics::IsSafeIdentifier(TEXT("../admin?file=x")), TEXT("Unsafe ID was accepted"));

	FBlueprintJsonStructure Blueprint;
	Blueprint.parse(nullptr);
	Verify(Blueprint.Downloads == 0 && Blueprint.TotalRatingCount == 0,
		   TEXT("Missing numeric fields lack safe defaults"));
	const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("_id"), TEXT("sample"));
	Json->SetNumberField(TEXT("downloads"), 4);
	const TSharedRef<FJsonObject> Tag = MakeShared<FJsonObject>();
	Tag->SetStringField(TEXT("_id"), TEXT("tag"));
	Json->SetArrayField(
		TEXT("tags"),
		{MakeShared<FJsonValueNull>(), MakeShared<FJsonValueString>(TEXT("bad")), MakeShared<FJsonValueObject>(Tag)});
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Blueprint.setJsonObject(Json);
		Blueprint.parse(nullptr);
		Verify(Blueprint.Tags.Num() == 1 && Blueprint.Downloads == 4,
			   TEXT("Repeated parsing accumulated tags or lost fields"));
	}
	Blueprint.setJsonObject(MakeShared<FJsonObject>());
	Blueprint.parse(nullptr);
	Verify(Blueprint.ID.IsEmpty() && Blueprint.Tags.IsEmpty() && Blueprint.Downloads == 0,
		   TEXT("Parsing retained stale fields"));
	FBlueprintPackJsonStructure Pack;
	Pack.setJsonObject(MakeShared<FJsonObject>());
	Pack.parse(nullptr);
	Verify(Pack.Blueprints.IsEmpty() && Pack.ImageUrl.IsEmpty(), TEXT("Empty pack produced an invalid preview"));
	FBlueprintInPackJsonStructure PackItem;
	PackItem.parse(nullptr);
	Verify(PackItem.ID.IsEmpty(), TEXT("Null pack item parse failed"));
	FBlueprintJsonColorStructure Icon;
	Icon.setJsonObject(MakeShared<FJsonObject>());
	Icon.parse(nullptr);
	Verify(Icon.Color == FLinearColor::White && Icon.IconID == 0, TEXT("Missing color channels lack safe defaults"));

	FFilterPostStruct Filter;
	Filter.Limit = -20;
	Filter.Skip = -1;
	Filter.Name = TEXT("quote\" and slash\\");
	Filter.Tags.Add(TEXT("tag"));
	TSharedPtr<FJsonObject> Serialized;
	Verify(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Filter.ToString()), Serialized),
		   TEXT("Filter JSON invalid"));
	Verify(Serialized && Serialized->GetIntegerField(TEXT("limit")) == 1 &&
			   Serialized->GetIntegerField(TEXT("skip")) == 0,
		   TEXT("Pagination limits not enforced"));
	FRatingPostStruct Rating;
	Rating.BlueprintID = TEXT("sample");
	Rating.Rating = 4;
	Serialized.Reset();
	Verify(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Rating.ToString()), Serialized) &&
			   Serialized->GetStringField(TEXT("blueprintId")) == Rating.BlueprintID &&
			   Serialized->GetIntegerField(TEXT("rating")) == 4,
		   TEXT("Concrete rating payload lost its fields"));

	uint8 Bytes[] = {1, 2, 3, 4, 5};
	FSBSResponseBuffer Buffer(4);
	Buffer.Serialize(Bytes, 4);
	Verify(!Buffer.IsError() && Buffer.mData.Num() == 4, TEXT("Receive buffer rejected its exact limit"));
	Buffer.Serialize(Bytes, 1);
	Verify(Buffer.IsError() && Buffer.mData.Num() == 4, TEXT("Receive buffer exceeded its memory limit"));

	const FString Directory =
		FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("SBS"))->GetBaseDir(), TEXT(".validation"),
						TEXT("files-") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
	IFileManager::Get().MakeDirectory(*Directory, true);
	const FString SbpPath = FPaths::Combine(Directory, TEXT("sample.sbp"));
	const FString ConfigPath = FPaths::Combine(Directory, TEXT("sample.sbpcfg"));
	const TArray<uint8> OldSbp{1, 2};
	const TArray<uint8> OldConfig{3, 4};
	const TArray<uint8> NewSbp{5, 6};
	const TArray<uint8> NewConfig{7, 8};
	Verify(FFileHelper::SaveArrayToFile(OldSbp, *SbpPath) && FFileHelper::SaveArrayToFile(OldConfig, *ConfigPath),
		   TEXT("Could not create rollback fixture"));
	const auto Matches = [](const FString& Path, const TArray<uint8>& Expected)
	{
		TArray<uint8> Actual;
		return FFileHelper::LoadFileToArray(Actual, *Path) && Actual == Expected;
	};
	{
		FSBSBlueprintFileTransaction Transaction(Directory, TEXT("sample"), NewSbp, NewConfig);
		Verify(Transaction.Stage(), TEXT("Staging failed"));
		Verify(Matches(SbpPath, OldSbp) && Matches(ConfigPath, OldConfig), TEXT("Staging modified existing files"));
		Verify(Transaction.Commit(), TEXT("File pair commit failed"));
		Verify(Matches(SbpPath, NewSbp) && Matches(ConfigPath, NewConfig), TEXT("Committed file contents differ"));
		Verify(Transaction.Rollback(), TEXT("Explicit rollback failed"));
		Verify(Matches(SbpPath, OldSbp) && Matches(ConfigPath, OldConfig), TEXT("Rollback lost existing files"));
	}
	{
		FSBSBlueprintFileTransaction Transaction(Directory, TEXT("sample"), NewSbp, NewConfig);
		Verify(Transaction.Stage() && Transaction.Commit(), TEXT("Unaccepted file pair commit failed"));
	}
	Verify(Matches(SbpPath, OldSbp) && Matches(ConfigPath, OldConfig),
		   TEXT("Abandoned commit did not restore original files"));
	{
		FSBSBlueprintFileTransaction Transaction(Directory, TEXT("sample"), NewSbp, NewConfig);
		Verify(Transaction.Stage() && Transaction.Commit(), TEXT("Second file pair commit failed"));
		Transaction.Accept();
	}
	Verify(Matches(SbpPath, NewSbp) && Matches(ConfigPath, NewConfig), TEXT("Accepted files did not survive cleanup"));
	{
		const FString BlockedConfig = FPaths::Combine(Directory, TEXT("blocked.sbpcfg"));
		const FString PreservedSbp = FPaths::Combine(Directory, TEXT("blocked.sbp"));
		IFileManager::Get().MakeDirectory(*BlockedConfig, false);
		FFileHelper::SaveArrayToFile(OldSbp, *PreservedSbp);
		FSBSBlueprintFileTransaction Transaction(Directory, TEXT("blocked"), NewSbp, NewConfig);
		Verify(Transaction.Stage() && !Transaction.Commit(), TEXT("Blocked second destination should reject the pair"));
		Verify(Matches(PreservedSbp, OldSbp), TEXT("Failed second rename did not restore the first file"));
	}
	Report->SetNumberField(TEXT("checks"), Checks);
	Report->SetArrayField(TEXT("failures"), Failures);
	Report->SetStringField(TEXT("directory"), Directory);
	FString Text;
	FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Text));
	return Text;
}
