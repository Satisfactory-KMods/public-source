// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/HelperClasses/KBFLCDOOverwriteBase.h"

#include "KBFLWorldCDOActorListener.generated.h"

class ULevel;

UENUM(BlueprintType)
enum class EKBFLWorldCDOActorListenerEvent : uint8
{
	SpawnedActorEvent,
	DestroyedActorEvent,
};

UCLASS(NotBlueprintable, NotBlueprintType)
class KBFL_API UKBFLWorldCDOActorListener : public UKBFLCDOOverwriteWorldBasedBase
{
	GENERATED_BODY()

public:

	virtual void Start() override;

	virtual void ApplyToActorsInWorld() override;

	virtual void Clear() override;

	UFUNCTION()
	void OnActorEvent(AActor* Actor);

	virtual void OnActorMatched(AActor* Actor) {}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Listener Settings")
	EKBFLWorldCDOActorListenerEvent mEventToListenFor = EKBFLWorldCDOActorListenerEvent::SpawnedActorEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Listener Settings")
	bool bUseSubclassCheck = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Listener Settings")
	bool bListenStreamingLevel = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowAbstract = "true"), Category = "Target Actors")
	TArray<TSubclassOf<AActor>> mActorClassesToListenFor;

protected:

	void OnLevelAddedToWorld(ULevel* Level, UWorld* World);

	FDelegateHandle mActorHandle;
	FDelegateHandle mLevelAddedHandle;
};
