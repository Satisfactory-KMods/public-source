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
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// UTickableWorldSubsystem interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

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
	TSet<class UKBFLCDOOverwriteWorldBasedBase*> mRegisteredWorldOverwrites;

	UPROPERTY()
	TSet<class UKBFLCDOOverwriteWorldBasedBase*> mTickableWorldOverwrites;

	FDelegateHandle mOnWorldPostInitHandle;
};
