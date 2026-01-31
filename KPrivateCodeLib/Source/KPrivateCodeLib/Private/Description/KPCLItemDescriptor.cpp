// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Description/KPCLItemDescriptor.h"

FText UKPCLItemDescriptor::BP_GetItemDescriptionInternal_Implementation(const FText& InText) const
{
	return InText;
}

FText UKPCLItemDescriptor::BP_GetItemNameInternal_Implementation(const FText& InText) const
{
	return InText;
}

FText UKPCLItemDescriptor::GetItemDescriptionInternal() const
{
	return BP_GetItemDescriptionInternal(Super::GetItemDescriptionInternal());
}

FText UKPCLItemDescriptor::GetItemNameInternal() const
{
	return BP_GetItemNameInternal(Super::GetItemDescriptionInternal());
}
