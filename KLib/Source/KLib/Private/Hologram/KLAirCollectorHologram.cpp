// Fill out your copyright notice in the Description page of Project Settings.


#include "Hologram/KLAirCollectorHologram.h"

#include "FGConstructDisqualifier.h"
#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Buildable/KLBuildableAirCollector.h"
#include "Buildables/FGBuildableFoundation.h"
#include "Kismet/KismetSystemLibrary.h"


void AKLAirCollectorHologram::GetSupportedBuildModes_Implementation(
	TArray<TSubclassOf<UFGBuildGunModeDescriptor>>& out_buildmodes) const
{
	Super::GetSupportedBuildModes_Implementation(out_buildmodes);

	if (mSphereMode)
	{
		out_buildmodes.Add(mSphereMode);
	}
}

void AKLAirCollectorHologram::OnBuildModeChanged(TSubclassOf<UFGHologramBuildModeDescriptor> buildMode)
{
	Super::OnBuildModeChanged(buildMode);
	mSphere->SetHiddenInGame(buildMode != mSphereMode);
}

AKLAirCollectorHologram::AKLAirCollectorHologram()
{
	mSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
	mSphere->SetupAttachment(GetRootComponent());
}

void AKLAirCollectorHologram::BeginPlay()
{
	Super::BeginPlay();

	mGameUserSettings = UFGGameUserSettings::GetFGGameUserSettings();
	const AKLBuildableAirCollector* Default = GetDefaultBuildable<AKLBuildableAirCollector>();
	if (IsValid(Default) && IsValid(Default->GetLightweightInstanceData()))
	{
		mFoundationMeshMapping = Default->mFoundationMeshMapping;

		for (UActorComponent* Component : GetComponents())
		{
			mStaticMesh = Cast<UStaticMeshComponent>(Component);
			if (IsValid(mStaticMesh))
			{
				if (Default->GetLightweightInstanceData()->GetInstanceData()[0].StaticMesh == mStaticMesh->
					GetStaticMesh())
				{
					break;
				}
				mStaticMesh = nullptr;
			}
		}

		mSphere->SetHiddenInGame(!IsCurrentBuildMode(mSphereMode));
		mSphere->SetWorldScale3D(FVector(GetScanRange() / 50));
	}
}

bool AKLAirCollectorHologram::TrySnapToActor(const FHitResult& hitResult)
{
	//FHitResult ResolvedHitResult;
	//UKPCLBlueprintFunctionLib::ResolveHitResult( GetWorld(), hitResult, ResolvedHitResult );

	bool Super = Super::TrySnapToActor(hitResult);

	if (Super && GetSnappedBuilding() && hitResult.bBlockingHit)
	{
		if (AFGBuildableFoundation* Foundation = ExactCast<AFGBuildableFoundation>(GetSnappedBuilding()))
		{
			const int Key = FMath::TruncToInt(Foundation->mHeight / 100);
			if (Key)
			{
				if (mFoundationMeshMapping.Contains(Key))
				{
					if (mFoundationMeshMapping[Key])
					{
						mStaticMesh->SetMobility(EComponentMobility::Stationary);
						mStaticMesh->SetStaticMesh(mFoundationMeshMapping[Key]);
						mStaticMesh->SetMobility(EComponentMobility::Movable);
					}
				}
			}
		}
	}
	else
	{
		mStaticMesh->SetMobility(EComponentMobility::Stationary);
		mStaticMesh->SetStaticMesh(mFoundationMeshMapping[1]);
		mStaticMesh->SetMobility(EComponentMobility::Movable);
	}

	return Super;
}

void AKLAirCollectorHologram::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	Super::SetHologramLocationAndRotation(hitResult);

	if (hitResult.bBlockingHit && hitResult.GetActor())
	{
		if (AFGBuildableFoundation* Foundation = Cast<AFGBuildableFoundation>(hitResult.GetActor()))
		{
			const int Key = FMath::TruncToInt(Foundation->mHeight / 100);
			if (Key)
			{
				if (mFoundationMeshMapping.Contains(Key))
				{
					if (mFoundationMeshMapping[Key])
					{
						mStaticMesh->SetMobility(EComponentMobility::Stationary);
						mStaticMesh->SetStaticMesh(mFoundationMeshMapping[Key]);
						mStaticMesh->SetMobility(EComponentMobility::Movable);
					}
				}
			}
		}
	}
	else
	{
		mStaticMesh->SetMobility(EComponentMobility::Stationary);
		mStaticMesh->SetStaticMesh(mFoundationMeshMapping[1]);
		mStaticMesh->SetMobility(EComponentMobility::Movable);
	}

	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = TArray<TEnumAsByte<EObjectTypeQuery>>{
		ObjectTypeQuery1, ObjectTypeQuery2
	};

	TArray<AActor*> ActorsToIgnore = {this};
	TArray<AActor*> OutActors = {};

	bool IsInRangeCache = UKismetSystemLibrary::SphereOverlapActors(this, GetActorLocation(), GetScanRange(),
	                                                                ObjectTypes,
	                                                                AKLBuildableAirCollector::StaticClass(),
	                                                                ActorsToIgnore, OutActors);

	mSphere->SetCustomPrimitiveDataVector3(0, IsInRangeCache
		                                          ? mGameUserSettings->mInvalidPlacementHologramColour
		                                          : mGameUserSettings->mBuildHologramColour);
}

float AKLAirCollectorHologram::GetScanRange() const
{
	AKLBuildableAirCollector* Default = GetDefaultBuildable<AKLBuildableAirCollector>();
	return Default->mRangeForFindCollectors;
}
