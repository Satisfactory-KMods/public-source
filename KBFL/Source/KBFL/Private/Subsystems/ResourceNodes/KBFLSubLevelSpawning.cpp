// Copyright Kyri123 / KMods 2026. All Rights Reserved.



#include "Subsystems/ResourceNodes/KBFLSubLevelSpawning.h"

#include "Engine/LevelStreamingDynamic.h"
#include "FGSaveSession.h"
#include "FGWorldSettings.h"
#include "KBFLLogging.h"
#include "Logging/StructuredLog.h"
#include "Subsystems/KBFLResourceNodeSubsystem.h"

#if WITH_ENGINE
UWorld* UKBFLSubLevelSpawning::GetWorld() const
{
	if (IsValid(mSubsystem))
	{
		return mSubsystem->GetWorld();
	}
	return Super::GetWorld();
}
#endif

void UKBFLSubLevelSpawning::InitSpawning()
{
	if (CheckWorld())
	{
		UE_LOG(KBFLSubLevelSpawnerLog, Log, TEXT("Init SubLevelSpawning %s"), *GetName());
		SpawnSubLevel();

	}
	else
	{
		UE_LOG(KBFLSubLevelSpawnerLog, Log, TEXT("Skip SubLevelSpawning because wrong world %s"), *GetName());
	}
}

void UKBFLSubLevelSpawning::SpawnSubLevel()
{
	UE_LOG(KBFLSubLevelSpawnerLog, Log, TEXT("Try to load total of %d levels"), mSubLevelArray.Num());
	for (TSoftObjectPtr<UWorld> Level : mSubLevelArray)
	{
		FName LevelName = FName(*Level.GetLongPackageName());
		UE_LOG(KBFLSubLevelSpawnerLog, Log, TEXT("Try to load Level %s"), *LevelName.ToString());

		bool HasLevelLoaded = false;
		TArray<ULevelStreaming*> FoundLevels = GetWorld()->GetStreamingLevels();
		for (ULevelStreaming* lvl : FoundLevels)
		{
			FName f = lvl->GetWorldAssetPackageFName();

			if (f == LevelName)
			{
				HasLevelLoaded = true;
			}
		}

		if (HasLevelLoaded)
		{
			UE_LOG(KBFLSubLevelSpawnerLog, Error, TEXT("Level %s is already loaded in save!"), *LevelName.ToString());
			continue;
		}

		FString PackageFileName;
		if (!FPackageName::DoesPackageExist(LevelName.ToString(), &PackageFileName))
		{
			UE_LOG(KBFLSubLevelSpawnerLog, Error, TEXT("trying to load invalid level %s"), *LevelName.ToString());
			continue;
		}

		FString UniqueLevelName = FString::Printf(TEXT("%s_KBFL"), *Level.GetAssetName());

		bool bOutSuccess = false;
		ULevelStreamingDynamic* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstance(
			GetWorld(), LevelName.ToString(), FVector::ZeroVector, FRotator::ZeroRotator, bOutSuccess, UniqueLevelName);

		if (StreamingLevel && bOutSuccess)
		{
			StreamingLevel->SetShouldBeLoaded(true);
			StreamingLevel->SetShouldBeVisible(true);
			StreamingLevel->bShouldBlockOnLoad = false;
			StreamingLevel->SetPriority(-100);

			UE_LOG(KBFLSubLevelSpawnerLog, Display, TEXT("Level streaming setup complete: %s (Instance: %s)"),
				   *LevelName.ToString(), *StreamingLevel->GetWorldAssetPackageName());

			mLevelStreaming.Add(StreamingLevel);

			StreamingLevel->OnLevelLoaded.AddDynamic(this, &UKBFLSubLevelSpawning::OnLevelLoaded);
			StreamingLevel->OnLevelShown.AddDynamic(this, &UKBFLSubLevelSpawning::OnLevelShown);
		}
		else
		{
			UE_LOG(KBFLSubLevelSpawnerLog, Error, TEXT("Failed to create streaming level for %s"),
				   *LevelName.ToString());
		}
	}
}

void UKBFLSubLevelSpawning::Reset()
{

	for (ULevelStreaming* LevelStreaming : mLevelStreaming)
	{
		if (IsValid(LevelStreaming))
		{
			LevelStreaming->OnLevelLoaded.RemoveDynamic(this, &UKBFLSubLevelSpawning::OnLevelLoaded);
			LevelStreaming->OnLevelShown.RemoveDynamic(this, &UKBFLSubLevelSpawning::OnLevelShown);
		}
	}

	mSubsystem = nullptr;
	mLevelStreaming.Empty();
	mAddedLevel.Empty();
	mCachedLevels.Empty();
}

void UKBFLSubLevelSpawning::StreamingLevelReceived()
{
	UE_LOG(KBFLSubLevelSpawnerLog, Log, TEXT("StreamingLevelReceived callback called for %s"), *GetName());

	for (ULevelStreaming* LevelStreaming : mLevelStreaming)
	{
		if (LevelStreaming)
		{

			if (mAddedLevel.Contains(LevelStreaming))
			{
				UE_LOG(KBFLSubLevelSpawnerLog, Log, TEXT("Level already processed, skipping: %s"),
					   *LevelStreaming->GetWorldAssetPackageName());
				continue;
			}

			UE_LOG(KBFLSubLevelSpawnerLog, Log, TEXT("Processing level: %s, HasLoadedLevel: %d"),
				   *LevelStreaming->GetWorldAssetPackageName(), LevelStreaming->HasLoadedLevel());

			LevelStreaming->SetShouldBeLoaded(true);
			LevelStreaming->SetShouldBeVisible(true);

			GetWorld()->FlushLevelStreaming(EFlushLevelStreamingType::Full);

			if (LevelStreaming->HasLoadedLevel())
			{
				ProcessLoadedLevel(LevelStreaming);
			}
			else
			{
				UE_LOG(KBFLSubLevelSpawnerLog, Warning, TEXT("Level not loaded yet: %s"),
					   *LevelStreaming->GetWorldAssetPackageName());
			}
		}
	}
}

void UKBFLSubLevelSpawning::OnLevelLoaded()
{
	UE_LOG(KBFLSubLevelSpawnerLog, Log, TEXT("OnLevelLoaded callback triggered for %s"), *GetName());

	for (ULevelStreaming* LevelStreaming : mLevelStreaming)
	{
		if (LevelStreaming && LevelStreaming->HasLoadedLevel() && !mAddedLevel.Contains(LevelStreaming))
		{
			ProcessLoadedLevel(LevelStreaming);
		}
	}
}

void UKBFLSubLevelSpawning::OnLevelShown()
{
	UE_LOG(KBFLSubLevelSpawnerLog, Log, TEXT("OnLevelShown callback triggered for %s"), *GetName());
}

void UKBFLSubLevelSpawning::ProcessLoadedLevel(ULevelStreaming* LevelStreaming)
{
	if (!LevelStreaming || !LevelStreaming->HasLoadedLevel())
	{
		return;
	}

	if (mAddedLevel.Contains(LevelStreaming))
	{
		return;
	}

	UE_LOG(KBFLSubLevelSpawnerLog, Log, TEXT("Processing loaded level: %s"),
		   *LevelStreaming->GetWorldAssetPackageName());

	LevelStreaming->SetShouldBeLoaded(true);
	ULevel* LoadLevel = LevelStreaming->GetLoadedLevel();

	if (!LoadLevel)
	{
		UE_LOG(KBFLSubLevelSpawnerLog, Warning, TEXT("LoadLevel is null for: %s"),
			   *LevelStreaming->GetWorldAssetPackageName());
		return;
	}

	mCachedLevels.AddUnique(LoadLevel);

	GetWorld()->AddLevel(LoadLevel);

	mAddedLevel.Add(LevelStreaming);

	if (UFGSaveSession* Session = UFGSaveSession::Get(GetWorld()))
	{
		Session->OnLevelAddedToWorld(LoadLevel, GetWorld());
		UE_LOGFMT(KBFLSubLevelSpawnerLog, Log, "Notify SaveSession about loaded Level: {0}", LoadLevel->GetName());
	}
	else
	{
		UE_LOG(KBFLSubLevelSpawnerLog, Warning, TEXT("No SaveSession found to notify about level load"));
	}

	if (IsValid(mSubsystem))
	{
		mSubsystem->Server_FinishedSpawningNodes();
	}
}

bool UKBFLSubLevelSpawning::CheckWorld() const
{
	if (GetWorld())
	{
#if WITH_EDITOR
		return FString("UEDPIE_0_").Append(mMapName) != GetWorld()->GetMapName();
#else
		return mMapName == GetWorld()->GetMapName();
#endif
	}
	return false;
}

bool UKBFLSubLevelSpawning::ExecuteAllowed_Implementation() const
{

	return bEnabled;
}
