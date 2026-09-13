// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Buildable/KPCLPressureRegulatorValveLogic.h"

namespace
{

	constexpr float MaximumFillPercent = 1.4f;
}

float FKPCLPressureRegulatorValveLogic::CalculateFillPercent(const FKPCLPressureValvePipeData& Pipe)
{
	return IsReadable(Pipe) ? FMath::Clamp(Pipe.Content / Pipe.MaxContent, 0.f, MaximumFillPercent) : 0.f;
}

FKPCLPressureValveEvaluation FKPCLPressureRegulatorValveLogic::Evaluate(const FKPCLPressureValvePipeData& PipeA,
																		const FKPCLPressureValvePipeData& PipeB,
																		float OnThreshold, float OffThreshold,
																		bool bValveIsOpen)
{
	FKPCLPressureValveEvaluation Result;
	Result.bShouldBeOpen = bValveIsOpen;

	const bool bHighFillMode = OnThreshold >= OffThreshold;
	const FKPCLPressureValvePipeData& MonitoredPipe = bHighFillMode ? PipeA : PipeB;

	Result.MonitoredFillPercent = CalculateFillPercent(MonitoredPipe);

	if (!IsReadable(MonitoredPipe) || !FMath::IsFinite(OnThreshold) || !FMath::IsFinite(OffThreshold))
	{

		Result.bShouldBeOpen = false;
		return Result;
	}

	if (bHighFillMode)
	{
		if (Result.MonitoredFillPercent > OnThreshold)
		{
			Result.bShouldBeOpen = true;
		}
		else if (Result.MonitoredFillPercent < OffThreshold)
		{
			Result.bShouldBeOpen = false;
		}
	}
	else
	{
		if (Result.MonitoredFillPercent < OnThreshold)
		{
			Result.bShouldBeOpen = true;
		}
		else if (Result.MonitoredFillPercent > OffThreshold)
		{
			Result.bShouldBeOpen = false;
		}
	}

	return Result;
}

bool FKPCLPressureRegulatorValveLogic::IsReadable(const FKPCLPressureValvePipeData& Pipe)
{
	return FMath::IsFinite(Pipe.Content) && FMath::IsFinite(Pipe.MaxContent) && Pipe.MaxContent > 0.f;
}
