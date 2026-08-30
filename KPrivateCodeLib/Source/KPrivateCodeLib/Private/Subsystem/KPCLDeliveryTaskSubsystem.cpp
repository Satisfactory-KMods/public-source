#include "Subsystem/KPCLDeliveryTaskSubsystem.h"

#include "AvailabilityDependencies/FGAvailabilityDependency.h"
#include "BFL/KBFL_Util.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "FGCharacterPlayer.h"
#include "FGGameRulesSubsystem.h"
#include "FGInventoryComponent.h"
#include "FGPlayerController.h"
#include "FGPushNotificationWidget.h"
#include "FGSchematicManager.h"
#include "KPrivateCodeLibModule.h"
#include "KPCLWorldModule.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "Replication/KPCLDefaultRCO.h"
#include "Resources/FGItemDescriptor.h"
#include "Subsystem/KPCLUnlockSubsystem.h"
#include "Subsystems/KAPIDataAssetSubsystem.h"
#include "UI/FGGameUI.h"
#include "Unlocks/KPCLDeliveryTaskInventorySlotUnlock.h"
#include "Widget/KPCLDeliveryTreeWidget.h"

namespace
{
	constexpr int32 InitialRewardInventorySize = 5;

	constexpr int32 ResolveRewardInventorySizeAfterUnlock(const int32 CurrentSize, const int32 ExplicitUnlockSlots)
	{
		if (ExplicitUnlockSlots <= 0)
		{
			return FMath::Max(0, CurrentSize);
		}
		return static_cast<int32>(FMath::Min<int64>(
			static_cast<int64>(FMath::Max(0, CurrentSize)) + static_cast<int64>(ExplicitUnlockSlots), MAX_int32));
	}

	static_assert(ResolveRewardInventorySizeAfterUnlock(5, 0) == 5);
	static_assert(ResolveRewardInventorySizeAfterUnlock(5, 2) == 7);
	static_assert(ResolveRewardInventorySizeAfterUnlock(MAX_int32, 1) == MAX_int32);

	int32 GetRewardInventorySlotsGrantedByTask(const UKAPIDeliveryTask* Task)
	{
		int64 GrantedSlots = 0;
		if (!IsValid(Task))
		{
			return 0;
		}

		for (const TObjectPtr<UFGUnlock>& Unlock : Task->mUnlocks)
		{
			const UKPCLDeliveryTaskInventorySlotUnlock* InventorySlotUnlock =
				Cast<UKPCLDeliveryTaskInventorySlotUnlock>(Unlock);
			if (IsValid(InventorySlotUnlock))
			{
				GrantedSlots += FMath::Max(1, InventorySlotUnlock->GetNumInventorySlotsToUnlock());
			}
		}
		return static_cast<int32>(FMath::Min<int64>(GrantedSlots, MAX_int32));
	}

	int32 GetRewardInventorySizeUnlockedByCompletedTasks(const TArray<FKPCLDeliveryTaskState>& TaskStates)
	{
		int64 UnlockedSize = InitialRewardInventorySize;
		for (const FKPCLDeliveryTaskState& TaskState : TaskStates)
		{
			const UKAPIDeliveryTask* Task = GetValid(TaskState.mTask);
			const int32 CompletedCount = FMath::Max(0, TaskState.mCompleted);
			if (!Task || CompletedCount == 0)
			{
				continue;
			}

			const int64 GrantedSlots = static_cast<int64>(GetRewardInventorySlotsGrantedByTask(Task)) * CompletedCount;
			UnlockedSize = FMath::Min<int64>(UnlockedSize + GrantedSlots, MAX_int32);
		}
		return static_cast<int32>(UnlockedSize);
	}

	constexpr bool ShouldRefreshEndlessTaskRequirements(const int32 MaterializedItemCount,
												  const int32 ExpectedItemCount, const bool bContainsOnlyEligibleItems)
	{
		return MaterializedItemCount != ExpectedItemCount || !bContainsOnlyEligibleItems;
	}

	static_assert(!ShouldRefreshEndlessTaskRequirements(5, 5, true));
	static_assert(ShouldRefreshEndlessTaskRequirements(14, 5, true));
	static_assert(ShouldRefreshEndlessTaskRequirements(5, 5, false));
	static_assert(!ShouldRefreshEndlessTaskRequirements(3, 3, true));

	bool HasSameItemAmounts(const TArray<FItemAmount>& Left, const TArray<FItemAmount>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (Left[Index].ItemClass != Right[Index].ItemClass || Left[Index].Amount != Right[Index].Amount)
			{
				return false;
			}
		}
		return true;
	}

	int32 GetEffectiveParentCompletionRequirement(const UKAPIDeliveryTask* Task, const UKAPIDeliveryTask* Parent)
	{
		return IsValid(Task) && IsValid(Parent) && Parent->bIsEndless
				   ? FMath::Max(1, Task->mRequiredParentCompletions)
				   : 1;
	}

	void AddItemAmount(TArray<FItemAmount>& Amounts, TSubclassOf<UFGItemDescriptor> Item, int32 Amount)
	{
		if (!IsValid(Item) || Amount <= 0)
		{
			return;
		}

		if (FItemAmount* Existing =
				Amounts.FindByPredicate([Item](const FItemAmount& Candidate) { return Candidate.ItemClass == Item; }))
		{
			Existing->Amount = static_cast<int32>(
				FMath::Min<int64>(static_cast<int64>(Existing->Amount) + static_cast<int64>(Amount), MAX_int32));
		}
		else
		{
			Amounts.Emplace(Item, Amount);
		}
	}

	int32 GetRemainingAmountAtIndex(const FKPCLDeliveryTaskState& TaskState, int32 Index)
	{
		const int32 DeliveredAmount = TaskState.mDelivered.IsValidIndex(Index) ? TaskState.mDelivered[Index].Amount : 0;
		return FMath::Max(0, TaskState.mRequired[Index].Amount - DeliveredAmount);
	}

	bool CallTextSetter(UObject* Target, const FName FunctionName, const FText& Text)
	{
		UFunction* Function = IsValid(Target) ? Target->FindFunction(FunctionName) : nullptr;
		struct FTextSetterParameters
		{
			FText Value;
		};

		if (!IsValid(Function) || Function->ParmsSize != sizeof(FTextSetterParameters))
		{
			return false;
		}

		FTextSetterParameters Parameters{Text};
		Target->ProcessEvent(Function, &Parameters);
		return true;
	}

	void CallIconSetter(UObject* Target, UTexture2D* Icon)
	{
		UFunction* Function = IsValid(Target) ? Target->FindFunction(TEXT("SetIcon")) : nullptr;
		struct FIconSetterParameters
		{
			UTexture2D* Value;
		};

		if (!IsValid(Function) || Function->ParmsSize != sizeof(FIconSetterParameters))
		{
			return;
		}

		FIconSetterParameters Parameters{Icon};
		Target->ProcessEvent(Function, &Parameters);
	}
}

AKPCLDeliveryTaskSubsystem::AKPCLDeliveryTaskSubsystem()
{
	PrimaryActorTick.bCanEverTick = false;
	mShouldSave = true;
	mDeliveryTreeWidgetClass = UKPCLDeliveryTreeWidget::StaticClass();
	mManualDeliveryInventory = CreateDefaultSubobject<UFGInventoryComponent>(TEXT("ManualDeliveryInventory"));
	mManualDeliveryInventory->SetDefaultSize(1);
	mManualDeliveryInventory->SetIsReplicated(true);
	mRewardInventory = CreateDefaultSubobject<UFGInventoryComponent>(TEXT("RewardInventory"));
	mRewardInventory->SetDefaultSize(InitialRewardInventorySize);
	mRewardInventory->SetIsReplicated(true);
}

AKPCLDeliveryTaskSubsystem* AKPCLDeliveryTaskSubsystem::Get(UObject* WorldContext)
{
	return UKBFL_Util::GetSubsystem<AKPCLDeliveryTaskSubsystem>(WorldContext);
}

void AKPCLDeliveryTaskSubsystem::BeginPlay()
{
	Super::BeginPlay();
	RebuildTaskStateCache();
	RebuildQueueCache();
	RebuildCompletedTaskCache();
	EnsureResearchTopologyCache();
	if (HasAuthority())
	{
		const TArray<FKPCLDeliveryTaskState> PreviousTaskStates = mDeliveredTaskStates;
		bool bRequirementsChanged = SyncAllEndlessTaskRequirements();
		for (FKPCLDeliveryTaskState& TaskState : mDeliveredTaskStates)
		{
			bRequirementsChanged |= SyncOneTimeTaskRequirements(TaskState);
		}
		if (bRequirementsChanged)
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mDeliveredTaskStates, this);
			OnRep_DeliveredTaskStates(PreviousTaskStates);
			ForceNetUpdate();
		}
	}

	if (UKAPIDataAssetSubsystem* DataSubsystem = UKAPIDataAssetSubsystem::Get(GetWorld()))
	{
		DataSubsystem->mOnDeliveryTasksChanged.AddUObject(this,
														  &AKPCLDeliveryTaskSubsystem::HandleDeliveryTasksChanged);
	}
	if (AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld()))
	{
		UnlockSubsystem->mOnDeliveryTaskSystemUnlockChanged.AddUniqueDynamic(
			this, &AKPCLDeliveryTaskSubsystem::HandleDeliveryTaskSystemUnlockChanged);
	}
	if (AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld()))
	{
		SchematicManager->PurchasedSchematicDelegate.AddUniqueDynamic(
			this, &AKPCLDeliveryTaskSubsystem::HandleSchematicUnlocked);
	}
	SanitizeQueueParentRules();

	if (IsValid(mManualDeliveryInventory) && !mManualDeliveryInventory->mItemFilter.IsBoundToObject(this))
	{
		mManualDeliveryInventory->mItemFilter.BindUObject(this,
														  &AKPCLDeliveryTaskSubsystem::FilterManualDeliveryInventory);
	}

	if (HasAuthority())
	{
		if (IsValid(mManualDeliveryInventory))
		{
			RefreshManualDeliveryInventorySize();
			mManualDeliveryInventory->SetReplicationRelevancyOwner(this);
			mManualDeliveryInventory->OnItemAddedDelegate_Native.BindUObject(
				this, &AKPCLDeliveryTaskSubsystem::OnManualDeliveryItemAdded);
			RefreshManualDeliveryInventorySlotSize();
			TryConsumeManualDeliveryInventory();
		}

		if (IsValid(mRewardInventory))
		{
			RefreshRewardInventorySize();
			mRewardInventory->SetReplicationRelevancyOwner(this);
			mRewardInventory->OnItemRemovedDelegate_Native.BindUObject(
				this, &AKPCLDeliveryTaskSubsystem::OnRewardInventoryItemRemoved);
			TryCompleteActiveTask();
		}

		ScheduleAutoCompleteScan();
		ScheduleFreeUnlockScan();
	}

	SetupDeliveryTreeWidget();
}

void AKPCLDeliveryTaskSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyDeliveryTreeWidget();
	StopTaskCompletionRetryTimer();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mAutoCompleteScanTimer);
		World->GetTimerManager().ClearTimer(mFreeUnlockScanTimer);
	}
	if (UKAPIDataAssetSubsystem* DataSubsystem = UKAPIDataAssetSubsystem::Get(GetWorld()))
	{
		DataSubsystem->mOnDeliveryTasksChanged.RemoveAll(this);
	}
	if (AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld()))
	{
		UnlockSubsystem->mOnDeliveryTaskSystemUnlockChanged.RemoveAll(this);
	}
	if (AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld()))
	{
		SchematicManager->PurchasedSchematicDelegate.RemoveAll(this);
	}
	if (IsValid(mManualDeliveryInventory))
	{
		mManualDeliveryInventory->OnItemAddedDelegate_Native.Unbind();
		mManualDeliveryInventory->mItemFilter.Unbind();
	}
	if (IsValid(mRewardInventory))
	{
		mRewardInventory->OnItemRemovedDelegate_Native.Unbind();
	}

	Super::EndPlay(EndPlayReason);
}

UKPCLDeliveryTreeWidget* AKPCLDeliveryTaskSubsystem::SetupDeliveryTreeWidget()
{
	if (IsValid(mDeliveryTreeWidget))
	{
		mDeliveryTreeWidget->SetupDeliveryTree(this);
		return mDeliveryTreeWidget;
	}
	if (GetNetMode() == NM_DedicatedServer || !IsValid(GetWorld()))
	{
		return nullptr;
	}

	TSubclassOf<UKPCLDeliveryTreeWidget> WidgetClass = mDeliveryTreeWidgetClass;
	if (!IsValid(WidgetClass))
	{
		WidgetClass = UKPCLDeliveryTreeWidget::StaticClass();
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	mDeliveryTreeWidget = IsValid(PlayerController)
		? CreateWidget<UKPCLDeliveryTreeWidget>(PlayerController, WidgetClass)
		: CreateWidget<UKPCLDeliveryTreeWidget>(GetWorld(), WidgetClass);
	if (!IsValid(mDeliveryTreeWidget))
	{
		return nullptr;
	}

	mDeliveryTreeWidget->SetupDeliveryTree(this);
	mOnDeliveryTreeWidgetSetup.Broadcast(mDeliveryTreeWidget);
	return mDeliveryTreeWidget;
}

void AKPCLDeliveryTaskSubsystem::DestroyDeliveryTreeWidget()
{
	if (!IsValid(mDeliveryTreeWidget))
	{
		mDeliveryTreeWidget = nullptr;
		return;
	}

	UKPCLDeliveryTreeWidget* Widget = mDeliveryTreeWidget;
	Widget->TeardownDeliveryTree();
	Widget->RemoveFromParent();
	mOnDeliveryTreeWidgetDestroyed.Broadcast(Widget);
	mDeliveryTreeWidget = nullptr;
}

void AKPCLDeliveryTaskSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.RepNotifyCondition = REPNOTIFY_OnChanged;
	DOREPLIFETIME_WITH_PARAMS_FAST(AKPCLDeliveryTaskSubsystem, mCompletedTasks, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(AKPCLDeliveryTaskSubsystem, mQueuedTasks, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(AKPCLDeliveryTaskSubsystem, mDeliveredTaskStates, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(AKPCLDeliveryTaskSubsystem, mRewardErrorText, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(AKPCLDeliveryTaskSubsystem, mManualDeliveryInventorySlotCount, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(AKPCLDeliveryTaskSubsystem, mRewardInventorySlotCount, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(AKPCLDeliveryTaskSubsystem, mQueueLimit, Params);
}

void AKPCLDeliveryTaskSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	Super::PostLoadGame_Implementation(saveVersion, gameVersion);
	if (!HasAuthority())
	{
		RebuildTaskStateCache();
		RebuildQueueCache();
		RebuildCompletedTaskCache();
		return;
	}

	const TArray<UKAPIDeliveryTask*> PreviousCompletedTasks = GetCompletedTasks();
	const TArray<UKAPIDeliveryTask*> PreviousQueuedTasks = GetQueuedTasks();
	const TArray<FKPCLDeliveryTaskState> PreviousTaskStates = mDeliveredTaskStates;

	KPCL_CLEAR_INVALID_OBJECT_PTRS(mCompletedTasks);
	KPCL_CLEAR_INVALID_OBJECT_PTRS(mQueuedTasks);

	TSet<UKAPIDeliveryTask*> SeenCompletedTasks;
	mCompletedTasks.RemoveAll(
		[&SeenCompletedTasks](const TObjectPtr<UKAPIDeliveryTask>& TaskPtr)
		{
			UKAPIDeliveryTask* Task = GetValid(TaskPtr);
			if (!Task || SeenCompletedTasks.Contains(Task))
			{
				return true;
			}
			SeenCompletedTasks.Add(Task);
			return false;
		});

	TSet<UKAPIDeliveryTask*> SeenOneTimeQueuedTasks;
	mQueuedTasks.RemoveAll(
		[&SeenOneTimeQueuedTasks](const TObjectPtr<UKAPIDeliveryTask>& TaskPtr)
		{
			UKAPIDeliveryTask* Task = GetValid(TaskPtr);
			if (!Task || Task->bIsEndless)
			{
				return false;
			}
			if (SeenOneTimeQueuedTasks.Contains(Task))
			{
				return true;
			}
			SeenOneTimeQueuedTasks.Add(Task);
			return false;
		});

	TSet<UKAPIDeliveryTask*> SeenTaskStates;
	mDeliveredTaskStates.RemoveAll(
		[&SeenTaskStates](const FKPCLDeliveryTaskState& State)
		{
			UKAPIDeliveryTask* Task = GetValid(State.mTask);
			if (!Task || SeenTaskStates.Contains(Task))
			{
				return true;
			}
			SeenTaskStates.Add(Task);
			return false;
		});

	for (FKPCLDeliveryTaskState& State : mDeliveredTaskStates)
	{
		State.mCompleted = FMath::Max(0, State.mCompleted);
		UKAPIDeliveryTask* Task = GetValid(State.mTask);
		if (Task && Task->bIsEndless && Task->mMaxTasks != INDEX_NONE && State.mCompleted >= Task->mMaxTasks)
		{
			State.mRequired.Reset();
			State.mDelivered.Reset();
		}
		else if (State.mRequired.IsEmpty())
		{
			InitializeTaskRequirements(State);
		}

		else if (!SyncOneTimeTaskRequirements(State))
		{
			NormalizeTaskState(State);
		}
	}
	bTaskStateCacheValid = false;
	RebuildTaskStateCache();
	SyncAllEndlessTaskRequirements();

	TMap<UKAPIDeliveryTask*, int32> RetainedEndlessQueueEntries;
	mQueuedTasks.RemoveAll(
		[this, &RetainedEndlessQueueEntries](const TObjectPtr<UKAPIDeliveryTask>& TaskPtr)
		{
			UKAPIDeliveryTask* Task = GetValid(TaskPtr);
			if (!Task)
			{
				return true;
			}
			if (!Task->bIsEndless || Task->mMaxTasks == INDEX_NONE)
			{
				return false;
			}

			const FKPCLDeliveryTaskState* State = FindTaskState(Task);
			const int32 CompletedCount = State ? State->mCompleted : 0;
			int32& RetainedCount = RetainedEndlessQueueEntries.FindOrAdd(Task);
			if (CompletedCount + RetainedCount >= Task->mMaxTasks)
			{
				return true;
			}
			++RetainedCount;
			return false;
		});

	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mCompletedTasks, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mQueuedTasks, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mDeliveredTaskStates, this);
	OnRep_CompletedTasks(PreviousCompletedTasks);
	OnRep_QueuedTasks(PreviousQueuedTasks);
	OnRep_DeliveredTaskStates(PreviousTaskStates);
	SanitizeQueueParentRules();
	ApplyCompletedTaskUnlocks();
	RefreshManualDeliveryInventorySize();
	RefreshRewardInventorySize();
	ForceNetUpdate();
	TryCompleteActiveTask();
	ScheduleAutoCompleteScan();
	ScheduleFreeUnlockScan();
}

void AKPCLDeliveryTaskSubsystem::ApplyCompletedTaskUnlocks()
{
	if (!HasAuthority())
	{
		return;
	}

	for (const TObjectPtr<UKAPIDeliveryTask>& TaskPtr : mCompletedTasks)
	{
		if (const UKAPIDeliveryTask* Task = GetValid(TaskPtr))
		{
			Task->ApplyUnlocks(this);
		}
	}
}

bool AKPCLDeliveryTaskSubsystem::IsTaskAutoCompletable(UKAPIDeliveryTask* Task) const
{
	if (!IsResearchTaskRegistered(Task) || IsTaskCompleted(Task) || !Task->IsSchematicOnlyTask())
	{
		return false;
	}

	const AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld());
	if (!IsValid(SchematicManager))
	{
		return false;
	}

	for (const TSubclassOf<UFGSchematic>& Schematic : Task->mSchematics)
	{
		if (IsValid(Schematic) && !SchematicManager->IsSchematicPurchased(Schematic))
		{
			return false;
		}
	}

	return true;
}

bool AKPCLDeliveryTaskSubsystem::AutoCompleteTask(UKAPIDeliveryTask* Task)
{
	if (!HasAuthority() || !IsValid(Task))
	{
		return false;
	}

	const TArray<UKAPIDeliveryTask*> PreviousCompletedTasks = GetCompletedTasks();
	const TArray<UKAPIDeliveryTask*> PreviousQueuedTasks = GetQueuedTasks();
	const TArray<FKPCLDeliveryTaskState> PreviousTaskStates = mDeliveredTaskStates;

	bool bCreatedTaskState = false;
	FKPCLDeliveryTaskState* TaskState = FindOrAddTaskState(Task, bCreatedTaskState);
	if (!TaskState)
	{
		return false;
	}

	if (TaskState->mRequired.IsEmpty())
	{
		InitializeTaskRequirements(*TaskState);
	}

	for (int32 Index = 0; Index < TaskState->mRequired.Num(); ++Index)
	{
		if (TaskState->mDelivered.IsValidIndex(Index))
		{
			TaskState->mDelivered[Index].Amount = TaskState->mRequired[Index].Amount;
		}
	}
	TaskState->mCompleted = FMath::Max(1, TaskState->mCompleted);

	mCompletedTasks.AddUnique(Task);
	const int32 RemovedQueueEntries =
		mQueuedTasks.RemoveAll([Task](const TObjectPtr<UKAPIDeliveryTask>& QueuedTask) { return QueuedTask == Task; });

	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mCompletedTasks, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mDeliveredTaskStates, this);
	if (RemovedQueueEntries > 0)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mQueuedTasks, this);
	}

	Task->ForwardAdaMessages(this);
	MulticastShowTaskToast(Task);

	OnRep_CompletedTasks(PreviousCompletedTasks);
	if (RemovedQueueEntries > 0)
	{
		OnRep_QueuedTasks(PreviousQueuedTasks);
	}
	OnRep_DeliveredTaskStates(PreviousTaskStates);
	ForceNetUpdate();
	return true;
}

bool AKPCLDeliveryTaskSubsystem::ProcessAutoCompletableTasks()
{
	if (!HasAuthority() || bIsProcessingAutoCompletion)
	{
		return false;
	}

	TGuardValue<bool> AutoCompletionGuard(bIsProcessingAutoCompletion, true);
	EnsureResearchTopologyCache();

	bool bCompletedAny = false;
	for (UKAPIDeliveryTask* Task : TArray<TObjectPtr<UKAPIDeliveryTask>>(mSchematicOnlyTasks))
	{
		if (IsTaskAutoCompletable(Task) && AutoCompleteTask(Task))
		{
			bCompletedAny = true;
		}
	}

	return bCompletedAny;
}

void AKPCLDeliveryTaskSubsystem::ScheduleAutoCompleteScan()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !IsValid(World) || World->GetTimerManager().IsTimerActive(mAutoCompleteScanTimer))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		mAutoCompleteScanTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { ProcessAutoCompletableTasks(); }),
		0.01f, false);
}

bool AKPCLDeliveryTaskSubsystem::IsNoUnlockCostEnabled() const
{
	const AFGGameRulesSubsystem* GameRules = AFGGameRulesSubsystem::Get(this);
	return IsValid(GameRules) && GameRules->GetNoUnlockCost();
}

bool AKPCLDeliveryTaskSubsystem::ProcessFreeUnlockQueue()
{
	if (!HasAuthority() || bIsProcessingFreeUnlocks || !IsNoUnlockCostEnabled())
	{
		return false;
	}

	TGuardValue<bool> FreeUnlockGuard(bIsProcessingFreeUnlocks, true);

	bool bCompletedAny = false;
	for (int32 RemainingEntries = mQueuedTasks.Num(); RemainingEntries > 0; --RemainingEntries)
	{

		UKAPIDeliveryTask* Task = GetActiveTask();
		FKPCLDeliveryTaskState* TaskState = FindTaskState(Task);
		if (!TaskState)
		{
			break;
		}

		if (TaskState->mRequired.IsEmpty())
		{
			InitializeTaskRequirements(*TaskState);
		}

		const TArray<FKPCLDeliveryTaskState> PreviousTaskStates = mDeliveredTaskStates;

		for (int32 Index = 0; Index < TaskState->mRequired.Num(); ++Index)
		{
			if (TaskState->mDelivered.IsValidIndex(Index))
			{
				TaskState->mDelivered[Index].Amount = TaskState->mRequired[Index].Amount;
			}
		}

		if (!TriggerTaskCompletion(TaskState))
		{

			MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mDeliveredTaskStates, this);
			OnRep_DeliveredTaskStates(PreviousTaskStates);
			ForceNetUpdate();
			break;
		}
		bCompletedAny = true;
	}

	return bCompletedAny;
}

void AKPCLDeliveryTaskSubsystem::ScheduleFreeUnlockScan()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !IsValid(World) || World->GetTimerManager().IsTimerActive(mFreeUnlockScanTimer))
	{
		return;
	}

	World->GetTimerManager().SetTimer(mFreeUnlockScanTimer,
									  FTimerDelegate::CreateWeakLambda(this, [this]() { ProcessFreeUnlockQueue(); }),
									  0.01f, false);
}

void AKPCLDeliveryTaskSubsystem::AddManualDeliveryInventorySlots(int32 NumSlots)
{
	if (!HasAuthority() || NumSlots <= 0)
	{
		return;
	}

	const int64 NewSlotCount = static_cast<int64>(GetManualDeliveryInventorySlotCount()) + static_cast<int64>(NumSlots);
	mManualDeliveryInventorySlotCount = static_cast<int32>(FMath::Min<int64>(NewSlotCount, MAX_int32));
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mManualDeliveryInventorySlotCount, this);
	RefreshManualDeliveryInventorySize();
	ForceNetUpdate();
}

void AKPCLDeliveryTaskSubsystem::AddRewardInventorySlots(int32 NumSlots)
{
	if (!HasAuthority() || NumSlots <= 0)
	{
		return;
	}

	const int32 PreGrantedSlots = FMath::Min(NumSlots, FMath::Max(0, mPreGrantedRewardInventorySlots));
	mPreGrantedRewardInventorySlots -= PreGrantedSlots;
	const int32 SlotsToGrant = NumSlots - PreGrantedSlots;
	if (SlotsToGrant > 0)
	{
		const int64 NewSlotCount = static_cast<int64>(GetRewardInventorySlotCount()) + static_cast<int64>(SlotsToGrant);
		mRewardInventorySlotCount = static_cast<int32>(FMath::Min<int64>(NewSlotCount, MAX_int32));
		MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mRewardInventorySlotCount, this);
		RefreshRewardInventorySize();
		ForceNetUpdate();
	}
}

void AKPCLDeliveryTaskSubsystem::AddQueueSlots(int32 NumSlots)
{
	if (!HasAuthority() || NumSlots <= 0)
	{
		return;
	}

	const int64 NewQueueLimit = static_cast<int64>(GetQueueLimit()) + static_cast<int64>(NumSlots);
	mQueueLimit = static_cast<int32>(FMath::Min<int64>(NewQueueLimit, MAX_int32));
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mQueueLimit, this);
	mOnQueueChanged.Broadcast();
	ForceNetUpdate();
}

bool AKPCLDeliveryTaskSubsystem::TryToDeliverAmounts(const TArray<FItemAmount>& AmountsToDeliver,
													 TArray<FItemAmount>& DeliveredAmounts)
{
	DeliveredAmounts.Reset();
	if (!HasAuthority() || !ensureMsgf(IsInGameThread(), TEXT("Delivery task state must mutate on the game thread")))
	{
		return false;
	}

	const AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld());
	if (!IsValid(UnlockSubsystem) || !UnlockSubsystem->GetIsDeliveryTaskSystemUnlocked())
	{
		return false;
	}

	TMap<TSubclassOf<UFGItemDescriptor>, int32> AvailableAmounts;
	for (const FItemAmount& OfferedAmount : AmountsToDeliver)
	{
		if (IsValid(OfferedAmount.ItemClass) && OfferedAmount.Amount > 0)
		{
			int32& AvailableAmount = AvailableAmounts.FindOrAdd(OfferedAmount.ItemClass);
			AvailableAmount = static_cast<int32>(FMath::Min<int64>(
				static_cast<int64>(AvailableAmount) + static_cast<int64>(OfferedAmount.Amount), MAX_int32));
		}
	}

	if (AvailableAmounts.IsEmpty())
	{
		return false;
	}

	bool bDeliveredAny = false;
	bool bPendingStateNotification = false;
	TArray<FKPCLDeliveryTaskState> PreviousTaskStates = mDeliveredTaskStates;

	while (!mQueuedTasks.IsEmpty())
	{
		UKAPIDeliveryTask* Task = GetValid(mQueuedTasks[0]);
		if (!Task)
		{
			const TArray<UKAPIDeliveryTask*> PreviousQueuedTasks = GetQueuedTasks();
			mQueuedTasks.RemoveAt(0);
			MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mQueuedTasks, this);
			OnRep_QueuedTasks(PreviousQueuedTasks);
			continue;
		}

		bool bCreatedTaskState = false;
		FKPCLDeliveryTaskState* TaskState = FindOrAddTaskState(Task, bCreatedTaskState);
		if (!TaskState)
		{
			break;
		}
		bPendingStateNotification |= bCreatedTaskState;

		bool bTaskProgressed = false;
		for (int32 RequiredIndex = 0; RequiredIndex < TaskState->mRequired.Num(); ++RequiredIndex)
		{
			const FItemAmount& RequiredAmount = TaskState->mRequired[RequiredIndex];
			if (!IsValid(RequiredAmount.ItemClass) || RequiredAmount.Amount <= 0 ||
				!TaskState->mDelivered.IsValidIndex(RequiredIndex))
			{
				continue;
			}

			FItemAmount& CurrentDelivered = TaskState->mDelivered[RequiredIndex];
			int32* AvailableAmount = AvailableAmounts.Find(RequiredAmount.ItemClass);
			const int32 MissingAmount = FMath::Max(0, RequiredAmount.Amount - CurrentDelivered.Amount);
			const int32 AmountToDeliver = AvailableAmount ? FMath::Min(*AvailableAmount, MissingAmount) : 0;
			if (AmountToDeliver <= 0)
			{
				continue;
			}

			CurrentDelivered.Amount += AmountToDeliver;
			*AvailableAmount -= AmountToDeliver;
			AddItemAmount(DeliveredAmounts, RequiredAmount.ItemClass, AmountToDeliver);
			bTaskProgressed = true;
			bDeliveredAny = true;
		}

		bPendingStateNotification |= bTaskProgressed;
		if (!IsTaskStateComplete(*TaskState))
		{

			break;
		}

		if (!TriggerTaskCompletion(TaskState))
		{
			break;
		}
		bPendingStateNotification = false;
		PreviousTaskStates = mDeliveredTaskStates;

		bool bHasRemainingItems = false;
		for (const TPair<TSubclassOf<UFGItemDescriptor>, int32>& Pair : AvailableAmounts)
		{
			if (Pair.Value > 0)
			{
				bHasRemainingItems = true;
				break;
			}
		}
		if (!bHasRemainingItems)
		{
			break;
		}
	}

	if (bPendingStateNotification)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mDeliveredTaskStates, this);
		OnRep_DeliveredTaskStates(PreviousTaskStates);
		ForceNetUpdate();
	}

	return bDeliveredAny;
}

bool AKPCLDeliveryTaskSubsystem::TriggerTaskCompletion(FKPCLDeliveryTaskState* TaskState)
{
	if (!HasAuthority() || !TaskState || TaskState->mCompleted == MAX_int32 || bIsProcessingTaskCompletion ||
		!IsTaskStateComplete(*TaskState))
	{
		return false;
	}

	UKAPIDeliveryTask* Task = GetValid(TaskState->mTask);
	if (!Task || GetActiveTask() != Task)
	{
		return false;
	}

	TGuardValue<bool> CompletionGuard(bIsProcessingTaskCompletion, true);
	if (!ensureMsgf(mPreGrantedRewardInventorySlots == 0,
					TEXT("A previous delivery task left explicit reward inventory slots pending")))
	{
		mPreGrantedRewardInventorySlots = 0;
	}
	FText RewardErrorText;
	if (!TryStoreTaskRewards(Task, TaskState->mCompleted, RewardErrorText))
	{
		SetRewardErrorText(RewardErrorText);
		StartTaskCompletionRetryTimer();
		return false;
	}
	StopTaskCompletionRetryTimer();
	SetRewardErrorText(FText::GetEmpty());

	const TArray<UKAPIDeliveryTask*> PreviousCompletedTasks = GetCompletedTasks();
	const TArray<UKAPIDeliveryTask*> PreviousQueuedTasks = GetQueuedTasks();
	const TArray<FKPCLDeliveryTaskState> PreviousTaskStates = mDeliveredTaskStates;

	++TaskState->mCompleted;
	UE_LOG(LogKPCL, Display, TEXT("Completed delivery task '%s' (completion %d)."), *Task->GetPathName(),
		   TaskState->mCompleted);
	const bool bAddedToCompletedTasks = !mCompletedTaskCache.Contains(Task);
	mCompletedTasks.AddUnique(Task);
	const int32 QueueIndex = mQueuedTasks.Find(Task);
	if (QueueIndex != INDEX_NONE)
	{
		mQueuedTasks.RemoveAt(QueueIndex);
	}

	const bool bEndlessLimitReached =
		Task->bIsEndless && Task->mMaxTasks != INDEX_NONE && TaskState->mCompleted >= Task->mMaxTasks;
	if (Task->bIsEndless && !bEndlessLimitReached)
	{
		InitializeTaskRequirements(*TaskState);
	}
	else if (bEndlessLimitReached)
	{
		TaskState->mRequired.Reset();
		TaskState->mDelivered.Reset();
		mQueuedTasks.RemoveAll([Task](const TObjectPtr<UKAPIDeliveryTask>& QueuedTask) { return QueuedTask == Task; });
	}
	if (bAddedToCompletedTasks)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mCompletedTasks, this);
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mQueuedTasks, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mDeliveredTaskStates, this);

	Task->HandleUnlock(this);
	if (!ensureMsgf(mPreGrantedRewardInventorySlots == 0,
					TEXT("Delivery task '%s' did not consume all pre-granted reward inventory slots"),
					*Task->GetPathName()))
	{
		mPreGrantedRewardInventorySlots = 0;
	}
	MulticastShowTaskToast(Task);

	if (bAddedToCompletedTasks)
	{
		OnRep_CompletedTasks(PreviousCompletedTasks);
	}
	if (QueueIndex != INDEX_NONE)
	{
		OnRep_QueuedTasks(PreviousQueuedTasks);
	}
	OnRep_DeliveredTaskStates(PreviousTaskStates);
	ForceNetUpdate();

	ProcessAutoCompletableTasks();
	return true;
}

void AKPCLDeliveryTaskSubsystem::MulticastShowTaskToast_Implementation(UKAPIDeliveryTask* Task) { ShowTaskToast(Task); }

void AKPCLDeliveryTaskSubsystem::ShowTaskToast(const UKAPIDeliveryTask* Task) const
{
	if (!IsValid(Task) || Task->mToastText.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	AFGPlayerController* PlayerController =
		IsValid(World) ? Cast<AFGPlayerController>(World->GetFirstPlayerController()) : nullptr;
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
	{
		return;
	}

	UFGGameUI* GameUI = PlayerController->GetGameUI();
	if (!IsValid(GameUI))
	{
		return;
	}

	static const TCHAR* NotificationClassPath =
		TEXT("/Game/FactoryGame/Interface/UI/InGame/HUDNotifications/BPW_HUDNotification_Default."
			 "BPW_HUDNotification_Default_C");
	UClass* NotificationClass = LoadClass<UFGPushNotificationWidget>(nullptr, NotificationClassPath);
	UFGPushNotificationWidget* Notification = IsValid(NotificationClass)
		? CreateWidget<UFGPushNotificationWidget>(PlayerController, NotificationClass)
		: nullptr;
	if (!IsValid(Notification) || !CallTextSetter(Notification, TEXT("SetTitle"), Task->mName) ||
		!CallTextSetter(Notification, TEXT("SetDescription"), Task->mToastText))
	{
		GameUI->ShowTextNotification(Task->mToastText);
		return;
	}

	UTexture2D* ToastIcon = IsValid(Task->mToastIcon) ? Task->mToastIcon.Get() : Task->mIcon.Get();
	CallIconSetter(Notification, ToastIcon);
	GameUI->PushNotificationWidget(Notification);
}

bool AKPCLDeliveryTaskSubsystem::CanQueueTask(UKAPIDeliveryTask* Task, TArray<FText>& ErrorMessages) const
{
	ErrorMessages.Reset();
	const AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld());
	if (!IsValid(UnlockSubsystem) || !UnlockSubsystem->GetIsDeliveryTaskSystemUnlocked())
	{
		ErrorMessages.Add(NSLOCTEXT("KPCLDeliveryTaskSubsystem", "DeliveryTaskSystemLocked",
									"Unlock the delivery research system before queueing tasks."));
		return false;
	}

	if (!IsValid(Task))
	{
		ErrorMessages.Add(NSLOCTEXT("KPCLDeliveryTaskSubsystem", "InvalidTask", "The selected task is invalid."));
		return false;
	}

	if (!IsResearchTaskRegistered(Task))
	{
		ErrorMessages.Add(NSLOCTEXT("KPCLDeliveryTaskSubsystem", "TaskNotRegistered",
									"This research task is disabled or not registered on the server."));
		return false;
	}

	if (!Task->AreTaskDependenciesMet(GetWorld()))
	{
		ErrorMessages.Add(NSLOCTEXT("KPCLDeliveryTaskSubsystem", "TaskDependenciesNotMet",
									"This task's availability requirements are not met."));
		return false;
	}

	if (mQueuedTasks.Num() >= GetQueueLimit())
	{
		ErrorMessages.Add(NSLOCTEXT("KPCLDeliveryTaskSubsystem", "QueueLimitReached",
									"You have reached the maximum number of queued tasks."));
		return false;
	}

	if (!Task->bIsEndless && mCompletedTaskCache.Contains(Task))
	{
		ErrorMessages.Add(
			NSLOCTEXT("KPCLDeliveryTaskSubsystem", "TaskAlreadyCompleted", "This task has already been completed."));
		return false;
	}

	if (!Task->bIsEndless && mQueueIndicesByTask.Contains(Task))
	{
		ErrorMessages.Add(NSLOCTEXT("KPCLDeliveryTaskSubsystem", "TaskAlreadyQueued", "This task is already queued."));
		return false;
	}

	if (Task->bIsEndless && Task->mMaxTasks != INDEX_NONE)
	{
		const FKPCLDeliveryTaskState* State = FindTaskState(Task);
		const int32 CompletedCount = State ? State->mCompleted : 0;
		const TArray<int32>* QueueIndices = mQueueIndicesByTask.Find(Task);
		const int32 QueuedCount = QueueIndices ? QueueIndices->Num() : 0;
		if (CompletedCount + QueuedCount >= Task->mMaxTasks)
		{
			ErrorMessages.Add(NSLOCTEXT("KPCLDeliveryTaskSubsystem", "EndlessTaskLimitReached",
										"This repeatable task has reached its completion limit."));
			return false;
		}
	}

	if (!AreTaskParentCompletionsMet(Task))
	{
		const bool bHasRepeatedParentRequirement =
			GetResearchParents(Task).ContainsByPredicate(
				[Task](UKAPIDeliveryTask* Parent)
				{ return GetEffectiveParentCompletionRequirement(Task, Parent) > 1; });
		if (bHasRepeatedParentRequirement)
		{
			ErrorMessages.Add(
				Task->bRequireAllParents
					? NSLOCTEXT("KPCLDeliveryTaskSubsystem", "AllTaskParentCountsMissing",
								"Complete every parent task the required number of times before this task.")
					: NSLOCTEXT("KPCLDeliveryTaskSubsystem", "AnyTaskParentCountMissing",
								"Complete at least one parent task the required number of times before this task."));
		}
		else
		{
			ErrorMessages.Add(Task->bRequireAllParents
								  ? NSLOCTEXT("KPCLDeliveryTaskSubsystem", "AllTaskParentsMissing",
											  "Complete every parent task before this task.")
								  : NSLOCTEXT("KPCLDeliveryTaskSubsystem", "AnyTaskParentMissing",
											  "Complete at least one parent task before this task."));
		}
		return false;
	}

	return true;
}

FKPCLDeliveryTaskState* AKPCLDeliveryTaskSubsystem::FindTaskState(const UKAPIDeliveryTask* Task)
{
	if (!bTaskStateCacheValid)
	{
		RebuildTaskStateCache();
	}
	const int32* StateIndex = mTaskStateIndexByTask.Find(Task);
	return StateIndex && mDeliveredTaskStates.IsValidIndex(*StateIndex) ? &mDeliveredTaskStates[*StateIndex] : nullptr;
}

const FKPCLDeliveryTaskState* AKPCLDeliveryTaskSubsystem::FindTaskState(const UKAPIDeliveryTask* Task) const
{
	if (!bTaskStateCacheValid)
	{
		const_cast<AKPCLDeliveryTaskSubsystem*>(this)->RebuildTaskStateCache();
	}
	const int32* StateIndex = mTaskStateIndexByTask.Find(Task);
	return StateIndex && mDeliveredTaskStates.IsValidIndex(*StateIndex) ? &mDeliveredTaskStates[*StateIndex] : nullptr;
}

FKPCLDeliveryTaskState* AKPCLDeliveryTaskSubsystem::FindOrAddTaskState(UKAPIDeliveryTask* Task, bool& bOutCreated)
{
	bOutCreated = false;
	if (FKPCLDeliveryTaskState* ExistingState = FindTaskState(Task))
	{
		return ExistingState;
	}

	if (!IsValid(Task) || !HasAuthority())
	{
		return nullptr;
	}

	FKPCLDeliveryTaskState& NewState = mDeliveredTaskStates.AddDefaulted_GetRef();
	NewState.mTask = Task;
	InitializeTaskRequirements(NewState);
	bTaskStateCacheValid = false;
	RebuildTaskStateCache();
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mDeliveredTaskStates, this);
	bOutCreated = true;
	return FindTaskState(Task);
}

bool AKPCLDeliveryTaskSubsystem::BuildPreviewTaskState(UKAPIDeliveryTask* Task, FKPCLDeliveryTaskState& OutState) const
{
	OutState = FKPCLDeliveryTaskState();
	if (!IsValid(Task) || Task->bIsEndless)
	{
		return false;
	}

	OutState.mTask = Task;
	OutState.mRequired = Task->mCosts;
	NormalizeTaskState(OutState);
	OutState.mCompleted = mCompletedTaskCache.Contains(Task) ? 1 : 0;
	return true;
}

bool AKPCLDeliveryTaskSubsystem::GetOrCreateTaskState(UKAPIDeliveryTask* Task, FKPCLDeliveryTaskState& OutState)
{
	OutState = FKPCLDeliveryTaskState();
	if (!IsValid(Task))
	{
		return false;
	}

	if (const FKPCLDeliveryTaskState* ExistingState = FindTaskState(Task))
	{
		OutState = *ExistingState;
		return true;
	}

	if (IsResearchTaskRegistered(Task))
	{
		const TArray<FKPCLDeliveryTaskState> PreviousTaskStates = mDeliveredTaskStates;
		bool bCreated = false;
		if (const FKPCLDeliveryTaskState* NewState = FindOrAddTaskState(Task, bCreated))
		{
			OutState = *NewState;
			if (bCreated)
			{
				OnRep_DeliveredTaskStates(PreviousTaskStates);
				ForceNetUpdate();
			}
			return true;
		}
	}

	return BuildPreviewTaskState(Task, OutState);
}

void AKPCLDeliveryTaskSubsystem::RebuildTaskStateCache()
{
	mTaskStateIndexByTask.Reset();
	mTaskStateIndexByTask.Reserve(mDeliveredTaskStates.Num());
	for (int32 StateIndex = 0; StateIndex < mDeliveredTaskStates.Num(); ++StateIndex)
	{
		if (UKAPIDeliveryTask* Task = GetValid(mDeliveredTaskStates[StateIndex].mTask))
		{
			mTaskStateIndexByTask.FindOrAdd(Task, StateIndex);
		}
	}
	bTaskStateCacheValid = true;
}

void AKPCLDeliveryTaskSubsystem::RebuildQueueCache()
{
	mQueueIndicesByTask.Reset();
	mQueueIndicesByTask.Reserve(mQueuedTasks.Num());
	mCachedActiveTask = nullptr;
	for (int32 QueueIndex = 0; QueueIndex < mQueuedTasks.Num(); ++QueueIndex)
	{
		if (UKAPIDeliveryTask* Task = GetValid(mQueuedTasks[QueueIndex]))
		{
			mQueueIndicesByTask.FindOrAdd(Task).Add(QueueIndex);
			if (!IsValid(mCachedActiveTask))
			{
				mCachedActiveTask = Task;
			}
		}
	}
}

void AKPCLDeliveryTaskSubsystem::RebuildCompletedTaskCache()
{
	mCompletedTaskCache.Reset();
	mCompletedTaskCache.Reserve(mCompletedTasks.Num());
	for (const TObjectPtr<UKAPIDeliveryTask>& TaskPtr : mCompletedTasks)
	{
		if (UKAPIDeliveryTask* Task = GetValid(TaskPtr))
		{
			mCompletedTaskCache.Add(Task);
		}
	}
}

void AKPCLDeliveryTaskSubsystem::EnsureResearchTopologyCache() const
{
	UKAPIDataAssetSubsystem* DataSubsystem = UKAPIDataAssetSubsystem::Get(GetWorld());
	const uint32 Revision = IsValid(DataSubsystem) ? DataSubsystem->Delivery_GetRevision() : MAX_uint32;
	if (mCachedDataAssetSubsystem.Get() != DataSubsystem || mCachedDeliveryTaskRevision != Revision)
	{
		RebuildResearchTopologyCache();
	}
}

void AKPCLDeliveryTaskSubsystem::RebuildResearchTopologyCache() const
{
	mSortedResearchTasks.Reset();
	mSchematicOnlyTasks.Reset();
	mResearchParentsByTask.Reset();
	mResearchChildrenByTask.Reset();
	mCachedTreeMinCoordinate = FIntVector2::ZeroValue;
	mCachedTreeMaxCoordinate = FIntVector2::ZeroValue;
	bHasCachedTreeBounds = false;

	UKAPIDataAssetSubsystem* DataSubsystem = UKAPIDataAssetSubsystem::Get(GetWorld());
	mCachedDataAssetSubsystem = DataSubsystem;
	mCachedDeliveryTaskRevision = IsValid(DataSubsystem) ? DataSubsystem->Delivery_GetRevision() : MAX_uint32;
	if (!IsValid(DataSubsystem))
	{
		return;
	}

	const TMap<FIntVector2, TObjectPtr<UKAPIDeliveryTask>>& Tasks = DataSubsystem->Delivery_GetAllView();
	mSortedResearchTasks.Reserve(Tasks.Num());
	mResearchParentsByTask.Reserve(Tasks.Num());
	mResearchChildrenByTask.Reserve(Tasks.Num());
	for (const TPair<FIntVector2, TObjectPtr<UKAPIDeliveryTask>>& Pair : Tasks)
	{
		if (UKAPIDeliveryTask* Task = GetValid(Pair.Value))
		{
			mSortedResearchTasks.Add(Task);

			if (Task->IsSchematicOnlyTask())
			{
				mSchematicOnlyTasks.Add(Task);
			}
			mResearchParentsByTask.FindOrAdd(Task);
			mResearchChildrenByTask.FindOrAdd(Task);
		}
	}
	mSortedResearchTasks.Sort(&UKAPIDeliveryTask::SortByCoordinate);

	for (UKAPIDeliveryTask* Child : mSortedResearchTasks)
	{
		TArray<TObjectPtr<UKAPIDeliveryTask>>& Parents = mResearchParentsByTask.FindOrAdd(Child);
		for (const FIntVector2& ParentCoordinate : Child->GetParentCoordinates())
		{
			const TObjectPtr<UKAPIDeliveryTask>* ParentPtr = Tasks.Find(ParentCoordinate);
			UKAPIDeliveryTask* Parent = ParentPtr ? GetValid(*ParentPtr) : nullptr;
			if (!Parent)
			{
				UE_LOG(LogKPCL, Warning,
					   TEXT("Delivery task '%s' references invalid optional parent coordinate (%d, %d) while building "
							"the research tree; parent ignored."),
					   *Child->GetPathName(), ParentCoordinate.X, ParentCoordinate.Y);
				continue;
			}
			Parents.Add(Parent);
			mResearchChildrenByTask.FindOrAdd(Parent).Add(Child);
		}
	}

	for (TPair<UKAPIDeliveryTask*, TArray<TObjectPtr<UKAPIDeliveryTask>>>& Pair : mResearchChildrenByTask)
	{
		Pair.Value.Sort(&UKAPIDeliveryTask::SortByCoordinate);
	}

	if (!mSortedResearchTasks.IsEmpty())
	{
		mCachedTreeMinCoordinate = mSortedResearchTasks[0]->mCoordinate;
		mCachedTreeMaxCoordinate = mCachedTreeMinCoordinate;
		for (UKAPIDeliveryTask* Task : mSortedResearchTasks)
		{
			mCachedTreeMinCoordinate.X = FMath::Min(mCachedTreeMinCoordinate.X, Task->mCoordinate.X);
			mCachedTreeMinCoordinate.Y = FMath::Min(mCachedTreeMinCoordinate.Y, Task->mCoordinate.Y);
			mCachedTreeMaxCoordinate.X = FMath::Max(mCachedTreeMaxCoordinate.X, Task->mCoordinate.X);
			mCachedTreeMaxCoordinate.Y = FMath::Max(mCachedTreeMaxCoordinate.Y, Task->mCoordinate.Y);
		}
		bHasCachedTreeBounds = true;
	}
}

void AKPCLDeliveryTaskSubsystem::InvalidateResearchTopologyCache()
{
	mCachedDataAssetSubsystem.Reset();
	mCachedDeliveryTaskRevision = MAX_uint32;
}

void AKPCLDeliveryTaskSubsystem::HandleDeliveryTasksChanged()
{
	InvalidateResearchTopologyCache();
	EnsureResearchTopologyCache();
	TArray<FKPCLDeliveryTaskState> PreviousTaskStates;
	bool bInitializedRequirements = false;
	if (HasAuthority())
	{
		PreviousTaskStates = mDeliveredTaskStates;
		bInitializedRequirements = SyncAllEndlessTaskRequirements();

		bool bSyncedRequirements = false;
		for (FKPCLDeliveryTaskState& TaskState : mDeliveredTaskStates)
		{
			bSyncedRequirements |= SyncOneTimeTaskRequirements(TaskState);
		}
		bInitializedRequirements |= bSyncedRequirements;
		SanitizeQueueParentRules();
		ProcessAutoCompletableTasks();
	}
	mOnResearchTreeChanged.Broadcast();
	if (bInitializedRequirements)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mDeliveredTaskStates, this);
		OnRep_DeliveredTaskStates(PreviousTaskStates);
		ForceNetUpdate();
		TryConsumeManualDeliveryInventory();
		TryCompleteActiveTask();
	}
}

void AKPCLDeliveryTaskSubsystem::HandleDeliveryTaskSystemUnlockChanged(bool)
{
	BroadcastAllResearchAvailabilityChanged();
}

void AKPCLDeliveryTaskSubsystem::HandleSchematicUnlocked(TSubclassOf<UFGSchematic>)
{

	ProcessAutoCompletableTasks();
	BroadcastAllResearchAvailabilityChanged();
}

void AKPCLDeliveryTaskSubsystem::BroadcastAllResearchAvailabilityChanged()
{
	EnsureResearchTopologyCache();
	for (UKAPIDeliveryTask* Task : mSortedResearchTasks)
	{
		BroadcastResearchNodeChanged(Task);
		TArray<FText> QueueErrors;
		mOnResearchAvailabilityChanged.Broadcast(Task, CanQueueTask(Task, QueueErrors));
	}
}

bool AKPCLDeliveryTaskSubsystem::SyncAllEndlessTaskRequirements()
{
	if (!HasAuthority())
	{
		return false;
	}

	EnsureResearchTopologyCache();
	bool bChanged = false;
	bool bAddedTaskState = false;
	for (UKAPIDeliveryTask* Task : mSortedResearchTasks)
	{
		if (!IsValid(Task) || !Task->bIsEndless)
		{
			continue;
		}

		FKPCLDeliveryTaskState* TaskState = FindTaskState(Task);
		if (!TaskState)
		{
			TaskState = &mDeliveredTaskStates.AddDefaulted_GetRef();
			TaskState->mTask = Task;
			bAddedTaskState = true;
		}

		bChanged |= SyncEndlessTaskRequirements(*TaskState);
	}

	if (bAddedTaskState)
	{
		bTaskStateCacheValid = false;
		RebuildTaskStateCache();
	}
	return bChanged || bAddedTaskState;
}

bool AKPCLDeliveryTaskSubsystem::SyncEndlessTaskRequirements(FKPCLDeliveryTaskState& TaskState) const
{
	UKAPIDeliveryTask* Task = GetValid(TaskState.mTask);
	if (!Task || !Task->bIsEndless)
	{
		return false;
	}

	if (Task->mMaxTasks != INDEX_NONE && TaskState.mCompleted >= Task->mMaxTasks)
	{
		if (TaskState.mRequired.IsEmpty() && TaskState.mDelivered.IsEmpty())
		{
			return false;
		}
		TaskState.mRequired.Reset();
		TaskState.mDelivered.Reset();
		return true;
	}

	TSet<TSubclassOf<UFGItemDescriptor>> EligibleItems;
	for (const FKAPIItemPoolElement& PoolElement : Task->mPoolElements)
	{
		if (!PoolElement.Valid() ||
			(PoolElement.mRequiredTas != INDEX_NONE && PoolElement.mRequiredTas > TaskState.mCompleted))
		{
			continue;
		}

		const EResourceForm ResourceForm = UFGItemDescriptor::GetForm(PoolElement.mItem);
		const TObjectPtr<UCurveFloat>* MaxAmountCurve = Task->mAbsolutMaxItemAmount.Find(ResourceForm);
		if (MaxAmountCurve && IsValid(*MaxAmountCurve))
		{
			EligibleItems.Add(PoolElement.mItem);
		}
	}

	const int32 ExpectedItemCount =
		Task->mMaxPoolItems == INDEX_NONE ? EligibleItems.Num() : FMath::Min(Task->mMaxPoolItems, EligibleItems.Num());
	FKPCLDeliveryTaskState SyncedState = TaskState;
	NormalizeTaskState(SyncedState);
	bool bContainsOnlyEligibleItems = true;
	for (const FItemAmount& RequiredAmount : SyncedState.mRequired)
	{
		if (!EligibleItems.Contains(RequiredAmount.ItemClass))
		{
			bContainsOnlyEligibleItems = false;
			break;
		}
	}

	if (ShouldRefreshEndlessTaskRequirements(SyncedState.mRequired.Num(), ExpectedItemCount,
											 bContainsOnlyEligibleItems))
	{
		const int32 PreviousItemCount = SyncedState.mRequired.Num();
		SyncedState.mRequired.Reset();
		Task->GetPoolElementsAmount(SyncedState.mRequired, SyncedState.mCompleted);
		NormalizeTaskState(SyncedState);
		UE_LOG(
			LogKPCL, Display,
			TEXT("Refreshed endless delivery task requirements for '%s' from %d to %d items after its pool changed."),
			*Task->GetPathName(), PreviousItemCount, SyncedState.mRequired.Num());
	}

	if (HasSameItemAmounts(SyncedState.mRequired, TaskState.mRequired) &&
		HasSameItemAmounts(SyncedState.mDelivered, TaskState.mDelivered))
	{
		return false;
	}

	TaskState.mRequired = MoveTemp(SyncedState.mRequired);
	TaskState.mDelivered = MoveTemp(SyncedState.mDelivered);
	return true;
}

bool AKPCLDeliveryTaskSubsystem::SyncOneTimeTaskRequirements(FKPCLDeliveryTaskState& TaskState) const
{
	UKAPIDeliveryTask* Task = GetValid(TaskState.mTask);
	if (!Task || Task->bIsEndless)
	{
		return false;
	}

	FKPCLDeliveryTaskState SyncedState = TaskState;
	SyncedState.mRequired = Task->mCosts;

	NormalizeTaskState(SyncedState);
	if (SyncedState.mCompleted > 0)
	{

		for (int32 Index = 0; Index < SyncedState.mRequired.Num(); ++Index)
		{
			SyncedState.mDelivered[Index].Amount = SyncedState.mRequired[Index].Amount;
		}
	}

	if (HasSameItemAmounts(SyncedState.mRequired, TaskState.mRequired) &&
		HasSameItemAmounts(SyncedState.mDelivered, TaskState.mDelivered))
	{
		return false;
	}

	UE_LOG(LogKPCL, Display, TEXT("Refreshed one-time delivery task requirements for '%s' after its costs changed."),
		   *Task->GetPathName());
	TaskState.mRequired = MoveTemp(SyncedState.mRequired);
	TaskState.mDelivered = MoveTemp(SyncedState.mDelivered);
	return true;
}

void AKPCLDeliveryTaskSubsystem::InitializeTaskRequirements(FKPCLDeliveryTaskState& TaskState) const
{
	TaskState.mRequired.Reset();
	TaskState.mDelivered.Reset();

	UKAPIDeliveryTask* Task = GetValid(TaskState.mTask);
	if (!Task)
	{
		return;
	}

	if (Task->bIsEndless)
	{
		Task->GetPoolElementsAmount(TaskState.mRequired, TaskState.mCompleted);
	}
	else
	{
		TaskState.mRequired = Task->mCosts;
	}

	NormalizeTaskState(TaskState);
}

void AKPCLDeliveryTaskSubsystem::NormalizeTaskState(FKPCLDeliveryTaskState& TaskState) const
{
	TArray<FItemAmount> NormalizedRequired;
	for (const FItemAmount& RequiredAmount : TaskState.mRequired)
	{
		AddItemAmount(NormalizedRequired, RequiredAmount.ItemClass, RequiredAmount.Amount);
	}

	TArray<FItemAmount> NormalizedDelivered;
	for (const FItemAmount& DeliveredAmount : TaskState.mDelivered)
	{
		AddItemAmount(NormalizedDelivered, DeliveredAmount.ItemClass, DeliveredAmount.Amount);
	}

	TaskState.mRequired = MoveTemp(NormalizedRequired);
	TaskState.mDelivered.Reset(TaskState.mRequired.Num());
	for (const FItemAmount& RequiredAmount : TaskState.mRequired)
	{
		const FItemAmount* ExistingDelivered =
			NormalizedDelivered.FindByPredicate([&RequiredAmount](const FItemAmount& Candidate)
												{ return Candidate.ItemClass == RequiredAmount.ItemClass; });
		TaskState.mDelivered.Emplace(
			RequiredAmount.ItemClass,
			ExistingDelivered ? FMath::Clamp(ExistingDelivered->Amount, 0, RequiredAmount.Amount) : 0);
	}
}

bool AKPCLDeliveryTaskSubsystem::IsTaskStateComplete(const FKPCLDeliveryTaskState& TaskState) const
{
	if (TaskState.mRequired.IsEmpty() || TaskState.mRequired.Num() != TaskState.mDelivered.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < TaskState.mRequired.Num(); ++Index)
	{
		const FItemAmount& RequiredAmount = TaskState.mRequired[Index];
		const FItemAmount& DeliveredAmount = TaskState.mDelivered[Index];
		if (!IsValid(RequiredAmount.ItemClass) || RequiredAmount.Amount <= 0 ||
			DeliveredAmount.ItemClass != RequiredAmount.ItemClass || DeliveredAmount.Amount < RequiredAmount.Amount)
		{
			return false;
		}
	}

	return true;
}

bool AKPCLDeliveryTaskSubsystem::TryToQueueTask(UKAPIDeliveryTask* Task, TArray<FText>& ErrorMessages)
{
	ErrorMessages.Reset();
	if (!CanQueueTask(Task, ErrorMessages))
	{
		return false;
	}

	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this))
		{
			RCO->Server_DeliveryTask_TryToQueue(this, Task);
			return true;
		}

		ErrorMessages.Add(NSLOCTEXT("KPCLDeliveryTaskSubsystem", "ServerUnavailable",
									"The server could not be reached. Please try again."));
		return false;
	}

	const TArray<UKAPIDeliveryTask*> PreviousQueuedTasks = GetQueuedTasks();
	const TArray<FKPCLDeliveryTaskState> PreviousTaskStates = mDeliveredTaskStates;

	bool bAddedTaskState = false;
	FindOrAddTaskState(Task, bAddedTaskState);

	if (Task->bIsEndless)
	{
		mQueuedTasks.Add(Task);
	}
	else
	{
		mQueuedTasks.AddUnique(Task);
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mQueuedTasks, this);

	if (bAddedTaskState)
	{
		OnRep_DeliveredTaskStates(PreviousTaskStates);
	}
	OnRep_QueuedTasks(PreviousQueuedTasks);
	ForceNetUpdate();

	ScheduleFreeUnlockScan();
	return true;
}

bool AKPCLDeliveryTaskSubsystem::TryToRemoveFromQueue(UKAPIDeliveryTask* Task)
{
	if (!IsValid(Task) || mQueuedTasks.IsEmpty() || mQueuedTasks.Last() != Task)
	{
		return false;
	}
	return TryToRemoveFromQueueAt(mQueuedTasks.Num() - 1);
}

UKAPIDeliveryTask*
AKPCLDeliveryTaskSubsystem::ValidateQueueMutation(int32 QueueIndex,
												  TFunctionRef<bool(TArray<UKAPIDeliveryTask*>&)> MutateCandidate) const
{
	if (!mQueuedTasks.IsValidIndex(QueueIndex))
	{
		return nullptr;
	}

	UKAPIDeliveryTask* Task = GetValid(mQueuedTasks[QueueIndex]);
	if (!Task)
	{
		return nullptr;
	}

	TArray<UKAPIDeliveryTask*> CandidateQueue = GetQueuedTasks();
	if (!CandidateQueue.IsValidIndex(QueueIndex) || !MutateCandidate(CandidateQueue))
	{
		return nullptr;
	}

	return IsQueueParentRulesValid(CandidateQueue) ? Task : nullptr;
}

void AKPCLDeliveryTaskSubsystem::CommitQueueMutation(TFunctionRef<void()> MutateQueue)
{
	const TArray<UKAPIDeliveryTask*> PreviousQueuedTasks = GetQueuedTasks();
	MutateQueue();
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mQueuedTasks, this);
	OnRep_QueuedTasks(PreviousQueuedTasks);
	ForceNetUpdate();
}

bool AKPCLDeliveryTaskSubsystem::TryToRemoveFromQueueAt(int32 QueueIndex)
{
	if (!CanRemoveFromQueueAt(QueueIndex))
	{
		return false;
	}

	UKAPIDeliveryTask* Task = ValidateQueueMutation(QueueIndex,
													[QueueIndex](TArray<UKAPIDeliveryTask*>& CandidateQueue)
													{
														CandidateQueue.RemoveAt(QueueIndex);
														return true;
													});
	if (!Task)
	{
		return false;
	}

	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this))
		{
			RCO->Server_DeliveryTask_TryToRemoveFromQueueAt(this, Task, QueueIndex);
			return true;
		}
		return false;
	}

	CommitQueueMutation([this, QueueIndex] { mQueuedTasks.RemoveAt(QueueIndex); });
	return true;
}

bool AKPCLDeliveryTaskSubsystem::CanRemoveFromQueueAt(int32 QueueIndex) const
{
	return mQueuedTasks.IsValidIndex(QueueIndex) && QueueIndex == mQueuedTasks.Num() - 1 &&
		IsValid(GetValid(mQueuedTasks[QueueIndex]));
}

bool AKPCLDeliveryTaskSubsystem::TryToDequeueFrom(int32 QueueIndex)
{
	if (!CanDequeueFrom(QueueIndex))
	{
		return false;
	}

	UKAPIDeliveryTask* Task =
		ValidateQueueMutation(QueueIndex,
							  [QueueIndex](TArray<UKAPIDeliveryTask*>& CandidateQueue)
							  {
								  CandidateQueue.RemoveAt(QueueIndex, CandidateQueue.Num() - QueueIndex);
								  return true;
							  });
	if (!Task)
	{
		return false;
	}

	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this))
		{
			RCO->Server_DeliveryTask_TryToDequeueFrom(this, Task, QueueIndex);
			return true;
		}
		return false;
	}

	CommitQueueMutation([this, QueueIndex] { mQueuedTasks.RemoveAt(QueueIndex, mQueuedTasks.Num() - QueueIndex); });
	return true;
}

bool AKPCLDeliveryTaskSubsystem::TryToDequeueTask(UKAPIDeliveryTask* Task)
{
	if (!IsValid(Task))
	{
		return false;
	}

	return TryToDequeueFrom(mQueuedTasks.FindLastByPredicate([Task](const TObjectPtr<UKAPIDeliveryTask>& QueuedTask)
															 { return QueuedTask == Task; }));
}

bool AKPCLDeliveryTaskSubsystem::CanDequeueFrom(int32 QueueIndex) const
{
	return mQueuedTasks.IsValidIndex(QueueIndex) && IsValid(GetValid(mQueuedTasks[QueueIndex]));
}

bool AKPCLDeliveryTaskSubsystem::TryToMoveQueueEntry(int32 FromIndex, int32 ToIndex)
{
	if (!mQueuedTasks.IsValidIndex(FromIndex) || !mQueuedTasks.IsValidIndex(ToIndex))
	{
		return false;
	}
	if (FromIndex == ToIndex)
	{
		return true;
	}

	UKAPIDeliveryTask* Task = ValidateQueueMutation(FromIndex,
													[FromIndex, ToIndex](TArray<UKAPIDeliveryTask*>& CandidateQueue)
													{
														if (!CandidateQueue.IsValidIndex(ToIndex))
														{
															return false;
														}
														UKAPIDeliveryTask* CandidateEntry = CandidateQueue[FromIndex];
														CandidateQueue.RemoveAt(FromIndex);
														CandidateQueue.Insert(CandidateEntry, ToIndex);
														return true;
													});
	if (!Task)
	{
		return false;
	}

	if (!HasAuthority())
	{
		if (UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this))
		{
			RCO->Server_DeliveryTask_TryToMoveQueueEntry(this, Task, FromIndex, ToIndex);
			return true;
		}
		return false;
	}

	CommitQueueMutation(
		[this, FromIndex, ToIndex]
		{
			const TObjectPtr<UKAPIDeliveryTask> QueueEntry = mQueuedTasks[FromIndex];
			mQueuedTasks.RemoveAt(FromIndex);
			mQueuedTasks.Insert(QueueEntry, ToIndex);
		});
	return true;
}

TArray<UKAPIDeliveryTask*> AKPCLDeliveryTaskSubsystem::GetCompletedTasks() const
{
	TArray<UKAPIDeliveryTask*> Tasks;
	Tasks.Reserve(mCompletedTasks.Num());
	for (const TObjectPtr<UKAPIDeliveryTask>& TaskPtr : mCompletedTasks)
	{
		if (UKAPIDeliveryTask* Task = GetValid(TaskPtr))
		{
			Tasks.Add(Task);
		}
	}
	return Tasks;
}

TArray<UKAPIDeliveryTask*> AKPCLDeliveryTaskSubsystem::GetQueuedTasks() const
{
	TArray<UKAPIDeliveryTask*> Tasks;
	Tasks.Reserve(mQueuedTasks.Num());
	for (const TObjectPtr<UKAPIDeliveryTask>& TaskPtr : mQueuedTasks)
	{
		if (UKAPIDeliveryTask* Task = GetValid(TaskPtr))
		{
			Tasks.Add(Task);
		}
	}
	return Tasks;
}

UKAPIDeliveryTask* AKPCLDeliveryTaskSubsystem::GetActiveTask() const { return GetValid(mCachedActiveTask); }

bool AKPCLDeliveryTaskSubsystem::IsActiveTaskFullyDelivered() const
{
	const FKPCLDeliveryTaskState* TaskState = FindTaskState(GetActiveTask());
	return TaskState && IsTaskStateComplete(*TaskState);
}

bool AKPCLDeliveryTaskSubsystem::GetCurrentQueueItem(FKPCLDeliveryTaskState& OutQueueItem) const
{
	OutQueueItem = FKPCLDeliveryTaskState();
	const FKPCLDeliveryTaskState* TaskState = FindTaskState(GetActiveTask());
	if (!TaskState)
	{
		return false;
	}

	OutQueueItem = *TaskState;
	return true;
}

TArray<FItemAmount> AKPCLDeliveryTaskSubsystem::GetCurrentTaskNeededAmounts() const
{
	TArray<FItemAmount> NeededAmounts;
	UKAPIDeliveryTask* Task = GetActiveTask();
	const FKPCLDeliveryTaskState* TaskState = FindTaskState(Task);
	if (!TaskState)
	{
		return NeededAmounts;
	}

	for (int32 Index = 0; Index < TaskState->mRequired.Num(); ++Index)
	{
		const FItemAmount& RequiredAmount = TaskState->mRequired[Index];
		if (!IsValid(RequiredAmount.ItemClass) || RequiredAmount.Amount <= 0)
		{
			continue;
		}

		AddItemAmount(NeededAmounts, RequiredAmount.ItemClass, GetRemainingAmountAtIndex(*TaskState, Index));
	}

	return NeededAmounts;
}

bool AKPCLDeliveryTaskSubsystem::TryGrabCurrentTaskItemsFromPlayer(AFGCharacterPlayer* Player)
{
	if (!IsValid(Player) || !IsValid(GetWorld()) || Player->GetWorld() != GetWorld())
	{
		return false;
	}

	if (!HasAuthority())
	{
		UKPCLDefaultRCO* RCO = UKPCLDefaultRCO::Get(this);
		const AFGPlayerController* Controller = IsValid(RCO) ? RCO->GetTypedOuter<AFGPlayerController>() : nullptr;
		if (!IsValid(Controller) || Controller->GetPawn() != Player)
		{
			return false;
		}

		RCO->Server_DeliveryTask_TryGrabCurrentTaskItems(this);
		return true;
	}

	if (bIsGrabbingPlayerInventory ||
		!ensureMsgf(IsInGameThread(), TEXT("Player inventory and delivery task state must mutate on the game thread")))
	{
		return false;
	}
	TGuardValue<bool> ProcessingGuard(bIsGrabbingPlayerInventory, true);

	UFGInventoryComponent* PlayerInventory = Player->GetInventory();
	if (!IsValid(PlayerInventory))
	{
		return false;
	}

	TArray<FItemAmount> AvailableAmounts;
	for (const FItemAmount& NeededAmount : GetCurrentTaskNeededAmounts())
	{
		if (!IsValid(NeededAmount.ItemClass) || NeededAmount.Amount <= 0)
		{
			continue;
		}

		const int32 AvailableAmount = FMath::Max(0, PlayerInventory->GetNumItems(NeededAmount.ItemClass));
		AddItemAmount(AvailableAmounts, NeededAmount.ItemClass, FMath::Min(AvailableAmount, NeededAmount.Amount));
	}

	TArray<FItemAmount> DeliveredAmounts;
	if (!TryToDeliverAmounts(AvailableAmounts, DeliveredAmounts))
	{
		return false;
	}

	for (const FItemAmount& DeliveredAmount : DeliveredAmounts)
	{
		PlayerInventory->Remove(DeliveredAmount.ItemClass, DeliveredAmount.Amount);
	}
	return true;
}

TArray<FItemAmount> AKPCLDeliveryTaskSubsystem::GetCurrentTaskRewards(UKAPIDeliveryTask* Task) const
{
	return GetTaskRewardsForCompletion(Task, GetTaskCompletionCount(Task));
}

TArray<FItemAmount> AKPCLDeliveryTaskSubsystem::GetTaskRewardsForCompletion(UKAPIDeliveryTask* Task,
																			int32 CompletionIndex) const
{
	TArray<FItemAmount> Rewards;
	bool bHasInvalidReward = false;
	CollectTaskRewards(Task, CompletionIndex, Rewards, bHasInvalidReward);
	return Rewards;
}

void AKPCLDeliveryTaskSubsystem::CollectTaskRewards(const UKAPIDeliveryTask* Task, int32 CompletionIndex,
													TArray<FItemAmount>& OutRewards, bool& bOutHasInvalidReward) const
{
	OutRewards.Reset();
	bOutHasInvalidReward = false;

	TArray<FItemAmount> ConfiguredRewards;
	GetTaskRewards(Task, CompletionIndex, ConfiguredRewards);
	for (const FItemAmount& Reward : ConfiguredRewards)
	{
		if (!IsValid(Reward.ItemClass) || Reward.Amount <= 0)
		{
			bOutHasInvalidReward = true;
			continue;
		}
		AddItemAmount(OutRewards, Reward.ItemClass, Reward.Amount);
	}
}

int32 AKPCLDeliveryTaskSubsystem::GetTaskCompletionCount(UKAPIDeliveryTask* Task) const
{
	const FKPCLDeliveryTaskState* TaskState = FindTaskState(Task);
	return TaskState ? FMath::Max(0, TaskState->mCompleted) : 0;
}

int32 AKPCLDeliveryTaskSubsystem::GetManualDeliveryRemainingAmount(TSubclassOf<UFGItemDescriptor> ItemClass) const
{
	if (!IsValid(ItemClass))
	{
		return 0;
	}

	const AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld());
	if (!IsValid(UnlockSubsystem) || !UnlockSubsystem->GetIsDeliveryTaskSystemUnlocked())
	{
		return 0;
	}

	const FKPCLDeliveryTaskState* TaskState = FindTaskState(GetActiveTask());
	if (!TaskState)
	{
		return 0;
	}
	for (int32 Index = 0; Index < TaskState->mRequired.Num(); ++Index)
	{
		const FItemAmount& RequiredAmount = TaskState->mRequired[Index];
		if (RequiredAmount.ItemClass != ItemClass)
		{
			continue;
		}
		return GetRemainingAmountAtIndex(*TaskState, Index);
	}
	return 0;
}

bool AKPCLDeliveryTaskSubsystem::IsManualDeliveryItemAllowed(TSubclassOf<UFGItemDescriptor> ItemClass) const
{
	return GetManualDeliveryRemainingAmount(ItemClass) > 0;
}

bool AKPCLDeliveryTaskSubsystem::FilterManualDeliveryInventory(TSubclassOf<UObject> ItemClass,
															   int32 InventoryIndex) const
{
	if ((InventoryIndex != INDEX_NONE &&
		 (!IsValid(mManualDeliveryInventory) || !mManualDeliveryInventory->IsValidIndex(InventoryIndex))) ||
		!IsValid(ItemClass) || !ItemClass->IsChildOf(UFGItemDescriptor::StaticClass()))
	{
		return false;
	}

	const TSubclassOf<UFGItemDescriptor> ItemDescriptor = TSubclassOf<UFGItemDescriptor>(ItemClass);
	const int32 RemainingAmount = GetManualDeliveryRemainingAmount(ItemDescriptor);
	if (RemainingAmount <= 0)
	{
		return false;
	}

	if (HasAuthority() && InventoryIndex != INDEX_NONE && IsValid(mManualDeliveryInventory) &&
		mManualDeliveryInventory->IsValidIndex(InventoryIndex))
	{
		FInventoryStack ExistingStack;
		const int32 ExistingAmount =
			mManualDeliveryInventory->GetStackFromIndex(InventoryIndex, ExistingStack) && ExistingStack.HasItems()
			? ExistingStack.NumItems
			: 0;
		mManualDeliveryInventory->AddArbitrarySlotSize(InventoryIndex, FMath::Max(1, ExistingAmount + RemainingAmount));
	}

	return true;
}

void AKPCLDeliveryTaskSubsystem::RefreshManualDeliveryInventorySize()
{
	if (!HasAuthority() || !IsValid(mManualDeliveryInventory))
	{
		return;
	}

	const int32 NormalizedSlotCount = FMath::Max(1, mManualDeliveryInventorySlotCount);
	if (NormalizedSlotCount != mManualDeliveryInventorySlotCount)
	{
		mManualDeliveryInventorySlotCount = NormalizedSlotCount;
		MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mManualDeliveryInventorySlotCount, this);
	}
	mManualDeliveryInventory->Resize(mManualDeliveryInventorySlotCount);
	RefreshManualDeliveryInventorySlotSize();
}

void AKPCLDeliveryTaskSubsystem::RefreshManualDeliveryInventorySlotSize()
{
	if (!HasAuthority() || !IsValid(mManualDeliveryInventory))
	{
		return;
	}

	int32 EmptySlotSize = 1;
	for (const FItemAmount& NeededAmount : GetCurrentTaskNeededAmounts())
	{
		EmptySlotSize = FMath::Max(EmptySlotSize, NeededAmount.Amount);
	}

	for (int32 InventoryIndex = 0; mManualDeliveryInventory->IsValidIndex(InventoryIndex); ++InventoryIndex)
	{
		int32 SlotSize = EmptySlotSize;
		FInventoryStack ExistingStack;
		if (mManualDeliveryInventory->GetStackFromIndex(InventoryIndex, ExistingStack) && ExistingStack.HasItems())
		{
			SlotSize = ExistingStack.NumItems + GetManualDeliveryRemainingAmount(ExistingStack.Item.GetItemClass());
		}
		mManualDeliveryInventory->AddArbitrarySlotSize(InventoryIndex, FMath::Max(1, SlotSize));
	}
}

void AKPCLDeliveryTaskSubsystem::RefreshRewardInventorySize()
{
	if (!HasAuthority() || !IsValid(mRewardInventory))
	{
		return;
	}

	const int32 CurrentInventorySize = mRewardInventory->GetSizeLinear();
	const int64 LegacyUnlockedSlots = FMath::Max<int64>(0, static_cast<int64>(mManualDeliveryInventorySlotCount) - 1);
	const int32 LegacyInventorySize = static_cast<int32>(
		FMath::Min<int64>(static_cast<int64>(InitialRewardInventorySize) + LegacyUnlockedSlots, MAX_int32));
	const int32 CompletedUnlockInventorySize = GetRewardInventorySizeUnlockedByCompletedTasks(mDeliveredTaskStates);
	const int32 NormalizedSlotCount =
		FMath::Max(FMath::Max(InitialRewardInventorySize, mRewardInventorySlotCount),
				   FMath::Max(FMath::Max(CurrentInventorySize, LegacyInventorySize), CompletedUnlockInventorySize));
	if (NormalizedSlotCount != mRewardInventorySlotCount)
	{
		mRewardInventorySlotCount = NormalizedSlotCount;
		MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mRewardInventorySlotCount, this);
	}
	if (CurrentInventorySize != mRewardInventorySlotCount)
	{
		mRewardInventory->Resize(mRewardInventorySlotCount);
	}
}

bool AKPCLDeliveryTaskSubsystem::TryConsumeManualDeliveryInventory()
{
	if (!HasAuthority() || bIsProcessingManualDeliveryInventory || !IsValid(mManualDeliveryInventory))
	{
		return false;
	}

	TGuardValue<bool> ProcessingGuard(bIsProcessingManualDeliveryInventory, true);
	bool bConsumedAny = false;
	for (int32 InventoryIndex = 0; mManualDeliveryInventory->IsValidIndex(InventoryIndex); ++InventoryIndex)
	{
		FInventoryStack Stack;
		if (mManualDeliveryInventory->IsIndexEmpty(InventoryIndex) ||
			!mManualDeliveryInventory->GetStackFromIndex(InventoryIndex, Stack) || !Stack.HasItems())
		{
			continue;
		}

		const TSubclassOf<UFGItemDescriptor> ItemClass = Stack.Item.GetItemClass();
		TArray<FItemAmount> OfferedAmounts;
		OfferedAmounts.Emplace(ItemClass, Stack.NumItems);

		TArray<FItemAmount> DeliveredAmounts;
		if (!TryToDeliverAmounts(OfferedAmounts, DeliveredAmounts))
		{
			continue;
		}

		int32 DeliveredAmount = 0;
		for (const FItemAmount& Amount : DeliveredAmounts)
		{
			if (Amount.ItemClass == ItemClass)
			{
				DeliveredAmount += Amount.Amount;
			}
		}
		DeliveredAmount = FMath::Clamp(DeliveredAmount, 0, Stack.NumItems);
		if (DeliveredAmount <= 0)
		{
			continue;
		}

		mManualDeliveryInventory->RemoveFromIndex(InventoryIndex, DeliveredAmount);
		mOnManualDeliveryProcessed.Broadcast(ItemClass, DeliveredAmount, Stack.NumItems - DeliveredAmount);
		bConsumedAny = true;
	}

	RefreshManualDeliveryInventorySlotSize();
	return bConsumedAny;
}

void AKPCLDeliveryTaskSubsystem::OnManualDeliveryItemAdded(TSubclassOf<UFGItemDescriptor> ItemClass, int32 NumAdded,
														   UFGInventoryComponent*)
{
	if (IsValid(ItemClass) && NumAdded > 0)
	{
		TryConsumeManualDeliveryInventory();
	}
}

void AKPCLDeliveryTaskSubsystem::OnRewardInventoryItemRemoved(TSubclassOf<UFGItemDescriptor>, int32 NumRemoved,
															  UFGInventoryComponent*)
{
	if (HasAuthority() && NumRemoved > 0 && !bIsProcessingTaskCompletion)
	{
		TryCompleteActiveTask();
	}
}

void AKPCLDeliveryTaskSubsystem::GetTaskRewards(const UKAPIDeliveryTask* Task, int32 CompletionIndex,
												TArray<FItemAmount>& OutRewards) const
{
	OutRewards.Reset();
	if (!IsValid(Task) || CompletionIndex < 0)
	{
		return;
	}

	if (!Task->bIsEndless)
	{
		OutRewards = Task->mRewards;
		return;
	}

	for (const FKAPITaskReward& TaskReward : Task->mTaskRewards)
	{
		if (TaskReward.mRequiredTask == INDEX_NONE || TaskReward.mRequiredTask == CompletionIndex)
		{
			OutRewards.Append(TaskReward.mRewards);
		}
	}
}

bool AKPCLDeliveryTaskSubsystem::TryStoreTaskRewards(const UKAPIDeliveryTask* Task, int32 CompletionIndex,
													 FText& OutErrorText)
{
	OutErrorText = FText::GetEmpty();
	if (!IsValid(Task) || CompletionIndex < 0)
	{
		OutErrorText = NSLOCTEXT("KPCLDeliveryTaskSubsystem", "InvalidRewardTask",
								 "Task cannot be completed because its reward configuration is invalid.");
		return false;
	}

	FFormatNamedArguments ErrorArguments;
	ErrorArguments.Add(TEXT("TaskName"), Task->mName);
	if (!IsValid(mRewardInventory))
	{
		OutErrorText = FText::Format(NSLOCTEXT("KPCLDeliveryTaskSubsystem", "RewardInventoryUnavailable",
											   "Cannot complete: reward inventory is unavailable."),
									 ErrorArguments);
		return false;
	}
	RefreshRewardInventorySize();

	TArray<FItemAmount> Rewards;
	bool bHasInvalidReward = false;
	CollectTaskRewards(Task, CompletionIndex, Rewards, bHasInvalidReward);
	if (bHasInvalidReward)
	{
		OutErrorText = FText::Format(NSLOCTEXT("KPCLDeliveryTaskSubsystem", "InvalidTaskReward",
											   "Cannot complete: configured reward is invalid."),
									 ErrorArguments);
		return false;
	}

	if (Rewards.IsEmpty())
	{
		return true;
	}

	TArray<FInventoryStack> RewardStacks;
	RewardStacks.Reserve(Rewards.Num());
	for (const FItemAmount& Reward : Rewards)
	{
		RewardStacks.Emplace(Reward.Amount, Reward.ItemClass);
	}

	if (!mRewardInventory->HasEnoughSpaceForStacks(RewardStacks))
	{
		const int32 PreviousInventorySize = mRewardInventory->GetSizeLinear();
		const int32 PreviousSlotCount = mRewardInventorySlotCount;
		const int32 ExplicitUnlockSlots = GetRewardInventorySlotsGrantedByTask(Task);
		const int32 ExpandedInventorySize =
			ResolveRewardInventorySizeAfterUnlock(PreviousInventorySize, ExplicitUnlockSlots);
		if (ExpandedInventorySize > PreviousInventorySize)
		{
			mRewardInventorySlotCount = ExpandedInventorySize;
			MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mRewardInventorySlotCount, this);
			mRewardInventory->Resize(ExpandedInventorySize);
		}

		if (!mRewardInventory->HasEnoughSpaceForStacks(RewardStacks))
		{
			mPreGrantedRewardInventorySlots = 0;
			mRewardInventorySlotCount = PreviousSlotCount;
			MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mRewardInventorySlotCount, this);
			if (mRewardInventory->GetSizeLinear() != PreviousInventorySize)
			{
				mRewardInventory->Resize(PreviousInventorySize);
			}
			OutErrorText =
				FText::Format(NSLOCTEXT("KPCLDeliveryTaskSubsystem", "RewardInventoryFull",
										"Cannot complete: reward inventory does not have enough free space."),
							  ErrorArguments);
			return false;
		}

		mPreGrantedRewardInventorySlots = ExplicitUnlockSlots;
	}

	mRewardInventory->AddStacks(RewardStacks);
	return true;
}

bool AKPCLDeliveryTaskSubsystem::TryCompleteActiveTask()
{
	if (!HasAuthority() || bIsProcessingTaskCompletion)
	{
		return false;
	}

	UKAPIDeliveryTask* Task = GetActiveTask();
	FKPCLDeliveryTaskState* TaskState = FindTaskState(Task);
	if (!TaskState || !IsTaskStateComplete(*TaskState))
	{
		StopTaskCompletionRetryTimer();
		SetRewardErrorText(FText::GetEmpty());
		return false;
	}

	return TriggerTaskCompletion(TaskState);
}

void AKPCLDeliveryTaskSubsystem::StartTaskCompletionRetryTimer()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !IsValid(World) || World->GetTimerManager().IsTimerActive(mTaskCompletionRetryTimer))
	{
		return;
	}

	World->GetTimerManager().SetTimer(mTaskCompletionRetryTimer, this, &AKPCLDeliveryTaskSubsystem::RetryTaskCompletion,
									  1.f, true);
}

void AKPCLDeliveryTaskSubsystem::StopTaskCompletionRetryTimer()
{
	if (UWorld* World = GetWorld(); IsValid(World))
	{
		World->GetTimerManager().ClearTimer(mTaskCompletionRetryTimer);
	}
}

void AKPCLDeliveryTaskSubsystem::RetryTaskCompletion() { TryCompleteActiveTask(); }

void AKPCLDeliveryTaskSubsystem::SetRewardErrorText(const FText& ErrorText)
{
	if (!HasAuthority() || mRewardErrorText.EqualTo(ErrorText))
	{
		return;
	}

	mRewardErrorText = ErrorText;
	if (!mRewardErrorText.IsEmpty())
	{
		UE_LOG(LogKPCL, Error, TEXT("Delivery task completion stopped: %s"), *mRewardErrorText.ToString());
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mRewardErrorText, this);
	OnRep_RewardErrorText();
	ForceNetUpdate();
}

TArray<FKPCLResearchTreeNode> AKPCLDeliveryTaskSubsystem::GetResearchTreeNodes() const
{
	TArray<FKPCLResearchTreeNode> Nodes;
	EnsureResearchTopologyCache();
	Nodes.Reserve(mSortedResearchTasks.Num());
	for (UKAPIDeliveryTask* Task : mSortedResearchTasks)
	{
		FKPCLResearchTreeNode Node;
		if (GetResearchNode(Task, Node))
		{
			Nodes.Add(MoveTemp(Node));
		}
	}
	return Nodes;
}

TArray<FKPCLResearchTreeEdge> AKPCLDeliveryTaskSubsystem::GetResearchTreeEdges() const
{
	TArray<FKPCLResearchTreeEdge> Edges;
	EnsureResearchTopologyCache();
	for (UKAPIDeliveryTask* Child : mSortedResearchTasks)
	{
		const TArray<TObjectPtr<UKAPIDeliveryTask>>* Parents = mResearchParentsByTask.Find(Child);
		if (!Parents)
		{
			continue;
		}
		const bool bChildUnlocked = AreTaskPrerequisitesMet(Child);
		for (UKAPIDeliveryTask* Parent : *Parents)
		{
			if (!IsValid(Parent))
			{
				continue;
			}

			FKPCLResearchTreeEdge& Edge = Edges.AddDefaulted_GetRef();
			Edge.mParent = Parent;
			Edge.mChild = Child;
			Edge.mParentCoordinate = Parent->mCoordinate;
			Edge.mChildCoordinate = Child->mCoordinate;
			Edge.bParentCompleted = IsParentRequirementMet(Child, Parent);
			Edge.bChildUnlocked = bChildUnlocked;
		}
	}
	return Edges;
}

bool AKPCLDeliveryTaskSubsystem::GetActiveResearchNode(UKAPIDeliveryTask*& OutTask,
													   FKPCLResearchTreeNode& OutNode) const
{
	OutTask = GetActiveTask();
	OutNode = FKPCLResearchTreeNode();
	return IsValid(OutTask) && GetResearchNode(OutTask, OutNode);
}

bool AKPCLDeliveryTaskSubsystem::GetResearchNode(UKAPIDeliveryTask* Task, FKPCLResearchTreeNode& OutNode) const
{
	OutNode = FKPCLResearchTreeNode();
	if (!IsResearchTaskRegistered(Task))
	{
		return false;
	}

	OutNode.mTask = Task;
	OutNode.mCoordinate = Task->mCoordinate;
	OutNode.bRequireAllParents = Task->bRequireAllParents;
	EnsureResearchTopologyCache();
	if (const TArray<TObjectPtr<UKAPIDeliveryTask>>* Parents = mResearchParentsByTask.Find(Task))
	{
		OutNode.mParents = *Parents;
	}
	if (const TArray<TObjectPtr<UKAPIDeliveryTask>>* CachedChildren = mResearchChildrenByTask.Find(Task))
	{
		OutNode.mChildren = *CachedChildren;
	}
	OutNode.mQueueIndices = GetTaskQueueIndices(Task);
	OutNode.bIsRepeatable = Task->bIsEndless;
	OutNode.mMaxCompletions = Task->bIsEndless ? Task->mMaxTasks : 1;
	OutNode.bPrerequisitesMet = AreTaskPrerequisitesMet(Task);

	const FKPCLDeliveryTaskState* TaskState = FindTaskState(Task);
	if (TaskState)
	{
		OutNode.mRequired = TaskState->mRequired;
		OutNode.mDelivered = TaskState->mDelivered;
		OutNode.mCompletedCount = TaskState->mCompleted;
		for (int32 Index = 0; Index < TaskState->mRequired.Num(); ++Index)
		{
			const FItemAmount& RequiredAmount = TaskState->mRequired[Index];
			const int32 DeliveredAmount =
				TaskState->mDelivered.IsValidIndex(Index) ? TaskState->mDelivered[Index].Amount : 0;
			AddItemAmount(OutNode.mRemaining, RequiredAmount.ItemClass,
						  FMath::Max(0, RequiredAmount.Amount - DeliveredAmount));
		}
	}
	else
	{
		FKPCLDeliveryTaskState PreviewState;
		if (BuildPreviewTaskState(Task, PreviewState))
		{
			OutNode.mRequired = PreviewState.mRequired;
			OutNode.mDelivered = PreviewState.mDelivered;
			OutNode.mRemaining = PreviewState.mRequired;
			OutNode.mCompletedCount = PreviewState.mCompleted;
		}
	}

	OutNode.mProgress = GetTaskResearchProgress(Task);
	OutNode.bCanQueue = CanQueueTask(Task, OutNode.mQueueErrors);

	const bool bMaxed = Task->bIsEndless && Task->mMaxTasks != INDEX_NONE && OutNode.mCompletedCount >= Task->mMaxTasks;
	const bool bActive = OutNode.mQueueIndices.Contains(0);
	const bool bQueued = !OutNode.mQueueIndices.IsEmpty();
	if (bMaxed)
	{
		OutNode.mState = EKPCLResearchNodeState::Maxed;
	}
	else if (bActive)
	{
		OutNode.mState = EKPCLResearchNodeState::Active;
	}
	else if (bQueued)
	{
		OutNode.mState = Task->bIsEndless ? EKPCLResearchNodeState::RepeatableQueued : EKPCLResearchNodeState::Queued;
	}
	else if (!Task->bIsEndless && IsTaskCompleted(Task))
	{
		OutNode.mState = EKPCLResearchNodeState::Completed;
	}
	else if (IsTaskLockedInternal(Task, OutNode.bPrerequisitesMet))
	{
		OutNode.mState = EKPCLResearchNodeState::Locked;
	}
	else if (Task->bIsEndless && OutNode.mCompletedCount > 0)
	{
		OutNode.mState = EKPCLResearchNodeState::RepeatableAvailable;
	}
	else
	{
		OutNode.mState = EKPCLResearchNodeState::Available;
	}

	return true;
}

bool AKPCLDeliveryTaskSubsystem::GetResearchNodeAtCoordinate(FIntVector2 Coordinate,
															 FKPCLResearchTreeNode& OutNode) const
{
	return GetResearchNode(GetResearchTaskAtCoordinate(Coordinate), OutNode);
}

bool AKPCLDeliveryTaskSubsystem::GetQueuedResearchNode(int32 QueueIndex, FKPCLResearchTreeNode& OutNode) const
{
	OutNode = FKPCLResearchTreeNode();
	return mQueuedTasks.IsValidIndex(QueueIndex) && GetResearchNode(GetValid(mQueuedTasks[QueueIndex]), OutNode);
}

UKAPIDeliveryTask* AKPCLDeliveryTaskSubsystem::GetResearchTaskAtCoordinate(FIntVector2 Coordinate) const
{
	UKAPIDataAssetSubsystem* DataSubsystem = UKAPIDataAssetSubsystem::Get(GetWorld());
	if (!IsValid(DataSubsystem))
	{
		return nullptr;
	}

	const TObjectPtr<UKAPIDeliveryTask>* Task = DataSubsystem->Delivery_GetAllView().Find(Coordinate);
	return Task ? GetValid(*Task) : nullptr;
}

TArray<UKAPIDeliveryTask*> AKPCLDeliveryTaskSubsystem::GetResearchParents(UKAPIDeliveryTask* Task) const
{
	TArray<UKAPIDeliveryTask*> Parents;
	if (!IsValid(Task))
	{
		return Parents;
	}

	EnsureResearchTopologyCache();
	if (const TArray<TObjectPtr<UKAPIDeliveryTask>>* CachedParents = mResearchParentsByTask.Find(Task))
	{
		Parents.Reserve(CachedParents->Num());
		for (UKAPIDeliveryTask* Parent : *CachedParents)
		{
			Parents.Add(Parent);
		}
	}
	return Parents;
}

TArray<UKAPIDeliveryTask*> AKPCLDeliveryTaskSubsystem::GetResearchChildren(UKAPIDeliveryTask* Task) const
{
	TArray<UKAPIDeliveryTask*> ChildTasks;
	if (!IsValid(Task))
	{
		return ChildTasks;
	}

	EnsureResearchTopologyCache();
	if (const TArray<TObjectPtr<UKAPIDeliveryTask>>* CachedChildren = mResearchChildrenByTask.Find(Task))
	{
		ChildTasks.Reserve(CachedChildren->Num());
		for (UKAPIDeliveryTask* Child : *CachedChildren)
		{
			ChildTasks.Add(Child);
		}
	}
	return ChildTasks;
}

TArray<UFGAvailabilityDependency*>
AKPCLDeliveryTaskSubsystem::GetTaskNotMetDependencies(UKAPIDeliveryTask* Task) const
{
	TArray<UFGAvailabilityDependency*> NotMetDependencies;
	if (!IsResearchTaskRegistered(Task))
	{
		return NotMetDependencies;
	}

	TArray<UFGAvailabilityDependency*> Dependencies;
	Task->GetAllDependencies(Dependencies);
	NotMetDependencies.Reserve(Dependencies.Num());
	for (UFGAvailabilityDependency* Dependency : Dependencies)
	{
		if (IsValid(Dependency) && !Dependency->AreDependenciesMet(GetWorld()))
		{
			NotMetDependencies.Add(Dependency);
		}
	}
	return NotMetDependencies;
}

TArray<FKPCLParentTaskNotMetReason>
AKPCLDeliveryTaskSubsystem::GetNotMetParentTask(UKAPIDeliveryTask* Task) const
{
	TArray<FKPCLParentTaskNotMetReason> NotMetParents;
	if (!IsResearchTaskRegistered(Task))
	{
		return NotMetParents;
	}

	const TArray<FIntVector2> ParentCoordinates = Task->GetParentCoordinates();
	NotMetParents.Reserve(ParentCoordinates.Num());
	bool bAnyParentMet = false;
	for (const FIntVector2& ParentCoordinate : ParentCoordinates)
	{
		UKAPIDeliveryTask* Parent = GetResearchTaskAtCoordinate(ParentCoordinate);
		if (!IsValid(Parent))
		{
			continue;
		}

		if (IsParentRequirementMet(Task, Parent))
		{
			bAnyParentMet = true;
			continue;
		}

		FKPCLParentTaskNotMetReason& Reason = NotMetParents.AddDefaulted_GetRef();
		Reason.mTask = Parent;
		const FText ParentDisplayName = !IsTaskLocked(Parent) && !Parent->mName.IsEmpty()
			? Parent->mName
			: NSLOCTEXT("KPCLDeliveryTaskSubsystem", "LockedParentTaskName", "???");
		FFormatNamedArguments ParentNameArguments;
		ParentNameArguments.Add(TEXT("ParentName"), ParentDisplayName);
		Reason.mReasonTitle = FText::Format(
			NSLOCTEXT("KPCLDeliveryTaskSubsystem", "QuotedParentTaskName", "\"{ParentName}\""), ParentNameArguments);

		const int32 RequiredCompletions = GetEffectiveParentCompletionRequirement(Task, Parent);
		FFormatNamedArguments ReasonArguments;
		ReasonArguments.Add(TEXT("ParentName"), Reason.mReasonTitle);
		if (RequiredCompletions == 1)
		{
			Reason.mReasonDescription =
				FText::Format(NSLOCTEXT("KPCLDeliveryTaskSubsystem", "ParentTaskNotCompletedDescription",
										"Complete {ParentName} before unlocking the selected task."),
							  ReasonArguments);
		}
		else
		{
			ReasonArguments.Add(TEXT("CompletedCompletions"), GetTaskCompletionCount(Parent));
			ReasonArguments.Add(TEXT("RequiredCompletions"), RequiredCompletions);
			Reason.mReasonDescription =
				FText::Format(NSLOCTEXT("KPCLDeliveryTaskSubsystem", "ParentTaskCompletionCountDescription",
										"Complete {ParentName} {RequiredCompletions} times. Progress: "
										"{CompletedCompletions}/{RequiredCompletions}."),
							  ReasonArguments);
		}
	}

	if (!Task->bRequireAllParents && bAnyParentMet)
	{
		NotMetParents.Reset();
	}
	return NotMetParents;
}

bool AKPCLDeliveryTaskSubsystem::AreTaskParentCompletionsMet(UKAPIDeliveryTask* Task) const
{
	if (!IsResearchTaskRegistered(Task))
	{
		return false;
	}

	EnsureResearchTopologyCache();
	const TArray<TObjectPtr<UKAPIDeliveryTask>>* Parents = mResearchParentsByTask.Find(Task);
	if (Task->GetParentCoordinates().IsEmpty())
	{
		return true;
	}
	if (!Parents || Parents->IsEmpty())
	{
		return true;
	}

	const auto IsParentSatisfied = [this, Task](UKAPIDeliveryTask* Parent)
	{ return IsParentRequirementMet(Task, Parent); };

	if (!Task->bRequireAllParents)
	{
		return Parents->ContainsByPredicate(IsParentSatisfied);
	}

	for (UKAPIDeliveryTask* Parent : *Parents)
	{
		if (!IsParentSatisfied(Parent))
		{
			return false;
		}
	}
	return true;
}

bool AKPCLDeliveryTaskSubsystem::AreTaskPrerequisitesMet(UKAPIDeliveryTask* Task) const
{
	return AreTaskParentCompletionsMet(Task) && Task->AreTaskDependenciesMet(GetWorld());
}

bool AKPCLDeliveryTaskSubsystem::AreTaskPrerequisitesMetWithQueue(UKAPIDeliveryTask* Task) const
{
	if (!IsResearchTaskRegistered(Task) || !Task->AreTaskDependenciesMet(GetWorld()))
	{
		return false;
	}
	return AreTaskQueuePrerequisitesMet(Task, GetQueuedTaskSet());
}

bool AKPCLDeliveryTaskSubsystem::IsTaskLocked(UKAPIDeliveryTask* Task) const
{
	return IsResearchTaskRegistered(Task) ? IsTaskLockedInternal(Task, AreTaskPrerequisitesMet(Task)) : true;
}

bool AKPCLDeliveryTaskSubsystem::IsTaskSelectable(UKAPIDeliveryTask* Task) const
{

	return IsResearchTaskRegistered(Task) && !IsTaskLockedInternal(Task, AreTaskPrerequisitesMetWithQueue(Task));
}

bool AKPCLDeliveryTaskSubsystem::IsTaskLockedInternal(UKAPIDeliveryTask* Task, bool bPrerequisitesMet) const
{
	if (!IsResearchTaskRegistered(Task))
	{
		return true;
	}

	if (!GetTaskQueueIndices(Task).IsEmpty())
	{
		return false;
	}

	if (Task->bIsEndless)
	{
		const FKPCLDeliveryTaskState* TaskState = FindTaskState(Task);
		const int32 CompletedCount = TaskState ? TaskState->mCompleted : 0;
		if (Task->mMaxTasks != INDEX_NONE && CompletedCount >= Task->mMaxTasks)
		{
			return false;
		}
	}
	else if (IsTaskCompleted(Task))
	{
		return false;
	}

	const AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(GetWorld());
	const bool bDeliverySystemUnlocked = IsValid(UnlockSubsystem) && UnlockSubsystem->GetIsDeliveryTaskSystemUnlocked();
	return !bDeliverySystemUnlocked || !bPrerequisitesMet;
}

bool AKPCLDeliveryTaskSubsystem::AreTaskQueuePrerequisitesMet(UKAPIDeliveryTask* Task,
															  const TArray<UKAPIDeliveryTask*>& Queue,
															  int32 TaskQueueIndex) const
{
	if (!IsResearchTaskRegistered(Task) || !Queue.IsValidIndex(TaskQueueIndex) || Queue[TaskQueueIndex] != Task)
	{
		return false;
	}

	TSet<UKAPIDeliveryTask*> PriorQueuedTasks;
	PriorQueuedTasks.Reserve(TaskQueueIndex);
	for (int32 QueueIndex = 0; QueueIndex < TaskQueueIndex; ++QueueIndex)
	{
		if (IsValid(Queue[QueueIndex]))
		{
			PriorQueuedTasks.Add(Queue[QueueIndex]);
		}
	}
	return AreTaskQueuePrerequisitesMet(Task, PriorQueuedTasks);
}

TSet<UKAPIDeliveryTask*> AKPCLDeliveryTaskSubsystem::GetQueuedTaskSet() const
{
	TSet<UKAPIDeliveryTask*> QueuedTasks;
	QueuedTasks.Reserve(mQueueIndicesByTask.Num());
	for (const TPair<UKAPIDeliveryTask*, TArray<int32>>& Pair : mQueueIndicesByTask)
	{
		QueuedTasks.Add(Pair.Key);
	}
	return QueuedTasks;
}

bool AKPCLDeliveryTaskSubsystem::AreTaskQueuePrerequisitesMet(UKAPIDeliveryTask* Task,
															  const TSet<UKAPIDeliveryTask*>& PriorQueuedTasks) const
{
	if (!IsResearchTaskRegistered(Task))
	{
		return false;
	}

	const TArray<FIntVector2> ParentCoordinates = Task->GetParentCoordinates();
	if (ParentCoordinates.IsEmpty())
	{
		return true;
	}

	EnsureResearchTopologyCache();
	const TArray<TObjectPtr<UKAPIDeliveryTask>>* Parents = mResearchParentsByTask.Find(Task);
	if (!Parents || Parents->IsEmpty())
	{
		return true;
	}

	const auto IsParentSatisfied = [this, Task, &PriorQueuedTasks](UKAPIDeliveryTask* Parent)
	{
		if (IsParentRequirementMet(Task, Parent))
		{
			return true;
		}

		return GetEffectiveParentCompletionRequirement(Task, Parent) == 1 && PriorQueuedTasks.Contains(Parent);
	};

	if (!Task->bRequireAllParents)
	{
		return Parents->ContainsByPredicate(IsParentSatisfied);
	}

	return !Parents->ContainsByPredicate([&IsParentSatisfied](UKAPIDeliveryTask* Parent)
										 { return !IsParentSatisfied(Parent); });
}

bool AKPCLDeliveryTaskSubsystem::IsQueueParentRulesValid(const TArray<UKAPIDeliveryTask*>& Queue) const
{
	TSet<UKAPIDeliveryTask*> PriorQueuedTasks;
	PriorQueuedTasks.Reserve(Queue.Num());
	for (UKAPIDeliveryTask* Task : Queue)
	{
		if (!AreTaskQueuePrerequisitesMet(Task, PriorQueuedTasks))
		{
			return false;
		}
		PriorQueuedTasks.Add(Task);
	}
	return true;
}

void AKPCLDeliveryTaskSubsystem::SanitizeQueueParentRules()
{
	if (!HasAuthority() || mQueuedTasks.IsEmpty())
	{
		return;
	}
	const UKAPIDataAssetSubsystem* DataSubsystem = UKAPIDataAssetSubsystem::Get(GetWorld());
	if (!IsValid(DataSubsystem) || !DataSubsystem->Delivery_HasCompletedScan())
	{
		return;
	}

	const TArray<UKAPIDeliveryTask*> PreviousQueuedTasks = GetQueuedTasks();
	TArray<TObjectPtr<UKAPIDeliveryTask>> SanitizedTasks;
	TSet<UKAPIDeliveryTask*> PriorQueuedTasks;
	SanitizedTasks.Reserve(mQueuedTasks.Num());
	PriorQueuedTasks.Reserve(mQueuedTasks.Num());

	for (const TObjectPtr<UKAPIDeliveryTask>& TaskPtr : mQueuedTasks)
	{
		UKAPIDeliveryTask* Task = GetValid(TaskPtr);
		if (!Task)
		{
			continue;
		}

		if (AreTaskQueuePrerequisitesMet(Task, PriorQueuedTasks))
		{
			SanitizedTasks.Add(Task);
			PriorQueuedTasks.Add(Task);
		}
	}

	if (SanitizedTasks.Num() == mQueuedTasks.Num())
	{
		return;
	}

	mQueuedTasks = MoveTemp(SanitizedTasks);
	MARK_PROPERTY_DIRTY_FROM_NAME(AKPCLDeliveryTaskSubsystem, mQueuedTasks, this);
	OnRep_QueuedTasks(PreviousQueuedTasks);
	ForceNetUpdate();
}

bool AKPCLDeliveryTaskSubsystem::IsResearchTaskRegistered(UKAPIDeliveryTask* Task) const
{
	if (!IsValid(Task))
	{
		return false;
	}

	UKAPIDataAssetSubsystem* DataSubsystem = UKAPIDataAssetSubsystem::Get(GetWorld());
	if (!IsValid(DataSubsystem))
	{
		return false;
	}

	const TObjectPtr<UKAPIDeliveryTask>* RegisteredTask = DataSubsystem->Delivery_GetAllView().Find(Task->mCoordinate);
	return RegisteredTask && RegisteredTask->Get() == Task;
}

bool AKPCLDeliveryTaskSubsystem::IsTaskCompleted(UKAPIDeliveryTask* Task) const
{
	if (!IsValid(Task))
	{
		return false;
	}
	const FKPCLDeliveryTaskState* State = FindTaskState(Task);
	return mCompletedTaskCache.Contains(Task) || (State && State->mCompleted > 0);
}

bool AKPCLDeliveryTaskSubsystem::IsParentRequirementMet(UKAPIDeliveryTask* Child, UKAPIDeliveryTask* Parent) const
{
	if (!IsValid(Child) || !IsValid(Parent))
	{
		return false;
	}

	const int32 RequiredCompletions = GetEffectiveParentCompletionRequirement(Child, Parent);
	return RequiredCompletions == 1 ? IsTaskCompleted(Parent) : GetTaskCompletionCount(Parent) >= RequiredCompletions;
}

float AKPCLDeliveryTaskSubsystem::GetTaskResearchProgress(UKAPIDeliveryTask* Task) const
{
	if (!IsValid(Task))
	{
		return 0.f;
	}

	const FKPCLDeliveryTaskState* State = FindTaskState(Task);
	const bool bMaxed =
		Task->bIsEndless && Task->mMaxTasks != INDEX_NONE && State && State->mCompleted >= Task->mMaxTasks;
	return bMaxed ? 1.f : CalculateTaskProgress(State);
}

TArray<int32> AKPCLDeliveryTaskSubsystem::GetTaskQueueIndices(UKAPIDeliveryTask* Task) const
{
	TArray<int32> QueueIndices;
	if (!IsValid(Task))
	{
		return QueueIndices;
	}

	if (const TArray<int32>* CachedIndices = mQueueIndicesByTask.Find(Task))
	{
		QueueIndices = *CachedIndices;
	}
	return QueueIndices;
}

bool AKPCLDeliveryTaskSubsystem::GetResearchTreeBounds(FIntVector2& OutMinCoordinate,
													   FIntVector2& OutMaxCoordinate) const
{
	EnsureResearchTopologyCache();
	if (!bHasCachedTreeBounds)
	{
		OutMinCoordinate = FIntVector2::ZeroValue;
		OutMaxCoordinate = FIntVector2::ZeroValue;
		return false;
	}

	OutMinCoordinate = mCachedTreeMinCoordinate;
	OutMaxCoordinate = mCachedTreeMaxCoordinate;
	return true;
}

void AKPCLDeliveryTaskSubsystem::BroadcastResearchNodeChanged(UKAPIDeliveryTask* Task)
{
	FKPCLResearchTreeNode Node;
	if (GetResearchNode(Task, Node))
	{
		mOnResearchNodeChanged.Broadcast(Node);
	}
}

float AKPCLDeliveryTaskSubsystem::CalculateTaskProgress(const FKPCLDeliveryTaskState* TaskState)
{
	if (!TaskState || TaskState->mRequired.IsEmpty())
	{
		return 0.f;
	}

	int64 TotalRequired = 0;
	int64 TotalDelivered = 0;
	for (int32 Index = 0; Index < TaskState->mRequired.Num(); ++Index)
	{
		const int32 Required = FMath::Max(0, TaskState->mRequired[Index].Amount);
		const int32 Delivered = TaskState->mDelivered.IsValidIndex(Index) ? TaskState->mDelivered[Index].Amount : 0;
		TotalRequired += Required;
		TotalDelivered += FMath::Clamp(Delivered, 0, Required);
	}
	return TotalRequired > 0
		? FMath::Clamp(static_cast<float>(TotalDelivered) / static_cast<float>(TotalRequired), 0.f, 1.f)
		: 0.f;
}

void AKPCLDeliveryTaskSubsystem::OnRep_CompletedTasks(const TArray<UKAPIDeliveryTask*>& PreviousCompletedTasks)
{
	TSet<UKAPIDeliveryTask*> PreviousCompletedTaskSet;
	PreviousCompletedTaskSet.Reserve(PreviousCompletedTasks.Num());
	for (UKAPIDeliveryTask* Task : PreviousCompletedTasks)
	{
		if (IsValid(Task))
		{
			PreviousCompletedTaskSet.Add(Task);
		}
	}
	RebuildCompletedTaskCache();
	TSet<UKAPIDeliveryTask*> ChangedTasks;
	for (UKAPIDeliveryTask* Task : PreviousCompletedTaskSet)
	{
		if (!mCompletedTaskCache.Contains(Task))
		{
			ChangedTasks.Add(Task);
		}
	}
	for (UKAPIDeliveryTask* Task : mCompletedTaskCache)
	{
		if (!PreviousCompletedTaskSet.Contains(Task))
		{
			ChangedTasks.Add(Task);
		}
	}
	if (ChangedTasks.IsEmpty())
	{
		return;
	}

	mOnCompletedTasksChanged.Broadcast();
	for (UKAPIDeliveryTask* Task : ChangedTasks)
	{
		BroadcastResearchNodeChanged(Task);
		TArray<FText> TaskQueueErrors;
		mOnResearchAvailabilityChanged.Broadcast(Task, CanQueueTask(Task, TaskQueueErrors));
		for (UKAPIDeliveryTask* Child : GetResearchChildren(Task))
		{
			BroadcastResearchNodeChanged(Child);
			TArray<FText> QueueErrors;
			mOnResearchAvailabilityChanged.Broadcast(Child, CanQueueTask(Child, QueueErrors));
		}
	}
}

void AKPCLDeliveryTaskSubsystem::OnRep_QueuedTasks(const TArray<UKAPIDeliveryTask*>& PreviousQueuedTasks)
{
	UKAPIDeliveryTask* PreviousActiveTask = nullptr;
	for (UKAPIDeliveryTask* Task : PreviousQueuedTasks)
	{
		if (IsValid(Task))
		{
			PreviousActiveTask = Task;
			break;
		}
	}

	TMap<UKAPIDeliveryTask*, TArray<int32>> PreviousIndicesByTask;
	TMap<UKAPIDeliveryTask*, int32> PreviousCounts;
	for (int32 QueueIndex = 0; QueueIndex < PreviousQueuedTasks.Num(); ++QueueIndex)
	{
		if (UKAPIDeliveryTask* Task = GetValid(PreviousQueuedTasks[QueueIndex]))
		{
			++PreviousCounts.FindOrAdd(Task);
			PreviousIndicesByTask.FindOrAdd(Task).Add(QueueIndex);
		}
	}
	RebuildQueueCache();

	for (int32 QueueIndex = 0; QueueIndex < mQueuedTasks.Num(); ++QueueIndex)
	{
		UKAPIDeliveryTask* Task = GetValid(mQueuedTasks[QueueIndex]);
		if (!Task)
		{
			continue;
		}

		int32* PreviousCount = PreviousCounts.Find(Task);
		if (PreviousCount && *PreviousCount > 0)
		{
			--*PreviousCount;
		}
		else
		{
			mOnTaskQueued.Broadcast(Task, QueueIndex);
		}
	}

	TMap<UKAPIDeliveryTask*, int32> RemainingCurrentCounts;
	for (const TPair<UKAPIDeliveryTask*, TArray<int32>>& Pair : mQueueIndicesByTask)
	{
		RemainingCurrentCounts.Add(Pair.Key, Pair.Value.Num());
	}

	for (int32 PreviousIndex = 0; PreviousIndex < PreviousQueuedTasks.Num(); ++PreviousIndex)
	{
		UKAPIDeliveryTask* Task = GetValid(PreviousQueuedTasks[PreviousIndex]);
		if (!Task)
		{
			continue;
		}

		int32* CurrentCount = RemainingCurrentCounts.Find(Task);
		if (CurrentCount && *CurrentCount > 0)
		{
			--*CurrentCount;
		}
		else
		{
			mOnTaskRemovedFromQueue.Broadcast(Task, PreviousIndex);
		}
	}

	TSet<UKAPIDeliveryTask*> ChangedTasks;
	TSet<UKAPIDeliveryTask*> MembershipChangedTasks;
	for (const TPair<UKAPIDeliveryTask*, TArray<int32>>& Pair : PreviousIndicesByTask)
	{
		const TArray<int32>* CurrentIndices = mQueueIndicesByTask.Find(Pair.Key);
		if (!CurrentIndices || Pair.Value != *CurrentIndices)
		{
			ChangedTasks.Add(Pair.Key);
		}
		if (!CurrentIndices)
		{
			MembershipChangedTasks.Add(Pair.Key);
		}
	}
	for (const TPair<UKAPIDeliveryTask*, TArray<int32>>& Pair : mQueueIndicesByTask)
	{
		const TArray<int32>* PreviousIndices = PreviousIndicesByTask.Find(Pair.Key);
		if (!PreviousIndices || *PreviousIndices != Pair.Value)
		{
			ChangedTasks.Add(Pair.Key);
		}
		if (!PreviousIndices)
		{
			MembershipChangedTasks.Add(Pair.Key);
		}
	}

	mOnQueueChanged.Broadcast();
	mOnResearchQueueOrderChanged.Broadcast();
	UKAPIDeliveryTask* CurrentActiveTask = GetActiveTask();
	if (PreviousActiveTask != CurrentActiveTask)
	{
		mOnActiveResearchChanged.Broadcast(PreviousActiveTask, CurrentActiveTask);
	}
	for (UKAPIDeliveryTask* Task : ChangedTasks)
	{
		BroadcastResearchNodeChanged(Task);
		TArray<FText> QueueErrors;
		mOnResearchAvailabilityChanged.Broadcast(Task, CanQueueTask(Task, QueueErrors));
		if (MembershipChangedTasks.Contains(Task))
		{
			for (UKAPIDeliveryTask* Child : GetResearchChildren(Task))
			{
				BroadcastResearchNodeChanged(Child);
				TArray<FText> ChildQueueErrors;
				mOnResearchAvailabilityChanged.Broadcast(Child, CanQueueTask(Child, ChildQueueErrors));
			}
		}
	}

	const bool bPreviousQueueAtLimit = PreviousQueuedTasks.Num() >= GetQueueLimit();
	const bool bCurrentQueueAtLimit = mQueuedTasks.Num() >= GetQueueLimit();
	if (bPreviousQueueAtLimit != bCurrentQueueAtLimit)
	{
		BroadcastAllResearchAvailabilityChanged();
	}
	if (HasAuthority())
	{
		TryConsumeManualDeliveryInventory();
		RefreshManualDeliveryInventorySlotSize();
		if (!bIsProcessingTaskCompletion)
		{
			TryCompleteActiveTask();
		}
	}
}

void AKPCLDeliveryTaskSubsystem::OnRep_DeliveredTaskStates(const TArray<FKPCLDeliveryTaskState>& PreviousTaskStates)
{
	TMap<UKAPIDeliveryTask*, const FKPCLDeliveryTaskState*> PreviousStatesByTask;
	PreviousStatesByTask.Reserve(PreviousTaskStates.Num());
	for (const FKPCLDeliveryTaskState& PreviousState : PreviousTaskStates)
	{
		if (UKAPIDeliveryTask* Task = GetValid(PreviousState.mTask))
		{
			PreviousStatesByTask.FindOrAdd(Task, &PreviousState);
		}
	}
	bTaskStateCacheValid = false;
	RebuildTaskStateCache();
	bool bAnyStateChanged = false;
	for (const FKPCLDeliveryTaskState& State : mDeliveredTaskStates)
	{
		UKAPIDeliveryTask* Task = GetValid(State.mTask);
		if (!Task)
		{
			continue;
		}

		const FKPCLDeliveryTaskState* const* PreviousStatePtr = PreviousStatesByTask.Find(Task);
		const FKPCLDeliveryTaskState* PreviousState = PreviousStatePtr ? *PreviousStatePtr : nullptr;
		if (PreviousState && FKPCLDeliveryTaskState::StaticStruct()->CompareScriptStruct(PreviousState, &State, 0))
		{
			PreviousStatesByTask.Remove(Task);
			continue;
		}

		bAnyStateChanged = true;
		mOnTaskStateChanged.Broadcast(Task);
		const float PreviousProgress = CalculateTaskProgress(PreviousState);
		const float CurrentProgress = GetTaskResearchProgress(Task);
		if (!FMath::IsNearlyEqual(PreviousProgress, CurrentProgress))
		{
			mOnResearchProgressChanged.Broadcast(Task, CurrentProgress);
		}
		if (State.mCompleted > (PreviousState ? PreviousState->mCompleted : 0))
		{
			mOnTaskCompleted.Broadcast(Task, State.mCompleted);
		}
		BroadcastResearchNodeChanged(Task);
		if (State.mCompleted != (PreviousState ? PreviousState->mCompleted : 0))
		{
			for (UKAPIDeliveryTask* Child : GetResearchChildren(Task))
			{
				BroadcastResearchNodeChanged(Child);
				TArray<FText> QueueErrors;
				mOnResearchAvailabilityChanged.Broadcast(Child, CanQueueTask(Child, QueueErrors));
			}
		}
		PreviousStatesByTask.Remove(Task);
	}

	for (const TPair<UKAPIDeliveryTask*, const FKPCLDeliveryTaskState*>& Pair : PreviousStatesByTask)
	{
		UKAPIDeliveryTask* PreviousTask = Pair.Key;
		if (IsValid(PreviousTask))
		{
			bAnyStateChanged = true;
			mOnTaskStateChanged.Broadcast(PreviousTask);
			mOnResearchProgressChanged.Broadcast(PreviousTask, 0.f);
			BroadcastResearchNodeChanged(PreviousTask);
		}
	}
	if (bAnyStateChanged)
	{
		RefreshManualDeliveryInventorySlotSize();
	}
}

void AKPCLDeliveryTaskSubsystem::OnRep_RewardErrorText() { mOnRewardErrorChanged.Broadcast(mRewardErrorText); }

TArray<FItemAmount> UKPCLDeliveryTaskStateLib::GetLeftoverItems(const FKPCLDeliveryTaskState& State)
{
	TArray<FItemAmount> Leftover;
	Leftover.Reserve(State.mRequired.Num());
	for (const FItemAmount& Required : State.mRequired)
	{
		const int32 Missing = GetLeftoverAmount(State, Required.ItemClass);
		if (Missing > 0)
		{
			Leftover.Emplace(Required.ItemClass, Missing);
		}
	}
	return Leftover;
}

TArray<FItemAmount> UKPCLDeliveryTaskStateLib::GetDeliveredItems(const FKPCLDeliveryTaskState& State)
{
	TArray<FItemAmount> Delivered;
	Delivered.Reserve(State.mRequired.Num());
	for (const FItemAmount& Required : State.mRequired)
	{
		const int32 Amount = GetDeliveredAmount(State, Required.ItemClass);
		if (Amount > 0)
		{
			Delivered.Emplace(Required.ItemClass, Amount);
		}
	}
	return Delivered;
}

TArray<TSubclassOf<UFGItemDescriptor>>
UKPCLDeliveryTaskStateLib::GetUniqueItemsToDeliver(const FKPCLDeliveryTaskState& State, bool bFilterDelivered)
{
	TArray<TSubclassOf<UFGItemDescriptor>> Items;
	Items.Reserve(State.mRequired.Num());
	for (const FItemAmount& Required : State.mRequired)
	{
		if (!IsValid(Required.ItemClass) || Required.Amount <= 0)
		{
			continue;
		}
		if (bFilterDelivered && GetLeftoverAmount(State, Required.ItemClass) <= 0)
		{
			continue;
		}

		Items.AddUnique(Required.ItemClass);
	}
	return Items;
}

float UKPCLDeliveryTaskStateLib::GetPercentDone(const FKPCLDeliveryTaskState& State)
{
	const int32 TotalRequired = GetTotalRequiredAmount(State);
	if (TotalRequired <= 0)
	{
		return 1.f;
	}
	return FMath::Clamp(static_cast<float>(GetTotalDeliveredAmount(State)) / static_cast<float>(TotalRequired), 0.f,
						1.f);
}

float UKPCLDeliveryTaskStateLib::GetPercentDelivered(const FKPCLDeliveryTaskState& State,
													 TSubclassOf<UFGItemDescriptor> ItemClass)
{
	const int32 Required = GetRequiredAmount(State, ItemClass);
	if (Required <= 0)
	{
		return 0.f;
	}
	return FMath::Clamp(static_cast<float>(GetDeliveredAmount(State, ItemClass)) / static_cast<float>(Required), 0.f,
						1.f);
}

int32 UKPCLDeliveryTaskStateLib::GetRequiredAmount(const FKPCLDeliveryTaskState& State,
												   TSubclassOf<UFGItemDescriptor> ItemClass)
{
	return FMath::Max(0, FItemAmount::GetAmountFromItemAmounts(State.mRequired, ItemClass));
}

int32 UKPCLDeliveryTaskStateLib::GetDeliveredAmount(const FKPCLDeliveryTaskState& State,
													TSubclassOf<UFGItemDescriptor> ItemClass)
{

	const int32 Delivered = FMath::Max(0, FItemAmount::GetAmountFromItemAmounts(State.mDelivered, ItemClass));
	return FMath::Min(Delivered, GetRequiredAmount(State, ItemClass));
}

int32 UKPCLDeliveryTaskStateLib::GetLeftoverAmount(const FKPCLDeliveryTaskState& State,
												   TSubclassOf<UFGItemDescriptor> ItemClass)
{
	return GetRequiredAmount(State, ItemClass) - GetDeliveredAmount(State, ItemClass);
}

int32 UKPCLDeliveryTaskStateLib::GetTotalRequiredAmount(const FKPCLDeliveryTaskState& State)
{
	int32 Total = 0;
	for (const FItemAmount& Required : State.mRequired)
	{
		Total += FMath::Max(0, Required.Amount);
	}
	return Total;
}

int32 UKPCLDeliveryTaskStateLib::GetTotalDeliveredAmount(const FKPCLDeliveryTaskState& State)
{
	int32 Total = 0;
	for (const FItemAmount& Required : State.mRequired)
	{
		Total += GetDeliveredAmount(State, Required.ItemClass);
	}
	return Total;
}

int32 UKPCLDeliveryTaskStateLib::GetTotalLeftoverAmount(const FKPCLDeliveryTaskState& State)
{
	return GetTotalRequiredAmount(State) - GetTotalDeliveredAmount(State);
}

bool UKPCLDeliveryTaskStateLib::IsFullyDelivered(const FKPCLDeliveryTaskState& State)
{
	return GetTotalLeftoverAmount(State) <= 0;
}

bool UKPCLDeliveryTaskStateLib::IsPartiallyDelivered(const FKPCLDeliveryTaskState& State)
{
	const int32 Delivered = GetTotalDeliveredAmount(State);
	return Delivered > 0 && Delivered < GetTotalRequiredAmount(State);
}

bool UKPCLDeliveryTaskStateLib::IsValidTaskState(const FKPCLDeliveryTaskState& State) { return IsValid(State.mTask); }
