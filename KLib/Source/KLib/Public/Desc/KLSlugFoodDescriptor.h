// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "KLSlugItemBase.h"

#include "KLSlugFoodDescriptor.generated.h"

UCLASS(Blueprintable, Abstract, HideCategories = (Icon, Preview), meta = (AutoJSON = true))
class KLIB_API UKLSlugFoodDescriptor : public UKLSlugItemBase
{
	GENERATED_BODY()
};
