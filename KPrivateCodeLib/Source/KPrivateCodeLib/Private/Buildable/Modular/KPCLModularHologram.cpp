// Fill out your copyright notice in the Description page of Project Settings.
// Fill out your copyright notice in the Description page of Project Settings.

#include "Buildable/Modular/KPCLModularHologram.h"

#include "FGConstructDisqualifier.h"
#include "Buildable/Modular/KPCLModularBuildingHandler.h"
#include "Buildable/Modular/KPCLModularBuildingInterface.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Net/UnrealNetwork.h"

AKPCLModularHologram::AKPCLModularHologram()
{
	mUseSimplifiedHologramMaterial = true;
	mNeedsValidFloor = false;
	mCanSnapWithAttachmentPoints = false;

	mMissingMasterModule = LoadClass<UFGConstructDisqualifier>(
		nullptr, TEXT("/KPrivateCodeLib/-Shared/Disqualifier/Disqualifier_MissingMaster.Disqualifier_MissingMaster_C"));
	mToMuchModules = LoadClass<UFGConstructDisqualifier>(
		nullptr, TEXT("/KPrivateCodeLib/-Shared/Disqualifier/Disqualifier_ToMuchModules.Disqualifier_ToMuchModules_C"));
	mModuleIsNotAllowed = LoadClass<UFGConstructDisqualifier>(
		nullptr, TEXT(
			"/KPrivateCodeLib/-Shared/Disqualifier/Disqualifier_ModuleNowAllowed.Disqualifier_ModuleNowAllowed_C"));
}

void AKPCLModularHologram::BeginPlay()
{
	Super::BeginPlay();

	auto Comps = GetComponentsByTag(UStaticMeshComponent::StaticClass(), FName("Top"));
	if (Comps.Num() > 1)
	{
		TopMesh = Cast<UStaticMeshComponent>(Comps[0]);
		TopMesh->SetHiddenInGame(true);
	}

	Comps = GetComponentsByTag(USkeletalMeshComponent::StaticClass(), FName("Top"));
	if (Comps.Num() > 1)
	{
		TopSkel = Cast<USkeletalMeshComponent>(Comps[0]);
		TopSkel->SetHiddenInGame(true);
	}
}

void AKPCLModularHologram::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKPCLModularHologram, mModuleMasterHit);
	DOREPLIFETIME(AKPCLModularHologram, mUpgradedActorRef);
}

bool AKPCLModularHologram::IsValidHitResult(const FHitResult& hitResult) const
{
	if (hitResult.IsValidBlockingHit() && hitResult.GetActor())
	{
		if (UKismetSystemLibrary::DoesImplementInterface(hitResult.GetActor(),
		                                                 UKPCLModularBuildingInterface::StaticClass()))
		{
			return true;
		}
	}
	return Super::IsValidHitResult(hitResult);
}

void AKPCLModularHologram::SetHologramLocationAndRotation(const FHitResult& hitResult)
{
	TSubclassOf<UFGConstructDisqualifier> Disqualifier = mMissingMasterModule;
	if (UKismetSystemLibrary::DoesImplementInterface(hitResult.GetActor(),
	                                                 UKPCLModularBuildingInterface::StaticClass()))
	{
		AFGBuildable* MasterTry = IKPCLModularBuildingInterface::Execute_GetMasterBuilding(hitResult.GetActor());
		mModuleMasterHit = MasterTry != nullptr ? MasterTry : Cast<AFGBuildable>(hitResult.GetActor());
		if (IKPCLModularBuildingInterface::Execute_GetCanHaveModules(mModuleMasterHit))
		{
			UKPCLModularBuildingHandlerBase* Handler = IKPCLModularBuildingInterface::Execute_GetModularHandler(
				mModuleMasterHit);
			if (Handler)
			{
				if (IsModuleAllowed(Handler, mModuleMasterHit, hitResult))
				{
					FTransform TestLocation = mModuleMasterHit->GetActorTransform();
					if (!mIsStacker)
					{
						TestLocation.SetLocation(hitResult.Location);
					}

					if (Handler->CanAttachToLocation(mAttachmentDescriptor, TestLocation, mNextSnapLocation,
					                                 mSnapDistance))
					{
						FRotator NewRotator = mNextSnapLocation.Rotator();
						NewRotator.Yaw += mRotation;
						SetActorLocationAndRotation(mNextSnapLocation.GetLocation(), NewRotator);

						// Play Snap Sound
						if (FVector::Distance(mNextSnapLocation.GetLocation(), mSnapLocation.GetLocation()) > 10)
						{
							OnSnap();
						}

						if (TopMesh)
						{
							if (TopMesh->bHiddenInGame)
							{
								TopMesh->SetHiddenInGame(!TopMesh->bHiddenInGame);
							}
						}
						if (TopSkel)
						{
							if (TopSkel->bHiddenInGame)
							{
								TopSkel->SetHiddenInGame(!TopSkel->bHiddenInGame);
							}
						}

						mSnapLocation = mNextSnapLocation;
						return;
					}
					Disqualifier = mToMuchModules;
				}
				else if (IsValid(mModuleIsNotAllowed))
				{
					Disqualifier = mModuleIsNotAllowed;
				}
			}
		}
	}

	AddConstructDisqualifier(Disqualifier);

	if (TopMesh)
	{
		if (!TopMesh->bHiddenInGame)
		{
			TopMesh->SetHiddenInGame(!TopMesh->bHiddenInGame);
		}
	}
	if (TopSkel)
	{
		if (!TopSkel->bHiddenInGame)
		{
			TopSkel->SetHiddenInGame(!TopSkel->bHiddenInGame);
		}
	}

	mSnapLocation.SetLocation(hitResult.Location);
	Super::SetHologramLocationAndRotation(hitResult);
}

void AKPCLModularHologram::ConfigureComponents(AFGBuildable* inBuildable) const
{
	Super::ConfigureComponents(inBuildable);

	if (mUpgradedActorRef)
	{
		IKPCLModularBuildingInterface::Execute_RemoveAttachedActor(mModuleMasterHit, mUpgradedActorRef);
	}
	IKPCLModularBuildingInterface::Execute_AttachedActor(mModuleMasterHit, inBuildable, mAttachmentDescriptor,
	                                                     mSnapLocation, mSnapDistance);
}

void AKPCLModularHologram::CheckValidPlacement()
{
	Super::CheckValidPlacement();

	if (!IsValid(mModuleMasterHit))
	{
		AddConstructDisqualifier(mMissingMasterModule);
	}
}

bool AKPCLModularHologram::IsModuleAllowed(UKPCLModularBuildingHandlerBase* Handler, AFGBuildable* TargetBuildable,
                                           const FHitResult& hitResult)
{
	if (TargetBuildable->Implements<UKPCLModularBuildingInterface>())
	{
		return IKPCLModularBuildingInterface::Execute_IsAllowedToSnap(TargetBuildable,
		                                                              GetDefaultBuildable<AFGBuildable>());
	}

	return true;
}

AActor* AKPCLModularHologram::GetUpgradedActor() const
{
	return mUpgradedActorRef;
}

bool AKPCLModularHologram::TryUpgrade(const FHitResult& hitResult)
{
	if (mPreventUpgrade)
	{
		mUpgradedActorRef = nullptr;
		return Super::TryUpgrade(hitResult);
	}

	if (hitResult.IsValidBlockingHit())
	{
		if (UKismetSystemLibrary::DoesImplementInterface(hitResult.GetActor(),
		                                                 UKPCLModularBuildingInterface::StaticClass()))
		{
			TSubclassOf<UKAPIModularAttachmentDescriptor> ATClass =
				IKPCLModularBuildingInterface::Execute_GetModularAttachmentClass(hitResult.GetActor());
			if (ATClass == mAttachmentDescriptor && hitResult.GetActor()->GetClass() != mBuildClass)
			{
				mModuleMasterHit = IKPCLModularBuildingInterface::Execute_GetMasterBuilding(hitResult.GetActor());
				mUpgradedActorRef = Cast<AFGBuildable>(hitResult.GetActor());
				SetActorLocationAndRotation(hitResult.GetActor()->GetActorLocation(),
				                            hitResult.GetActor()->GetActorRotation());
				return mUpgradedActorRef != nullptr;
			}
		}
	}

	mUpgradedActorRef = nullptr;
	return Super::TryUpgrade(hitResult);
}

void AKPCLModularHologram::Scroll(int32 Delta)
{
	if (mCanRotate)
	{
		mRotation += 45.0f * Delta;
		if (mRotation == 360.0f)
		{
			mRotation = 0;
		}
	}
	Super::Scroll(Delta);
}
