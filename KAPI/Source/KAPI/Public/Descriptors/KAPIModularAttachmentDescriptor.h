// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "KAPIModularAttachmentDescriptor.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable)
class KAPI_API UKAPIModularAttachmentDescriptor : public UObject
{
	GENERATED_BODY()

public:
	UKAPIModularAttachmentDescriptor() { mName = FText::FromString("Unnamed Attachment Descriptor"); }

	UPROPERTY(EditDefaultsOnly)
	FText mName;
};
