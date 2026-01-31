// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Hologram/FGResourceExtractorHologram.h"
#include "UObject/Object.h"
#include "KLHologramModularMiner.generated.h"

/**
 * 
 */
UCLASS()
class KLIB_API AKLHologramModularMiner : public AFGResourceExtractorHologram
{
	GENERATED_BODY()

	virtual void CheckValidPlacement() override;

	UPROPERTY(EditDefaultsOnly, Category="KMods")
	TSubclassOf<UFGConstructDisqualifier> mInvalidKnowledge;
};
