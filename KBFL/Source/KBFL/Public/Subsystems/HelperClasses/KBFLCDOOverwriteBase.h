

#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/StructuredLog.h"
#include "Runtime/Engine/Classes/Engine/DataAsset.h"

#include "KBFLCDOOverwriteBase.generated.h"
DECLARE_LOG_CATEGORY_EXTERN(LogKBFLCDOOverwrite, Log, All);

inline DEFINE_LOG_CATEGORY(LogKBFLCDOOverwrite);

class UKBFLContentCDOHelperSubsystem;

UCLASS()
class KBFL_API UKBFLCDOOverwriteBase : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	virtual void Start();

	virtual void ApplyToInstances() {};

	void TryApplyToClass(UClass* NewClass);

	virtual bool ShouldCallForInstance(UClass* NewClass) { return false; }

	virtual void ApplyToInstance(UObject* Instance) {};

	virtual void Clear();

	template <typename T>
	static TSubclassOf<T> LoadSoftClass(TSoftClassPtr<T> SoftClassPtr);

	template <typename T>
	static TArray<TSubclassOf<T>> LoadSoftClassesArray(TArray<TSoftClassPtr<T>> SoftClassPtrs);

	virtual bool Requirements_IsMet(UObject* TargetInstance);

	virtual void Requirements_NotifyOnModify(UObject* TargetInstance);

	virtual void Requirements_NotifyOnModified(UObject* TargetInstance);

	virtual void Requirements_NotifyOnFinishedAll();

	virtual void LoadRequirements();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic Settings")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic Settings")
	int32 mCallPrio = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements")
	TArray<TSubclassOf<class UKBFLCDOCallRequirement>> mRequirements;

protected:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UKBFLCDOCallRequirement>> mCachedRequirements;

public:

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UKBFLContentCDOHelperSubsystem> mSubsystem = nullptr;

	bool bWasApplied = false;

	UPROPERTY(Transient)
	TSet<TObjectPtr<UObject>> mAppliedInstances;
};

template <typename T>
TSubclassOf<T> UKBFLCDOOverwriteBase::LoadSoftClass(TSoftClassPtr<T> SoftClassPtr)
{
	if (!SoftClassPtr.IsValid())
	{
		if (UClass* LoadedClass = SoftClassPtr.LoadSynchronous())
		{
			return LoadedClass;
		}
	}
	return SoftClassPtr.Get();
}

template <typename T>
TArray<TSubclassOf<T>> UKBFLCDOOverwriteBase::LoadSoftClassesArray(TArray<TSoftClassPtr<T>> SoftClassPtrs)
{
	TArray<TSubclassOf<T>> Result;
	for (TSoftClassPtr<T> SoftClassPtr : SoftClassPtrs)
	{
		if (TSubclassOf<T> LoadedClass = LoadSoftClass<T>(SoftClassPtr))
		{
			Result.Add(LoadedClass);
		}
	}
	return Result;
}

UCLASS(NotBlueprintable, NotBlueprintType, Abstract)
class KBFL_API UKBFLCDOOverwriteWorldBasedBase : public UKBFLCDOOverwriteBase
{
	GENERATED_BODY()

public:

	template <typename T>
	TArray<T*> GetAllActorsInWorldOfClass(TSubclassOf<T> ActorClass);

	virtual void Start() override;

	virtual void Clear() override;

	virtual void LoadRequirements() override;

	UFUNCTION(BlueprintCallable)
	virtual void ApplyToActorsInWorld() {};

	virtual class UWorld* GetWorld() const override;

	void SetWorld(UWorld* World) { mWorld = World; }

	virtual bool ShouldTick() const { return bTickable; }

	virtual void Tick(float dt);

	UPROPERTY(EditDefaultsOnly, Category = "World Settings")
	bool bTickable = false;

protected:
	UPROPERTY()
	TObjectPtr<UWorld> mWorld;

	UPROPERTY(Transient)
	TSet<TObjectPtr<class UKBFLWorldCDOCallRequirement>> mTickableRequirements;
};

template <typename T>
TArray<T*> UKBFLCDOOverwriteWorldBasedBase::GetAllActorsInWorldOfClass(TSubclassOf<T> ActorClass)
{
	TArray<T*> Result;
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ActorClass, FoundActors);

	for (AActor* Actor : FoundActors)
	{
		if (T* TypedActor = Cast<T>(Actor))
		{
			Result.Add(TypedActor);
		}
	}

	return Result;
}
