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

	/**
	 * Called by the lazy-load listener when a single class finishes loading.
	 * Dispatches to ShouldCallForInstance + ApplyToInstance.
	 */
	void TryApplyToClass(UClass* NewClass);

	/** Returns true if this overwrite should be applied to the given class. Override in subclasses. */
	virtual bool ShouldCallForInstance(UClass* NewClass) { return false; }

	/** Applies this overwrite to a single CDO instance. Override in subclasses. */
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

	// ===== Basic Settings =====
	/** Enable or disable this CDO overwrite */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic Settings")
	bool bEnabled = true;

	/**
	 * Execution priority - lower numbers are applied first.
	 * If multiple have the same priority, the order is undefined.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Basic Settings")
	int32 mCallPrio = 0;

	// ===== Requirements =====
	/** Requirements that must be met before applying this overwrite */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements")
	TArray<TSubclassOf<class UKBFLCDOCallRequirement>> mRequirements;

protected:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UKBFLCDOCallRequirement>> mCachedRequirements;

public:
	// ===== Internal State =====
	/**
	 * Will be set by the subsystem when registering
	 * Can be used to call other functions from the subsystem
	 */
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

	// ===== World Settings =====
	/** Enable tick functionality for this world-based overwrite */
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
