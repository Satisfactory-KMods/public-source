// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Description/KPCLBuildingDescriptor.h"

FText UKPCLBuildingDescriptor::GetItemDescriptionInternal() const
{
	if (IsValid(mBuildableClass))
	{
		return BP_GetItemDescriptionInternal(Super::GetItemDescriptionInternal(), mBuildableClass);
	}
	return Super::GetItemDescriptionInternal();
}

FText UKPCLBuildingDescriptor::GetItemNameInternal() const
{
	if (IsValid(mBuildableClass))
	{
		return BP_GetItemNameInternal(Super::GetItemNameInternal(), mBuildableClass);
	}
	return Super::GetItemNameInternal();
}

FText UKPCLBuildingDescriptor::BP_GetItemNameInternal_Implementation(const FText& InText,
                                                                     TSubclassOf<AFGBuildable> BuildableClass) const
{
	return InText;
}

FText UKPCLBuildingDescriptor::BP_GetItemDescriptionInternal_Implementation(
	const FText& InText, TSubclassOf<AFGBuildable> BuildableClass) const
{
	return InText;
}
