// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Hologram/FGResourceExtractorHologram.h"
#include "UObject/Object.h"

#include "KLHologramModularMiner.generated.h"

UCLASS()
class KLIB_API AKLHologramModularMiner : public AFGResourceExtractorHologram
{
	GENERATED_BODY()

public:
	//~ Begin AFGResourceExtractorHologram Interface
	virtual void CheckValidPlacement() override;
	//~ End AFGResourceExtractorHologram Interface

	UPROPERTY(EditDefaultsOnly, Category = "KMods")
	TSubclassOf<UFGConstructDisqualifier> mInvalidKnowledge;
};
