// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FGSaveInterface.h"
#include "KPCLModularBuildingInterface.h"
#include "Buildables/FGBuildable.h"
#include "Components/ActorComponent.h"

#include "KPCLModularBuildingHandlerBase.generated.h"

DECLARE_DELEGATE_OneParam(FTryConnectTo, AFGBuildable*);

USTRUCT(BlueprintType)
struct FAttachmentPointLocation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FTransform mLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 mIndex;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 mMaxStackCount;
};

USTRUCT(BlueprintType)
struct FAttachmentLocations
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="!mDisplayinEditor"))
	TArray<FAttachmentPointLocation> mLocations;

	void GetTransformSortedByIndex(TArray<FTransform>& Transforms, bool rev = false)
	{
		Transforms.Empty();
		mLocations.Sort([rev](const FAttachmentPointLocation& A, const FAttachmentPointLocation& B)
		{
			if (rev)
			{
				return A.mIndex > B.mIndex;
			}
			return A.mIndex < B.mIndex;
		});

		for (FAttachmentPointLocation Location : mLocations)
		{
			Transforms.Add(Location.mLocation);
		}
	};
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHandlerTriggerUpdate);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class KPRIVATECODELIB_API UKPCLModularBuildingHandlerBase : public UActorComponent, public IFGSaveInterface
{
	GENERATED_BODY()

	// Begin IFGSaveInterface
	FORCEINLINE virtual bool ShouldSave_Implementation() const override { return true; }
	// End IFGSaveInterface

public:
	bool HasAuthority() const;

	// Sets default values for this component's properties
	UKPCLModularBuildingHandlerBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;

	virtual void InitArrays()
	{
	}

	virtual int FindAttachmentIndex(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment) const;

	virtual bool AddNewActorToAttachment(AFGBuildable* Actor, TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment,
	                                     FTransform Location, float Distance = 500.0f) { return false; }

	virtual void AttachedActorRemoved(AFGBuildable* Actor);
	virtual void TryToConnectPower(AFGBuildable* OtherActor);

	bool GetLocationMap(TMap<TSubclassOf<UKAPIModularAttachmentDescriptor>, FAttachmentLocations>& OutMap);

	UFUNCTION(BlueprintPure, BlueprintCallable)
	void GetAttachedActorsOfType(TArray<AFGBuildable*>& Out, uint8 Type);

	UFUNCTION(BlueprintPure, BlueprintCallable)
	virtual bool CanAttach(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment) const;

	UFUNCTION(BlueprintPure, BlueprintCallable)
	virtual bool CanAttachToLocation(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment, FTransform TestLocation,
	                                 FTransform& OutLocation, float Distance = 500.0f) const { return false; };

	UFUNCTION(BlueprintPure, BlueprintCallable)
	virtual bool GetSnapPointInRange(FTransform TestLocation, FTransform& SnapLocation, float AllowedDistance,
	                                 TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment) { return false; };

	UFUNCTION(BlueprintPure, BlueprintCallable)
	virtual AFGBuildable* GetAttachedActorByClass(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment);

	/**
	* Internal version for GetAttachedActorByClass
	*/
	template <class T>
	T* GetAttachedActor_Internal(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment);


	UPROPERTY(BlueprintAssignable)
	FOnHandlerTriggerUpdate OnHandlerTriggerUpdate;

	FORCEINLINE void BroadcastTrigger()
	{
		if (OnHandlerTriggerUpdate.IsBound())
		{
			OnHandlerTriggerUpdate.Broadcast();
		}
	};

	FORCEINLINE void NotifyBuildingWasUpdated()
	{
		if (GetOwner())
		{
			IKPCLModularBuildingInterface::Execute_OnModulesUpdated(GetOwner());
		}
	};

	UFUNCTION(BlueprintPure, BlueprintCallable)
	virtual TArray<AFGBuildable*> GetAttachedActorsByClass(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment);

	/**
	* Internal version for GetAttachedActorsByClass
	*/
	template <class T>
	void GetAttachedActors_Internal(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment, TArray<T*>& OutActors);

	template <class T>
	void GetAllAttachedActors_Internal(TArray<T*>& OutActors);

	UFUNCTION(BlueprintCallable)
	virtual void GetAttachedActorsByIndex(TArray<AFGBuildable*>& Out, uint8 index);

	/**
	* Internal version for GetAttachedActorsByClass
	*/
	template <class T>
	void GetAttachedActorByIndex(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment, TArray<T*>& OutActors);

	UFUNCTION(BlueprintCallable)
	virtual void GetAttachedActors(TArray<AFGBuildable*>& Out);

	FTryConnectTo OverwriteTryToConnectPower;
};

template <class T>
T* UKPCLModularBuildingHandlerBase::GetAttachedActor_Internal(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment)
{
	return Cast<T>(GetAttachedActorByClass(Attachment));
}

template <class T>
void UKPCLModularBuildingHandlerBase::GetAttachedActors_Internal(
	TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment, TArray<T*>& OutActors)
{
	TArray<AFGBuildable*> Actors = GetAttachedActorsByClass(Attachment);
	for (AFGBuildable* Actor : Actors)
	{
		if (T* Casted = Cast<T>(Actor))
		{
			OutActors.Add(Casted);
		}
	}
}

template <class T>
void UKPCLModularBuildingHandlerBase::GetAllAttachedActors_Internal(TArray<T*>& OutActors)
{
	TArray<AFGBuildable*> Actors;
	GetAttachedActors(Actors);
	for (AFGBuildable* Actor : Actors)
	{
		if (T* Building = Cast<T>(Actor))
		{
			OutActors.Add(Building);
		}
	}
}

template <class T>
void UKPCLModularBuildingHandlerBase::GetAttachedActorByIndex(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment,
                                                              TArray<T*>& OutActors)
{
	TArray<AFGBuildable*> Actors = GetAttachedActorsByClass(Attachment);
	for (AFGBuildable* Actor : Actors)
	{
		if (T* Casted = Cast<T>(Actor))
		{
			OutActors.Add(Casted);
		}
	}
}
