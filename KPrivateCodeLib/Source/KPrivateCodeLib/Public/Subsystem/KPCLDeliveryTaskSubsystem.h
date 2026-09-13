// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DataAssets/KAPIDeliveryTask.h"
#include "ItemAmount.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystem/KPCLModSubsystem.h"
#include "TimerManager.h"

#include "KPCLDeliveryTaskSubsystem.generated.h"

class AFGCharacterPlayer;
class UFGInventoryComponent;
class UFGSchematic;
class UKAPIDataAssetSubsystem;
class UKPCLDeliveryTreeWidget;

USTRUCT(BlueprintType)
struct FKPCLDeliveryTaskState
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TObjectPtr<UKAPIDeliveryTask> mTask;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TArray<FItemAmount> mRequired;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TArray<FItemAmount> mDelivered;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 mCompleted = 0;
};

UCLASS()
class KPRIVATECODELIB_API UKPCLDeliveryTaskStateLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "DeliveryTask|State")
	static TArray<FItemAmount> GetLeftoverItems(const FKPCLDeliveryTaskState& State);

	UFUNCTION(BlueprintPure, Category = "DeliveryTask|State")
	static TArray<FItemAmount> GetDeliveredItems(const FKPCLDeliveryTaskState& State);

	UFUNCTION(BlueprintPure, Category = "DeliveryTask|State")
	static TArray<TSubclassOf<UFGItemDescriptor>> GetUniqueItemsToDeliver(const FKPCLDeliveryTaskState& State,
																		  bool bFilterDelivered = false);

	UFUNCTION(BlueprintPure, Category = "DeliveryTask|State")
	static float GetPercentDone(const FKPCLDeliveryTaskState& State);

	UFUNCTION(BlueprintPure, Category = "DeliveryTask|State")
	static float GetPercentDelivered(const FKPCLDeliveryTaskState& State, TSubclassOf<UFGItemDescriptor> ItemClass);

	UFUNCTION(BlueprintPure, Category = "DeliveryTask|State")
	static int32 GetRequiredAmount(const FKPCLDeliveryTaskState& State, TSubclassOf<UFGItemDescriptor> ItemClass);

	UFUNCTION(BlueprintPure, Category = "DeliveryTask|State")
	static int32 GetDeliveredAmount(const FKPCLDeliveryTaskState& State, TSubclassOf<UFGItemDescriptor> ItemClass);

	UFUNCTION(BlueprintPure, Category = "DeliveryTask|State")
	static int32 GetLeftoverAmount(const FKPCLDeliveryTaskState& State, TSubclassOf<UFGItemDescriptor> ItemClass);

	UFUNCTION(BlueprintPure, Category = "DeliveryTask|State")
	static int32 GetTotalRequiredAmount(const FKPCLDeliveryTaskState& State);

	UFUNCTION(BlueprintPure, Category = "DeliveryTask|State")
	static int32 GetTotalDeliveredAmount(const FKPCLDeliveryTaskState& State);

	UFUNCTION(BlueprintPure, Category = "DeliveryTask|State")
	static int32 GetTotalLeftoverAmount(const FKPCLDeliveryTaskState& State);

	UFUNCTION(BlueprintPure, Category = "DeliveryTask|State")
	static bool IsFullyDelivered(const FKPCLDeliveryTaskState& State);

	UFUNCTION(BlueprintPure, Category = "DeliveryTask|State")
	static bool IsPartiallyDelivered(const FKPCLDeliveryTaskState& State);

	UFUNCTION(BlueprintPure, Category = "DeliveryTask|State")
	static bool IsValidTaskState(const FKPCLDeliveryTaskState& State);
};

UENUM(BlueprintType)
enum class EKPCLResearchNodeState : uint8
{
	Locked,
	Available,
	Queued,
	Active,
	Completed,
	RepeatableAvailable,
	RepeatableQueued,
	Maxed
};

USTRUCT(BlueprintType)
struct FKPCLResearchTreeNode
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UKAPIDeliveryTask> mTask;

	UPROPERTY(BlueprintReadOnly)
	FIntVector2 mCoordinate = FIntVector2::ZeroValue;

	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<UKAPIDeliveryTask>> mParents;

	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<UKAPIDeliveryTask>> mChildren;

	UPROPERTY(BlueprintReadOnly)
	bool bRequireAllParents = true;

	UPROPERTY(BlueprintReadOnly)
	EKPCLResearchNodeState mState = EKPCLResearchNodeState::Locked;

	UPROPERTY(BlueprintReadOnly)
	TArray<FItemAmount> mRequired;

	UPROPERTY(BlueprintReadOnly)
	TArray<FItemAmount> mDelivered;

	UPROPERTY(BlueprintReadOnly)
	TArray<FItemAmount> mRemaining;

	UPROPERTY(BlueprintReadOnly)
	TArray<int32> mQueueIndices;

	UPROPERTY(BlueprintReadOnly)
	TArray<FText> mQueueErrors;

	UPROPERTY(BlueprintReadOnly)
	int32 mCompletedCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 mMaxCompletions = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	float mProgress = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bPrerequisitesMet = false;

	UPROPERTY(BlueprintReadOnly)
	bool bCanQueue = false;

	UPROPERTY(BlueprintReadOnly)
	bool bIsRepeatable = false;
};

USTRUCT(BlueprintType)
struct FKPCLResearchTreeEdge
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UKAPIDeliveryTask> mParent;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UKAPIDeliveryTask> mChild;

	UPROPERTY(BlueprintReadOnly)
	FIntVector2 mParentCoordinate = FIntVector2::ZeroValue;

	UPROPERTY(BlueprintReadOnly)
	FIntVector2 mChildCoordinate = FIntVector2::ZeroValue;

	UPROPERTY(BlueprintReadOnly)
	bool bParentCompleted = false;

	UPROPERTY(BlueprintReadOnly)
	bool bChildUnlocked = false;
};

USTRUCT(BlueprintType)
struct FKPCLParentTaskNotMetReason
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UKAPIDeliveryTask> mTask;

	UPROPERTY(BlueprintReadOnly)
	FText mReasonDescription;

	UPROPERTY(BlueprintReadOnly)
	FText mReasonTitle;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKPCLOnDeliveryTaskCollectionChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKPCLOnDeliveryTaskStateChanged, UKAPIDeliveryTask*, Task);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKPCLOnDeliveryTaskQueueEntryChanged, UKAPIDeliveryTask*, Task, int32,
											 QueueIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKPCLOnDeliveryTaskCompleted, UKAPIDeliveryTask*, Task, int32,
											 CompletionCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKPCLOnResearchTreeChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKPCLOnResearchNodeChanged, const FKPCLResearchTreeNode&, Node);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKPCLOnActiveResearchChanged, UKAPIDeliveryTask*, PreviousTask,
											 UKAPIDeliveryTask*, CurrentTask);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKPCLOnResearchProgressChanged, UKAPIDeliveryTask*, Task, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKPCLOnResearchAvailabilityChanged, UKAPIDeliveryTask*, Task, bool,
											 bIsAvailable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKPCLOnManualDeliveryProcessed, TSubclassOf<UFGItemDescriptor>,
											   ItemClass, int32, DeliveredAmount, int32, RemainingAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKPCLOnDeliveryTaskRewardErrorChanged, FText, ErrorText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKPCLOnDeliveryTreeWidgetLifecycle, UKPCLDeliveryTreeWidget*, Widget);

UCLASS(BlueprintType, Blueprintable)
class KPRIVATECODELIB_API AKPCLDeliveryTaskSubsystem : public AKPCLModSubsystem
{
	GENERATED_BODY()

public:
	AKPCLDeliveryTaskSubsystem();

	UFUNCTION(BlueprintPure, Category = "Subsystem", DisplayName = "GetKPCLDeliveryTaskSubsystem",
			  meta = (DefaultToSelf = "WorldContext"))
	static AKPCLDeliveryTaskSubsystem* Get(UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "KMods|DeliveryTree")
	UKPCLDeliveryTreeWidget* SetupDeliveryTreeWidget();

	UFUNCTION(BlueprintCallable, Category = "KMods|DeliveryTree")
	void DestroyDeliveryTreeWidget();

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTree")
	UKPCLDeliveryTreeWidget* GetDeliveryTreeWidget() const { return mDeliveryTreeWidget; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KMods|DeliveryTask")
	bool TryToDeliverAmounts(const TArray<FItemAmount>& AmountsToDeliver, TArray<FItemAmount>& DeliveredAmounts);

	bool TriggerTaskCompletion(FKPCLDeliveryTaskState* TaskState);

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	bool IsNoUnlockCostEnabled() const;

	void ScheduleFreeUnlockScan();

	UFUNCTION(BlueprintCallable, Category = "KMods|DeliveryTask")
	bool TryToQueueTask(UKAPIDeliveryTask* Task, TArray<FText>& ErrorMessages);

	UFUNCTION(BlueprintCallable, Category = "KMods|DeliveryTask")
	bool CanQueueTask(UKAPIDeliveryTask* Task, TArray<FText>& ErrorMessages) const;

	UFUNCTION(BlueprintCallable, Category = "KMods|DeliveryTask")
	bool TryToRemoveFromQueue(UKAPIDeliveryTask* Task);

	UFUNCTION(BlueprintCallable, Category = "KMods|DeliveryTask")
	bool TryToRemoveFromQueueAt(int32 QueueIndex);

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	bool CanRemoveFromQueueAt(int32 QueueIndex) const;

	UFUNCTION(BlueprintCallable, Category = "KMods|DeliveryTask")
	bool TryToDequeueFrom(int32 QueueIndex);

	UFUNCTION(BlueprintCallable, Category = "KMods|DeliveryTask")
	bool TryToDequeueTask(UKAPIDeliveryTask* Task);

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	bool CanDequeueFrom(int32 QueueIndex) const;

	UFUNCTION(BlueprintCallable, Category = "KMods|DeliveryTask")
	bool TryToMoveQueueEntry(int32 FromIndex, int32 ToIndex);

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	TArray<UKAPIDeliveryTask*> GetCompletedTasks() const;

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	TArray<UKAPIDeliveryTask*> GetQueuedTasks() const;

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	int32 GetQueuedTaskCount() const { return mQueuedTasks.Num(); }

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	TArray<FKPCLDeliveryTaskState> GetDeliveredTaskStates() const { return mDeliveredTaskStates; }

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	UKAPIDeliveryTask* GetActiveTask() const;

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	bool HasActiveTask() const { return IsValid(GetActiveTask()); }

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	bool IsActiveTaskFullyDelivered() const;

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	bool GetCurrentQueueItem(FKPCLDeliveryTaskState& OutQueueItem) const;

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	TArray<FItemAmount> GetCurrentTaskNeededAmounts() const;

	UFUNCTION(BlueprintCallable, Category = "KMods|DeliveryTask")
	bool TryGrabCurrentTaskItemsFromPlayer(AFGCharacterPlayer* Player);

	UFUNCTION(BlueprintCallable, Category = "KMods|DeliveryTask")
	bool GetOrCreateTaskState(UKAPIDeliveryTask* Task, FKPCLDeliveryTaskState& OutState);

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask|ManualDelivery")
	UFGInventoryComponent* GetManualDeliveryInventory() const { return mManualDeliveryInventory.Get(); }

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask|ManualDelivery")
	int32 GetManualDeliveryInventorySlotCount() const { return FMath::Max(1, mManualDeliveryInventorySlotCount); }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KMods|DeliveryTask|ManualDelivery")
	void AddManualDeliveryInventorySlots(int32 NumSlots);

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	int32 GetQueueLimit() const { return FMath::Max(1, mQueueLimit); }

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	int32 GetRemainingQueueSlots() const { return FMath::Max(0, GetQueueLimit() - GetQueuedTaskCount()); }

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	bool IsQueueFull() const { return GetRemainingQueueSlots() == 0; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KMods|DeliveryTask")
	void AddQueueSlots(int32 NumSlots);

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask|Rewards")
	UFGInventoryComponent* GetRewardInventory() const { return mRewardInventory.Get(); }

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask|Rewards")
	int32 GetRewardInventorySlotCount() const { return FMath::Max(5, mRewardInventorySlotCount); }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KMods|DeliveryTask|Rewards")
	void AddRewardInventorySlots(int32 NumSlots);

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask|Rewards")
	FText GetRewardErrorText() const { return mRewardErrorText; }

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask|Status")
	FText GetCurrentErrorText() const { return mRewardErrorText; }

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask|Status")
	bool HasCurrentError() const { return !mRewardErrorText.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask|Rewards")
	TArray<FItemAmount> GetCurrentTaskRewards(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask|Rewards")
	TArray<FItemAmount> GetTaskRewardsForCompletion(UKAPIDeliveryTask* Task, int32 CompletionIndex) const;

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask")
	int32 GetTaskCompletionCount(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask|ManualDelivery")
	int32 GetManualDeliveryRemainingAmount(TSubclassOf<UFGItemDescriptor> ItemClass) const;

	UFUNCTION(BlueprintPure, Category = "KMods|DeliveryTask|ManualDelivery")
	bool IsManualDeliveryItemAllowed(TSubclassOf<UFGItemDescriptor> ItemClass) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KMods|DeliveryTask|ManualDelivery")
	bool TryConsumeManualDeliveryInventory();

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	TArray<FKPCLResearchTreeNode> GetResearchTreeNodes() const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	TArray<FKPCLResearchTreeEdge> GetResearchTreeEdges() const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	bool GetResearchNode(UKAPIDeliveryTask* Task, FKPCLResearchTreeNode& OutNode) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	bool GetActiveResearchNode(UKAPIDeliveryTask*& OutTask, FKPCLResearchTreeNode& OutNode) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	bool GetResearchNodeAtCoordinate(FIntVector2 Coordinate, FKPCLResearchTreeNode& OutNode) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	bool GetQueuedResearchNode(int32 QueueIndex, FKPCLResearchTreeNode& OutNode) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	UKAPIDeliveryTask* GetResearchTaskAtCoordinate(FIntVector2 Coordinate) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	TArray<UKAPIDeliveryTask*> GetResearchParents(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	TArray<UKAPIDeliveryTask*> GetResearchChildren(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	TArray<UFGAvailabilityDependency*> GetTaskNotMetDependencies(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	TArray<FKPCLParentTaskNotMetReason> GetNotMetParentTask(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	bool AreTaskParentCompletionsMet(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	bool AreTaskPrerequisitesMet(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	bool AreTaskPrerequisitesMetWithQueue(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	bool IsTaskLocked(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	bool IsTaskSelectable(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	bool IsResearchTaskRegistered(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	bool IsTaskCompleted(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	float GetTaskResearchProgress(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	TArray<int32> GetTaskQueueIndices(UKAPIDeliveryTask* Task) const;

	UFUNCTION(BlueprintPure, Category = "KMods|ResearchTree")
	bool GetResearchTreeBounds(FIntVector2& OutMinCoordinate, FIntVector2& OutMaxCoordinate) const;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Replicated, Category = "KMods|DeliveryTask")
	int32 mQueueLimit = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KMods|DeliveryTree")
	TSubclassOf<UKPCLDeliveryTreeWidget> mDeliveryTreeWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "KMods|DeliveryTree")
	TObjectPtr<UKPCLDeliveryTreeWidget> mDeliveryTreeWidget;

	UPROPERTY(BlueprintAssignable, Category = "KMods|DeliveryTree|Events")
	FKPCLOnDeliveryTreeWidgetLifecycle mOnDeliveryTreeWidgetSetup;

	UPROPERTY(BlueprintAssignable, Category = "KMods|DeliveryTree|Events")
	FKPCLOnDeliveryTreeWidgetLifecycle mOnDeliveryTreeWidgetDestroyed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "KMods|DeliveryTask|ManualDelivery")
	TObjectPtr<UFGInventoryComponent> mManualDeliveryInventory;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Replicated,
			  Category = "KMods|DeliveryTask|ManualDelivery")
	int32 mManualDeliveryInventorySlotCount = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "KMods|DeliveryTask|Rewards")
	TObjectPtr<UFGInventoryComponent> mRewardInventory;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Replicated, Category = "KMods|DeliveryTask|Rewards")
	int32 mRewardInventorySlotCount = 5;

	UPROPERTY(BlueprintAssignable, Category = "KMods|DeliveryTask|ManualDelivery|Events")
	FKPCLOnManualDeliveryProcessed mOnManualDeliveryProcessed;

	UPROPERTY(BlueprintAssignable, Category = "KMods|DeliveryTask|Rewards|Events")
	FKPCLOnDeliveryTaskRewardErrorChanged mOnRewardErrorChanged;

	UPROPERTY(BlueprintAssignable, Category = "KMods|DeliveryTask|Events")
	FKPCLOnDeliveryTaskCollectionChanged mOnQueueChanged;

	UPROPERTY(BlueprintAssignable, Category = "KMods|DeliveryTask|Events")
	FKPCLOnDeliveryTaskQueueEntryChanged mOnTaskQueued;

	UPROPERTY(BlueprintAssignable, Category = "KMods|DeliveryTask|Events")
	FKPCLOnDeliveryTaskQueueEntryChanged mOnTaskRemovedFromQueue;

	UPROPERTY(BlueprintAssignable, Category = "KMods|DeliveryTask|Events")
	FKPCLOnDeliveryTaskStateChanged mOnTaskStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "KMods|DeliveryTask|Events")
	FKPCLOnDeliveryTaskCompleted mOnTaskCompleted;

	UPROPERTY(BlueprintAssignable, Category = "KMods|DeliveryTask|Events")
	FKPCLOnDeliveryTaskCollectionChanged mOnCompletedTasksChanged;

	UPROPERTY(BlueprintAssignable, Category = "KMods|ResearchTree|Events")
	FKPCLOnResearchTreeChanged mOnResearchTreeChanged;

	UPROPERTY(BlueprintAssignable, Category = "KMods|ResearchTree|Events")
	FKPCLOnResearchNodeChanged mOnResearchNodeChanged;

	UPROPERTY(BlueprintAssignable, Category = "KMods|ResearchTree|Events")
	FKPCLOnActiveResearchChanged mOnActiveResearchChanged;

	UPROPERTY(BlueprintAssignable, Category = "KMods|ResearchTree|Events")
	FKPCLOnResearchProgressChanged mOnResearchProgressChanged;

	UPROPERTY(BlueprintAssignable, Category = "KMods|ResearchTree|Events")
	FKPCLOnResearchAvailabilityChanged mOnResearchAvailabilityChanged;

	UPROPERTY(BlueprintAssignable, Category = "KMods|ResearchTree|Events")
	FKPCLOnDeliveryTaskCollectionChanged mOnResearchQueueOrderChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;

private:
	friend class FKPCLDeliveryTaskCurrentQueueItemTest;

	FKPCLDeliveryTaskState* FindTaskState(const UKAPIDeliveryTask* Task);
	const FKPCLDeliveryTaskState* FindTaskState(const UKAPIDeliveryTask* Task) const;

	FKPCLDeliveryTaskState* FindOrAddTaskState(UKAPIDeliveryTask* Task, bool& bOutCreated);

	bool BuildPreviewTaskState(UKAPIDeliveryTask* Task, FKPCLDeliveryTaskState& OutState) const;
	void RebuildTaskStateCache();
	void RebuildQueueCache();
	void RebuildCompletedTaskCache();

	void ApplyCompletedTaskUnlocks();

	bool IsTaskAutoCompletable(UKAPIDeliveryTask* Task) const;

	bool AutoCompleteTask(UKAPIDeliveryTask* Task);

	bool ProcessAutoCompletableTasks();

	void ScheduleAutoCompleteScan();

	bool ProcessFreeUnlockQueue();

	void EnsureResearchTopologyCache() const;
	void RebuildResearchTopologyCache() const;
	void InvalidateResearchTopologyCache();

	bool SyncAllEndlessTaskRequirements();
	bool SyncEndlessTaskRequirements(FKPCLDeliveryTaskState& TaskState) const;

	bool SyncOneTimeTaskRequirements(FKPCLDeliveryTaskState& TaskState) const;
	void InitializeTaskRequirements(FKPCLDeliveryTaskState& TaskState) const;
	void NormalizeTaskState(FKPCLDeliveryTaskState& TaskState) const;
	bool IsTaskStateComplete(const FKPCLDeliveryTaskState& TaskState) const;

	bool IsParentRequirementMet(UKAPIDeliveryTask* Child, UKAPIDeliveryTask* Parent) const;

	bool IsTaskLockedInternal(UKAPIDeliveryTask* Task, bool bPrerequisitesMet) const;
	bool AreTaskQueuePrerequisitesMet(UKAPIDeliveryTask* Task, const TArray<UKAPIDeliveryTask*>& Queue,
									  int32 TaskQueueIndex) const;
	bool AreTaskQueuePrerequisitesMet(UKAPIDeliveryTask* Task, const TSet<UKAPIDeliveryTask*>& PriorQueuedTasks) const;

	TSet<UKAPIDeliveryTask*> GetQueuedTaskSet() const;
	bool IsQueueParentRulesValid(const TArray<UKAPIDeliveryTask*>& Queue) const;

	UKAPIDeliveryTask* ValidateQueueMutation(int32 QueueIndex,
											 TFunctionRef<bool(TArray<UKAPIDeliveryTask*>&)> MutateCandidate) const;

	void CommitQueueMutation(TFunctionRef<void()> MutateQueue);
	void SanitizeQueueParentRules();
	void BroadcastResearchNodeChanged(UKAPIDeliveryTask* Task);
	void BroadcastAllResearchAvailabilityChanged();
	void HandleDeliveryTasksChanged();

	UFUNCTION()
	void HandleDeliveryTaskSystemUnlockChanged(bool bIsUnlocked);

	UFUNCTION()
	void HandleSchematicUnlocked(TSubclassOf<UFGSchematic> UnlockedSchematic);
	bool FilterManualDeliveryInventory(TSubclassOf<UObject> ItemClass, int32 InventoryIndex) const;
	void RefreshManualDeliveryInventorySize();
	void RefreshManualDeliveryInventorySlotSize();
	void RefreshRewardInventorySize();
	void OnManualDeliveryItemAdded(TSubclassOf<UFGItemDescriptor> ItemClass, int32 NumAdded,
								   UFGInventoryComponent* SourceInventory);
	void OnRewardInventoryItemRemoved(TSubclassOf<UFGItemDescriptor> ItemClass, int32 NumRemoved,
									  UFGInventoryComponent* TargetInventory);
	void GetTaskRewards(const UKAPIDeliveryTask* Task, int32 CompletionIndex, TArray<FItemAmount>& OutRewards) const;

	void CollectTaskRewards(const UKAPIDeliveryTask* Task, int32 CompletionIndex, TArray<FItemAmount>& OutRewards,
							bool& bOutHasInvalidReward) const;
	bool TryStoreTaskRewards(const UKAPIDeliveryTask* Task, int32 CompletionIndex, FText& OutErrorText);
	bool TryCompleteActiveTask();
	void StartTaskCompletionRetryTimer();
	void StopTaskCompletionRetryTimer();
	void RetryTaskCompletion();
	void ShowTaskToast(const UKAPIDeliveryTask* Task) const;
	void SetRewardErrorText(const FText& ErrorText);
	static float CalculateTaskProgress(const FKPCLDeliveryTaskState* TaskState);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastShowTaskToast(UKAPIDeliveryTask* Task);

	UFUNCTION()
	void OnRep_CompletedTasks(const TArray<UKAPIDeliveryTask*>& PreviousCompletedTasks);

	UFUNCTION()
	void OnRep_QueuedTasks(const TArray<UKAPIDeliveryTask*>& PreviousQueuedTasks);

	UFUNCTION()
	void OnRep_DeliveredTaskStates(const TArray<FKPCLDeliveryTaskState>& PreviousTaskStates);

	UFUNCTION()
	void OnRep_RewardErrorText();

	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_CompletedTasks)
	TArray<TObjectPtr<UKAPIDeliveryTask>> mCompletedTasks;

	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_QueuedTasks)
	TArray<TObjectPtr<UKAPIDeliveryTask>> mQueuedTasks;

	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_DeliveredTaskStates)
	TArray<FKPCLDeliveryTaskState> mDeliveredTaskStates;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_RewardErrorText)
	FText mRewardErrorText;

	bool bIsProcessingManualDeliveryInventory = false;
	bool bIsGrabbingPlayerInventory = false;
	bool bIsProcessingTaskCompletion = false;
	bool bIsProcessingAutoCompletion = false;
	bool bIsProcessingFreeUnlocks = false;
	int32 mPreGrantedRewardInventorySlots = 0;
	FTimerHandle mTaskCompletionRetryTimer;
	FTimerHandle mAutoCompleteScanTimer;
	FTimerHandle mFreeUnlockScanTimer;

	mutable bool bTaskStateCacheValid = false;
	mutable TMap<const UKAPIDeliveryTask*, int32> mTaskStateIndexByTask;
	TMap<UKAPIDeliveryTask*, TArray<int32>> mQueueIndicesByTask;
	TSet<UKAPIDeliveryTask*> mCompletedTaskCache;
	TObjectPtr<UKAPIDeliveryTask> mCachedActiveTask;

	mutable TWeakObjectPtr<UKAPIDataAssetSubsystem> mCachedDataAssetSubsystem;
	mutable uint32 mCachedDeliveryTaskRevision = MAX_uint32;
	mutable TArray<TObjectPtr<UKAPIDeliveryTask>> mSortedResearchTasks;

	mutable TArray<TObjectPtr<UKAPIDeliveryTask>> mSchematicOnlyTasks;
	mutable TMap<UKAPIDeliveryTask*, TArray<TObjectPtr<UKAPIDeliveryTask>>> mResearchParentsByTask;
	mutable TMap<UKAPIDeliveryTask*, TArray<TObjectPtr<UKAPIDeliveryTask>>> mResearchChildrenByTask;
	mutable FIntVector2 mCachedTreeMinCoordinate = FIntVector2::ZeroValue;
	mutable FIntVector2 mCachedTreeMaxCoordinate = FIntVector2::ZeroValue;
	mutable bool bHasCachedTreeBounds = false;
};
