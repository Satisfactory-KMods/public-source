// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/RSSNewTemplateSubsystem.h"

#include "Subsystem/RSSImageSubsystem.h"
#include "Subsystem/RSSTemplateSubsystem.h"

// Sets default values
ARSSNewTemplateSubsystem::ARSSNewTemplateSubsystem()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
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
		if (ARSSImageSubsystem::CheckString(FileContent, {"mSignType", "mSignTypeSize", "mTemplates"}))
		{
			mTemplates = StringToTemplates(this, FileContent);
			return true;
		}
		SaveTemplatesToFile();
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

	FString Data = TemplatesToString(this, mTemplates);
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
