#pragma once

#include "CoreMinimal.h"

struct FKPCLPressureValvePipeData
{
	float Content = 0.f;
	float MaxContent = 0.f;
};

struct FKPCLPressureValveEvaluation
{
	bool bShouldBeOpen = false;
	float MonitoredFillPercent = 0.f;
};

class KPRIVATECODELIB_API FKPCLPressureRegulatorValveLogic
{
public:

	static float CalculateFillPercent(const FKPCLPressureValvePipeData& Pipe);

	static FKPCLPressureValveEvaluation Evaluate(const FKPCLPressureValvePipeData& PipeA,
												 const FKPCLPressureValvePipeData& PipeB, float OnThreshold,
												 float OffThreshold, bool bValveIsOpen);

private:
	static bool IsReadable(const FKPCLPressureValvePipeData& Pipe);
};
