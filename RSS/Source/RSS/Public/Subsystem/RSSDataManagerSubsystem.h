// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "BFL/KBFL_Util.h"
#include "Configuration/ModConfiguration.h"
#include "Cpp/KBFLSortedActorDistanceArray.h"
#include "Subsystem/ModSubsystem.h"

#include "EnumStruc/RssStruc.h"
#include "RSSModule.h"
#include "Structures/KPCLFunctionalStructure.h"

#include "RSSDataManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKeyPressed, FKey, Key);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKeyReleased, FKey, Key);

class URssWidgetRenderComponent;

/**
 * Thread-safe component pool bucket for a specific URssWidgetRenderComponent subclass.
 * Each bucket manages its own pool of components with acquire/return operations.
 */
struct FRssComponentPoolBucket
{
	TSubclassOf<URssWidgetRenderComponent> mComponentClass;
	TQueue<TWeakObjectPtr<URssWidgetRenderComponent>> mAvailableQueue;
	TSet<TWeakObjectPtr<URssWidgetRenderComponent>> mActiveSet;
	mutable FCriticalSection mBucketLock;

	int32 mTotalCreated = 0;
	int32 mPoolHits = 0;
	int32 mPoolMisses = 0;

	FRssComponentPoolBucket() = default;

	explicit FRssComponentPoolBucket(TSubclassOf<URssWidgetRenderComponent> InClass) : mComponentClass(InClass) {}

	void InitializePool(AActor* SubsystemOwner, int32 MinPoolSize,
						TArray<TObjectPtr<URssWidgetRenderComponent>>& OutAllComponents);

	URssWidgetRenderComponent* AcquireComponent(AActor* SubsystemOwner, int32 MaxPoolSize,
												TArray<TObjectPtr<URssWidgetRenderComponent>>& OutAllComponents,
												bool bForceLoad = false);

	void ReturnComponent(URssWidgetRenderComponent* Component, int32 MaxPoolSize);
	void TickActiveComponents(float DeltaTime);
	void GetStats(int32& OutActive, int32& OutAvailable, int32& OutTotal, int32& OutHits, int32& OutMisses) const;
	void Cleanup();
};

UCLASS()
class RSS_API ARssDataManagerSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	ARssDataManagerSubsystem();

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContext"), Category = "RSS Subsystems")
	static ARssDataManagerSubsystem* GetRSSDataManagerSubsystem(UObject* WorldContext);

	//~ Begin AActor Interface
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface

	void LoadConfig();
	void CheckKeys();
	bool CheckCopy();

	float GetSignRenderingDistance() const;

	static void MoveComponentToActor(UActorComponent* Component, AActor* SourceActor, AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "Data Manager", meta = (WorldContext = "WorldContext"))
	static FString SignDataToString(UObject* WorldContext, FRssSignData SignData);

	UFUNCTION(BlueprintCallable, Category = "Data Manager", meta = (WorldContext = "WorldContext"))
	static FRssSignData StringToSignData(UObject* WorldContext, FString SignData, bool& WasSuccess);

	FORCEINLINE void SetLastRotation(int32 newRotation) { mLastRotation = newRotation; }
	FORCEINLINE int32 GetLastRotation() const { return mLastRotation; }

	UFUNCTION(BlueprintPure, Category = "Data Manager")
	FORCEINLINE FRssSignData GetCopiedSignData() const { return mCopiedSignData; }

	UFUNCTION(BlueprintCallable, Category = "Data Manager")
	FString CopySignDataToClipboard(FRssSignData SignData);

	UFUNCTION(BlueprintPure, Category = "Data Manager|Key Manager")
	FORCEINLINE bool IsKeyDown(FKey Key) const { return mKeyDown.Contains(Key); }

	AActor* GetLastCopiedActor() const;
	void SetLastCopiedActor(AActor* Actor);

	// Component Pooling System
	bool AcquirePooledComponent(AActor* Owner, TSubclassOf<URssWidgetRenderComponent> ComponentClass);
	void ReturnPooledComponent(URssWidgetRenderComponent* Component);
	void TickAllActiveComponents(float DeltaTime);
	void CleanupComponentPools();
	void LogPoolStats();
	bool HasActorAlreadyAssignedComponent(AActor* Actor) const;

	// Pending Component Request Queue System
	void ProcessPendingComponentRequests();
	void RemoveFromPendingQueue(AActor* Actor);
	void CreateComponentForActor(AActor* OwnerActor, URssWidgetRenderComponent* Component);

	UFUNCTION(BlueprintCallable, Category = "Data Manager|Sign Loading")
	void ForceLoadSign(AActor* SignActor);

	void UpdateVisibleComponentsAsync();
	bool IsComponentInFrustum(URssWidgetRenderComponent* Component) const;

	UPROPERTY(BlueprintAssignable, Category = "Data Manager")
	FOnKeyPressed OnKeyPressed;

	UPROPERTY(BlueprintAssignable, Category = "Data Manager")
	FOnKeyReleased OnKeyReleased;

	UPROPERTY(EditDefaultsOnly, Category = "Data Manager")
	TArray<FKey> mKeysToCheck;

	UPROPERTY(EditDefaultsOnly, Category = "Data Manager")
	TSubclassOf<UModConfiguration> mModConfig;

private:
	struct FFrustumCheckResult
	{
		TWeakObjectPtr<URssWidgetRenderComponent> Component;
		bool bIsVisible;
	};

	TSet<TWeakObjectPtr<URssWidgetRenderComponent>> mVisibleComponents;
	FCriticalSection mVisibilityLock;
	FThreadSafeBool bFrustumCheckInProgress;
	float mFrustumCheckAccumulator = 0.0f;
	float mFrustumCheckInterval = 0.1f;

	// Component Pool Buckets - one bucket per component class
	TMap<TSubclassOf<URssWidgetRenderComponent>, TSharedPtr<FRssComponentPoolBucket>> mComponentPoolBuckets;
	FCriticalSection mPoolMapLock;

	// Master list that keeps strong references to all pooled components (prevents GC)
	UPROPERTY(Transient)
	TArray<TObjectPtr<URssWidgetRenderComponent>> mAllPooledComponents;

	// Pending Component Request Queue - distance-sorted queue for delayed component acquisition
	FSortedActorDistanceArray<AActor> mPendingComponentRequests;

	UPROPERTY(Transient)
	TMap<TObjectPtr<AActor>, TSubclassOf<URssWidgetRenderComponent>> mPendingRequestClasses;

	FCriticalSection mPendingQueueLock;
	int32 mSortTickCounter = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Pooling")
	FSmartTimer mPoolStatsLogInterval = FSmartTimer(5.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Pooling")
	FSmartTimer mSignFPS = FSmartTimer(0.0333333f); // 30 FPS

	UPROPERTY(EditDefaultsOnly, Category = "Pooling")
	FSmartTimer mComponentRequestProcessTimer = FSmartTimer(0.1f);

	UPROPERTY(EditDefaultsOnly, Category = "Pooling")
	int32 mMaxComponentRequestsPerTick = 5;

	bool bEnableComponentPooling = true;
	bool bEnablePoolStats = true;
	int32 mMinPoolSize = 100;
	int32 mMaxPoolSize = 350;
	int32 mSignRenderingDistance = 350;

	TWeakObjectPtr<AActor> mLastCopiedActor;

	UPROPERTY()
	TArray<FKey> mKeyDown;

	UPROPERTY()
	int32 mLastRotation = 0;

	UPROPERTY()
	FRssSignData mCopiedSignData = FRssSignData();

	FSmartTimer mTimer = FSmartTimer(1.5f);
};
