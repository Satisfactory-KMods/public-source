// Fill out your copyright notice in the Description page of Project Settings.


#include "Hologram/RSSSignHologramPipeAndBelts.h"

#include "Buildable/RSSBuildableSignPole.h"
#include "Buildables/FGBuildableConveyorBelt.h"
#include "Buildables/FGBuildablePipeline.h"
#include "FGConstructDisqualifier.h"
#include "FGFactoryConnectionComponent.h"
#include "Interface/RssSignInterface.h"
#include "Subsystem/RSSDataManagerSubsystem.h"

void ARSSSignHologramPipeAndBelts::BeginPlay()
{
	Super::BeginPlay();

	mSubsystem = ARssDataManagerSubsystem::GetRSSDataManagerSubsystem(this);
	if (mSubsystem)
	{
		Rotation = mSubsystem->GetLastRotation();
	}
}

void ARSSSignHologramPipeAndBelts::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (mSubsystem)
	{
		mSubsystem->SetLastRotation(Rotation);
	}
	Super::EndPlay(EndPlayReason);
}

int32 ARSSSignHologramPipeAndBelts::GetRotationStep() const
{
	// int32 Rota = Rotation * mRoationStep;
	return Super::GetRotationStep();
}

bool ARSSSignHologramPipeAndBelts::IsValidHitResult(const FHitResult& hitResult) const
{
	if (hitResult.IsValidBlockingHit())
	{
		if (hitResult.GetActor())
		{
			const AActor* HitActor = hitResult.GetActor();
			const UClass* HitActorClass = HitActor->GetClass();

			if (HitActor->IsA(AFGBuildablePipeline::StaticClass()) && mShouldSnapOnPipe)
			{
				return true;
			}

			if (HitActor->IsA(AFGBuildableConveyorBelt::StaticClass()) && mShouldSnapOnBelt)
			{
				return true;
			}
		}
	}

	return Super::IsValidHitResult(hitResult);
}

void ARSSSignHologramPipeAndBelts::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	if (mBuildStep == 0)
	{
		mLastActorType = GetTargetActor(hitResult);
		mTargetActor = hitResult.GetActor();

		if (mTargetActor)
		{
			if (mLastActorType == ERssTargetActorType::Pipe)
			{
				HandlePipeSnap(hitResult);
				return;
			}

			if (mLastActorType == ERssTargetActorType::Belt)
			{
				HandleBeltSnap(hitResult);
				return;
			}
			mIsValid = false;
		}

		Super::SetHologramLocationAndRotation(hitResult);
	}
}

void ARSSSignHologramPipeAndBelts::Scroll(int32 Delta)
{
	if (mBuildStep > 0)
	{
		FRotator Rotator = GetActorRotation();

		Rotator.Roll += Delta * 180;
		SetActorRotation(Rotator);
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

		Super::Scroll(Delta);
	}
}

void ARSSSignHologramPipeAndBelts::CheckValidPlacement()
{
	Super::CheckValidPlacement();

	if (!mIsValid)
	{
		AddConstructDisqualifier(UFGCDInvalidPlacement::StaticClass());
	}
}

void ARSSSignHologramPipeAndBelts::OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode)
{
	mIsMir = GetDefaultBuildGunMode() != mReversedBuildMode;
}

void ARSSSignHologramPipeAndBelts::HandlePipeSnap(const FHitResult& hitResult)
{
	AFGBuildablePipeline* Pipeline = Cast<AFGBuildablePipeline>(mTargetActor);
	if (Pipeline)
	{
		const FVector& WorldLocation = hitResult.Location;
		SetActorLocation(FindSplineLocation(WorldLocation, Pipeline->GetSplineComponent()));
		FRotator Rotator = FindSplineRotator(WorldLocation, Pipeline->GetSplineComponent());

		Rotator.Roll += GetRotationFromSteps();
		Rotator.Pitch += mIsMir * 180;

		SetActorRotation(Rotator);
		mIsValid = true;
	}
}

void ARSSSignHologramPipeAndBelts::HandleBeltSnap(const FHitResult& hitResult)
{
	AFGBuildableConveyorBelt* Belt = Cast<AFGBuildableConveyorBelt>(mTargetActor);
	if (Belt)
	{
		const FVector& WorldLocation = hitResult.Location;
		SetActorLocation(FindSplineLocation(WorldLocation, Belt->GetSplineComponent()));

		FRotator Rotator = FindSplineRotator(WorldLocation, Belt->GetSplineComponent());
		Rotator.Roll += mIsMir * 180;
		Rotator.Pitch += mIsMir * 180;
		SetActorRotation(Rotator);
		mIsValid = true;
	}
}

void ARSSSignHologramPipeAndBelts::GetSupportedBuildModes_Implementation(
	TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const
{
	out_buildmodes.AddUnique(mDefaultBuildMode);
	out_buildmodes.AddUnique(mReversedBuildMode);
}

FVector ARSSSignHologramPipeAndBelts::FindSplineLocation(const FVector& WorldLocation, USplineComponent* Spline)
{
	return Spline->FindLocationClosestToWorldLocation(WorldLocation, ESplineCoordinateSpace::World);
}

FRotator ARSSSignHologramPipeAndBelts::FindSplineRotator(const FVector& WorldLocation, USplineComponent* Spline)
{
	return Spline->FindRotationClosestToWorldLocation(WorldLocation, ESplineCoordinateSpace::World);
}

float ARSSSignHologramPipeAndBelts::GetRotationFromSteps() const { return mRoationStep * Rotation; }

ERssTargetActorType ARSSSignHologramPipeAndBelts::GetTargetActor(const FHitResult& hitResult) const
{
	if (hitResult.IsValidBlockingHit())
	{
		if (hitResult.GetActor())
		{
			AActor* HitActor = hitResult.GetActor();

			if (HitActor->IsA(AFGBuildablePipeline::StaticClass()) && mShouldSnapOnPipe)
			{
				return ERssTargetActorType::Pipe;
			}

			if (HitActor->IsA(AFGBuildableConveyorBelt::StaticClass()) && mShouldSnapOnBelt)
			{
				return ERssTargetActorType::Belt;
			}
		}
	}

	return ERssTargetActorType::Invalid;
}
