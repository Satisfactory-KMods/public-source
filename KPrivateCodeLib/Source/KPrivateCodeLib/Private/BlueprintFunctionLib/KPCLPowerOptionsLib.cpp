// Copyright Coffee Stain Studios. All Rights Reserved.
#include "BlueprintFunctionLib/KPCLPowerOptionsLib.h"

bool UKPCLPowerOptionsLib::IsValid(FPowerOptions PowerOption) { return PowerOption.IsValid(); }

void UKPCLPowerOptionsLib::MergePowerOptions(FPowerOptions& PowerOption, FPowerOptions OtherPowerOption)
{
	return PowerOption.MergePowerOptions(OtherPowerOption);
}

void UKPCLPowerOptionsLib::OverWritePowerOptions(FPowerOptions& PowerOption, FPowerOptions OtherPowerOption)
{
	return PowerOption.OverWritePowerOptions(OtherPowerOption);
}

float UKPCLPowerOptionsLib::GetMaxPowerConsume(FPowerOptions PowerOption) { return PowerOption.GetMaxPowerConsume(); }

float UKPCLPowerOptionsLib::GetPowerConsume(FPowerOptions PowerOption) { return PowerOption.GetPowerConsume(); }

float UKPCLPowerOptionsLib::GetCurrentVariablePower(FPowerOptions PowerOption)
{
	return PowerOption.GetCurrentVariablePower();
}

bool UKPCLPowerOptionsLib::IsPowerVariable(FPowerOptions PowerOption) { return PowerOption.IsPowerVariable(); }
