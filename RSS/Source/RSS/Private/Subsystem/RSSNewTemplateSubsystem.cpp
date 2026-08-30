

#include "Subsystem/RSSNewTemplateSubsystem.h"

#include "Serialization/RssJsonSerializer.h"
#include "Subsystem/RSSImageSubsystem.h"
#include "Subsystem/RSSTemplateSubsystem.h"

ARSSNewTemplateSubsystem::ARSSNewTemplateSubsystem()
{

	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnLocal;
}

void ARSSNewTemplateSubsystem::BeginPlay() { Super::BeginPlay(); }

bool ARSSNewTemplateSubsystem::GenerateFromFileInVar()
{
	FString FileContent;
	if (ReadTemplatesFromFile(FileContent))
	{

		FString TrimmedContent = FileContent.TrimStartAndEnd();
		if (TrimmedContent.StartsWith(TEXT("{")))
		{

			TArray<FRssSignData> Templates;
			if (FRssJsonSerializer::JsonToTemplates(FileContent, Templates))
			{
				mTemplates.mTemplates = Templates;
				return true;
			}
		}
		else if (!TrimmedContent.IsEmpty())
		{

			if (ARSSImageSubsystem::CheckString(FileContent, {"mSignType", "mSignTypeSize", "mTemplates"}))
			{

				mTemplates = StringToTemplates(this, FileContent);

				SaveTemplatesToFile();
				return true;
			}
		}
	}
	return false;
}

bool ARSSNewTemplateSubsystem::ReadTemplatesFromFile(FString& FileContent)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	FString FilePath = GetFilePath();

	if (FileManager.FileExists(*FilePath))
	{
		return FFileHelper::LoadFileToString(FileContent, *FilePath, FFileHelper::EHashOptions::None);
	}

	return false;
}

FString ARSSNewTemplateSubsystem::GetFilePath()
{
	FString file = FPaths::ProjectConfigDir();
	if (mAddFilePath != "")
	{
		file.Append(mAddFilePath);
	}
	file.Append(mFileName);

	return file;
}

bool ARSSNewTemplateSubsystem::MigrateFromOld()
{
	SaveTemplatesToFile();
	if (HasAuthority())
	{
		GenerateFromFileInVar();

		ARSSTemplateSubsystem* Subsystem = ARSSTemplateSubsystem::GetOldRSSTemplateSubsystem(this);
		if (Subsystem)
		{
			for (FRssSignData Template : Subsystem->mTemplates)
			{
				AddTemplate(Template);
			}
			Subsystem->ClearTemplates();
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool ARSSNewTemplateSubsystem::SaveTemplatesToFile()
{
	const FString FilePath = GetFilePath();

	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	const FString BackupPath = FilePath + TEXT(".bak");
	if (FileManager.FileExists(*FilePath) && !FileManager.FileExists(*BackupPath))
	{
		if (!FileManager.CopyFile(*BackupPath, *FilePath))
		{

			UE_LOG(LogTemp, Error, TEXT("RSS: failed to back up %s aborting save"), *FilePath);
			return false;
		}
	}

	FString Data = FRssJsonSerializer::TemplatesToJson(mTemplates.mTemplates);
	return FFileHelper::SaveStringToFile(Data, *FilePath);
}

FString ARSSNewTemplateSubsystem::TemplatesToString(UObject* WorldContext, FRssTemplates Templates)
{
	const UScriptStruct* Struct = Templates.StaticStruct();
	FString Output = TEXT("");
	Struct->ExportText(Output, &Templates, new FRssTemplates, WorldContext,
					   (PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited | PPF_IncludeTransient), nullptr);
	return Output;
}

FRssTemplates ARSSNewTemplateSubsystem::StringToTemplates(UObject* WorldContext, FString Templates)
{
	FRssTemplates OutStruc = FRssTemplates();

	static UScriptStruct* Struct = OutStruc.StaticStruct();
	Struct->ImportText(*Templates, &OutStruc, WorldContext, (PPF_Copy | PPF_Delimited | PPF_IncludeTransient), nullptr,
					   "FRssTemplates");

	return OutStruc;
}

TArray<FRssTemplateSignData> ARSSNewTemplateSubsystem::GetTemplates(ESignType SignType)
{
	TArray<FRssTemplateSignData> Array;
	for (int i = 0; i < mTemplates.mTemplates.Num(); ++i)
	{
		FRssTemplateSignData Info;
		Info.mIndex = i;
		Info.mSignData = mTemplates.mTemplates[i];
		if (mTemplates.mTemplates[i].mSignType == SignType)
		{
			Array.Add(Info);
		}
	}
	return Array;
}

TArray<FRssTemplateSignData> ARSSNewTemplateSubsystem::GetAllTemplates()
{
	TArray<FRssTemplateSignData> Array;
	for (int i = 0; i < mTemplates.mTemplates.Num(); ++i)
	{
		FRssTemplateSignData Info;
		Info.mIndex = i;
		Info.mSignData = mTemplates.mTemplates[i];
		Array.Add(Info);
	}

	return Array;
}

TArray<FRssTemplateSignData> ARSSNewTemplateSubsystem::GetTemplatesOfSignSize(ESignSize SignSize,
																			  ESignType SignType) const
{
	TArray<FRssTemplateSignData> Array;
	for (int i = 0; i < mTemplates.mTemplates.Num(); ++i)
	{
		FRssTemplateSignData Info;
		Info.mIndex = i;
		Info.mSignData = mTemplates.mTemplates[i];
		if (mTemplates.mTemplates[i].mSignTypeSize == SignSize && mTemplates.mTemplates[i].mSignType == SignType)
		{
			Array.Add(Info);
		}
	}
	return Array;
}

TArray<FRssTemplateSignData> ARSSNewTemplateSubsystem::GetAllTemplatesOfSignSize(ESignSize SignSize) const
{
	TArray<FRssTemplateSignData> Array;
	for (int i = 0; i < mTemplates.mTemplates.Num(); ++i)
	{
		FRssTemplateSignData Info;
		Info.mIndex = i;
		Info.mSignData = mTemplates.mTemplates[i];
		if (mTemplates.mTemplates[i].mSignTypeSize == SignSize)
		{
			Array.Add(Info);
		}
	}
	return Array;
}

void ARSSNewTemplateSubsystem::RenameTemplate(const FText& NewName, int TemplateIdx)
{
	if (mTemplates.mTemplates.IsValidIndex(TemplateIdx))
	{
		mTemplates.mTemplates[TemplateIdx].mTemplateData.mTemplateName = NewName;
		SaveTemplatesToFile();
	}
}

void ARSSNewTemplateSubsystem::AddTemplate(FRssSignData Data)
{
	mTemplates.mTemplates.Add(Data);
	SaveTemplatesToFile();
}

void ARSSNewTemplateSubsystem::RemoveTemplate(int TemplateIdx)
{
	if (!mTemplates.mTemplates.IsValidIndex(TemplateIdx))
	{
		return;
	}
	mTemplates.mTemplates.RemoveAt(TemplateIdx);
	SaveTemplatesToFile();
}
