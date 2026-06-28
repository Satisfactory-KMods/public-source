// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Descriptors/KAPIModularAttachmentDescriptor.h"

#include "KPCLModularSnapPoint.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KPRIVATECODELIB_API UKPCLModularSnapPoint : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UKAPIModularAttachmentDescriptor> mAttachmentClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 mIndex;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 mStackerCount = 5;
};
