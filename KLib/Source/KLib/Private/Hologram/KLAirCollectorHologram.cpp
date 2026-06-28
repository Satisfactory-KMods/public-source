// Fill out your copyright notice in the Description page of Project Settings.

#include "Hologram/KLAirCollectorHologram.h"

#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Buildable/KLBuildableAirCollector.h"
#include "Buildables/FGBuildableFoundation.h"
#include "FGConstructDisqualifier.h"
#include "Kismet/KismetSystemLibrary.h"


AKLAirCollectorHologram::AKLAirCollectorHologram()
{
	mSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
	mSphere->SetupAttachment(GetRootComponent());
}

void AKLAirCollectorHologram::GetSupportedBuildModes_Implementation(
	TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const
{
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);

	if (mSphereMode)
	{
		out_buildmodes.AddUnique(mSphereMode);
	}
}

void AKLAirCollectorHologram::OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode)
{
	Super::OnBuildModeChanged(buildMode);
	mSphere->SetHiddenInGame(buildMode != mSphereMode);
}

void AKLAirCollectorHologram::BeginPlay()
{
	Super::BeginPlay();

	mGameUserSettings = UFGGameUserSettings::GetFGGameUserSettings();
	const AKLBuildableAirCollector* Default = GetDefaultBuildable<AKLBuildableAirCollector>();
	if (IsValid(Default) && IsValid(Default->GetLightweightInstanceData()))
	{
		mFoundationMeshMapping = Default->mFoundationMeshMapping;
		mCachedScanRange = Default->mRangeForFindCollectors;

		for (UActorComponent* Component : GetComponents())
		{
			mStaticMesh = Cast<UStaticMeshComponent>(Component);
			if (IsValid(mStaticMesh))
			{
				if (Default->GetLightweightInstanceData()->GetInstanceData()[0].StaticMesh ==
					mStaticMesh->GetStaticMesh())
				{
					break;
				}
				mStaticMesh = nullptr;
			}
		}

		mSphere->SetHiddenInGame(!IsCurrentBuildMode(mSphereMode));
		mSphere->SetWorldScale3D(FVector(mCachedScanRange / 50));
	}
}

void AKLAirCollectorHologram::UpdateFoundationMesh(const AActor* HitActor)
{
	if (!IsValid(mStaticMesh))
	{
		return;
	}

	UStaticMesh* NewMesh = nullptr;

	if (const AFGBuildableFoundation* Foundation = Cast<AFGBuildableFoundation>(HitActor))
	{
		const int32 Key = FMath::TruncToInt(Foundation->mHeight / 100);
		if (Key != 0)
		{
			if (TObjectPtr<UStaticMesh>* Found = mFoundationMeshMapping.Find(Key))
			{
				NewMesh = Found->Get();
			}
		}
	}

	// Fall back to the default (key 1) mesh when not on a matching foundation.
	if (!NewMesh)
	{
		if (TObjectPtr<UStaticMesh>* Fallback = mFoundationMeshMapping.Find(1))
		{
			NewMesh = Fallback->Get();
		}
	}

	if (NewMesh)
	{
		mStaticMesh->SetMobility(EComponentMobility::Stationary);
		mStaticMesh->SetStaticMesh(NewMesh);
		mStaticMesh->SetMobility(EComponentMobility::Movable);
	}
}

bool AKLAirCollectorHologram::TrySnapToActor(const FHitResult& hitResult)
{
	const bool bSnapped = Super::TrySnapToActor(hitResult);

	if (bSnapped && GetSnappedBuilding() && hitResult.bBlockingHit)
	{
		UpdateFoundationMesh(ExactCast<AFGBuildableFoundation>(GetSnappedBuilding()));
	}
	else
	{
		UpdateFoundationMesh(nullptr);
	}

	return bSnapped;
}

void AKLAirCollectorHologram::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	Super::SetHologramLocationAndRotation(hitResult);

	if (hitResult.bBlockingHit && hitResult.GetActor())
	{
		UpdateFoundationMesh(Cast<AFGBuildableFoundation>(hitResult.GetActor()));
	}
	else
	{
		UpdateFoundationMesh(nullptr);
	}

	// Only run the range-overlap check and sphere color update in sphere build mode —
	// the sphere is hidden otherwise and the overlap cost is wasted.
	if (!IsCurrentBuildMode(mSphereMode) || !IsValid(mGameUserSettings))
	{
		return;
	}

	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes =
		TArray<TEnumAsByte<EObjectTypeQuery>>{ObjectTypeQuery1, ObjectTypeQuery2};

	TArray<AActor*> ActorsToIgnore = {this};
	TArray<AActor*> OutActors = {};

	const bool bIsInRange =
		UKismetSystemLibrary::SphereOverlapActors(this, GetActorLocation(), mCachedScanRange, ObjectTypes,
		                                          AKLBuildableAirCollector::StaticClass(), ActorsToIgnore, OutActors);

	mSphere->SetCustomPrimitiveDataVector3(
		0, bIsInRange ? mGameUserSettings->mInvalidPlacementHologramColour : mGameUserSettings->mBuildHologramColour);
}

float AKLAirCollectorHologram::GetScanRange() const
{
	AKLBuildableAirCollector* Default = GetDefaultBuildable<AKLBuildableAirCollector>();
	return Default->mRangeForFindCollectors;
}
