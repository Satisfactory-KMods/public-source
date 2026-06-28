// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "FGInventoryComponent.h"
#include "Structures/KPCLFunctionalStructure.h"

#include "KLSlugDescriptor.h"

#include "KLSlugEggDescriptor.generated.h"

UCLASS(Blueprintable, Abstract, HideCategories = (Icon, Preview), meta = (AutoJSON = true))
class KLIB_API UKLSlugEggDescriptor : public UKLSlugItemBase
{
	GENERATED_BODY()
};
