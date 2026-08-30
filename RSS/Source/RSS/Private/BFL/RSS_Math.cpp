#include "RSS_Math.h"

#include "Kismet/KismetMathLibrary.h"

TArray<FVector> URSS_Math::CalculateOrbitPoints(float Ellipse, float Radius, float Tilt, int PointCount)
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
