// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGConstructDisqualifier.h"
#include "Hologram/FGFactoryHologram.h"

#include "KPCLNetworkDeliveryHologram.generated.h"

class AKPCLFaxitSubsystem;
class AKPCLNetworkCore;

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkDeliveryHologram : public AFGFactoryHologram
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void CheckValidPlacement() override;
	virtual void ConfigureActor(AFGBuildable* InBuildable) const override;

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask|Hologram")
	AKPCLNetworkCore* GetTargetNexus() const { return mTargetNexus; }

private:
	void RefreshTargetNexus();

	UPROPERTY(Transient)
	TObjectPtr<AKPCLFaxitSubsystem> mFaxitSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<AKPCLNetworkCore> mTargetNexus;
};

UCLASS()
class KPRIVATECODELIB_API UKPCLCDNoNexusAvailable : public UFGConstructDisqualifier
{
	GENERATED_BODY()

public:
	UKPCLCDNoNexusAvailable()
	{
		mDisqfualifyingText =
			NSLOCTEXT("KPrivateCodeLib", "ConstructDisqualifier_NoNexusAvailable", "No Nexus available");
	}
};

UCLASS()
class KPRIVATECODELIB_API UKPCLCDNexusBuildingLimitReached : public UFGConstructDisqualifier
{
	GENERATED_BODY()

public:
	UKPCLCDNexusBuildingLimitReached()
	{
		mDisqfualifyingText = NSLOCTEXT("KPrivateCodeLib", "ConstructDisqualifier_NexusBuildingLimitReached",
										"All Nexus building limits reached");
	}
};
