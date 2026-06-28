// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Resources/FGItemDescriptor.h"

#include "KPCLItemDescriptor.generated.h"

UCLASS()
class KPRIVATECODELIB_API UKPCLItemDescriptor : public UFGItemDescriptor
{
	GENERATED_BODY()

public:
	virtual FText GetItemDescriptionInternal() const override;
	virtual FText GetItemNameInternal() const override;

	UFUNCTION(BlueprintNativeEvent)
	FText BP_GetItemDescriptionInternal(const FText& InText) const;

	UFUNCTION(BlueprintNativeEvent)
	FText BP_GetItemNameInternal(const FText& InText) const;
};
