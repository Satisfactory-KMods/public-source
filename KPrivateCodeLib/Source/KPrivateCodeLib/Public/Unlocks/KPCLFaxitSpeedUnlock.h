// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "KPCLFaxitUnlockBase.h"

#include "KPCLFaxitSpeedUnlock.generated.h"

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class KPRIVATECODELIB_API UKPCLFaxitSpeedUnlock : public UKPCLFaxitUnlockBase
{
	GENERATED_BODY()

public:
	virtual void ApplyToFaxit(class AKPCLFaxitSubsystem* Subsystem) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 mAmount = 15;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bIsFluid = false;
};
