// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FGConstructDisqualifier.h"
#include "Hologram/FGPipelineAttachmentHologram.h"

#include "KPCLPressureRegulatorValveHologram.generated.h"

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

UCLASS()
class KPRIVATECODELIB_API AKPCLPressureRegulatorValveHologram : public AFGPipelineAttachmentHologram
{
	GENERATED_BODY()

public:

	virtual bool TrySnapToActor(const FHitResult& HitResult) override;

protected:

	virtual void CheckValidPlacement() override;

	UPROPERTY(EditDefaultsOnly, Category = "KMods|Valve Hologram", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float mMaxVerticalComponent = 0.3f;

private:

	bool bLastSnapRejectedDueToVertical = false;
};
