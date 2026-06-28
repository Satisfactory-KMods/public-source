// Fill out your copyright notice in the Description page of Project Settings.

#include "Buildable/Modular/KPCLModularBuildingHandlerStacker.h"

#include "Buildable/Modular/KPCLModularBuildingInterface.h"
#include "FGPowerConnectionComponent.h"
#include "Kismet/KismetMathLibrary.h"

#include "Net/UnrealNetwork.h"

// Start FAttachmentData

int32 FAttachmentDataStacker::GetIndexFromActor(AFGBuildable* Actor) const
{
	for (int i = 0; i < mSnappedActors.Num(); ++i)
	{
		if (mSnappedActors[i] == Actor)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

bool FAttachmentDataStacker::CanSnapTo(int32 MaxModuleCount) const
{
	// UE_LOG(LogTemp, Warning, TEXT("%s, %d/%d"), mSnappedActors.Num() < MaxModuleCount ? *FString("Y") :
	// *FString("N"), mSnappedActors.Num(), MaxModuleCount);
	return mSnappedActors.Num() < MaxModuleCount;
}

FTransform FAttachmentDataStacker::GetSnapLocation() const
{
	FTransform Transform = mMainSnapLocations;
	FVector Location = Transform.GetLocation();
	Location.Z += GetAllHeights();
	Transform.SetLocation(Location);

	return Transform;
}

float FAttachmentDataStacker::GetAllHeights() const
{
	float Height = 0;
	if (mSnappedActors.Num() > 0)
	{
		for (AFGBuildable* SnappedActor : mSnappedActors)
		{
			if (SnappedActor)
			{
				IKPCLModularBuildingInterface::Execute_Stacker_AddBuildingHeight(SnappedActor, Height);
			}
		}
	}
	return Height;
}

AFGBuildable* FAttachmentDataStacker::GetActorFromIndex(int32 Index) const
{
	if (mSnappedActors.IsValidIndex(Index))
	{
		return mSnappedActors[Index];
	}
	return nullptr;
}

void FAttachmentDataStacker::RemoveActorFromData(AFGBuildable* Actor)
{
	mSnappedActors.Remove(Actor);
	mSnappedActors.Remove(nullptr);

	// Re-apply modular indices so saved mModularIndex values stay in sync after the array shifts.
	for (int32 i = 0; i < mSnappedActors.Num(); ++i)
	{
		if (IsValid(mSnappedActors[i]))
		{
			IKPCLModularBuildingInterface::Execute_ApplyModularIndex(mSnappedActors[i], i);
		}
	}
}

bool FAttachmentDataStacker::AddActorToData(AFGBuildable* Actor)
{
	int32 Index = mSnappedActors.AddUnique(Actor);
	IKPCLModularBuildingInterface::Execute_ApplyModularIndex(Actor, Index);
	return Index != -1;
}

TArray<AFGBuildable*> FAttachmentDataStacker::GetActorsUpperIndex(int32 Index)
{
	TArray<AFGBuildable*> Outer;
	if (mSnappedActors.IsValidIndex(Index + 1))
	{
		for (int i = Index + 1; i < mSnappedActors.Num(); ++i)
		{
			Outer.Add(mSnappedActors[i]);
		}
	}
	return Outer;
}

//~ End FAttachmentData

// Start UKPCLModularBuildingHandlerStacker
UKPCLModularBuildingHandlerStacker::UKPCLModularBuildingHandlerStacker()
{
	PrimaryComponentTick.bCanEverTick = false;
	this->SetIsReplicatedByDefault(true);
	this->bAutoActivate = true;
}

void UKPCLModularBuildingHandlerStacker::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKPCLModularBuildingHandlerStacker, mAttachmentDatas);
}

void UKPCLModularBuildingHandlerStacker::InitArrays()
{
	TMap<TSubclassOf<UKAPIModularAttachmentDescriptor>, FAttachmentLocations> LocMap;
	if (GetLocationMap(LocMap))
	{
		for (TTuple<TSubclassOf<UKAPIModularAttachmentDescriptor>, FAttachmentLocations> Map : LocMap)
		{
			int32 Index = FindAttachmentIndex(Map.Key);
			if (Index == INDEX_NONE)
			{
				int32 NewIndex = 0;
				for (FAttachmentInfosStacker& Information : mAttachmentInformations)
				{
					if (!Information.mAttachmentClass)
					{
						Information.mAttachmentClass = Map.Key;
						Index = NewIndex;
						break;
					}
					NewIndex++;
				}
			}

			if (Index != INDEX_NONE)
			{
				TArray<FTransform> Transforms;
				Map.Value.GetTransformSortedByIndex(Transforms);
				if (Transforms.IsValidIndex(0))
				{
					mAttachmentInformations[Index].mWorldMainSnapPoint = Transforms[0];
					mAttachmentInformations[Index].mMaxStackingModuleCount = Map.Value.mLocations[0].mMaxStackCount;
				}
			}
		}
	}

	if (mAttachmentDatas.Num() != mAttachmentInformations.Num())
	{
		mAttachmentDatas.SetNum(mAttachmentInformations.Num());
	}

	for (int i = 0; i < mAttachmentInformations.Num(); ++i)
	{
		mAttachmentDatas[i].mMainSnapLocations = mAttachmentInformations[i].mWorldMainSnapPoint;
	}

	for (FAttachmentDataStacker& Data : mAttachmentDatas)
	{
		Data.mSnappedActors.Remove(nullptr);
		if (Data.mSnappedActors.Num() > 0)
		{
			for (AFGBuildable* Actor : Data.mSnappedActors)
			{
				IKPCLModularBuildingInterface::Execute_SetMasterBuilding(Actor, Cast<AFGBuildable>(GetOwner()));
			}
		}
	}
}

int UKPCLModularBuildingHandlerStacker::FindAttachmentIndex(
	TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment) const
{
	return mAttachmentInformations.IndexOfByKey(Attachment);
}

void UKPCLModularBuildingHandlerStacker::OnRep_AttachmentDatas()
{
	IKPCLModularBuildingInterface::Execute_OnModulesUpdated(GetOwner());

	BroadcastTrigger();
}

bool UKPCLModularBuildingHandlerStacker::AddNewActorToAttachment(
	AFGBuildable* Actor, TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment, FTransform Location, float Distance)
{
	if (AActor* Owner = GetOwner(); Owner && !Owner->HasAuthority())
	{
		return false;
	}

	const int AttachmentIndex = FindAttachmentIndex(Attachment);

	if (AttachmentIndex >= 0)
	{
		if (mAttachmentDatas[AttachmentIndex].AddActorToData(Actor))
		{
			IKPCLModularBuildingInterface::Execute_SetMasterBuilding(Actor, Cast<AFGBuildable>(GetOwner()));
			TryToConnectPower(Actor);
			OnRep_AttachmentDatas();
			return true;
		}
	}

	BroadcastTrigger();
	return false;
}

void UKPCLModularBuildingHandlerStacker::AttachedActorRemoved(AFGBuildable* Actor)
{
	if (AActor* Owner = GetOwner(); Owner && !Owner->HasAuthority())
	{
		return;
	}

	for (FAttachmentDataStacker& AttachmentData : mAttachmentDatas)
	{
		AttachmentData.RemoveActorFromData(Actor);
	}
	Super::AttachedActorRemoved(Actor);
	OnRep_AttachmentDatas();
}

bool UKPCLModularBuildingHandlerStacker::CanAttachToLocation(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment,
															 FTransform TestLocation, FTransform& OutLocation,
															 float Distance) const
{
	if (CanAttach(Attachment))
	{
		const int AttachmentIndex = FindAttachmentIndex(Attachment);
		if (AttachmentIndex >= 0)
		{
			if (mAttachmentDatas[AttachmentIndex].CanSnapTo(
					mAttachmentInformations[AttachmentIndex].mMaxStackingModuleCount))
			{
				OutLocation = mAttachmentDatas[AttachmentIndex].GetSnapLocation();
				return true;
			}
		}
	}
	return false;
}

bool UKPCLModularBuildingHandlerStacker::GetSnapPointInRange(FTransform TestLocation, FTransform& SnapLocation,
															 float AllowedDistance,
															 TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment)
{
	if (CanAttach(Attachment))
	{
		const int AttachmentIndex = FindAttachmentIndex(Attachment);
		if (AttachmentIndex >= 0)
		{
			if (mAttachmentDatas[AttachmentIndex].CanSnapTo(
					mAttachmentInformations[AttachmentIndex].mMaxStackingModuleCount))
			{
				SnapLocation = mAttachmentDatas[AttachmentIndex].GetSnapLocation();
				return true;
			}
		}
	}
	return false;
}

AFGBuildable*
UKPCLModularBuildingHandlerStacker::GetAttachedActorByClass(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment)
{
	const int AttachmentIndex = FindAttachmentIndex(Attachment);
	if (AttachmentIndex >= 0)
	{
		if (mAttachmentDatas[AttachmentIndex].mSnappedActors.IsValidIndex(0))
		{
			return mAttachmentDatas[AttachmentIndex].mSnappedActors[0];
		}
	}
	return nullptr;
}

TArray<AFGBuildable*>
UKPCLModularBuildingHandlerStacker::GetAttachedActorsByClass(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment)
{
	const int AttachmentIndex = FindAttachmentIndex(Attachment);
	if (AttachmentIndex >= 0)
	{
		return mAttachmentDatas[AttachmentIndex].mSnappedActors;
	}
	return {};
}

void UKPCLModularBuildingHandlerStacker::GetAttachedActors(TArray<AFGBuildable*>& Out)
{
	for (const FAttachmentDataStacker& Data : mAttachmentDatas)
	{
		for (const TObjectPtr<AFGBuildable>& Actor : Data.mSnappedActors)
		{
			if (IsValid(Actor))
			{
				Out.Add(Actor);
			}
		}
	}
}

//~ End UKPCLModularBuildingHandlerStacker
