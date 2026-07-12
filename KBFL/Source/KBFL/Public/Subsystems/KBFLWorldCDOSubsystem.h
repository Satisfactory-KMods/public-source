#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "KBFLWorldCDOSubsystem.generated.h"

UCLASS()
class KBFL_API UKBFLWorldCDOSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	/** Initializes the subsystem and subscribes to the world's OnWorldBeginPlay to drive OnWorldPostInit. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Unsubscribes from OnWorldBeginPlay and calls Clear() on every registered world-based overwrite. */
	virtual void Deinitialize() override;

	// UTickableWorldSubsystem interface
	/** Ticks every registered world overwrite that opted in via ShouldTick(). */
	virtual void Tick(float DeltaTime) override;

	/** Returns the stat id used for tick profiling. */
	virtual TStatId GetStatId() const override;

	/** Returns this subsystem for the world resolved from WorldContext, or null if unavailable. */
	static UKBFLWorldCDOSubsystem* Get(UObject* WorldContext);

	/**
	 * Called after the world and actors are loaded but BEFORE BeginPlay
	 * Perfect timing to apply world-based CDO overwrites
	 * This happens after InitGameState (where save game is loaded) but before BeginPlay
	 */
	void OnWorldPostInit();

	/**
	 * Register a world-based CDO overwrite to be applied when the world begins play
	 */
	UFUNCTION(BlueprintCallable, Category = "KBFL|World CDO")
	void RegisterWorldCDOOverwrite(class UKBFLCDOOverwriteWorldBasedBase* Overwrite);

	/**
	 * Apply all registered world-based CDO overwrites
	 */
	UFUNCTION(BlueprintCallable, Category = "KBFL|World CDO")
	void ApplyAllWorldCDOOverwrites();

protected:
	UPROPERTY()
	bool bInitialized = false;

	UPROPERTY()
	bool bWorldPostInitCalled = false;

	UPROPERTY()
	TSet<TObjectPtr<class UKBFLCDOOverwriteWorldBasedBase>> mRegisteredWorldOverwrites;

	UPROPERTY()
	TSet<TObjectPtr<class UKBFLCDOOverwriteWorldBasedBase>> mTickableWorldOverwrites;

	FDelegateHandle mOnWorldPostInitHandle;
};
