// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildablePipelinePump.h"

#include "KPCLPressureRegulatorValve.generated.h"

/**
 * Fill-level-controlled inline valve for fluid pipelines.
 *
 * BASE CLASS CHOICE — AFGBuildablePipelinePump vs AFGBuildablePipelineAttachment:
 * AFGBuildablePipelinePump is chosen because it exposes SetUserFlowLimit(), which correctly
 * maps "0 = block all flow / positive = allow up to N m³/s" without us having to manipulate
 * FFluidBox::FlowLimit directly (where 0 means *unlimited* — the opposite of what we want).
 * Pump headlift is disabled via SetMaxHeadLift(0, 0) in BeginPlay so the building acts as a
 * purely passive, bidirectional gating element driven by network pressure.
 *
 * BLUEPRINT SETUP REQUIRED:
 *   - Set mMaxPressure = 0 and mDesignPressure = 0 in Blueprint defaults to suppress the pump
 *     indicator from showing spurious headlift values (those fields are private in the base class).
 *   - Leave all pump audio events (mPipelinePistonSound, mHeadLiftSound, etc.) null in the
 *     Blueprint default class so no pump sounds play.
 *   - Do NOT add a UFGPowerConnectionComponent; this valve is passive and needs no power.
 *
 * FLOW DIRECTION:
 *   With AddedPressure = 0 (headlift off), fluid travels in whichever direction the network
 *   pressure pushes it. Both pipe connections share the same internal FFluidBox; there is no
 *   per-port direction enforcement at the C++ level. The "fill level monitoring" applies to
 *   the combined fill of that shared box. True one-sided isolation (Port A always feeds box,
 *   Port B gated) is a topological concern of the pipeline layout, not enforced here.
 *   Engine limitation: one FFluidBox per fluid integrant.
 *
 * FLUID BOX SIZING:
 *   mFluidBoxVolume (~15 m³) is set in the constructor via the protected parent field.
 *   mFluidBoxHeight (~6 m) is a new property applied to mFluidBox.Height in BeginPlay.
 *   Both can be overridden per-blueprint via EditDefaultsOnly properties.
 *
 * VALVE LOGIC SUMMARY:
 *   Standard mode  (bInvertThresholdBehavior = false):
 *     fill >= mOnThreshold  → open  (SetUserFlowLimit = mMaxFlowRate)
 *     fill <= mOffThreshold → close (SetUserFlowLimit = 0)
 *     between thresholds    → hysteresis, keep current state
 *   Inverted mode (bInvertThresholdBehavior = true):
 *     fill >= mOnThreshold  → close
 *     fill <= mOffThreshold → open
 *     between thresholds    → hysteresis, keep current state
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKPCLOnValveOpenAlphaChanged, float, ValveOpenAlpha);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKPCLOnFillLevelChanged, float, NormalizedFillLevel);

UCLASS()
class KPRIVATECODELIB_API AKPCLPressureRegulatorValve : public AFGBuildablePipelinePump
{
	GENERATED_BODY()

public:
	AKPCLPressureRegulatorValve();

	//~ Begin AActor Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	//~ End AActor Interface

	//~ Begin IFGConditionalReplicationInterface
	virtual void GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const override;
	//~ End IFGConditionalReplicationInterface

	//~ Begin AFGBuildableFactory Interface
	/** Always returns true so Factory_Tick fires while the building is significant, even with headlift disabled. */
	virtual bool CanProduce_Implementation() const override;
	/** Tick used for all fill-level monitoring and valve-state updates. Server: evaluates thresholds. Client: no-op. */
	virtual void Factory_Tick(float Dt) override;
	//~ End AFGBuildableFactory Interface

	/** Normalised fill level of the internal FluidBox in [0, 1]. */
	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	float GetNormalizedFillLevel() const;

	/** Minimum possible content of the FluidBox [m³]. Always 0 — exposed for UI slider binding. */
	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	FORCEINLINE float GetMinFillLevel() const { return 0.f; }

	/** Maximum possible content of the FluidBox [m³]. Equal to mFluidBox.MaxContent. */
	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	FORCEINLINE float GetMaxFillLevel() const { return mFluidBox.MaxContent; }

	/** Normalised open amount: 0.0 = fully closed, 1.0 = fully open. */
	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	FORCEINLINE float GetValveOpenAlpha() const { return mValveOpenAlpha; }

	/** True when the valve is open and flow is permitted. */
	UFUNCTION(BlueprintPure, Category = "KMods|Valve")
	FORCEINLINE bool IsValveOpen() const { return bValveIsOpen; }

	/** Clamps value to [0, 1]. HasAuthority guard is inside; safe to expose to Blueprint. */
	UFUNCTION(BlueprintCallable, Category = "KMods|Valve")
	void SetOnThreshold(float NewThreshold);

	/** Clamps value to [0, 1]. HasAuthority guard is inside; safe to expose to Blueprint. */
	UFUNCTION(BlueprintCallable, Category = "KMods|Valve")
	void SetOffThreshold(float NewThreshold);

	/** HasAuthority guard is inside; safe to expose to Blueprint. */
	UFUNCTION(BlueprintCallable, Category = "KMods|Valve")
	void SetInvertThresholdBehavior(bool bInvert);

	/** Fired on ALL machines when the valve transitions between open/closed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "KMods|Valve")
	void OnValveStateChanged(bool bIsOpen);

	/** Called on ALL machines whenever any UI-relevant state changes. Implement in Blueprint to refresh the widget. */
	UFUNCTION(BlueprintImplementableEvent, Category = "KMods|Valve")
	void OnUiRequireUpdate();

	/** Broadcast whenever mValveOpenAlpha changes (0 = closed, 1 = open). */
	UPROPERTY(BlueprintAssignable, Category = "KMods|Valve")
	FKPCLOnValveOpenAlphaChanged OnValveOpenAlphaChanged;

	/** Broadcast whenever the replicated normalised fill level changes. */
	UPROPERTY(BlueprintAssignable, Category = "KMods|Valve")
	FKPCLOnFillLevelChanged OnFillLevelChanged;

protected:
	/** Reads fill level, applies hysteresis, and updates bValveIsOpen + mValveOpenAlpha. */
	void UpdateValveState(float NormalizedFill);

	/** Calls SetUserFlowLimit based on the current bValveIsOpen state. */
	void ApplyValveFlowLimit();

	UFUNCTION()
	virtual void OnRep_ValveIsOpen();

	UFUNCTION()
	void OnRep_ValveOpenAlpha();

	UFUNCTION()
	void OnRep_FillLevel();

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Valve", meta = (ClampMin = "0.01"))
	float mMaxFlowRate = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Valve", meta = (ClampMin = "0.1"))
	float mFluidBoxHeight = 6.f;

	UPROPERTY(SaveGame, EditDefaultsOnly, BlueprintReadOnly, meta = (FGReplicated, ClampMin = "0.0", ClampMax = "1.0"),
			  Category = "KMods|Valve")
	float mOnThreshold = 0.8f;

	UPROPERTY(SaveGame, EditDefaultsOnly, BlueprintReadOnly, meta = (FGReplicated, ClampMin = "0.0", ClampMax = "1.0"),
			  Category = "KMods|Valve")
	float mOffThreshold = 0.2f;

	UPROPERTY(SaveGame, EditDefaultsOnly, BlueprintReadOnly, meta = (FGReplicated), Category = "KMods|Valve")
	bool bInvertThresholdBehavior = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, meta = (FGReplicatedUsing = OnRep_ValveIsOpen), Category = "KMods|Valve")
	bool bValveIsOpen = false;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ValveOpenAlpha, Category = "KMods|Valve")
	float mValveOpenAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_FillLevel, Category = "KMods|Valve")
	float mReplicatedFillLevel = 0.f;
};
