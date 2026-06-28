// Fill out your copyright notice in the Description page of Project Settings.

#include "Hologram/RSSSignHologram.h"

#include "Buildable/RSSBuildableSignPole.h"
#include "Buildables/FGBuildablePowerPole.h"
#include "Subsystem/RSSDataManagerSubsystem.h"

void ARSSSignHologram::BeginPlay()
{
	Super::BeginPlay();

	mSubsystem = ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(this);
	if (mSubsystem)
	{
		Rotation = mSubsystem->GetLastRotation();
	}
}

void ARSSSignHologram::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (mSubsystem)
	{
		mSubsystem->SetLastRotation(Rotation);
	}
	Super::EndPlay(EndPlayReason);
}

int32 ARSSSignHologram::GetRotationStep() const
{
	return IsCurrentBuildMode(mFineRotationMode) ? mFineRotationSteps : Rotation;
}

void ARSSSignHologram::GetSupportedBuildModes_Implementation(
	TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const
{
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);

	out_buildmodes.AddUnique(mFineRotationMode);
}

void ARSSSignHologram::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	Super::SetHologramLocationAndRotation(hitResult);

	if (IsCurrentBuildMode(mFineRotationMode))
	{
		FRotator Rotator = GetActorRotation();
		Rotator.Yaw = GetRotationFromSteps();
		SetActorRotation(Rotator);
	}
}

bool ARSSSignHologram::TrySnapToActor(const FHitResult& hitResult)
{
	bool Super = Super::TrySnapToActor(hitResult);
	/*if(Super)
	{
		FRotator Rotator =  GetActorRotation();
		Rotator.Yaw = GetRotationFromSteps();
		SetActorRotation(Rotator);
	}*/
	return Super;
}

void ARSSSignHologram::Scroll(int32 Delta)
{
	Rotation += Delta;
	if (Rotation >= 360)
	{
		Rotation = 0;
	}

	if (Rotation <= -360)
	{
		Rotation = 0;
	}

	Super::Scroll(Delta);
}

float ARSSSignHologram::GetRotationFromSteps() const { return GetRotationStep() * Rotation; }
