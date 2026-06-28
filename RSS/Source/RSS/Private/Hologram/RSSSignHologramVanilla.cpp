// Fill out your copyright notice in the Description page of Project Settings.

#include "Hologram/RSSSignHologramVanilla.h"

#include "Buildable/RSSBuildableSignPole.h"
#include "Buildables/FGBuildablePowerPole.h"
#include "Hologram/FGSignPoleHologram.h"
#include "Subsystem/RSSDataManagerSubsystem.h"

void ARSSSignHologramVanilla::BeginPlay()
{
	Super::BeginPlay();

	mCachedRotationSteps = mRotationStep;

	mSubsystem = ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(this);
	if (mSubsystem)
	{
		Rotation = mSubsystem->GetLastRotation();
	}
}

void ARSSSignHologramVanilla::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (mSubsystem)
	{
		mSubsystem->SetLastRotation(Rotation);
	}
	Super::EndPlay(EndPlayReason);
}

void ARSSSignHologramVanilla::GetSupportedBuildModes_Implementation(
	TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const
{
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);

	if (mIsValid)
	{
		out_buildmodes.AddUnique(mFineRotationMode);
	}
}

bool ARSSSignHologramVanilla::IsValidHitResult(const FHitResult& hitResult) const
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

bool ARSSSignHologramVanilla::TrySnapToActor(const FHitResult& hitResult)
{
	if (hitResult.IsValidBlockingHit())
	{
		if (hitResult.GetActor())
		{
			mTargetActor = hitResult.GetActor();
			if ((mTargetActor->IsA(ARSSBuildableSignPole::StaticClass()) ||
				 mTargetActor->IsA(AFGBuildablePowerPole::StaticClass())) &&
				mShouldSnapOnSignPole)
			{
				HandlePoleSnap(hitResult);

				mIsValid = true;
				return true;
			}
		}
	}

	mIsValid = false;
	return Super::TrySnapToActor(hitResult);
}

void ARSSSignHologramVanilla::Scroll(int32 Delta)
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

int32 ARSSSignHologramVanilla::GetRotationStep() const
{
	return IsCurrentBuildMode(mFineRotationMode) ? mFineRotationSteps : mRotationStep;
}

void ARSSSignHologramVanilla::SpawnChildren(AActor* hologramOwner, FVector SpawnLocation, APawn* HologramInstigator)
{
	if (!mIsValid)
	{
		Super::SpawnChildren(hologramOwner, SpawnLocation, HologramInstigator);
	}
	else
	{
		if (mChildSignPoleHologram)
		{
			mChildSignPoleHologram->Destroy();
		}
	}
}

void ARSSSignHologramVanilla::CheckValidFloor()
{
	Super::CheckValidFloor();

	if (mIsValid)
	{
		mNeedsValidFloor = false;
	}
}

void ARSSSignHologramVanilla::HandlePoleSnap(const FHitResult& hitResult)
{
	FVector TargetActorLocation = mTargetActor->GetActorLocation();
	TargetActorLocation.Z = RoundInSteps(hitResult.Location.Z, TargetActorLocation.Z, false);
	SetActorRotation(FRotator(0, GetRotationFromSteps(), 0));

	FRotator newRot = GetActorRotation();
	newRot.Yaw += mPoleRoationYaw;
	SetActorLocationAndRotation(TargetActorLocation + GetActorForwardVector() * mTargetPoleOffset, newRot);
}

float ARSSSignHologramVanilla::RoundInSteps(float HitLocation, float MainLocation, bool CanBeNegative,
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

float ARSSSignHologramVanilla::GetRotationFromSteps() const { return GetRotationStep() * Rotation; }
