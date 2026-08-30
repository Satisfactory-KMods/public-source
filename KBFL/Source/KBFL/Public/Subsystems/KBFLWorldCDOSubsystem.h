#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "KBFLWorldCDOSubsystem.generated.h"

UCLASS()
class KBFL_API UKBFLWorldCDOSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;

	virtual TStatId GetStatId() const override;

	static UKBFLWorldCDOSubsystem* Get(UObject* WorldContext);

	void OnWorldPostInit();

	UFUNCTION(BlueprintCallable, Category = "KBFL|World CDO")
	void RegisterWorldCDOOverwrite(class UKBFLCDOOverwriteWorldBasedBase* Overwrite);

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
