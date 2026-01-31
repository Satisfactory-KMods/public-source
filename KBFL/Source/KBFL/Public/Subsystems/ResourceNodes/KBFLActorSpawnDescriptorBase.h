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
	void ConfigureActor(class UKBFLActorSpawnDescriptorBase* Target, AActor* Actor);

	UFUNCTION(BlueprintNativeEvent)
	void OnActorSpawned(class UKBFLActorSpawnDescriptorBase* Target, AActor* Actor);

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

	virtual void ApplyMaterialData(AActor* Actor, TMap<uint8, UMaterialInterface*> MaterialInfo);

	virtual TArray<TSubclassOf<AActor>> GetSearchingActorClasses();

	virtual bool IsAllowedToRemoveActor(AActor* InActor);
	bool CheckWorld() const;

	virtual void ModifyValues();

	virtual TSubclassOf<AActor> GetActorClass();
	virtual TSubclassOf<AActor> GetActorFreeClass();

	// Bool
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool mDisabled = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool mAllowToMove = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool mRemoveOld = true;

	UPROPERTY(meta = (NoAutoJson = true))
	bool mPreventSpawningByOverlapFreeActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config",
			  meta = (editcondition = "mPreventSpawningByOverlapFreeActorClass"))
	TSubclassOf<AActor> mActorFreeClass = AActor::StaticClass();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	TArray<TSubclassOf<UKBFLSpawnRequirement>> mSpawnRequirements;

	// Floats
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float mCheckRange = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FString mMapName = "Persistent_Level";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	bool mNeedAuth = false;

	UPROPERTY(Transient, BlueprintReadWrite)
	class UKBFLResourceNodeSubsystem* mSubsystem = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config|SphereCheckChannels")
	TArray<TEnumAsByte<EObjectTypeQuery>> mObjectTypeQuery =
		TArray<TEnumAsByte<EObjectTypeQuery>>{ObjectTypeQuery1, ObjectTypeQuery2, ObjectTypeQuery5};

	UPROPERTY(Transient)
	TArray<AActor*> mAllActors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UKBFLSpawnRequirement>> mRequirements;
};
