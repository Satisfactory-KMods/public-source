#pragma once

#include "CoreMinimal.h"
#include "Subsystems/HelperClasses/KBFLCDOOverwriteBase.h"

#include "KBFLWorldCDOActorDestroyer.generated.h"

UCLASS(NotBlueprintable, NotBlueprintType)
class KBFL_API UKBFLWorldCDOActorDestroyer : public UKBFLCDOOverwriteWorldBasedBase
{
	GENERATED_BODY()

public:

	virtual void ApplyToActorsInWorld() override;

	void DestoryAll();

	virtual void Start() override;

	virtual void Clear() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Destroyer Settings")
	bool bShouldDestroySpawnedActors = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Destroyer Settings")
	bool bUseSubclassCheck = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowAbstract = "true"), Category = "Target Actors")
	TArray<TSubclassOf<AActor>> mActorClassesToDestroy;

	UFUNCTION()
	void OnActorEvent(AActor* Actor);

protected:
	FDelegateHandle mActorHandle;

	virtual void HandleDestroyActor(AActor* Actor);
};
