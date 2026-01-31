#pragma once

#include "CoreMinimal.h"
#include "Module/GameWorldModule.h"
#include "Subsystems/WorldSubsystem.h"

#include "KBFLResourceNodeSubsystem.generated.h"


// Native
USTRUCT()
struct FKBFLActorArray
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<AActor*> mActors;
};

/**
 *
 */
UCLASS()
class KBFL_API UKBFLResourceNodeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	/** Implement this for initialization of instances of the system */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	void SpawnSubLevel();
	void SpawnDescriptors();

	UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable)
	void Server_FinishedSpawningNodes();

private:
	bool Initialized = false;

	UPROPERTY()
	TArray<class UKBFLSubLevelSpawning*> mCalledSubLevelSpawning;

	UPROPERTY()
	TArray<class UKBFLActorSpawnDescriptorBase*> mCalledDescriptors;
};
