

#pragma once

#include "CoreMinimal.h"
#include "KBFLSubLevelSpawning.generated.h"

UCLASS(Blueprintable, BlueprintType)
class KBFL_API UKBFLSubLevelSpawning : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
#if WITH_ENGINE
	virtual UWorld* GetWorld() const override;
#endif

	virtual void InitSpawning();

	virtual void SpawnSubLevel();

	virtual void Reset();

	UFUNCTION()
	virtual void StreamingLevelReceived();

	UFUNCTION()
	virtual void OnLevelLoaded();

	UFUNCTION()
	virtual void OnLevelShown();

	virtual void ProcessLoadedLevel(ULevelStreaming* LevelStreaming);

	bool CheckWorld() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool ExecuteAllowed() const;
	virtual bool ExecuteAllowed_Implementation() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FString mMapName = "Persistent_Level";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool mNeedAuth = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool mDetailedDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SubLevelSpawing")
	TArray<TSoftObjectPtr<UWorld>> mSubLevelArray;

	UPROPERTY(Transient, BlueprintReadWrite)
	TObjectPtr<class UKBFLResourceNodeSubsystem> mSubsystem = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<ULevelStreaming>> mLevelStreaming;

	UPROPERTY()
	TArray<TObjectPtr<ULevelStreaming>> mAddedLevel;

	UPROPERTY()
	TArray<TObjectPtr<ULevel>> mCachedLevels;
};
