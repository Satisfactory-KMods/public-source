// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "KPCLFaxitUnlockBase.h"

#include "KPCLFaxitFeatureUnlock.generated.h"

UENUM(BlueprintType)
enum class EKPCLFaxitFeature : uint8
{
	RemoteAccess UMETA(DisplayName = "Remote Access"),
	Sink UMETA(DisplayName = "Sink"),
	Depot UMETA(DisplayName = "Depot"),
};

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class KPRIVATECODELIB_API UKPCLFaxitFeatureUnlock : public UKPCLFaxitUnlockBase
{
	GENERATED_BODY()

public:
	virtual void ApplyToFaxit(class AKPCLFaxitSubsystem* Subsystem) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EKPCLFaxitFeature mFeature = EKPCLFaxitFeature::Sink;
};
