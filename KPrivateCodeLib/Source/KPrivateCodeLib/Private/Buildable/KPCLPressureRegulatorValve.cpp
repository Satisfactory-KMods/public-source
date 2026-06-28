// ILikeBanas

#include "Buildable/KPCLPressureRegulatorValve.h"

#include "Async/TaskGraphInterfaces.h"
#include "Net/UnrealNetwork.h"
#include "Replication/KPCLDefaultRCO.h"

AKPCLPressureRegulatorValve::AKPCLPressureRegulatorValve()
{
	// --- FluidBox sizing ---
	// mFluidBoxVolume is EditDefaultsOnly + protected in AFGBuildablePipelineAttachment.
	// The parent BeginPlay maps this value to mFluidBox.MaxContent.
	// Override in Blueprint defaults if a different tank size is needed.
	mFluidBoxVolume = 15.f; // [m³]

	// mRadius is used by the parent to approximate volume from pipe geometry;
	// we set it explicitly so the parent has a valid non-zero value even if the
	// attachment's constructor left it at 0.  Since we override mFluidBoxVolume
	// directly, the exact radius has no effect on simulation volume.
	mRadius = 0.5f; // [m] — representative of a large-pipe junction
}

void AKPCLPressureRegulatorValve::BeginPlay()
{
	Super::BeginPlay();

	// --- Disable pump headlift ---
	// This building is a passive, bidirectional valve, not an active pump.
	// SetMaxHeadLift zeroes the runtime pressure values.
	// Also set mMaxPressure = 0 and mDesignPressure = 0 in the Blueprint default class
	// (they are private in AFGBuildablePipelinePump and unreachable from C++ here).
	SetMaxHeadLift(0.f, 0.f);

	// --- FluidBox height ---
	// Apply after Super::BeginPlay() so we overwrite any value the parent may have set.
	// Height drives pressure-column calculations; 6 m matches the intended building footprint.
	// LaminarHeight must satisfy 0 < LaminarHeight <= Height (FFluidBox invariant).
	mFluidBox.Height = mFluidBoxHeight;
	mFluidBox.LaminarHeight = FMath::Min(mFluidBox.LaminarHeight, mFluidBoxHeight);
	if (mFluidBox.LaminarHeight <= 0.f)
	{
		mFluidBox.LaminarHeight = mFluidBoxHeight;
	}

	// Apply the initial flow limit based on the default bValveIsOpen value loaded from save.
	ApplyValveFlowLimit();
}

void AKPCLPressureRegulatorValve::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLPressureRegulatorValve, mValveOpenAlpha);
	DOREPLIFETIME(AKPCLPressureRegulatorValve, mReplicatedFillLevel);
}

void AKPCLPressureRegulatorValve::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const
{
	Super::GetConditionalReplicatedProps(outProps);

	FG_DOREPCONDITIONAL(ThisClass, mOnThreshold);
	FG_DOREPCONDITIONAL(ThisClass, mOffThreshold);
	FG_DOREPCONDITIONAL(ThisClass, bInvertThresholdBehavior);
	FG_DOREPCONDITIONAL_WITH_NOTIFY(ThisClass, bValveIsOpen, OnRep_ValveIsOpen);
}

bool AKPCLPressureRegulatorValve::CanProduce_Implementation() const
{
	// Always allow Factory_Tick to run when the building is significant.
	// We deliberately do NOT call Super (AFGBuildablePipelinePump::CanProduce) because the
	// pump's check is headlift-dependent and would return false with headlift = 0.
	// A passive valve has no production cycle; CanProduce here only gates Factory_Tick.
	return true;
}

void AKPCLPressureRegulatorValve::Factory_Tick(float Dt)
{
	// Let the base pump run its internal indicator / flow-limit bookkeeping.
	// With headlift = 0 and audio events left null in Blueprint defaults, this has
	// no observable side effects beyond what the valve itself sets.
	Super::Factory_Tick(Dt);

	// Valve logic is server-authoritative only; clients receive state via FGReplicated.
	if (!HasAuthority())
	{
		return;
	}

	const float FillPct = GetNormalizedFillLevel();
	UpdateValveState(FillPct);

	// Replicate fill level for water-fill mesh display.
	// Only write when the value changes meaningfully to avoid redundant replication packets.
	if (!FMath::IsNearlyEqual(mReplicatedFillLevel, FillPct, 0.005f))
	{
		mReplicatedFillLevel = FillPct;

		// Factory_Tick runs on a worker thread — dynamic multicast delegate broadcasts and
		// BlueprintNativeEvent calls must be marshaled to the game thread (Phase 2 fix).
		const float LocalFill = mReplicatedFillLevel;
		FFunctionGraphTask::CreateAndDispatchWhenReady(
			[this, LocalFill]()
			{
				if (IsValid(this))
				{
					OnFillLevelChanged.Broadcast(LocalFill);
					OnUiRequireUpdate();
				}
			},
			TStatId(), nullptr, ENamedThreads::GameThread);
	}
}

// ---- Getters -------------------------------------------------------------------

float AKPCLPressureRegulatorValve::GetNormalizedFillLevel() const
{
	if (mFluidBox.MaxContent <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(mFluidBox.Content / mFluidBox.MaxContent, 0.f, 1.f);
}

// ---- Valve state ---------------------------------------------------------------

void AKPCLPressureRegulatorValve::UpdateValveState(float NormalizedFill)
{
	// Robustness: if the designer sets thresholds in the wrong order (On < Off), swap them
	// so hysteresis still works correctly rather than causing rapid chattering.
	const float SafeOn = FMath::Max(mOnThreshold, mOffThreshold);
	const float SafeOff = FMath::Min(mOnThreshold, mOffThreshold);

	bool bShouldBeOpen = bValveIsOpen; // default: hold current state (hysteresis)

	if (!bInvertThresholdBehavior)
	{
		// Standard mode: open when tank fills up, close when it empties.
		if (NormalizedFill >= SafeOn)
		{
			bShouldBeOpen = true;
		}
		else if (NormalizedFill <= SafeOff)
		{
			bShouldBeOpen = false;
		}
		// In [SafeOff, SafeOn]: keep current state — this is the hysteresis band.
	}
	else
	{
		// Inverted mode: close when tank fills up, open when it empties.
		// Useful for overflow protection or demand-driven supply valves.
		if (NormalizedFill >= SafeOn)
		{
			bShouldBeOpen = false;
		}
		else if (NormalizedFill <= SafeOff)
		{
			bShouldBeOpen = true;
		}
		// In [SafeOff, SafeOn]: keep current state — hysteresis band.
	}

	if (bShouldBeOpen == bValveIsOpen)
	{
		return; // No state change; skip replication mark and BP event.
	}

	bValveIsOpen = bShouldBeOpen;
	mPropertyReplicator.MarkPropertyDirty(FName("bValveIsOpen")); // MarkPropertyDirty is safe from Factory_Tick threads

	mValveOpenAlpha = bValveIsOpen ? 1.f : 0.f;

	ApplyValveFlowLimit();

	// Factory_Tick runs on a worker thread — dynamic multicast delegate broadcasts and
	// BlueprintNativeEvent calls must be marshaled to the game thread (Phase 2 fix).
	// State writes (bValveIsOpen, MarkPropertyDirty, ApplyValveFlowLimit) stay here; they
	// follow the established per-tick pattern already used throughout this class's bases.
	const float LocalAlpha = mValveOpenAlpha;
	const bool bLocalOpen = bValveIsOpen;
	FFunctionGraphTask::CreateAndDispatchWhenReady(
		[this, LocalAlpha, bLocalOpen]()
		{
			if (IsValid(this))
			{
				OnValveOpenAlphaChanged.Broadcast(LocalAlpha); // server-side broadcast; clients receive via OnRep
				// Fire Blueprint events on the server side.
				// The corresponding OnRep functions fire the same events on clients.
				OnValveStateChanged(bLocalOpen);
				OnUiRequireUpdate();
			}
		},
		TStatId(), nullptr, ENamedThreads::GameThread);
}

void AKPCLPressureRegulatorValve::ApplyValveFlowLimit()
{
	// SetUserFlowLimit is the pump's public API for controlling FFluidBox::FlowLimit:
	//   0.f  → block all flow (valve closed)
	//   > 0  → cap flow at that rate in m³/s (valve open, capped at mMaxFlowRate)
	// We use mMaxFlowRate directly (rather than -1 = "use neighbour-derived default")
	// so the valve respects the configured maximum regardless of pipeline sizing.
	//
	// Note on FFluidBox::FlowLimit:
	//   The raw field uses 0 to mean *unlimited*.  SetUserFlowLimit abstracts this:
	//   passing 0 here means "no flow allowed" — the pump translates it internally.
	SetUserFlowLimit(bValveIsOpen ? mMaxFlowRate : 0.f);
}

void AKPCLPressureRegulatorValve::OnRep_ValveIsOpen()
{
	mValveOpenAlpha = bValveIsOpen ? 1.f : 0.f;
	OnValveStateChanged(bValveIsOpen);
	OnUiRequireUpdate();
}

void AKPCLPressureRegulatorValve::OnRep_ValveOpenAlpha() { OnValveOpenAlphaChanged.Broadcast(mValveOpenAlpha); }

void AKPCLPressureRegulatorValve::OnRep_FillLevel()
{
	OnFillLevelChanged.Broadcast(mReplicatedFillLevel);
	OnUiRequireUpdate();
}

// ---- Setters -------------------------------------------------------------------

void AKPCLPressureRegulatorValve::SetOnThreshold(float NewThreshold)
{
	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this))
		{
			RCO->Server_Valve_SetOnThreshold(this, NewThreshold);
		}
		return;
	}
	mOnThreshold = FMath::Clamp(NewThreshold, 0.f, 1.f);
	mPropertyReplicator.MarkPropertyDirty(FName("mOnThreshold"));
	OnUiRequireUpdate();
}

void AKPCLPressureRegulatorValve::SetOffThreshold(float NewThreshold)
{
	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this))
		{
			RCO->Server_Valve_SetOffThreshold(this, NewThreshold);
		}
		return;
	}
	mOffThreshold = FMath::Clamp(NewThreshold, 0.f, 1.f);
	mPropertyReplicator.MarkPropertyDirty(FName("mOffThreshold"));
	OnUiRequireUpdate();
}

void AKPCLPressureRegulatorValve::SetInvertThresholdBehavior(bool bInvert)
{
	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this))
		{
			RCO->Server_Valve_SetInvertThresholdBehavior(this, bInvert);
		}
		return;
	}
	bInvertThresholdBehavior = bInvert;
	mPropertyReplicator.MarkPropertyDirty(FName("bInvertThresholdBehavior"));
	OnUiRequireUpdate();
}
