#pragma once

#include "CoreMinimal.h"
#include "Subsystems/HelperClasses/KBFLCDOOverwriteBase.h"

#include "KBFLWorldCDOActorDestroyer.generated.h"

/**
 * WIP: Destroys actors in the world based on specified criteria
 * More advanced than ContentRemover, can use custom logic for destruction
 */
UCLASS(NotBlueprintable, NotBlueprintType)
class KBFL_API UKBFLWorldCDOActorDestroyer : public UKBFLCDOOverwriteWorldBasedBase
{
	GENERATED_BODY()

public:
	virtual void ApplyToActorsInWorld() override;
	void DestoryAll();
	virtual void Start() override;
	virtual void Clear() override;

	// Specify which actor classes to destroy
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowAbstract = "true"), Category = "Actor Destroyer")
	TArray<TSubclassOf<AActor>> mActorClassesToDestroy;

	// Called when a relevant actor is spawned
	UFUNCTION()
	void OnActorEvent(AActor* Actor);

protected:
	FDelegateHandle mActorHandle;

	// Override this to implement custom destruction logic
	virtual void HandleDestroyActor(AActor* Actor);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actor Destroyer")
	bool bShouldDestroySpawnedActors = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actor Destroyer")
	bool bUseSubclassCheck = true;
};
