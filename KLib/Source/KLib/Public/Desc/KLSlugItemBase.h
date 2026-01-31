// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Resources/FGItemDescriptor.h"
#include "KLSlugItemBase.generated.h"


/**
 * 
 */
UCLASS(Blueprintable, Abstract, HideCategories=(Icon, Preview), meta=(AutoJSON=true))
class KLIB_API UKLSlugItemBase : public UFGItemDescriptor
{
	GENERATED_BODY()
};
