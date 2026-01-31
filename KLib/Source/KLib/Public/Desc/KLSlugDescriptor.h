// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGInventoryComponent.h"
#include "KLSlugFoodDescriptor.h"
#include "KLSlugItemBase.h"
#include "Logging.h"
#include "Kismet/KismetMathLibrary.h"
#include "Resources/FGItemDescriptor.h"
#include "Resources/FGNoneDescriptor.h"
#include "KLSlugDescriptor.generated.h"


/**
 * 
 */
UCLASS(Blueprintable, Abstract, HideCategories=(Icon, Preview), meta=(AutoJSON=true))
class KLIB_API UKLSlugDescriptor : public UKLSlugItemBase
{
	GENERATED_BODY()
};
