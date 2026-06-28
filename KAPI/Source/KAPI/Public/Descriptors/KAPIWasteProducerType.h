// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "KAPIWasteProducerType.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable)
class KAPI_API UKAPIWasteProducerType : public UObject
{
	GENERATED_BODY()

public:
	UKAPIWasteProducerType() { mName = FText::FromString("Unnamed Waste Descriptor"); }

	UPROPERTY(EditDefaultsOnly)
	FText mName;
};
