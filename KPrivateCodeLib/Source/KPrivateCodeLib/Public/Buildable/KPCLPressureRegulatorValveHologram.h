// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGConstructDisqualifier.h"
#include "Hologram/FGPipelineAttachmentHologram.h"

#include "KPCLPressureRegulatorValveHologram.generated.h"

/** Shown when the player tries to snap the valve onto a non-horizontal pipe connection. */
UCLASS()
class KPRIVATECODELIB_API UKPCLCDVerticalPipeConnection : public UFGConstructDisqualifier
{
	GENERATED_BODY()

	friend class AKPCLPressureRegulatorValveHologram;

	UKPCLCDVerticalPipeConnection()
	{
		mDisqfualifyingText = NSLOCTEXT("KPrivateCodeLib", "ConstructDisqualifier_VerticalPipeConnection",
										"Valve can only be placed on horizontal pipe connections.");
	}
};

/**
 * Hologram for AKPCLPressureRegulatorValve.
 *
 * Extends AFGPipelineAttachmentHologram so the building snaps to fluid pipelines using the
 * standard pipe-attachment snap system. Overrides TrySnapToActor to reject connections
 * whose forward vector points too far off the horizontal plane, ensuring the valve can only
 * be placed on level pipe runs — not on vertical risers or angled transitions.
 *
 * When a vertical pipe connection is detected, UKPCLCDVerticalPipeConnection is added via
 * CheckValidPlacement so the player sees an explanatory message in the build gun HUD.
 *
 * HORIZONTAL FILTER:
 *   A pipe connection is considered horizontal when the absolute Z component of its forward
 *   vector is below mMaxVerticalComponent (default 0.3, ~17° from horizontal).
 *   Adjust this value in the Blueprint default class if a wider or tighter angle is needed.
 *
 * BLUEPRINT SETUP:
 *   - Assign this class as mHologramClass in the Blueprint CDO of the valve buildable.
 *   - Set mAllowSnapToPipeline = true in Blueprint defaults to allow in-line pipeline snapping.
 *   - Leave mHasPipeRotationBuildStep false unless a rotation-adjust build step is needed.
 */
UCLASS()
class KPRIVATECODELIB_API AKPCLPressureRegulatorValveHologram : public AFGPipelineAttachmentHologram
{
	GENERATED_BODY()

public:
	//~ Begin AFGHologram Interface
	/**
	 * Forwards to AFGPipelineAttachmentHologram::TrySnapToActor, then rejects the snap if
	 * the matched pipe connection is not approximately horizontal.
	 */
	virtual bool TrySnapToActor(const FHitResult& HitResult) override;
	//~ End AFGHologram Interface

protected:
	//~ Begin AFGBuildableHologram Interface
	/** Adds UKPCLCDVerticalPipeConnection when the player is hovering over a vertical pipe. */
	virtual void CheckValidPlacement() override;
	//~ End AFGBuildableHologram Interface

	/**
	 * Maximum absolute Z component a pipe connection's forward vector may have before it is
	 * rejected as "too vertical". Range [0, 1]; default 0.3 ≈ 17° from horizontal.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "KMods|Valve Hologram", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float mMaxVerticalComponent = 0.3f;

private:
	/** True when the last TrySnapToActor call found a pipe but rejected it as too vertical. */
	bool bLastSnapRejectedDueToVertical = false;
};
