#include "BFL/KBFL_Math.h"

#include "Kismet/KismetMathLibrary.h"

TArray<FVector> UKBFL_Math::CalculateOrbitPoints(float Ellipse, float Radius, float Tilt, int PointCount)
{
	TArray<FVector> LocalSplinePoints = {};
	for (int i = 0; i < PointCount; i++)
	{
		float Degree = 360 / (PointCount + 1);
		float CurDegree = i * Degree;
		float X = UKismetMathLibrary::DegSin(CurDegree) * (2 - Ellipse) * Radius;
		float Y = UKismetMathLibrary::DegCos(CurDegree) * Radius * Ellipse;
		float Z = UKismetMathLibrary::DegCos(CurDegree) * Radius * Tilt;
		LocalSplinePoints.Add(FVector(X, Y, Z));
	}
	return LocalSplinePoints;
}

float UKBFL_Math::CalculateItemsPerMinFloat(float ItemCount, float ProductionTime)
{
	return (60.0f / ProductionTime) * ItemCount;
}

float UKBFL_Math::CalculateM3PerMinFloat(float ItemCount, float ProductionTime)
{
	return (60.0f / ProductionTime) * ItemCount / 1000.0f;
}

float UKBFL_Math::CalculateItemsPerMin(int ItemCount, float ProductionTime)
{
	return CalculateItemsPerMinFloat(static_cast<float>(ItemCount), ProductionTime);
}

float UKBFL_Math::CalculateM3PerMin(int ItemCount, float ProductionTime)
{
	return CalculateM3PerMinFloat(static_cast<float>(ItemCount), ProductionTime);
}

float UKBFL_Math::CalculateM3PerMinForWidget(int ItemCount, float ProductionTime)
{
	return (60.0f / ProductionTime) * static_cast<float>(ItemCount) / 1000.0f / 60.0f;
}
