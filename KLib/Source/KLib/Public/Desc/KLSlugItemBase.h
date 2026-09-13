// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Resources/FGItemDescriptor.h"

#include "KLSlugItemBase.generated.h"

UCLASS(Blueprintable, Abstract, HideCategories = (Icon, Preview), meta = (AutoJSON = true))
class KLIB_API UKLSlugItemBase : public UFGItemDescriptor
{
	GENERATED_BODY()
};
