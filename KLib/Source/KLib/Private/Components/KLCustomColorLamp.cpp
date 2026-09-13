// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Components/KLCustomColorLamp.h"

#include "FGBuildableSubsystem.h"
#include "FGColorInterface.h"

UKLCustomColorLamp::UKLCustomColorLamp() { PrimaryComponentTick.bCanEverTick = false; }

void UKLCustomColorLamp::UpdateColor(FLinearColor Colour)
{
	if (mInstanceHandle.IsInstanced())
	{

		FFactoryCustomizationData Data = IFGColorInterface::Execute_GetCustomizationData(GetOwner());
		Data.Data[17] = Colour.R;
		Data.Data[18] = Colour.R;
		Data.Data[19] = Colour.R;
		IFGColorInterface::Execute_SetCustomizationData(GetOwner(), Data);

	}
	SetCustomPrimitiveDataFloat(17, Colour.R);
	SetCustomPrimitiveDataFloat(18, Colour.G);
	SetCustomPrimitiveDataFloat(19, Colour.B);
}

void UKLCustomColorLamp::UpdateEnableByIndex(int Index, bool isEnabled)
{
	if (mInstanceHandle.IsInstanced())
	{
		FFactoryCustomizationData Data = IFGColorInterface::Execute_GetCustomizationData(GetOwner());
		if (Data.Data.IsValidIndex(Index + 12))
		{
			Data.Data[Index + 12] = isEnabled;
		}
		IFGColorInterface::Execute_SetCustomizationData(GetOwner(), Data);

	}
	SetCustomPrimitiveDataFloat(Index + 12, isEnabled);
}
