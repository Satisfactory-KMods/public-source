// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Unlocks/FGUnlockScannableResource.h"
#include "KLUnlockExtractorScanner.generated.h"

/**
 * @deprecated use normal UFGUnlockScannableResource
 */
UCLASS(Blueprintable, EditInlineNew, abstract, DefaultToInstanced)
class KLIB_API UKLUnlockExtractorScanner : public UFGUnlockScannableResource
{
	GENERATED_BODY()
};
