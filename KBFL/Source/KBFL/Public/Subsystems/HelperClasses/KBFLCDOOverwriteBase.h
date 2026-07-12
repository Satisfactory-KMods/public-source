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
	/**
	 * Entry point invoked by the subsystem. Skips when disabled, loads requirements, runs ApplyToInstances,
	 * retains every modified instance via the subsystem (to prevent GC), then fires the finished-all notifications.
	 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	virtual void Start();

	/** Override in subclasses to perform the actual CDO modifications. Base implementation does nothing. */
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

	/** Resets transient state: clears applied instances, cached requirements, and the subsystem back-reference. */
	virtual void Clear();

	/** Synchronously resolves a soft class pointer, loading it if necessary, and returns the loaded class. */
	template <typename T>
	static TSubclassOf<T> LoadSoftClass(TSoftClassPtr<T> SoftClassPtr);

	/** Resolves an array of soft class pointers, returning only the entries that successfully load. */
	template <typename T>
	static TArray<TSubclassOf<T>> LoadSoftClassesArray(TArray<TSoftClassPtr<T>> SoftClassPtrs);

	/** Returns true only if every cached requirement reports met for the given target instance. */
	virtual bool Requirements_IsMet(UObject* TargetInstance);
	/** Notifies all cached requirements that the target is about to be modified. */
	virtual void Requirements_NotifyOnModify(UObject* TargetInstance);
	/** Notifies all cached requirements that the target was just modified. */
	virtual void Requirements_NotifyOnModified(UObject* TargetInstance);
	/** Notifies all cached requirements that the full apply pass finished and dispatches any deferred calls. */
	virtual void Requirements_NotifyOnFinishedAll();

	/** Instantiates requirement objects from the mRequirements classes and caches them for this run. */
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
	/** Returns all actors of the given class found in this overwrite's world. */
	template <typename T>
	TArray<T*> GetAllActorsInWorldOfClass(TSubclassOf<T> ActorClass);

	/** Validates that the overwrite is enabled and has a valid world, runs base Start once, then applies to actors. */
	virtual void Start() override;
	/** Clears base state plus the world reference, the applied flag, and the tickable requirement cache. */
	virtual void Clear() override;
	/** Like the base, but also injects the world into UKBFLWorldCDOCallRequirement instances and tracks tickable ones.
	 */
	virtual void LoadRequirements() override;

	/** Override to modify actors in the world. Base implementation does nothing. */
	UFUNCTION(BlueprintCallable)
	virtual void ApplyToActorsInWorld() {};

	/** Returns the world previously assigned via SetWorld. */
	virtual class UWorld* GetWorld() const override;

	/** Set the world for this world-based overwrite */
	void SetWorld(UWorld* World) { mWorld = World; }

	/** Returns whether this overwrite wants to receive Tick (driven by bTickable). */
	virtual bool ShouldTick() const { return bTickable; }
	/** Forwards the tick to all cached tickable world requirements. */
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
