//

#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/StructuredLog.h"
#include "Runtime/Engine/Classes/Engine/DataAsset.h"

#include "KBFLCDOOverwriteBase.generated.h"
DECLARE_LOG_CATEGORY_EXTERN(LogKBFLCDOOverwrite, Log, All);

inline DEFINE_LOG_CATEGORY(LogKBFLCDOOverwrite);

// Forward declaration to avoid circular dependency
class UKBFLContentCDOHelperSubsystem;

/**
 *
 */
UCLASS()
class KBFL_API UKBFLCDOOverwriteBase : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual void Start();

	virtual void ApplyToInstances() {};

	virtual void Clear();

	template <typename T>
	static TSubclassOf<T> LoadSoftClass(TSoftClassPtr<T> SoftClassPtr);

	template <typename T>
	static TArray<TSubclassOf<T>> LoadSoftClassesArray(TArray<TSoftClassPtr<T>> SoftClassPtrs);

	template <typename T>
	static TArray<T*> LoadSoftObjectCDOsArray(TArray<TSoftClassPtr<T>> SoftObjectPtrs);

	template <typename T>
	static T* LoadSoftObjectCDO(TSoftClassPtr<T> SoftObjectPtr);

	virtual bool Requirements_IsMet(UObject* TargetInstance);
	virtual void Requirements_NotifyOnModify(UObject* TargetInstance);
	virtual void Requirements_NotifyOnModified(UObject* TargetInstance);
	virtual void Requirements_NotifyOnFinishedAll();

	virtual void LoadRequirements();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO")
	TArray<TSubclassOf<class UKBFLCDOCallRequirement>> mRequirements;

protected:
	UPROPERTY(Transient)
	TArray<UKBFLCDOCallRequirement*> mCachedRequirements;

public:
	/**
	 * Will be set by the subsystem when registering
	 * Can be used to call other functions from the subsystem
	 */
	UPROPERTY(BlueprintReadWrite)
	class UKBFLContentCDOHelperSubsystem* mSubsystem = nullptr;

	bool bWasApplied = false;

	/** The target class whose properties we want to override */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO")
	bool bEnabled = true;

	/**
	 * First are all applied in order of ascending priority (lower numbers first).
	 * If multiple have the same priority, the order is undefined.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CDO")
	int32 mCallPrio = 0;

	UPROPERTY(Transient)
	TSet<UObject*> mAppliedInstances;
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

template <typename T>
TArray<T*> UKBFLCDOOverwriteBase::LoadSoftObjectCDOsArray(TArray<TSoftClassPtr<T>> SoftObjectPtrs)
{
	TArray<T*> Result;
	for (TSoftClassPtr<T> SoftObjectPtr : SoftObjectPtrs)
	{
		if (T* LoadedCDO = LoadSoftObjectCDO<T>(SoftObjectPtr))
		{
			Result.Add(LoadedCDO);
		}
	}
	return Result;
}

template <typename T>
T* UKBFLCDOOverwriteBase::LoadSoftObjectCDO(TSoftClassPtr<T> SoftObjectPtr)
{
	if (TSubclassOf<T> LoadedClass = LoadSoftClass<T>(SoftObjectPtr))
	{
		return Cast<T>(LoadedClass->GetDefaultObject());
	}
	return nullptr;
}

/**
 * A WIP base class for CDO overwrites that need a UWorld context
 * Like overwriting actors in the world too
 * @Note This class can be called once per world, so make sure to handle that in derived classes
 */
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

	/** Set the world for this world-based overwrite */
	void SetWorld(UWorld* World) { mWorld = World; }

	virtual bool ShouldTick() const { return bTickable; }
	virtual void Tick(float dt);

protected:
	UPROPERTY()
	UWorld* mWorld;

	UPROPERTY(Transient)
	TSet<class UKBFLWorldCDOCallRequirement*> mTickableRequirements;

	UPROPERTY(EditDefaultsOnly, Category = "CDO|World Based")
	bool bTickable = false;
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
