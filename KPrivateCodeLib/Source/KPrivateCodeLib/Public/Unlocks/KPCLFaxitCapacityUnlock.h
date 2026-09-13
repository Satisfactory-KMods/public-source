// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "KPCLFaxitUnlockBase.h"

#include "KPCLFaxitCapacityUnlock.generated.h"

UENUM(BlueprintType)
enum class EKPCLFaxitCapacityType : uint8
{
	MachineNetwork UMETA(DisplayName = "Machine Network"),
	Network UMETA(DisplayName = "Network"),
	Drive UMETA(DisplayName = "Drive"),
	BufferInventory UMETA(DisplayName = "Buffer Inventory"),
};

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class KPRIVATECODELIB_API UKPCLFaxitCapacityUnlock : public UKPCLFaxitUnlockBase
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void ApplyToFaxit(class AKPCLFaxitSubsystem* Subsystem) const override;

	UFUNCTION(BlueprintPure, Category = "Faxit")
	int32 GetAmount() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EKPCLFaxitCapacityType mCapacityType = EKPCLFaxitCapacityType::Network;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 mAmount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AdvancedDisplay)
	bool bUseCapacityTypeDefaultAmount = true;
};
