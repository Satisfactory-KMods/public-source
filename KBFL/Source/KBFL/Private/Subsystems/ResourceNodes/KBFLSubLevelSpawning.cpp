// Copyright Coffee Stain Studios. All Rights Reserved.


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
		// Don't call StreamingLevelReceived() here - it will be called via LatentActionInfo callback
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

		// Create a unique but deterministic name for this level instance
		// This ensures the same name is used across saves/loads
		FString UniqueLevelName = FString::Printf(TEXT("%s_KBFL"), *Level.GetAssetName());

		// Use LoadLevelInstance with a fixed unique ID for consistent naming
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

			// For World Partition levels, we need to wait for the level to be fully loaded
			// The callback will be triggered automatically when the level is ready
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
	// Unbind the dynamic delegates we registered on the streaming-level objects. They point at THIS
	// persistent data asset (reused across worlds): leaving them bound leaks references into the dying
	// world's streaming objects and lets stale callbacks fire on this asset during world travel.
	for (ULevelStreaming* LevelStreaming : mLevelStreaming)
	{
		if (IsValid(LevelStreaming))
		{
			LevelStreaming->OnLevelLoaded.RemoveDynamic(this, &UKBFLSubLevelSpawning::OnLevelLoaded);
			LevelStreaming->OnLevelShown.RemoveDynamic(this, &UKBFLSubLevelSpawning::OnLevelShown);
		}
	}

	// Empty all Ref if we load to a other savegame
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
			// Check if we allready add that stuff to the Session
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

			// Check if we have a valid Level
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

	// Process all levels that might be ready now
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

	// Check if we already added this level
	if (mAddedLevel.Contains(LevelStreaming))
	{
		return;
	}

	UE_LOG(KBFLSubLevelSpawnerLog, Log, TEXT("Processing loaded level: %s"),
		   *LevelStreaming->GetWorldAssetPackageName());

	// make sure that the Level is loaded should not needed because we use ULevelStreamingAlwaysLoaded
	LevelStreaming->SetShouldBeLoaded(true);
	ULevel* LoadLevel = LevelStreaming->GetLoadedLevel();

	if (!LoadLevel)
	{
		UE_LOG(KBFLSubLevelSpawnerLog, Warning, TEXT("LoadLevel is null for: %s"),
			   *LevelStreaming->GetWorldAssetPackageName());
		return;
	}

	// Cache the level
	mCachedLevels.AddUnique(LoadLevel);

	// world! "Levels.AddUnique( InLevel );"
	GetWorld()->AddLevel(LoadLevel);

	// Mark as added BEFORE notifying save session
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
	// for BP uses for example config etc.
	return bEnabled;
}
