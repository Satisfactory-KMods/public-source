// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "Buildables/FGBuildable.h"
#include "Descriptors/KAPIModularAttachmentDescriptor.h"
#include "KPCLModularBuildingHandlerBase.h"

#include "KPCLModularBuildingHandler.generated.h"

USTRUCT(BlueprintType)
struct FAttachmentInfos
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UKAPIModularAttachmentDescriptor> mAttachmentClass;

	TArray<FTransform> mSnapWorldLocations;

	bool operator==(TSubclassOf<UKAPIModularAttachmentDescriptor> other) const { return mAttachmentClass == other; };
};

USTRUCT(BlueprintType)
struct FAttachmentPointData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FTransform mLocations = {};

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TObjectPtr<AFGBuildable> mSnappedActors = nullptr;

	void Clear() { mSnappedActors = nullptr; }

	bool IsAttached() const { return mSnappedActors != nullptr; }

	bool operator==(const AFGBuildable* other) const;
};

USTRUCT(BlueprintType)
struct FAttachmentData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<FAttachmentPointData> mAttachmentPointDatas;

	AFGBuildable* GetActorFromIndex(int Index) const;
	AFGBuildable* GetActorFromLocation(FTransform TestLocation, FTransform& OutLocation, float MaxDistance) const;
	bool IsLocationFree(FTransform TestLocation, FTransform& OutLocation, float MaxDistance) const;
	void RemoveActorFromData(AFGBuildable* Actor);
	int32 AddActorToData(AFGBuildable* Actor, FTransform Location, float Distance = 50.f);
	void Init(TArray<FTransform>& Transforms);
	bool HasSpace() const;
	bool HasInRange(FTransform TestLocation, float MaxDistance) const;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KPRIVATECODELIB_API UKPCLModularBuildingHandler : public UKPCLModularBuildingHandlerBase
{
	GENERATED_BODY()

public:
	UKPCLModularBuildingHandler();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitArrays() override;

	virtual bool AddNewActorToAttachment(AFGBuildable* Actor, TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment,
										 FTransform Location, float Distance = 500.0f) override;
	virtual void AttachedActorRemoved(AFGBuildable* Actor) override;
	virtual bool CanAttachToLocation(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment, FTransform TestLocation,
									 FTransform& OutLocation, float Distance = 500.0f) const override;
	virtual bool GetSnapPointInRange(FTransform TestLocation, FTransform& SnapLocation, float AllowedDistance,
									 TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment) override;
	virtual AFGBuildable* GetAttachedActorByClass(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment) override;
	virtual TArray<AFGBuildable*>
	GetAttachedActorsByClass(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment) override;
	virtual void GetAttachedActors(TArray<AFGBuildable*>& Out) override;
	virtual int FindAttachmentIndex(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment) const override;

	template <class T>
	T* GetClosedActorFromLocation(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment, FTransform Location);

	template <class T>
	T* GetActorFromModularIndex(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment, int32 Index);

	UFUNCTION()
	void OnRep_AttachmentDatas();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modular Handler", meta = (EditFixedSize))
	TArray<FAttachmentInfos> mAttachmentInformations = {
		FAttachmentInfos(), FAttachmentInfos(), FAttachmentInfos(), FAttachmentInfos(), FAttachmentInfos(),
		FAttachmentInfos(), FAttachmentInfos(), FAttachmentInfos(), FAttachmentInfos(), FAttachmentInfos()};

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Modular Handler", Replicated,
			  ReplicatedUsing = "OnRep_AttachmentDatas", meta = (EditFixedSize))
	TArray<FAttachmentData> mAttachmentDatas = {
		FAttachmentData(), FAttachmentData(), FAttachmentData(), FAttachmentData(), FAttachmentData(),
		FAttachmentData(), FAttachmentData(), FAttachmentData(), FAttachmentData(), FAttachmentData()};
};

template <class T>
T* UKPCLModularBuildingHandler::GetClosedActorFromLocation(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment,
														   FTransform Location)
{
	const int AttachmentIndex = FindAttachmentIndex(Attachment);
	if (AttachmentIndex >= 0)
	{
		FTransform PH;
		return Cast<T>(mAttachmentDatas[AttachmentIndex].GetActorFromLocation(Location, PH, 1000000.f));
	}
	return nullptr;
}

template <class T>
T* UKPCLModularBuildingHandler::GetActorFromModularIndex(TSubclassOf<UKAPIModularAttachmentDescriptor> Attachment,
														 int32 Index)
{
	const int AttachmentIndex = FindAttachmentIndex(Attachment);
	if (AttachmentIndex >= 0)
	{
		if (mAttachmentDatas[AttachmentIndex].mAttachmentPointDatas.IsValidIndex(Index))
		{
			return Cast<T>(mAttachmentDatas[AttachmentIndex].mAttachmentPointDatas[Index].mSnappedActors);
		}
	}
	return nullptr;
}
