#pragma once

#include "CoreMinimal.h"
#include "Logging/StructuredLog.h"

#include "KBFLActorSpawnDescriptorBase.generated.h"

UCLASS()
class UKBFLSpawnRequirement : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	void OnInit();

	UFUNCTION(BlueprintNativeEvent)
	void OnClear();

	UFUNCTION(BlueprintNativeEvent)
	bool IsRequirementMet(class UKBFLActorSpawnDescriptorBase* Target);

	UFUNCTION(BlueprintNativeEvent)
	bool IsRequirementMetActor(class UKBFLActorSpawnDescriptorBase* Target, AActor* Actor);

	UFUNCTION(BlueprintNativeEvent)
	void OnActorSpawned(class UKBFLActorSpawnDescriptorBase* Target, AActor* Actor);

	UFUNCTION(BlueprintNativeEvent)
	void ConfigureActor(class UKBFLActorSpawnDescriptorBase* Target, AActor* Actor);

	virtual UWorld* GetWorld() const override { return mWorld; };

	UPROPERTY()
	TObjectPtr<UWorld> mWorld;
};

UCLASS(Blueprintable, EditInlineNew, abstract, DefaultToInstanced)
class KBFL_API UKBFLActorSpawnDescriptorBase : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	virtual void BeginSpawning();

	virtual void InitRequirements();
	virtual void ClearRequirements();
	virtual bool AreRequirementsMet();
	virtual bool AreRequirementsMetActor(AActor* Actor);
	virtual void ConfigureActor(AActor* Actor);
	virtual void OnActorSpawned(AActor* Actor);

	virtual void ForeachLocations(TArray<AActor*>& ActorArray);

	virtual bool CheckActorInRange(FTransform Transform, AActor*& OutActor);
	virtual void ModifyCheckActor(AActor*& InActor, FTransform FoundTransform);
	virtual bool IsRangeFree(FTransform Transform);
	virtual void RemoveWrongActors(TArray<AActor*>& ActorArray);
	virtual AActor* SpawnActorAtLocation(FTransform Transform, TSubclassOf<AActor> ClassToSpawn);
	virtual void ModifySpawnedActorPreSpawn(AActor*& InActor);
	virtual void ModifySpawnedActorPostSpawn(AActor*& InActor);
	virtual void AfterSpawning();

	virtual TArray<TEnumAsByte<EObjectTypeQuery>> GetSphereCheckChannels();
	virtual void SetSphereCheckChannels(TArray<TEnumAsByte<EObjectTypeQuery>> Channels);

	virtual void ApplyMaterialData(AActor* Actor, TMap<uint8, TObjectPtr<UMaterialInterface>> MaterialInfo);

	virtual TArray<TSubclassOf<AActor>> GetSearchingActorClasses();

	virtual bool IsAllowedToRemoveActor(AActor* InActor);
	bool CheckWorld() const;

	virtual void ModifyValues();

	virtual TSubclassOf<AActor> GetActorClass();
	virtual TSubclassOf<AActor> GetActorFreeClass();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Basic Settings")
	bool mDisabled = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Basic Settings")
	bool mAllowToMove = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Basic Settings")
	bool mRemoveOld = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic Settings")
	bool mNeedAuth = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Spawn Requirements")
	TArray<TSubclassOf<UKBFLSpawnRequirement>> mSpawnRequirements;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Collision & Overlap")
	float mCheckRange = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Collision & Overlap")
	TArray<TEnumAsByte<EObjectTypeQuery>> mObjectTypeQuery =
		TArray<TEnumAsByte<EObjectTypeQuery>>{ObjectTypeQuery1, ObjectTypeQuery2, ObjectTypeQuery5};

	UPROPERTY(meta = (NoAutoJson = true))
	bool mPreventSpawningByOverlapFreeActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Collision & Overlap",
			  meta = (editcondition = "mPreventSpawningByOverlapFreeActorClass"))
	TSubclassOf<AActor> mActorFreeClass = AActor::StaticClass();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "World Settings")
	FString mMapName = "Persistent_Level";

	UPROPERTY(Transient, BlueprintReadWrite)
	TObjectPtr<class UKBFLResourceNodeSubsystem> mSubsystem = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> mAllActors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UKBFLSpawnRequirement>> mRequirements;

	virtual UWorld* GetWorld() const override;
};
