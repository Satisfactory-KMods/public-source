// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "KAPIWasteProducerType.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable)
class KAPI_API UKAPIWasteProducerType : public UObject
{
	GENERATED_BODY()

public:
	UKAPIWasteProducerType()
	{
		mName = FText::FromString("Unnamed Waste Descriptor");
	}

	UPROPERTY(EditDefaultsOnly)
	FText mName;
};
