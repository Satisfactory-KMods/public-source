// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "KBFLSubLevelSpawning.generated.h"

/**
 * Class for adding Sublevel in the world
 * Dont need any registration will automaticly added to the world if our subsystem is loaded
 */
UCLASS(Blueprintable, BlueprintType)
class KBFL_API UKBFLSubLevelSpawning : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
#if WITH_ENGINE
	virtual UWorld* GetWorld() const override;
#endif

	// Init the Spawning sequenz
	virtual void InitSpawning();

	// Spawn all Level in the world
	virtual void SpawnSubLevel();

	// Called from subsystem to reset Informations
	virtual void Reset();

	// Will Called after InitSpawning() and if the Level was fully added to the world >> Register all saving stuff
	UFUNCTION()
	virtual void StreamingLevelReceived();

	// Callbacks for level streaming events (World Partition compatible)
	UFUNCTION()
	virtual void OnLevelLoaded();

	UFUNCTION()
	virtual void OnLevelShown();

	// Process a level that has been loaded
	virtual void ProcessLoadedLevel(ULevelStreaming* LevelStreaming);

	// Check if mMapName match the Map (should only spawn in these map)
	bool CheckWorld() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool ExecuteAllowed() const;
	virtual bool ExecuteAllowed_Implementation() const;

	// Map where we want to load our Sublevel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FString mMapName = "Persistent_Level";

	// Do the Player need auth to spawn?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool mNeedAuth = false;

	// Should we print more debugs?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool mDetailedDebug = true;

	// Should we print more debugs?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnabled = true;

	// All Worlds that need to add to these world
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SubLevelSpawing")
	TArray<TSoftObjectPtr<UWorld>> mSubLevelArray;

	UPROPERTY(Transient, BlueprintReadWrite)
	class UKBFLResourceNodeSubsystem* mSubsystem = nullptr;

	UPROPERTY()
	TArray<ULevelStreaming*> mLevelStreaming;

	UPROPERTY()
	TArray<ULevelStreaming*> mAddedLevel;

	UPROPERTY()
	TArray<ULevel*> mCachedLevels;
};
