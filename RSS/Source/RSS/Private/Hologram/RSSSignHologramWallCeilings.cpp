// Fill out your copyright notice in the Description page of Project Settings.

#include "Hologram/RSSSignHologramWallCeilings.h"

#include "Buildable/RSSBuildableSignPole.h"
#include "Buildables/FGBuildablePowerPole.h"
#include "Subsystem/RSSDataManagerSubsystem.h"

void ARSSSignHologramWallCeilings::BeginPlay()
{
	Super::BeginPlay();

	mCachedRotationSteps = mRotationStep;

	mSubsystem = ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(this);
	if (mSubsystem)
	{
		Rotation = mSubsystem->GetLastRotation();
	}
}

void ARSSSignHologramWallCeilings::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (mSubsystem)
	{
		mSubsystem->SetLastRotation(Rotation);
	}
	Super::EndPlay(EndPlayReason);
}

int32 ARSSSignHologramWallCeilings::GetRotationStep() const
{
	return IsCurrentBuildMode(mFineRotationMode) ? mFineRotationSteps : mRotationStep;
}

void ARSSSignHologramWallCeilings::OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode)
{
	Super::OnBuildModeChanged(buildMode);

	// mRotationStep = IsCurrentBuildMode(mFineRotationMode) ? mFineRotationSteps : mCachedRotationSteps;
}

void ARSSSignHologramWallCeilings::GetSupportedBuildModes_Implementation(
	TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const
{
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);

	out_buildmodes.AddUnique(mFineRotationMode);
}

bool ARSSSignHologramWallCeilings::IsValidHitResult(const FHitResult& hitResult) const
{
	if (hitResult.IsValidBlockingHit())
	{
		if (hitResult.GetActor())
		{
			const AActor* HitActor = hitResult.GetActor();

			if ((HitActor->IsA(ARSSBuildableSignPole::StaticClass()) ||
				 HitActor->IsA(AFGBuildablePowerPole::StaticClass())) &&
				mShouldSnapOnSignPole)
			{
				return true;
			}
		}
	}

	return Super::IsValidHitResult(hitResult);
}

void ARSSSignHologramWallCeilings::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	Super::SetHologramLocationAndRotation(hitResult);

	mLastActorType = GetTargetActor(hitResult);
	mTargetActor = hitResult.GetActor();

	if (mTargetActor)
	{
		if (mLastActorType == ERssTargetActorType::SignPole)
		{
			HandlePoleSnap(hitResult);
			return;
		}
	}

	mIsValid = false;
}

void ARSSSignHologramWallCeilings::Scroll(int32 Delta)
{
	if (IsCurrentBuildMode(mFineRotationMode))
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

void ARSSSignHologramWallCeilings::CheckValidPlacement()
{
	Super::CheckValidPlacement();

	if (mIsValid)
	{
		ResetConstructDisqualifiers();
	}
}

void ARSSSignHologramWallCeilings::HandlePoleSnap(const FHitResult& hitResult)
{
	FVector TargetActorLocation = mTargetActor->GetActorLocation();
	TargetActorLocation.Z = RoundInSteps(hitResult.Location.Z, TargetActorLocation.Z, false);
	SetActorLocation(TargetActorLocation);
	if (IsCurrentBuildMode(mFineRotationMode))
	{
		SetActorRotation(FRotator(0, GetRotationFromSteps(), 0));
	}
	mIsValid = true;
}

float ARSSSignHologramWallCeilings::RoundInSteps(float HitLocation, float MainLocation, bool CanBeNegative,
												 int32 maxStepSize) const
{
	float differnce = HitLocation - MainLocation;
	int32 Steps = FMath::FloorToInt(differnce / mGridSteps);

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

float ARSSSignHologramWallCeilings::GetRotationFromSteps() const { return mFineRotationSteps * Rotation; }

ERssTargetActorType ARSSSignHologramWallCeilings::GetTargetActor(const FHitResult& hitResult) const
{
	if (hitResult.IsValidBlockingHit())
	{
		if (hitResult.GetActor())
		{
			AActor* HitActor = hitResult.GetActor();

			if ((HitActor->IsA(ARSSBuildableSignPole::StaticClass()) ||
				 HitActor->IsA(AFGBuildablePowerPole::StaticClass())) &&
				mShouldSnapOnSignPole)
			{
				return ERssTargetActorType::SignPole;
			}
		}
	}

	return ERssTargetActorType::Invalid;
}
