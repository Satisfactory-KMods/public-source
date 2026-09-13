// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Download/SBSBlueprintFileTransaction.h"

#include "HAL/FileManager.h"
#include "Logging.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Structures/ApiStatics.h"

FSBSBlueprintFileTransaction::FSBSBlueprintFileTransaction(FString Directory, FString Name, TArray<uint8> Sbp,
														   TArray<uint8> Config) :
	mDirectory(MoveTemp(Directory)), mName(MoveTemp(Name)), mSbp(MoveTemp(Sbp)), mConfig(MoveTemp(Config))
{
	mStagingDirectory = FPaths::Combine(mDirectory, TEXT(".sbs-") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
}

FSBSBlueprintFileTransaction::~FSBSBlueprintFileTransaction()
{
	if (bInstalled[0] || bInstalled[1] || bBackedUp[0] || bBackedUp[1])
	{
		Rollback();
	}
	Cleanup();
}

FString FSBSBlueprintFileTransaction::Destination(int32 Index) const
{
	return FPaths::Combine(mDirectory, mName + (Index == 0 ? TEXT(".sbp") : TEXT(".sbpcfg")));
}

FString FSBSBlueprintFileTransaction::Staged(int32 Index) const
{
	return FPaths::Combine(mStagingDirectory, Index == 0 ? TEXT("new.sbp") : TEXT("new.sbpcfg"));
}

FString FSBSBlueprintFileTransaction::Backup(int32 Index) const
{
	return FPaths::Combine(mStagingDirectory, Index == 0 ? TEXT("old.sbp") : TEXT("old.sbpcfg"));
}

bool FSBSBlueprintFileTransaction::Stage()
{
	if (mDirectory.IsEmpty() || !FSBSStatics::IsSafeFileName(mName) || mSbp.IsEmpty() || mConfig.IsEmpty() ||
		!FPaths::IsUnderDirectory(mStagingDirectory, mDirectory) ||
		!IFileManager::Get().MakeDirectory(*mStagingDirectory, true))
	{
		return false;
	}
	const bool bSaved =
		FFileHelper::SaveArrayToFile(mSbp, *Staged(0)) && FFileHelper::SaveArrayToFile(mConfig, *Staged(1));
	mSbp.Empty();
	mConfig.Empty();
	return bSaved;
}

bool FSBSBlueprintFileTransaction::Commit()
{
	IFileManager& Files = IFileManager::Get();
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const FString Target = Destination(Index);
		if (!Files.FileExists(*Staged(Index)) || !FPaths::IsUnderDirectory(Target, mDirectory))
		{
			Rollback();
			return false;
		}
		if (Files.FileExists(*Target))
		{
			if (!Files.Move(*Backup(Index), *Target, false, false, false, true))
			{
				Rollback();
				return false;
			}
			bBackedUp[Index] = true;
		}
	}
	for (int32 Index = 0; Index < 2; ++Index)
	{
		if (!Files.Move(*Destination(Index), *Staged(Index), false, false, false, true))
		{
			Rollback();
			return false;
		}
		bInstalled[Index] = true;
	}
	return true;
}

bool FSBSBlueprintFileTransaction::Rollback()
{
	IFileManager& Files = IFileManager::Get();
	bool bRestored = true;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		if (bInstalled[Index])
		{
			if (!Files.Delete(*Destination(Index), false, false, true))
			{
				bRestored = false;
				continue;
			}
			bInstalled[Index] = false;
		}
		if (bBackedUp[Index])
		{
			if (!Files.Move(*Destination(Index), *Backup(Index), false, false, false, true))
			{
				bRestored = false;
				continue;
			}
			bBackedUp[Index] = false;
		}
	}
	bKeepRecoveryFiles = !bRestored;
	if (!bRestored)
	{
		UE_LOG(LogSBS, Error, TEXT("Blueprint rollback incomplete. Original files preserved in %s"),
			   *mStagingDirectory);
	}
	return bRestored;
}

void FSBSBlueprintFileTransaction::Accept()
{
	for (int32 Index = 0; Index < 2; ++Index)
	{
		bInstalled[Index] = false;
		bBackedUp[Index] = false;
	}
	Cleanup();
}

void FSBSBlueprintFileTransaction::Cleanup()
{
	if (bKeepRecoveryFiles)
	{
		return;
	}
	IFileManager& Files = IFileManager::Get();
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Files.Delete(*Staged(Index), false, false, true);
		Files.Delete(*Backup(Index), false, false, true);
	}
	Files.DeleteDirectory(*mStagingDirectory, false, false);
}
