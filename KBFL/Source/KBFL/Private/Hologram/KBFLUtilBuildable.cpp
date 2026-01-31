// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "Hologram/KBFLUtilBuildable.h"
#include "Subsystems/KBFLLocationSubsystem.h"

void AKBFLUtilBuildable::BeginPlay()
{
	Super::BeginPlay();
	AKBFLLocationSubsystem* Subsystem = AKBFLLocationSubsystem::GetAKBFLLocationSubsystem(GetWorld());
	if (Subsystem)
	{
		SetActorScale3D(Scale);
		Subsystem->SaveLocation(GetClass(), GetActorTransform());
	}
}

void AKBFLUtilBuildable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		if (AKBFLLocationSubsystem* Subsystem = AKBFLLocationSubsystem::GetAKBFLLocationSubsystem(GetWorld()))
		{
			Subsystem->RemoveLocation(GetClass(), GetActorTransform());
		}
	}
	Super::EndPlay(EndPlayReason);
}
