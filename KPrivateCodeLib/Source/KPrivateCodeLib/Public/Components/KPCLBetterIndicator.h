// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "KPCLColoredStaticMesh.h"

#include "KPCLBetterIndicator.generated.h"

UENUM(BlueprintType)
enum class ENewProductionState : uint8
{
	Invalid = 0 UMETA(DisplayName = "Invalid"),
	NoPower = 1 UMETA(DisplayName = "No Power"),
	Paused = 2 UMETA(DisplayName = "Pause"),
	Idle = 3 UMETA(DisplayName = "Idle"),
	Producing = 4 UMETA(DisplayName = "Producing"),
	Error = 5 UMETA(DisplayName = "Error"),
	ExtraState_1 = 6 UMETA(DisplayName = "ExtraState_1"),
	ExtraState_2 = 7 UMETA(DisplayName = "ExtraState_2"),
	ExtraState_3 = 8 UMETA(DisplayName = "ExtraState_3"),
	ExtraState_4 = 9 UMETA(DisplayName = "ExtraState_4"),
	ExtraState_5 = 10 UMETA(DisplayName = "ExtraState_5"),
	Max = 11 UMETA(Hidden)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBIndicatorStateChanged, ENewProductionState, NewState);

/**
* Proxy placed in buildings to be replaced with an instance on creation, supports coloring.
* Todo: This is a DEPARTED Component and need to remove SOON
*/
UCLASS(Blueprintable, meta = ( BlueprintSpawnableComponent ))
class KPRIVATECODELIB_API UKPCLBetterIndicator : public UKPCLColoredStaticMesh
{
	GENERATED_BODY()

public:
	UKPCLBetterIndicator();

	// GetCurrent Emissive for the Light
	UFUNCTION(BlueprintPure, Category="KMods Indicator")
	float GetEmissive() const;

	UFUNCTION(BlueprintCallable, Category="KMods Indicator")
	void SetState(ENewProductionState NewState, bool MarkStateDirty = true);

	UFUNCTION(BlueprintCallable, Category="KMods Indicator")
	void SetEmissiveOverwrite(float NewIntensity, bool MarkStateDirty = true);

	UFUNCTION(BlueprintCallable, Category="KMods Indicator")
	FLinearColor GetColorByState(ENewProductionState State) const;

	UFUNCTION(BlueprintCallable, Category="KMods Indicator")
	FLinearColor GetCurrentColor() const;

	UFUNCTION(BlueprintPure, Category="KMods Indicator")
	ENewProductionState GetState() const;

private:
	TEnumAsByte<ENewProductionState> mCurrentState;

	UPROPERTY(EditDefaultsOnly, Category="KMods", meta=(ArraySizeEnum="ENewProductionState"))
	FLinearColor mStateColors[11];

	UPROPERTY(BlueprintAssignable, Category="KMods|Events")
	FOnBIndicatorStateChanged OnIndicatorStateChanged;

	UPROPERTY(EditDefaultsOnly, Category="KMods")
	TArray<TEnumAsByte<ENewProductionState>> mPulsStates = {
		ENewProductionState::Error, ENewProductionState::Idle, ENewProductionState::Paused
	};

	UPROPERTY(EditDefaultsOnly, Category="KMods")
	float mEmissiveIntensity = 2.0f;
	float mEmissiveIntensityOverwrite = -1.0f;
};
