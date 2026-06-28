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

	/**
	 * If the requirements is not met the actor will not be spawned and destroyed again
	 */
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

/**
 *
 */
UCLASS(Blueprintable, EditInlineNew, abstract, DefaultToInstanced)
class KBFL_API UKBFLActorSpawnDescriptorBase : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual void BeginSpawning();

	// Begin Requirements Handling
	virtual void InitRequirements();
	virtual void ClearRequirements();
	virtual bool AreRequirementsMet();
	virtual bool AreRequirementsMetActor(AActor* Actor);
	virtual void ConfigureActor(AActor* Actor);
	virtual void OnActorSpawned(AActor* Actor);
	// Ends Requirements Handling


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

	// ===== Basic Settings =====
	/** If true, this descriptor is disabled and won't spawn actors */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Basic Settings")
	bool mDisabled = false;

	/** If true, existing actors can be moved to new locations */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Basic Settings")
	bool mAllowToMove = true;

	/** If true, remove old/wrong actors before spawning new ones */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Basic Settings")
	bool mRemoveOld = true;

	/** If true, requires authority (server-side only spawning) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic Settings")
	bool mNeedAuth = false;

	// ===== Spawn Requirements =====
	/** Custom requirements that must be met for spawning */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Spawn Requirements")
	TArray<TSubclassOf<UKBFLSpawnRequirement>> mSpawnRequirements;

	// ===== Collision & Overlap =====
	/** Check range for detecting existing actors */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Collision & Overlap")
	float mCheckRange = 500.0f;

	/** Object types to check for overlap detection */
	UPROPERTY(EditDefaultsOnly, Category = "Collision & Overlap")
	TArray<TEnumAsByte<EObjectTypeQuery>> mObjectTypeQuery =
		TArray<TEnumAsByte<EObjectTypeQuery>>{ObjectTypeQuery1, ObjectTypeQuery2, ObjectTypeQuery5};

	/** Prevent spawning if this actor class overlaps the spawn location */
	UPROPERTY(meta = (NoAutoJson = true))
	bool mPreventSpawningByOverlapFreeActorClass;

	/** Actor class to check for overlap prevention */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Collision & Overlap",
			  meta = (editcondition = "mPreventSpawningByOverlapFreeActorClass"))
	TSubclassOf<AActor> mActorFreeClass = AActor::StaticClass();

	// ===== World Settings =====
	/** Map name to spawn actors in (e.g., "Persistent_Level") */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "World Settings")
	FString mMapName = "Persistent_Level";

	// ===== Internal State =====
	UPROPERTY(Transient, BlueprintReadWrite)
	TObjectPtr<class UKBFLResourceNodeSubsystem> mSubsystem = nullptr;


	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> mAllActors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UKBFLSpawnRequirement>> mRequirements;

	virtual UWorld* GetWorld() const override;
};
