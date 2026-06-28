// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGInventoryComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Resources/FGItemDescriptor.h"
#include "Resources/FGNoneDescriptor.h"

#include "KLSlugFoodDescriptor.h"
#include "KLSlugItemBase.h"
#include "Logging.h"

#include "KLSlugDescriptor.generated.h"

UCLASS(Blueprintable, Abstract, HideCategories = (Icon, Preview), meta = (AutoJSON = true))
class KLIB_API UKLSlugDescriptor : public UKLSlugItemBase
{
	GENERATED_BODY()
};
