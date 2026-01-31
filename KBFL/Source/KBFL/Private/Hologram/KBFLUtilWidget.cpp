// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "Hologram/KBFLUtilWidget.h"
#include "Subsystems/KBFLLocationSubsystem.h"

void UKBFLUtilWidget::OnNewStats_Implementation(const FVector& NewScale, const FVector& NewLocation,
												const FRotator& NewRotation)
{
	Rotation = NewRotation;
	Scale = NewScale;
	Location = NewLocation;
}
