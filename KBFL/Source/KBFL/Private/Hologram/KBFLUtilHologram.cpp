// Copyright Coffee Stain Studios. All Rights Reserved.

#pragma once

#include "Hologram/KBFLUtilHologram.h"

#include "BFL/KBFL_Player.h"
#include "Hologram/KBFLUtilBuildable.h"

DECLARE_LOG_CATEGORY_EXTERN(KBFLUtilHologramLog, Log, All)

DEFINE_LOG_CATEGORY(KBFLUtilHologramLog)

void AKBFLUtilHologram::BeginPlay()
{
	Super::BeginPlay();
	AKBFLLocationSubsystem* Subsystem = AKBFLLocationSubsystem::GetAKBFLLocationSubsystem(this);
	if (Subsystem)
	{
		if (Subsystem->LastClass != mBuildClass)
		{
			Subsystem->LastClass = mBuildClass;
			Subsystem->AddRotation = AddRotation;
			Subsystem->AddVector = AddVector;
			Subsystem->Scale = Scale;
		}

		if (UseRotationFromSubsystem)
		{
			AddRotation = Subsystem->AddRotation;
		}
		if (UseVectorFromSubsystem)
		{
			AddVector = Subsystem->AddVector;
		}
		if (UseScaleFromSubsystem)
		{
			Scale = Subsystem->Scale;
		}
	}

	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget->ReleaseSlateResources(true);
	}

	AFGPlayerController* Controller = UKBFL_Player::GetFGController(this);
	if (Controller && WidgetClass)
	{
		Widget = CreateWidget<UKBFLUtilWidget>(Controller, WidgetClass);
		Widget->AddToViewport();
		Widget->AddToPlayerScreen();
		UpdateWidget();
	}
}

bool AKBFLUtilHologram::IsValidHitResult(const FHitResult& hitResult) const { return hitResult.IsValidBlockingHit(); }

void AKBFLUtilHologram::UpdateWidget()
{
	if (Widget)
	{
		Widget->OnNewStats(Scale, AddVector, AddRotation);
	}
}

void AKBFLUtilHologram::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AKBFLLocationSubsystem* Subsystem = AKBFLLocationSubsystem::GetAKBFLLocationSubsystem(this);

	if (Subsystem)
	{
		Subsystem->AddRotation = AddRotation;
		Subsystem->AddVector = AddVector;
		Subsystem->Scale = Scale;
	}

	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget->ReleaseSlateResources(true);
	}

	Super::EndPlay(EndPlayReason);
}

void AKBFLUtilHologram::GetSupportedBuildModes_Implementation(
	TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const
{
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);
	for (auto Class : mBuildModes)
	{
		out_buildmodes.AddUnique(Class);
	}
}

void AKBFLUtilHologram::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	if (hitResult.IsValidBlockingHit())
	{
		SetActorLocationAndRotation(hitResult.Location + AddVector, AddRotation);
		SetActorScale3D(Scale);
	}
}

void AKBFLUtilHologram::Scroll(int32 delta)
{
	if (IsCurrentBuildMode(mBuildModes[0]))
	{
		AddRotation.Yaw += ScrollMulti * delta;
	}
	if (IsCurrentBuildMode(mBuildModes[1]))
	{
		AddRotation.Pitch += ScrollMulti * delta;
	}
	if (IsCurrentBuildMode(mBuildModes[2]))
	{
		AddRotation.Roll += ScrollMulti * delta;
	}
	if (IsCurrentBuildMode(mBuildModes[3]))
	{
		AddVector.Z += ScrollMulti * delta;
	}
	if (IsCurrentBuildMode(mBuildModes[4]))
	{
		Scale.X += ScaleMulti * delta;
		Scale.Y += ScaleMulti * delta;
		Scale.Z += ScaleMulti * delta;
	}
	if (IsCurrentBuildMode(mBuildModes[5]))
	{
		AddVector.Y += ScrollMulti * delta;
	}
	if (IsCurrentBuildMode(mBuildModes[6]))
	{
		AddVector.X += ScrollMulti * delta;
	}
	UpdateWidget();
	// Super::Scroll(delta);
}

void AKBFLUtilHologram::ScrollRotate(int32 delta, int32 step)
{
	if (IsCurrentBuildMode(mBuildModes[0]))
	{
		AddRotation.Yaw += ScrollMulti * delta;
	}
	if (IsCurrentBuildMode(mBuildModes[1]))
	{
		AddRotation.Pitch += ScrollMulti * delta;
	}
	if (IsCurrentBuildMode(mBuildModes[2]))
	{
		AddRotation.Roll += ScrollMulti * delta;
	}
	if (IsCurrentBuildMode(mBuildModes[3]))
	{
		AddVector.Z += ScrollMulti * delta;
	}
	if (IsCurrentBuildMode(mBuildModes[4]))
	{
		Scale.X += ScaleMulti * delta;
		Scale.Y += ScaleMulti * delta;
		Scale.Z += ScaleMulti * delta;
	}
	UpdateWidget();
	// Super::ScrollRotate(delta, step);
}

void AKBFLUtilHologram::ConfigureComponents(AFGBuildable* inBuildable) const
{
	Super::ConfigureComponents(inBuildable);

	if (AKBFLUtilBuildable* Buildable = Cast<AKBFLUtilBuildable>(inBuildable))
	{
		Buildable->Scale = Scale;
	}
}
