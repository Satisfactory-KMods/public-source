#include "Components/KLCustomColorLamp.h"

#include "FGBuildableSubsystem.h"
#include "FGColorInterface.h"


UKLCustomColorLamp::UKLCustomColorLamp() { PrimaryComponentTick.bCanEverTick = false; }

void UKLCustomColorLamp::UpdateColor(FLinearColor Colour)
{
	if (mInstanceHandle.IsInstanced())
	{
		// mInstanceHandle.SetCustomDataById(17, Colour.R);
		// mInstanceHandle.SetCustomDataById(18, Colour.G);
		// mInstanceHandle.SetCustomDataById(19, Colour.B);

		FFactoryCustomizationData Data = IFGColorInterface::Execute_GetCustomizationData(GetOwner());
		Data.Data[17] = Colour.R;
		Data.Data[18] = Colour.R;
		Data.Data[19] = Colour.R;
		IFGColorInterface::Execute_SetCustomizationData(GetOwner(), Data);

		/*AFGBuildableSubsystem* Subsystem = AFGBuildableSubsystem::Get(this);
		if(Subsystem)
			if(Subsystem->GetColoredInstanceManager(this))
				Subsystem->GetColoredInstanceManager(this)->UpdateColorForInstanceFromDataArray(mInstanceHandle);*/
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
		/*mInstanceHandle.SetCustomDataById(FMath::Clamp(Index + 12, 0, 19), isEnabled);

		AFGBuildableSubsystem* Subsystem = AFGBuildableSubsystem::Get(this);
		if(Subsystem)
			if(Subsystem->GetColoredInstanceManager(this))
				Subsystem->GetColoredInstanceManager(this)->UpdateColorForInstanceFromDataArray(mInstanceHandle);*/
	}
	SetCustomPrimitiveDataFloat(Index + 12, isEnabled);
}
