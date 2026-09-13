// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FGSaveInterface.h"
#include "GameFramework/Actor.h"
#include "Subsystem/ModSubsystem.h"

#include "KPCLModSubsystem.generated.h"

class AKPCLModSubsystem;

USTRUCT()
struct KPRIVATECODELIB_API FSubsystemTick : public FTickFunction
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Transient)
	TObjectPtr<AKPCLModSubsystem> mTarget;

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
	AKPCLModSubsystem();

	virtual void Init() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintNativeEvent, Category = "KMods|Events")
	void OnInit();

	virtual void OnInit_Implementation() {}

	UFUNCTION()
	virtual void OnOptionsUpdated(FString UpdatedCVar) {}

	FORCEINLINE virtual bool ShouldSave_Implementation() const override { return mShouldSave; }

	virtual void SubsytemTick(float dt) {};

	UPROPERTY(EditDefaultsOnly, Category = "KMods|System")
	bool mShouldSave = false;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|System")
	FSubsystemTick mSubsystemTick = FSubsystemTick();

	UPROPERTY(EditDefaultsOnly, Category = "KMods|System")
	bool bUseSubsystemTick = false;
};
