#pragma once

#include "CoreMinimal.h"
#include "Subsystems/HelperClasses/KBFLCDOOverwriteBase.h"

#include "KBFLWorldCDOActorListener.generated.h"

UENUM(BlueprintType)
enum class EKBFLWorldCDOActorListenerEvent : uint8
{
	SpawnedActorEvent,
	DestroyedActorEvent,
};

/**
 * WIP: Listens for actor spawns in the world and can apply modifications
 * Can be used to modify actors as they are spawned
 */
UCLASS(NotBlueprintable, NotBlueprintType)
class KBFL_API UKBFLWorldCDOActorListener : public UKBFLCDOOverwriteWorldBasedBase
{
	GENERATED_BODY()

public:
	virtual void Start() override;
	virtual void ApplyToActorsInWorld() override;
	virtual void Clear() override;

	// Called when a relevant actor is spawned
	UFUNCTION()
	void OnActorEvent(AActor* Actor);

	// Specify which actor classes to listen for
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowAbstract = "true"), Category = "Actor Listener")
	TArray<TSubclassOf<AActor>> mActorClassesToListenFor;

protected:
	FDelegateHandle mActorHandle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Listener")
	EKBFLWorldCDOActorListenerEvent mEventToListenFor = EKBFLWorldCDOActorListenerEvent::SpawnedActorEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actor Listener")
	bool bUseSubclassCheck = true;
};
