// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGConstructDisqualifier.h"
#include "Hologram/FGFactoryHologram.h"

#include "Network/Buildings/KPCLNetworkCore.h"

#include "KPCLNetworkCoreHologram.generated.h"

UCLASS()
class KPRIVATECODELIB_API AKPCLNetworkCoreHologram : public AFGFactoryHologram
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void CheckValidPlacement() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<AKPCLFaxitSubsystem> mFaxitSubsystem;
};

UCLASS()
class KPRIVATECODELIB_API UKPCLCDMaxCountReached : public UFGConstructDisqualifier
{
	GENERATED_BODY()

	friend AKPCLNetworkCoreHologram;

	UKPCLCDMaxCountReached()
	{
		mDisqfualifyingText = NSLOCTEXT("KPrivateCodeLib", "ConstructDisqualifier_MaxCountReached",
										"Reached global max count for this building (5)");
	}

	static void UpdateCount(int32 NewCount)
	{
		UKPCLCDMaxCountReached* Instance = GetMutableDefault<UKPCLCDMaxCountReached>();
		FFormatNamedArguments Args;
		Args.Add(TEXT("Count"), NewCount);
		Instance->mDisqfualifyingText =
			FText::Format(FText::FromString(TEXT("Reached global max count for this building ({Count})")), Args);
	}
};
