// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Resources/FGBuildingDescriptor.h"
#include "UObject/Object.h"

#include "KPCLBuildingDescriptor.generated.h"

UCLASS()
class KPRIVATECODELIB_API UKPCLBuildingDescriptor : public UFGBuildingDescriptor
{
	GENERATED_BODY()

public:
	virtual FText GetItemDescriptionInternal() const override;
	virtual FText GetItemNameInternal() const override;

	UFUNCTION(BlueprintNativeEvent)
	FText BP_GetItemDescriptionInternal(const FText& InText, TSubclassOf<AFGBuildable> BuildableClass) const;

	UFUNCTION(BlueprintNativeEvent)
	FText BP_GetItemNameInternal(const FText& InText, TSubclassOf<AFGBuildable> BuildableClass) const;
};
