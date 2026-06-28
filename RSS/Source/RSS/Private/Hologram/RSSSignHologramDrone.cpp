// Fill out your copyright notice in the Description page of Project Settings.

#include "Hologram/RSSSignHologramDrone.h"

#include "Buildable/RSSBuildableSignPole.h"
#include "Buildables/FGBuildablePowerPole.h"
#include "Subsystem/RSSDataManagerSubsystem.h"

void ARSSSignHologramDrone::BeginPlay()
{
	Super::BeginPlay();

	mSubsystem = ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(this);
	if (mSubsystem)
	{
		Rotation = mSubsystem->GetLastRotation();
	}
}

void ARSSSignHologramDrone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (mSubsystem)
	{
		mSubsystem->SetLastRotation(Rotation);
	}
	Super::EndPlay(EndPlayReason);
}

void ARSSSignHologramDrone::GetSupportedBuildModes_Implementation(
	TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const
{
	if (mBuildStep != EDroneBuildStep::UpDown)
	{
		Super::GetSupportedBuildModes_Implementation(out_buildmodes);
	}
	else
	{
		out_buildmodes.Empty();
		out_buildmodes.AddUnique(mUpDownMode);
		out_buildmodes.AddUnique(mRotationMode);
	}
}

bool ARSSSignHologramDrone::IsValidHitResult(const FHitResult& hitResult) const
{
	if (mBuildStep == EDroneBuildStep::UpDown)
	{
		return true;
	}

	return Super::IsValidHitResult(hitResult);
}

int32 ARSSSignHologramDrone::GetRotationStep() const
{
	// int32 Rota = Rotation * mRoationStep;
	return mRoationStep;
}

void ARSSSignHologramDrone::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	FVector Location = mBuildStep == EDroneBuildStep::Placement ? GetActorLocation() : mPlaceLocation;

	if (mBuildStep == EDroneBuildStep::Placement)
	{
		Super::SetHologramLocationAndRotation(hitResult);
		Location = GetActorLocation();
		Location.Z += mDownMin;
	}
	else
	{
		Location.Z += DroneHeight;
	}

	FRotator Rotator = GetActorRotation();
	Rotator.Yaw = GetRotationFromSteps();

	SetActorLocationAndRotation(Location, Rotator);
}

bool ARSSSignHologramDrone::TrySnapToActor(const FHitResult& hitResult)
{
	if (mBuildStep == EDroneBuildStep::Placement)
	{
		mPlaceLocation = GetActorLocation();
		return Super::TrySnapToActor(hitResult);
	}
	return false;
}

void ARSSSignHologramDrone::Scroll(int32 Delta)
{
	if (mBuildStep == EDroneBuildStep::UpDown && IsCurrentBuildMode(mUpDownMode))
	{
		DroneHeight += mGridSteps * Delta;
		DroneHeight = FMath::Clamp(DroneHeight, 0.0f, mUpMax);
	}
	else
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
	}

	Super::Scroll(Delta);
}

bool ARSSSignHologramDrone::DoMultiStepPlacement(bool bIsInputFromARelease)
{
	bool Superbool = Super::DoMultiStepPlacement(bIsInputFromARelease);
	// SetBuildMode(mUpDownMode);
	mBuildStep = mBuildStep == EDroneBuildStep::Placement ? EDroneBuildStep::UpDown : EDroneBuildStep::Build;
	return Superbool && mBuildStep == EDroneBuildStep::Build;
}

float ARSSSignHologramDrone::RoundInSteps(float HitLocation, float MainLocation, bool CanBeNegative,
										  int32 maxStepSize) const
{
	float differnce = HitLocation - MainLocation;
	int32 Steps = 0;

	Steps = FMath::FloorToInt(differnce / mGridSteps);

	if (!CanBeNegative && Steps < 0)
	{
		Steps = 0;
	}

	if (maxStepSize > 0)
	{
		if ((Steps < 0 ? Steps * -1 : Steps) > maxStepSize)
		{
			Steps = Steps < 0 ? maxStepSize * -1 : maxStepSize;
		}
	}

	return MainLocation + Steps * mGridSteps;
}

float ARSSSignHologramDrone::GetRotationFromSteps() const { return mRoationStep * Rotation; }
