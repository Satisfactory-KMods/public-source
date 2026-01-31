// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGSaveInterface.h"
#include "GameFramework/Actor.h"
#include "Subsystem/ModSubsystem.h"
#include "KPCLModSubsystem.generated.h"

class AKPCLModSubsystem;
/**
 * Tick function that calls AFGBuildable::TickFactory
 */
USTRUCT()
struct KPRIVATECODELIB_API FSubsystemTick : public FTickFunction
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Transient)
	AKPCLModSubsystem* mTarget;

	virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread,
	                         const FGraphEventRef& MyCompletionGraphEvent) override;
};

template <>
struct TStructOpsTypeTraits<FSubsystemTick> : TStructOpsTypeTraitsBase2<FSubsystemTick>
{
	enum
	{
		WithCopy = false
	};
};

UCLASS()
class KPRIVATECODELIB_API AKPCLModSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

public:
	/** ----- Patreon End ----- */

	// Sets default values for this actor's properties
	AKPCLModSubsystem();

	virtual void Init() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintNativeEvent, Category="KMods|Events")
	void OnInit();

	virtual void OnInit_Implementation()
	{
	}

	UFUNCTION()
	virtual void OnOptionsUpdated(FString UpdatedCVar)
	{
	}

	FORCEINLINE virtual bool ShouldSave_Implementation() const override { return mShouldSave; }

	/** Threaded tick for special uses */
	virtual void SubsytemTick(float dt)
	{
	};

	UPROPERTY(EditDefaultsOnly, Category="KMods|System")
	bool mShouldSave = false;

	/** Our Threaded Tick function */
	UPROPERTY(EditDefaultsOnly, Category="KMods|System")
	FSubsystemTick mSubsystemTick = FSubsystemTick();

	/** Should we use the Subsystem Tick? */
	UPROPERTY(EditDefaultsOnly, Category="KMods|System")
	bool bUseSubsystemTick = false;
};
