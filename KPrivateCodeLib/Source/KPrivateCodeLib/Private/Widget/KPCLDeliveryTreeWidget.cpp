// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Widget/KPCLDeliveryTreeWidget.h"

#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "Widget/KPCLDeliverNodeWidget.h"
#include "Widget/KPCLDeliveryPreviewWidget.h"
#include "Widget/KPCLDeliveryQueueWidget.h"

int32 UKPCLDeliveryTreeConnectionLayer::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
													const FSlateRect& MyCullingRect,
													FSlateWindowElementList& OutDrawElements, int32 LayerId,
													const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId,
											  InWidgetStyle, bParentEnabled);
	if (!IsValid(mDeliveryTreeWidget) || !mDeliveryTreeWidget->bAutomaticallyDrawResearchConnections)
	{
		return MaxLayer;
	}

	FPaintContext Context(AllottedGeometry, MyCullingRect, OutDrawElements, MaxLayer, InWidgetStyle, bParentEnabled);

	mDeliveryTreeWidget->DrawResearchConnectionsInternal(Context, FVector2D::ZeroVector, 1.0f);
	return FMath::Max(MaxLayer, Context.MaxLayer);
}

UKPCLDeliveryTreeWidget::UKPCLDeliveryTreeWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	mDeliveryNodeWidgetClass = UKPCLDeliverNodeWidget::StaticClass();
	mNavigationDragMouseButton = EKeys::LeftMouseButton;
}

void UKPCLDeliveryTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	InitializeDeliveryTreeNavigationOffset();
	SetupDeliveryTree(mDeliveryTaskSubsystem);
	SynchronizeNodeCanvas();
}

void UKPCLDeliveryTreeWidget::NativeDestruct()
{
	EndDeliveryTreeNavigationDrag();
	SetHoveredDeliveryTask(nullptr);
	Super::NativeDestruct();
}

void UKPCLDeliveryTreeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	InitializeDeliveryTreeNavigationOffset();
	ProcessIncrementalDeliveryTreeTick();
	if (bValidateCachedNodesOnTick)
	{
		ValidateCachedDeliveryNodes();
	}
}

FReply UKPCLDeliveryTreeWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	if (Reply.IsEventHandled())
	{
		return Reply;
	}

	const FVector2D LocalPointerPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	const bool bStartedNavigation = bEnableDragNavigation &&
		InMouseEvent.GetEffectingButton() == mNavigationDragMouseButton &&
		BeginDeliveryTreeNavigationDrag(LocalPointerPosition);
	return bStartedNavigation ? FReply::Handled().CaptureMouse(TakeWidget()) : Reply;
}

FReply UKPCLDeliveryTreeWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const bool bFinishesNavigation =
		bIsNavigationDragging && InMouseEvent.GetEffectingButton() == mNavigationDragMouseButton;
	if (bFinishesNavigation)
	{
		EndDeliveryTreeNavigationDrag();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UKPCLDeliveryTreeWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bIsNavigationDragging)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	if (!bEnableDragNavigation || !InMouseEvent.IsMouseButtonDown(mNavigationDragMouseButton))
	{
		EndDeliveryTreeNavigationDrag();
		return FReply::Handled().ReleaseMouseCapture();
	}

	const FVector2D LocalPointerPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	UpdateDeliveryTreeNavigationDrag(LocalPointerPosition);
	return FReply::Handled();
}

FReply UKPCLDeliveryTreeWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const float WheelDelta = InMouseEvent.GetWheelDelta();
	if (!bEnableZoomNavigation || FMath::IsNearlyZero(WheelDelta))
	{
		return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
	}

	const FVector2D LocalPointerPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	SetDeliveryTreeZoomAroundPosition(mZoomLevel + FMath::Max(0.001f, mZoomStep) * WheelDelta, LocalPointerPosition);
	return FReply::Handled();
}

void UKPCLDeliveryTreeWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	EndDeliveryTreeNavigationDrag();
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

bool UKPCLDeliveryTreeWidget::SetupDeliveryTree(AKPCLDeliveryTaskSubsystem* DeliveryTaskSubsystem)
{
	AKPCLDeliveryTaskSubsystem* ResolvedSubsystem =
		IsValid(DeliveryTaskSubsystem) ? DeliveryTaskSubsystem : AKPCLDeliveryTaskSubsystem::Get(this);
	if (!IsValid(ResolvedSubsystem))
	{
		return false;
	}

	if (bIsSetup && mDeliveryTaskSubsystem == ResolvedSubsystem)
	{
		const bool bRefreshed = RefreshDeliveryTree();
		SynchronizeDeliveryAuxiliaryWidgets();
		return bRefreshed;
	}

	TeardownDeliveryTree();
	InitializeDeliveryTreeNavigationOffset();
	mDeliveryTaskSubsystem = ResolvedSubsystem;
	BindSubsystemEvents();
	bIsSetup = true;
	RefreshDeliveryTree();
	SynchronizeDeliveryAuxiliaryWidgets();
	OnDeliveryTreeSetup(mDeliveryTaskSubsystem);
	return true;
}

void UKPCLDeliveryTreeWidget::TeardownDeliveryTree()
{
	const bool bWasSetup = bIsSetup;
	EndDeliveryTreeNavigationDrag();
	TeardownDeliveryAuxiliaryWidgets();
	UnbindSubsystemEvents();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mDetachedUpdateTimerHandle);
	}
	mDetachedUpdateTimerHandle.Invalidate();
	UKAPIDeliveryTask* PreviousHoveredTask = mHoveredTask;
	UKAPIDeliveryTask* PreviousSelectedTask = mSelectedTask;
	mHoveredTask = nullptr;
	mSelectedTask = nullptr;
	mDependencyHighlightedTasks.Reset();
	UpdateDeliveryNodeInteractionStates();
	if (IsValid(PreviousHoveredTask))
	{
		OnHoveredDeliveryTaskChanged(PreviousHoveredTask, nullptr);
	}
	if (IsValid(PreviousSelectedTask))
	{
		OnSelectedDeliveryTaskChanged(PreviousSelectedTask, nullptr);
	}
	OnDeliveryDependencyHighlightChanged(nullptr);
	DestroyConnectionLayer();
	DestroyAllDeliveryNodeWidgets();
	mDeliveryTaskSubsystem = nullptr;
	mResearchNodes.Reset();
	mResearchEdges.Reset();
	mQueuedTasks.Reset();
	mActiveTask = nullptr;
	mTreeMinCoordinate = FIntVector2::ZeroValue;
	mTreeMaxCoordinate = FIntVector2::ZeroValue;
	bHasTreeBounds = false;
	bIsSetup = false;
	mVisibleResearchEdgeCount = 0;
	ResetPendingDeliveryTreeWork();
	bFullRefreshRequested = false;
	bRefreshCompletionPending = false;
	bIsTreeUpdateInProgress = false;
	mCompletedTreeUpdateOperations = 0;
	mTotalTreeUpdateOperations = 0;
	mNextNodeValidationIndex = 0;
	mResearchNodeIndexByTask.Reset();
	mResearchNodeIndexByCoordinate.Reset();
	mDeliveryNodeWidgetByTask.Reset();
	mResearchParentsByTask.Reset();
	mParentEdgeIndicesByTask.Reset();
	mChildEdgeIndicesByTask.Reset();
	mAppliedHoveredTask.Reset();
	mAppliedSelectedTask.Reset();
	mAppliedDependencyHighlightedTasks.Reset();
	mNavigationOffset = FVector2D::ZeroVector;
	mNavigationDragLastLocalPosition = FVector2D::ZeroVector;
	bNavigationOffsetInitialized = false;
	ApplyDeliveryTreeNavigationOffset();

	if (bWasSetup)
	{
		OnDeliveryTreeDestroyed();
	}
}

int32 UKPCLDeliveryTreeWidget::ValidateCachedDeliveryNodes(int32 MaxNodeValidations)
{
	if (!bIsSetup || !IsValid(mDeliveryTaskSubsystem) || mResearchNodes.IsEmpty() || bFullRefreshRequested)
	{
		return 0;
	}

	const int32 ValidationLimit =
		MaxNodeValidations < 0 ? FMath::Max(0, mMaxNodeValidationsPerTick) : FMath::Max(0, MaxNodeValidations);
	if (ValidationLimit == 0)
	{
		return 0;
	}

	int32 ChangedNodes = 0;
	const int32 NodesToValidate = FMath::Min(ValidationLimit, mResearchNodes.Num());
	for (int32 ValidationIndex = 0; ValidationIndex < NodesToValidate; ++ValidationIndex)
	{
		mNextNodeValidationIndex %= mResearchNodes.Num();
		UKAPIDeliveryTask* Task = GetValid(mResearchNodes[mNextNodeValidationIndex++].mTask);
		if (!Task)
		{
			RequestDeliveryTreeRefresh();
			break;
		}

		const int32* CachedNodeIndex = mResearchNodeIndexByTask.Find(Task);
		FKPCLResearchTreeNode CurrentNode;
		if (!CachedNodeIndex || !mResearchNodes.IsValidIndex(*CachedNodeIndex) ||
			!mDeliveryTaskSubsystem->GetResearchNode(Task, CurrentNode))
		{
			RequestDeliveryTreeRefresh();
			break;
		}
		if (!HasSameNodeData(mResearchNodes[*CachedNodeIndex], CurrentNode))
		{
			HandleResearchNodeChanged(CurrentNode);
			++ChangedNodes;
		}
	}
	return ChangedNodes;
}

bool UKPCLDeliveryTreeWidget::RefreshDeliveryTree()
{
	if (!IsValid(mDeliveryTaskSubsystem))
	{
		return false;
	}

	bFullRefreshRequested = false;
	if (!PrepareDeliveryTreeRefresh())
	{
		return false;
	}
	MarkDirty(EKPCLDeliveryWidgetDirty::Data | EKPCLDeliveryWidgetDirty::State | EKPCLDeliveryWidgetDirty::Queue |
			  EKPCLDeliveryWidgetDirty::Layout | EKPCLDeliveryWidgetDirty::Connections);
	if (!bIncrementalTreeUpdates)
	{
		FlushPendingDeliveryTreeUpdates();
	}
	return true;
}

void UKPCLDeliveryTreeWidget::RequestDeliveryTreeRefresh()
{
	if (!IsValid(mDeliveryTaskSubsystem))
	{
		return;
	}
	bFullRefreshRequested = true;
	if (!bIncrementalTreeUpdates)
	{
		FlushPendingDeliveryTreeUpdates();
	}
	else
	{
		ScheduleDetachedDeliveryTreeUpdate();
	}
}

bool UKPCLDeliveryTreeWidget::RequestDeliveryNodeRefresh(UKAPIDeliveryTask* Task)
{
	if (!IsValid(mDeliveryTaskSubsystem) || !IsCachedTask(Task))
	{
		return false;
	}
	QueueDeliveryNodeUpdate(Task, false);
	if (!bIncrementalTreeUpdates)
	{
		FlushPendingDeliveryTreeUpdates();
	}
	return true;
}

int32 UKPCLDeliveryTreeWidget::ProcessPendingDeliveryTreeUpdates(int32 MaxNodeOperations, int32 MaxConnectionOperations,
																 float TimeBudgetMs)
{
	if (!IsValid(mDeliveryTaskSubsystem) || !HasPendingDeliveryTreeUpdates())
	{
		return 0;
	}

	const int32 NodeLimit = MaxNodeOperations < 0 ? FMath::Max(0, mMaxNodeOperationsPerTick) : MaxNodeOperations;
	const int32 ConnectionLimit =
		MaxConnectionOperations < 0 ? FMath::Max(0, mMaxConnectionOperationsPerTick) : MaxConnectionOperations;
	const float ResolvedTimeBudgetMs = TimeBudgetMs < 0.0f ? FMath::Max(0.0f, mTreeUpdateTimeBudgetMs) : TimeBudgetMs;
	const double StartTime = FPlatformTime::Seconds();
	const auto IsWithinTimeBudget = [StartTime, ResolvedTimeBudgetMs]()
	{
		return ResolvedTimeBudgetMs <= 0.0f ||
			(FPlatformTime::Seconds() - StartTime) * 1000.0 < static_cast<double>(ResolvedTimeBudgetMs);
	};

	if (!bIsTreeUpdateInProgress)
	{
		bIsTreeUpdateInProgress = true;
		mCompletedTreeUpdateOperations = 0;
		mTotalTreeUpdateOperations = FMath::Max(1, GetPendingDeliveryTreeOperationCount());
		OnDeliveryTreeUpdateStarted(mTotalTreeUpdateOperations);
	}

	int32 ProcessedOperations = 0;
	int32 ProcessedNodeOperations = 0;
	int32 ProcessedConnectionOperations = 0;
	bool bConnectionsChanged = false;
	if (bFullRefreshRequested && IsWithinTimeBudget())
	{
		bFullRefreshRequested = false;
		PrepareDeliveryTreeRefresh();
		++ProcessedOperations;
		++mCompletedTreeUpdateOperations;
	}

	while (mPendingNodeRemovalIndex < mPendingNodeRemovals.Num() &&
		   (NodeLimit == 0 || ProcessedNodeOperations < NodeLimit) && IsWithinTimeBudget())
	{
		const TWeakObjectPtr<UKAPIDeliveryTask> Task = mPendingNodeRemovals[mPendingNodeRemovalIndex++];
		mPendingNodeRemovalSet.Remove(Task);
		if (Task.IsValid())
		{
			DestroyDeliveryNodeWidget(GetDeliveryNodeWidget(Task.Get()));
		}
		++ProcessedNodeOperations;
		++ProcessedOperations;
		++mCompletedTreeUpdateOperations;
	}

	while (mPendingNodeUpdateIndex < mPendingNodeUpdates.Num() &&
		   (NodeLimit == 0 || ProcessedNodeOperations < NodeLimit) && IsWithinTimeBudget())
	{
		const TWeakObjectPtr<UKAPIDeliveryTask> Task = mPendingNodeUpdates[mPendingNodeUpdateIndex++];
		mPendingNodeUpdateSet.Remove(Task);
		const bool bUseCachedNode = mPendingCachedNodeTasks.Remove(Task) > 0;
		if (Task.IsValid())
		{
			ApplyPendingDeliveryNodeUpdate(Task.Get(), bUseCachedNode);
		}
		++ProcessedNodeOperations;
		++ProcessedOperations;
		++mCompletedTreeUpdateOperations;
	}

	while (mVisibleResearchEdgeCount < mResearchEdges.Num() &&
		   (ConnectionLimit == 0 || ProcessedConnectionOperations < ConnectionLimit) && IsWithinTimeBudget())
	{
		++mVisibleResearchEdgeCount;
		++ProcessedConnectionOperations;
		++ProcessedOperations;
		++mCompletedTreeUpdateOperations;
		bConnectionsChanged = true;
	}

	if (mPendingNodeRemovalIndex >= mPendingNodeRemovals.Num())
	{
		mPendingNodeRemovals.Reset();
		mPendingNodeRemovalIndex = 0;
	}
	if (mPendingNodeUpdateIndex >= mPendingNodeUpdates.Num())
	{
		mPendingNodeUpdates.Reset();
		mPendingNodeUpdateIndex = 0;
	}
	if (bConnectionsChanged)
	{
		InvalidateConnectionLayer();
	}

	const int32 RemainingOperations = GetPendingDeliveryTreeOperationCount();
	mTotalTreeUpdateOperations =
		FMath::Max(mTotalTreeUpdateOperations, mCompletedTreeUpdateOperations + RemainingOperations);
	OnDeliveryTreeUpdateProgress(mCompletedTreeUpdateOperations, mTotalTreeUpdateOperations);
	const bool bOnlyCompletionRemains = !bFullRefreshRequested &&
		mPendingNodeRemovalIndex >= mPendingNodeRemovals.Num() &&
		mPendingNodeUpdateIndex >= mPendingNodeUpdates.Num() && mVisibleResearchEdgeCount >= mResearchEdges.Num();
	if (bOnlyCompletionRemains)
	{
		FinalizePendingDeliveryTreeUpdates();
	}
	return ProcessedOperations;
}

void UKPCLDeliveryTreeWidget::FlushPendingDeliveryTreeUpdates()
{
	if (!IsValid(mDeliveryTaskSubsystem))
	{
		ResetPendingDeliveryTreeWork();
		bFullRefreshRequested = false;
		bRefreshCompletionPending = false;
		bIsTreeUpdateInProgress = false;
		return;
	}
	int32 SafetyCounter = 0;
	while (HasPendingDeliveryTreeUpdates() && SafetyCounter++ < 1024)
	{
		ProcessPendingDeliveryTreeUpdates(0, 0, 0.0f);
	}
}

bool UKPCLDeliveryTreeWidget::HasPendingDeliveryTreeUpdates() const
{
	return bFullRefreshRequested || bRefreshCompletionPending ||
		mPendingNodeRemovalIndex < mPendingNodeRemovals.Num() || mPendingNodeUpdateIndex < mPendingNodeUpdates.Num() ||
		mVisibleResearchEdgeCount < mResearchEdges.Num();
}

int32 UKPCLDeliveryTreeWidget::GetPendingDeliveryTreeOperationCount() const
{
	const int32 PendingRefresh = bFullRefreshRequested || bRefreshCompletionPending ? 1 : 0;
	return PendingRefresh + FMath::Max(0, mPendingNodeRemovals.Num() - mPendingNodeRemovalIndex) +
		FMath::Max(0, mPendingNodeUpdates.Num() - mPendingNodeUpdateIndex) +
		FMath::Max(0, mResearchEdges.Num() - mVisibleResearchEdgeCount);
}

float UKPCLDeliveryTreeWidget::GetDeliveryTreeUpdateProgress() const
{
	if (!bIsTreeUpdateInProgress)
	{
		return 1.0f;
	}
	return mTotalTreeUpdateOperations > 0 ? FMath::Clamp(static_cast<float>(mCompletedTreeUpdateOperations) /
															 static_cast<float>(mTotalTreeUpdateOperations),
														 0.0f, 1.0f)
										  : 0.0f;
}

void UKPCLDeliveryTreeWidget::SetNodeCanvas(UCanvasPanel* NodeCanvas)
{
	if (mNodeCanvas == NodeCanvas)
	{
		SynchronizeNodeCanvas();
		return;
	}

	UCanvasPanel* PreviousCanvas = mNodeCanvas;
	if (IsValid(mConnectionLayerWidget) && mConnectionLayerWidget->GetParent() == PreviousCanvas)
	{
		mConnectionLayerWidget->RemoveFromParent();
	}
	for (UKPCLDeliverNodeWidget* NodeWidget : mDeliveryNodeWidgets)
	{
		if (IsValid(NodeWidget) && NodeWidget->GetParent() == PreviousCanvas)
		{
			NodeWidget->RemoveFromParent();
		}
	}
	if (IsValid(PreviousCanvas))
	{
		PreviousCanvas->SetRenderTranslation(FVector2D::ZeroVector);
	}
	mNodeCanvas = NodeCanvas;
	SynchronizeNodeCanvas();
	InvalidateConnectionLayer();
}

bool UKPCLDeliveryTreeWidget::SynchronizeNodeCanvas()
{
	if (!IsValid(mNodeCanvas))
	{
		return false;
	}

	InitializeDeliveryTreeNavigationOffset();
	ApplyDeliveryTreeNavigationOffset();
	EnsureConnectionLayer();
	for (UKPCLDeliverNodeWidget* NodeWidget : mDeliveryNodeWidgets)
	{
		AttachDeliveryNodeWidgetToCanvas(NodeWidget);
	}
	MarkDirty(EKPCLDeliveryWidgetDirty::Layout);
	return true;
}

bool UKPCLDeliveryTreeWidget::AttachDeliveryNodeWidgetToCanvas(UKPCLDeliverNodeWidget* NodeWidget)
{
	if (!IsValid(mNodeCanvas) || !IsValid(NodeWidget))
	{
		return false;
	}

	if (NodeWidget->GetParent() != mNodeCanvas)
	{
		NodeWidget->RemoveFromParent();
		if (!IsValid(mNodeCanvas->AddChildToCanvas(NodeWidget)))
		{
			return false;
		}
	}
	return PositionDeliveryNodeWidget(NodeWidget);
}

bool UKPCLDeliveryTreeWidget::PositionDeliveryNodeWidget(UKPCLDeliverNodeWidget* NodeWidget)
{
	UCanvasPanelSlot* CanvasSlot = GetDeliveryNodeCanvasSlot(NodeWidget);
	if (!IsValid(CanvasSlot))
	{
		return false;
	}

	CanvasSlot->SetPosition(GetNodePosition(NodeWidget->mNode.mCoordinate) + mNodePositionOffset);
	CanvasSlot->SetAlignment(mNodeAlignment);
	CanvasSlot->SetAutoSize(bAutoSizeNodeSlots);
	if (!bAutoSizeNodeSlots)
	{
		CanvasSlot->SetSize(mNodeSlotSize);
	}
	CanvasSlot->SetZOrder(mNodeZOrder);
	return true;
}

UCanvasPanelSlot* UKPCLDeliveryTreeWidget::GetDeliveryNodeCanvasSlot(UKPCLDeliverNodeWidget* NodeWidget) const
{
	return IsValid(NodeWidget) && NodeWidget->GetParent() == mNodeCanvas ? Cast<UCanvasPanelSlot>(NodeWidget->Slot)
																		 : nullptr;
}

void UKPCLDeliveryTreeWidget::SetDeliveryQueueWidget(UKPCLDeliveryQueueWidget* DeliveryQueueWidget)
{
	if (mDeliveryQueueWidget == DeliveryQueueWidget)
	{
		if (bIsSetup && IsValid(mDeliveryQueueWidget))
		{
			mDeliveryQueueWidget->SetupDeliveryQueue(this);
		}
		return;
	}

	if (IsValid(mDeliveryQueueWidget))
	{
		mDeliveryQueueWidget->TeardownDeliveryQueue();
	}
	mDeliveryQueueWidget = DeliveryQueueWidget;
	if (bIsSetup && IsValid(mDeliveryQueueWidget))
	{
		mDeliveryQueueWidget->SetupDeliveryQueue(this);
	}
}

void UKPCLDeliveryTreeWidget::SetDeliveryPreviewWidget(UKPCLDeliveryPreviewWidget* DeliveryPreviewWidget)
{
	if (mDeliveryPreviewWidget == DeliveryPreviewWidget)
	{
		if (bIsSetup && IsValid(mDeliveryPreviewWidget))
		{
			mDeliveryPreviewWidget->SetupDeliveryPreview(this);
		}
		return;
	}

	if (IsValid(mDeliveryPreviewWidget))
	{
		mDeliveryPreviewWidget->TeardownDeliveryPreview();
	}
	mDeliveryPreviewWidget = DeliveryPreviewWidget;
	if (bIsSetup && IsValid(mDeliveryPreviewWidget))
	{
		mDeliveryPreviewWidget->SetupDeliveryPreview(this);
	}
}

void UKPCLDeliveryTreeWidget::SynchronizeDeliveryAuxiliaryWidgets()
{
	if (!bIsSetup)
	{
		return;
	}
	if (IsValid(mDeliveryQueueWidget))
	{
		mDeliveryQueueWidget->SetupDeliveryQueue(this);
	}
	if (IsValid(mDeliveryPreviewWidget))
	{
		mDeliveryPreviewWidget->SetupDeliveryPreview(this);
	}
}

UPanelSlot* UKPCLDeliveryTreeWidget::AttachToPanel(UPanelWidget* TargetPanel, FMargin SlotPadding, int32 Index)
{
	if (!IsValid(TargetPanel))
	{
		return nullptr;
	}
	if (GetParent() == TargetPanel)
	{
		return Slot;
	}

	AKPCLDeliveryTaskSubsystem* DeliveryTaskSubsystem = mDeliveryTaskSubsystem;
	RemoveFromParent();
	const int32 ResolvedIndex =
		Index == INDEX_NONE ? INDEX_NONE : FMath::Clamp(Index, 0, TargetPanel->GetChildrenCount());
	UPanelSlot* AttachedSlot =
		UKPCLBlueprintFunctionLib::InjectWidgetAt(TargetPanel, this, SlotPadding, ResolvedIndex, false);
	SetupDeliveryTree(DeliveryTaskSubsystem);
	return AttachedSlot;
}

void UKPCLDeliveryTreeWidget::DetachFromPanel() { RemoveFromParent(); }

bool UKPCLDeliveryTreeWidget::BeginDeliveryTreeNavigationDrag(FVector2D LocalPointerPosition)
{
	if (!bEnableDragNavigation || bIsNavigationDragging || !IsValid(mNodeCanvas))
	{
		return false;
	}

	InitializeDeliveryTreeNavigationOffset();
	mNavigationDragLastLocalPosition = LocalPointerPosition;
	bIsNavigationDragging = true;
	OnDeliveryTreeNavigationDragStarted(mNavigationOffset);
	return true;
}

bool UKPCLDeliveryTreeWidget::UpdateDeliveryTreeNavigationDrag(FVector2D LocalPointerPosition)
{
	if (!bIsNavigationDragging)
	{
		return false;
	}

	const FVector2D DragDelta =
		(LocalPointerPosition - mNavigationDragLastLocalPosition) * FMath::Max(0.0f, mNavigationDragSensitivity);
	mNavigationDragLastLocalPosition = LocalPointerPosition;
	return SetDeliveryTreeNavigationOffset(mNavigationOffset + DragDelta);
}

void UKPCLDeliveryTreeWidget::EndDeliveryTreeNavigationDrag()
{
	if (!bIsNavigationDragging)
	{
		return;
	}

	bIsNavigationDragging = false;
	OnDeliveryTreeNavigationDragFinished(mNavigationOffset);
}

bool UKPCLDeliveryTreeWidget::SetDeliveryTreeNavigationOffset(FVector2D NavigationOffset)
{
	InitializeDeliveryTreeNavigationOffset();
	const FVector2D ResolvedOffset = ClampDeliveryTreeNavigationOffset(NavigationOffset);
	if (mNavigationOffset.Equals(ResolvedOffset))
	{
		return false;
	}

	const FVector2D PreviousOffset = mNavigationOffset;
	mNavigationOffset = ResolvedOffset;

	bNavigationOffsetInitialized = true;
	ApplyDeliveryTreeNavigationOffset();
	MarkDirty(EKPCLDeliveryWidgetDirty::Layout | EKPCLDeliveryWidgetDirty::Connections);
	OnDeliveryTreeNavigationOffsetChanged(PreviousOffset, mNavigationOffset);
	return true;
}

void UKPCLDeliveryTreeWidget::ResetDeliveryTreeNavigationOffset()
{
	if (bFocusInitialCoordinateOnSetup && FocusDeliveryTreeCoordinate(mInitialFocusCoordinate))
	{
		return;
	}
	SetDeliveryTreeNavigationOffset(mInitialNavigationOffset);
}

bool UKPCLDeliveryTreeWidget::FocusDeliveryTreeCoordinate(FIntVector2 Coordinate)
{
	if (GetDeliveryTreeViewSize().IsNearlyZero())
	{
		return false;
	}

	SetDeliveryTreeNavigationOffset(GetDeliveryTreeFocusOffset(Coordinate));
	return true;
}

bool UKPCLDeliveryTreeWidget::FocusDeliveryTask(UKAPIDeliveryTask* Task)
{
	if (!IsValid(Task))
	{
		return false;
	}

	FKPCLResearchTreeNode Node;
	const FIntVector2 Coordinate = GetCachedNode(Task, Node) ? Node.mCoordinate : Task->mCoordinate;
	return FocusDeliveryTreeCoordinate(Coordinate);
}

bool UKPCLDeliveryTreeWidget::FocusActiveDeliveryTask()
{
	UKAPIDeliveryTask* ActiveTask = mActiveTask.Get();
	if (!IsValid(ActiveTask) && IsValid(mDeliveryTaskSubsystem))
	{
		ActiveTask = mDeliveryTaskSubsystem->GetActiveTask();
	}
	return FocusDeliveryTask(ActiveTask);
}

FVector2D UKPCLDeliveryTreeWidget::GetDeliveryTreeFocusOffset(FIntVector2 Coordinate) const
{
	return GetDeliveryTreeViewSize() * 0.5 - GetDeliveryNodeCenter(Coordinate) * mZoomLevel;
}

FVector2D UKPCLDeliveryTreeWidget::GetDeliveryNodeCenter(FIntVector2 Coordinate) const
{

	return GetNodePosition(Coordinate) + mNodePositionOffset +
		GetDeliveryNodeSize(Coordinate) * (FVector2D(0.5, 0.5) - mNodeAlignment);
}

FVector2D UKPCLDeliveryTreeWidget::GetDeliveryNodeSize(FIntVector2 Coordinate) const
{
	if (!bAutoSizeNodeSlots)
	{
		return mNodeSlotSize;
	}

	FKPCLResearchTreeNode Node;
	if (GetCachedNodeAtCoordinate(Coordinate, Node))
	{
		if (const UKPCLDeliverNodeWidget* NodeWidget = GetDeliveryNodeWidget(Node.mTask.Get()))
		{
			const FVector2D DesiredSize = NodeWidget->GetDesiredSize();
			if (!DesiredSize.IsNearlyZero())
			{
				return DesiredSize;
			}
		}
	}
	return mNodeSlotSize;
}

FVector2D UKPCLDeliveryTreeWidget::GetDeliveryTreeViewSize() const
{

	const FVector2D CanvasSize =
		IsValid(mNodeCanvas) ? mNodeCanvas->GetCachedGeometry().GetLocalSize() : FVector2D::ZeroVector;
	return CanvasSize.IsNearlyZero() ? GetCachedGeometry().GetLocalSize() : CanvasSize;
}

float UKPCLDeliveryTreeWidget::ClampDeliveryTreeZoom(float ZoomLevel) const
{
	const float MinZoom = FMath::Max(0.01f, FMath::Min(mMinZoomLevel, mMaxZoomLevel));
	const float MaxZoom = FMath::Max(MinZoom, mMaxZoomLevel);
	return FMath::Clamp(ZoomLevel, MinZoom, MaxZoom);
}

bool UKPCLDeliveryTreeWidget::SetDeliveryTreeZoom(float ZoomLevel)
{
	return SetDeliveryTreeZoomAroundPosition(ZoomLevel, GetDeliveryTreeViewSize() * 0.5);
}

bool UKPCLDeliveryTreeWidget::SetDeliveryTreeZoomAroundPosition(float ZoomLevel, FVector2D LocalPivot)
{
	InitializeDeliveryTreeNavigationOffset();
	const float ResolvedZoom = ClampDeliveryTreeZoom(ZoomLevel);
	const float PreviousZoom = FMath::Max(0.01f, mZoomLevel);
	if (FMath::IsNearlyEqual(ResolvedZoom, PreviousZoom))
	{
		return false;
	}

	const FVector2D PivotTreePosition = (LocalPivot - mNavigationOffset) / PreviousZoom;
	mZoomLevel = ResolvedZoom;
	mNavigationOffset = ClampDeliveryTreeNavigationOffset(LocalPivot - PivotTreePosition * mZoomLevel);
	bNavigationOffsetInitialized = true;
	ApplyDeliveryTreeNavigationOffset();
	MarkDirty(EKPCLDeliveryWidgetDirty::Layout | EKPCLDeliveryWidgetDirty::Connections);
	OnDeliveryTreeZoomChanged(PreviousZoom, mZoomLevel);
	return true;
}

bool UKPCLDeliveryTreeWidget::StepDeliveryTreeZoom(int32 Steps)
{
	return SetDeliveryTreeZoom(mZoomLevel + FMath::Max(0.001f, mZoomStep) * Steps);
}

void UKPCLDeliveryTreeWidget::ResetDeliveryTreeZoom() { SetDeliveryTreeZoom(mInitialZoomLevel); }

FVector2D UKPCLDeliveryTreeWidget::ClampDeliveryTreeNavigationOffset(FVector2D NavigationOffset) const
{
	if (!bClampNavigationOffset)
	{
		return NavigationOffset;
	}

	const FVector2D MinOffset(FMath::Min(mMinNavigationOffset.X, mMaxNavigationOffset.X),
							  FMath::Min(mMinNavigationOffset.Y, mMaxNavigationOffset.Y));
	const FVector2D MaxOffset(FMath::Max(mMinNavigationOffset.X, mMaxNavigationOffset.X),
							  FMath::Max(mMinNavigationOffset.Y, mMaxNavigationOffset.Y));
	return FVector2D(FMath::Clamp(NavigationOffset.X, MinOffset.X, MaxOffset.X),
					 FMath::Clamp(NavigationOffset.Y, MinOffset.Y, MaxOffset.Y));
}

bool UKPCLDeliveryTreeWidget::GetCachedNodeByIndex(const int32* NodeIndex, FKPCLResearchTreeNode& OutNode) const
{
	if (NodeIndex == nullptr || !mResearchNodes.IsValidIndex(*NodeIndex))
	{
		OutNode = FKPCLResearchTreeNode();
		return false;
	}
	OutNode = mResearchNodes[*NodeIndex];
	return true;
}

bool UKPCLDeliveryTreeWidget::GetCachedNode(UKAPIDeliveryTask* Task, FKPCLResearchTreeNode& OutNode) const
{
	return GetCachedNodeByIndex(mResearchNodeIndexByTask.Find(Task), OutNode);
}

bool UKPCLDeliveryTreeWidget::GetCachedNodeAtCoordinate(FIntVector2 Coordinate, FKPCLResearchTreeNode& OutNode) const
{
	return GetCachedNodeByIndex(mResearchNodeIndexByCoordinate.Find(Coordinate), OutNode);
}

void UKPCLDeliveryTreeWidget::SetHoveredDeliveryTask(UKAPIDeliveryTask* Task)
{
	UKAPIDeliveryTask* ResolvedTask = IsCachedTask(Task) ? Task : nullptr;
	if (mHoveredTask == ResolvedTask)
	{
		return;
	}

	UKAPIDeliveryTask* PreviousTask = mHoveredTask;
	mHoveredTask = ResolvedTask;
	RebuildDependencyHighlight();
	MarkDirty(EKPCLDeliveryWidgetDirty::Selection | EKPCLDeliveryWidgetDirty::Connections);
	OnHoveredDeliveryTaskChanged(PreviousTask, mHoveredTask);
}

void UKPCLDeliveryTreeWidget::SelectDeliveryTask(UKAPIDeliveryTask* Task)
{

	if (IsValid(Task) && !CanSelectDeliveryTask(Task))
	{
		OnDeliveryTaskSelectionAttempted(Task, false, IsDeliveryTaskLocked(Task));
		return;
	}

	UKAPIDeliveryTask* ResolvedTask = IsCachedTask(Task) ? Task : nullptr;
	if (mSelectedTask != ResolvedTask)
	{
		UKAPIDeliveryTask* PreviousTask = mSelectedTask;
		mSelectedTask = ResolvedTask;
		RebuildDependencyHighlight();
		MarkDirty(EKPCLDeliveryWidgetDirty::Selection | EKPCLDeliveryWidgetDirty::Connections);
		OnSelectedDeliveryTaskChanged(PreviousTask, mSelectedTask);
		BroadcastSelectedNodeChange();
	}

	OnDeliveryTaskSelectionAttempted(Task, IsValid(mSelectedTask) && mSelectedTask == ResolvedTask, false);
}

void UKPCLDeliveryTreeWidget::BroadcastSelectedNodeChange()
{
	FKPCLResearchTreeNode Node;
	const bool bHasSelection = IsValid(mSelectedTask) && GetCachedNode(mSelectedTask, Node);
	OnSelectedNodeChange(Node, bHasSelection);
	if (IsValid(mDeliveryPreviewWidget))
	{
		mDeliveryPreviewWidget->MarkDirty(EKPCLDeliveryWidgetDirty::Selection);
		mDeliveryPreviewWidget->OnSelectedNodeChange(Node, bHasSelection);
	}
}

void UKPCLDeliveryTreeWidget::ClearDeliveryTaskSelection() { SelectDeliveryTask(nullptr); }

UKAPIDeliveryTask* UKPCLDeliveryTreeWidget::GetDependencyInteractionTask() const
{
	return IsCachedTask(mHoveredTask) ? mHoveredTask.Get()
		: IsCachedTask(mSelectedTask) ? mSelectedTask.Get()
									  : nullptr;
}

bool UKPCLDeliveryTreeWidget::IsDeliveryTaskHovered(UKAPIDeliveryTask* Task) const
{
	return IsValid(Task) && mHoveredTask == Task;
}

bool UKPCLDeliveryTreeWidget::IsDeliveryTaskSelected(UKAPIDeliveryTask* Task) const
{
	return IsValid(Task) && mSelectedTask == Task;
}

bool UKPCLDeliveryTreeWidget::IsDeliveryTaskDependencyHighlighted(UKAPIDeliveryTask* Task) const
{
	return IsValid(Task) && mDependencyHighlightedTasks.Contains(Task);
}

bool UKPCLDeliveryTreeWidget::IsDeliveryTaskLocked(UKAPIDeliveryTask* Task) const
{
	if (!IsValid(Task))
	{
		return true;
	}

	FKPCLResearchTreeNode Node;
	if (GetCachedNode(Task, Node))
	{
		return Node.mState == EKPCLResearchNodeState::Locked;
	}

	return !IsValid(mDeliveryTaskSubsystem) || mDeliveryTaskSubsystem->IsTaskLocked(Task);
}

bool UKPCLDeliveryTreeWidget::CanSelectDeliveryTask(UKAPIDeliveryTask* Task) const
{
	if (!IsCachedTask(Task))
	{
		return false;
	}
	if (bAllowSelectingLockedTasks || !IsDeliveryTaskLocked(Task))
	{
		return true;
	}

	return IsValid(mDeliveryTaskSubsystem) && mDeliveryTaskSubsystem->IsTaskSelectable(Task);
}

bool UKPCLDeliveryTreeWidget::IsResearchConnectionDependencyHighlighted(const FKPCLResearchTreeEdge& Edge) const
{
	UKAPIDeliveryTask* InteractionTask = GetDependencyInteractionTask();
	return IsValid(InteractionTask) && IsDeliveryTaskDependencyHighlighted(Edge.mParent) &&
		(Edge.mChild == InteractionTask || IsDeliveryTaskDependencyHighlighted(Edge.mChild));
}

FVector2D UKPCLDeliveryTreeWidget::CoordinateToPosition(FIntVector2 Coordinate, FVector2D NodeSpacing)
{
	return FVector2D(static_cast<double>(Coordinate.X) * NodeSpacing.X,
					 static_cast<double>(Coordinate.Y) * NodeSpacing.Y);
}

FVector2D UKPCLDeliveryTreeWidget::GetNodePosition(FIntVector2 Coordinate, bool bRelativeToTreeBounds) const
{
	const FIntVector2 LayoutCoordinate =
		bRelativeToTreeBounds && bHasTreeBounds ? Coordinate - mTreeMinCoordinate : Coordinate;
	return CoordinateToPosition(LayoutCoordinate, mNodeSpacing);
}

EKPCLDeliveryTreeConnectionState UKPCLDeliveryTreeWidget::GetResearchConnectionState(const FKPCLResearchTreeEdge& Edge)
{
	if (Edge.bParentCompleted)
	{
		return EKPCLDeliveryTreeConnectionState::Completed;
	}
	return Edge.bChildUnlocked ? EKPCLDeliveryTreeConnectionState::Unlocked : EKPCLDeliveryTreeConnectionState::Locked;
}

TArray<FVector2D> UKPCLDeliveryTreeWidget::GetResearchConnectionPoints(const FKPCLResearchTreeEdge& Edge) const
{
	return GetResearchConnectionPointsInternal(Edge, mNavigationOffset, mZoomLevel);
}

TArray<FVector2D> UKPCLDeliveryTreeWidget::GetResearchConnectionPointsInternal(const FKPCLResearchTreeEdge& Edge,
																			   FVector2D AdditionalOffset,
																			   float AdditionalScale) const
{
	const FVector2D Start =
		AdditionalOffset + (GetDeliveryNodeCenter(Edge.mParentCoordinate) + mConnectionParentOffset) * AdditionalScale;
	const FVector2D End =
		AdditionalOffset + (GetDeliveryNodeCenter(Edge.mChildCoordinate) + mConnectionChildOffset) * AdditionalScale;
	if (mConnectionDrawMode != EKPCLDeliveryTreeConnectionDrawMode::Orthogonal)
	{
		return {Start, End};
	}

	const double BendY = FMath::Lerp(Start.Y, End.Y, FMath::Clamp(mOrthogonalBendRatio, 0.0f, 1.0f));
	const FVector2D ParentBend(Start.X, BendY);
	const FVector2D ChildBend(End.X, BendY);
	return {Start, ParentBend, ParentBend, ChildBend, ChildBend, End};
}

FLinearColor UKPCLDeliveryTreeWidget::GetResearchConnectionColor(const FKPCLResearchTreeEdge& Edge) const
{
	FLinearColor Color = mLockedConnectionColor;
	if (IsResearchConnectionDependencyHighlighted(Edge))
	{
		Color = IsValid(mHoveredTask) ? mHoveredDependencyConnectionColor : mSelectedDependencyConnectionColor;
	}
	else
	{
		switch (GetResearchConnectionState(Edge))
		{
		case EKPCLDeliveryTreeConnectionState::Completed:
			Color = mCompletedConnectionColor;
			break;
		case EKPCLDeliveryTreeConnectionState::Unlocked:
			Color = mUnlockedConnectionColor;
			break;
		default:
			break;
		}
	}

	Color.A *= FMath::Clamp(mConnectionOpacity, 0.0f, 1.0f);
	return Color;
}

float UKPCLDeliveryTreeWidget::GetResearchConnectionThickness(const FKPCLResearchTreeEdge& Edge) const
{
	if (IsResearchConnectionDependencyHighlighted(Edge))
	{
		return FMath::Max(0.1f,
						  IsValid(mHoveredTask) ? mHoveredDependencyConnectionThickness
												: mSelectedDependencyConnectionThickness);
	}
	return FMath::Max(0.1f, mConnectionLineThickness);
}

void UKPCLDeliveryTreeWidget::DrawResearchConnections(FPaintContext& Context) const
{
	DrawResearchConnectionsInternal(Context, mNavigationOffset, mZoomLevel);
}

void UKPCLDeliveryTreeWidget::DrawResearchConnectionsInternal(FPaintContext& Context, FVector2D AdditionalOffset,
															  float AdditionalScale) const
{
	if (!bDrawResearchConnections)
	{
		return;
	}

	const int32 VisibleEdgeCount = FMath::Min(mVisibleResearchEdgeCount, mResearchEdges.Num());
	for (int32 EdgeIndex = 0; EdgeIndex < VisibleEdgeCount; ++EdgeIndex)
	{
		const FKPCLResearchTreeEdge& Edge = mResearchEdges[EdgeIndex];
		const TArray<FVector2D> Points = GetResearchConnectionPointsInternal(Edge, AdditionalOffset, AdditionalScale);
		if (Points.Num() < 2)
		{
			continue;
		}

		const FLinearColor Color = GetResearchConnectionColor(Edge);
		const float Thickness = GetResearchConnectionThickness(Edge) * AdditionalScale;
		if (mConnectionDrawMode == EKPCLDeliveryTreeConnectionDrawMode::Spline)
		{
			const FVector2D Delta = Points[1] - Points[0];
			const float TangentScale = FMath::Max(0.0f, mSplineTangentScale);
			const FVector2D Tangent = FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y)
				? FVector2D(Delta.X * TangentScale, 0.0)
				: FVector2D(0.0, Delta.Y * TangentScale);
			UWidgetBlueprintLibrary::DrawSpline(Context, Points[0], Tangent, Points[1], Tangent, Color, Thickness);
		}
		else
		{
			UWidgetBlueprintLibrary::DrawLines(Context, Points, Color, bAntiAliasResearchConnections, Thickness);
		}
	}
}

bool UKPCLDeliveryTreeWidget::TryQueueTask(UKAPIDeliveryTask* Task, TArray<FText>& ErrorMessages)
{
	bool bWasQueued = false;
	if (!IsValid(mDeliveryTaskSubsystem))
	{
		ErrorMessages.Reset();
	}
	else
	{
		bWasQueued = mDeliveryTaskSubsystem->TryToQueueTask(Task, ErrorMessages);
	}

	OnDeliveryTaskQueueAttempted(Task, bWasQueued, ErrorMessages);
	return bWasQueued;
}

bool UKPCLDeliveryTreeWidget::TryRemoveLastQueueEntry(int32 QueueIndex)
{
	return IsValid(mDeliveryTaskSubsystem) && mDeliveryTaskSubsystem->TryToRemoveFromQueueAt(QueueIndex);
}

bool UKPCLDeliveryTreeWidget::TryMoveQueueEntry(int32 FromIndex, int32 ToIndex)
{
	return IsValid(mDeliveryTaskSubsystem) && mDeliveryTaskSubsystem->TryToMoveQueueEntry(FromIndex, ToIndex);
}

UKPCLDeliverNodeWidget* UKPCLDeliveryTreeWidget::CreateDeliveryNodeWidget(const FKPCLResearchTreeNode& Node)
{
	if (!IsValid(Node.mTask) || !IsValid(GetWorld()))
	{
		return nullptr;
	}
	if (UKPCLDeliverNodeWidget* ExistingWidget = GetDeliveryNodeWidget(Node.mTask))
	{
		ExistingWidget->UpdateDeliveryNode(Node);
		PositionDeliveryNodeWidget(ExistingWidget);
		return ExistingWidget;
	}

	TSubclassOf<UKPCLDeliverNodeWidget> NodeWidgetClass = mDeliveryNodeWidgetClass;
	if (!IsValid(NodeWidgetClass))
	{
		NodeWidgetClass = UKPCLDeliverNodeWidget::StaticClass();
	}
	APlayerController* PlayerController = GetOwningPlayer();
	UKPCLDeliverNodeWidget* NodeWidget = IsValid(PlayerController)
		? CreateWidget<UKPCLDeliverNodeWidget>(PlayerController, NodeWidgetClass)
		: CreateWidget<UKPCLDeliverNodeWidget>(GetWorld(), NodeWidgetClass);
	if (!IsValid(NodeWidget) || !NodeWidget->SetupDeliveryNode(this, Node))
	{
		return nullptr;
	}

	mDeliveryNodeWidgets.Add(NodeWidget);
	mDeliveryNodeWidgetByTask.Add(Node.mTask, NodeWidget);
	AttachDeliveryNodeWidgetToCanvas(NodeWidget);
	NodeWidget->UpdateDeliveryNodeInteractionState(IsDeliveryTaskHovered(Node.mTask),
												   IsDeliveryTaskSelected(Node.mTask),
												   IsDeliveryTaskDependencyHighlighted(Node.mTask));
	OnDeliveryNodeWidgetCreated(NodeWidget);
	return NodeWidget;
}

void UKPCLDeliveryTreeWidget::DestroyDeliveryNodeWidget(UKPCLDeliverNodeWidget* NodeWidget)
{
	if (!IsValid(NodeWidget))
	{
		mDeliveryNodeWidgets.Remove(NodeWidget);
		RebuildDeliveryNodeWidgetLookup();
		return;
	}

	UKAPIDeliveryTask* Task = NodeWidget->mNode.mTask;
	if (mHoveredTask == Task)
	{
		SetHoveredDeliveryTask(nullptr);
	}
	if (mSelectedTask == Task)
	{
		SelectDeliveryTask(nullptr);
	}
	NodeWidget->TeardownDeliveryNode();
	NodeWidget->RemoveFromParent();
	mDeliveryNodeWidgets.Remove(NodeWidget);
	mDeliveryNodeWidgetByTask.Remove(Task);
	OnDeliveryNodeWidgetDestroyed(NodeWidget);
}

void UKPCLDeliveryTreeWidget::DestroyAllDeliveryNodeWidgets()
{
	const TArray<TObjectPtr<UKPCLDeliverNodeWidget>> NodeWidgets = mDeliveryNodeWidgets;
	for (UKPCLDeliverNodeWidget* NodeWidget : NodeWidgets)
	{
		DestroyDeliveryNodeWidget(NodeWidget);
	}
	mDeliveryNodeWidgets.Reset();
	mDeliveryNodeWidgetByTask.Reset();
}

UKPCLDeliverNodeWidget* UKPCLDeliveryTreeWidget::GetDeliveryNodeWidget(UKAPIDeliveryTask* Task) const
{
	UKPCLDeliverNodeWidget* const* NodeWidget = mDeliveryNodeWidgetByTask.Find(Task);
	return NodeWidget != nullptr && IsValid(*NodeWidget) ? *NodeWidget : nullptr;
}

void UKPCLDeliveryTreeWidget::BindSubsystemEvents()
{
	Super::BindSubsystemEvents();
	if (!IsValid(mDeliveryTaskSubsystem))
	{
		return;
	}

	mDeliveryTaskSubsystem->mOnResearchTreeChanged.AddUniqueDynamic(
		this, &UKPCLDeliveryTreeWidget::HandleResearchTreeChanged);
}

void UKPCLDeliveryTreeWidget::UnbindSubsystemEvents()
{
	if (IsValid(mDeliveryTaskSubsystem))
	{
		mDeliveryTaskSubsystem->mOnResearchTreeChanged.RemoveAll(this);
	}
	Super::UnbindSubsystemEvents();
}

void UKPCLDeliveryTreeWidget::RefreshQueuedTasks()
{
	mQueuedTasks.Reset();
	if (!IsValid(mDeliveryTaskSubsystem))
	{
		return;
	}
	for (UKAPIDeliveryTask* Task : mDeliveryTaskSubsystem->GetQueuedTasks())
	{
		if (IsValid(Task))
		{
			mQueuedTasks.Add(Task);
		}
	}
}

bool UKPCLDeliveryTreeWidget::PrepareDeliveryTreeRefresh()
{
	if (!IsValid(mDeliveryTaskSubsystem))
	{
		return false;
	}

	TArray<FKPCLResearchTreeNode> PreviousNodes = MoveTemp(mResearchNodes);
	TMap<UKAPIDeliveryTask*, const FKPCLResearchTreeNode*> PreviousNodesByTask;
	PreviousNodesByTask.Reserve(PreviousNodes.Num());
	for (const FKPCLResearchTreeNode& PreviousNode : PreviousNodes)
	{
		if (IsValid(PreviousNode.mTask))
		{
			PreviousNodesByTask.Add(PreviousNode.mTask, &PreviousNode);
		}
	}

	mResearchNodes = mDeliveryTaskSubsystem->GetResearchTreeNodes();
	bool bTopologyChanged = PreviousNodes.Num() != mResearchNodes.Num();
	if (!bTopologyChanged)
	{
		for (const FKPCLResearchTreeNode& Node : mResearchNodes)
		{
			const FKPCLResearchTreeNode* const* PreviousNode = PreviousNodesByTask.Find(Node.mTask);
			if (PreviousNode == nullptr || !HasSameNodeTopology(**PreviousNode, Node))
			{
				bTopologyChanged = true;
				break;
			}
		}
	}

	RebuildResearchEdges();
	RecalculateTreeBounds();
	RefreshQueuedTasks();
	mActiveTask = mDeliveryTaskSubsystem->GetActiveTask();
	RebuildResearchLookupCaches();
	ResetPendingDeliveryTreeWork();

	const TArray<TObjectPtr<UKPCLDeliverNodeWidget>> ExistingNodeWidgets = mDeliveryNodeWidgets;
	for (UKPCLDeliverNodeWidget* NodeWidget : ExistingNodeWidgets)
	{
		if (!IsValid(NodeWidget) || !IsValid(NodeWidget->mNode.mTask))
		{
			DestroyDeliveryNodeWidget(NodeWidget);
		}
		else if (!IsCachedTask(NodeWidget->mNode.mTask))
		{
			QueueDeliveryNodeRemoval(NodeWidget->mNode.mTask);
		}
	}

	for (const FKPCLResearchTreeNode& Node : mResearchNodes)
	{
		const FKPCLResearchTreeNode* const* PreviousNode = PreviousNodesByTask.Find(Node.mTask);
		if (bTopologyChanged || !IsValid(GetDeliveryNodeWidget(Node.mTask)) || PreviousNode == nullptr ||
			!HasSameNodeData(**PreviousNode, Node))
		{
			QueueDeliveryNodeUpdate(Node.mTask, true);
		}
	}

	mVisibleResearchEdgeCount = bTopologyChanged ? 0 : mResearchEdges.Num();
	bRefreshCompletionPending = true;
	RebuildDependencyHighlight();
	InvalidateConnectionLayer();
	InvalidateLayoutAndVolatility();
	ScheduleDetachedDeliveryTreeUpdate();
	return true;
}

void UKPCLDeliveryTreeWidget::ResetPendingDeliveryTreeWork()
{
	mPendingNodeUpdates.Reset();
	mPendingNodeRemovals.Reset();
	mPendingNodeUpdateSet.Reset();
	mPendingNodeRemovalSet.Reset();
	mPendingCachedNodeTasks.Reset();
	mPendingNodeUpdateIndex = 0;
	mPendingNodeRemovalIndex = 0;
}

void UKPCLDeliveryTreeWidget::QueueDeliveryNodeUpdate(UKAPIDeliveryTask* Task, bool bUseCachedNode)
{
	if (!IsValid(Task))
	{
		return;
	}
	const TWeakObjectPtr<UKAPIDeliveryTask> WeakTask = Task;
	if (bUseCachedNode)
	{
		mPendingCachedNodeTasks.Add(WeakTask);
	}
	if (!mPendingNodeUpdateSet.Contains(WeakTask))
	{
		mPendingNodeUpdateSet.Add(WeakTask);
		mPendingNodeUpdates.Add(WeakTask);
	}
	if (bIsTreeUpdateInProgress)
	{
		mTotalTreeUpdateOperations = FMath::Max(
			mTotalTreeUpdateOperations, mCompletedTreeUpdateOperations + GetPendingDeliveryTreeOperationCount());
	}
	ScheduleDetachedDeliveryTreeUpdate();
}

void UKPCLDeliveryTreeWidget::QueueDeliveryNodeRemoval(UKAPIDeliveryTask* Task)
{
	if (!IsValid(Task))
	{
		return;
	}
	const TWeakObjectPtr<UKAPIDeliveryTask> WeakTask = Task;
	if (!mPendingNodeRemovalSet.Contains(WeakTask))
	{
		mPendingNodeRemovalSet.Add(WeakTask);
		mPendingNodeRemovals.Add(WeakTask);
	}
	ScheduleDetachedDeliveryTreeUpdate();
}

bool UKPCLDeliveryTreeWidget::ApplyPendingDeliveryNodeUpdate(UKAPIDeliveryTask* Task, bool bUseCachedNode)
{
	if (!IsValid(Task) || !IsValid(mDeliveryTaskSubsystem))
	{
		return false;
	}

	FKPCLResearchTreeNode Node;
	if (!bUseCachedNode)
	{
		if (!mDeliveryTaskSubsystem->GetResearchNode(Task, Node))
		{
			RequestDeliveryTreeRefresh();
			return false;
		}
		const int32* NodeIndex = mResearchNodeIndexByTask.Find(Task);
		if (NodeIndex == nullptr || !mResearchNodes.IsValidIndex(*NodeIndex) ||
			!HasSameNodeTopology(mResearchNodes[*NodeIndex], Node))
		{
			RequestDeliveryTreeRefresh();
			return false;
		}
		mResearchNodes[*NodeIndex] = Node;
	}
	else if (!GetCachedNode(Task, Node))
	{
		RequestDeliveryTreeRefresh();
		return false;
	}

	UKPCLDeliverNodeWidget* NodeWidget = GetDeliveryNodeWidget(Task);
	if (IsValid(NodeWidget))
	{
		const FIntVector2 PreviousCoordinate = NodeWidget->mNode.mCoordinate;
		NodeWidget->UpdateDeliveryNode(Node);
		if (PreviousCoordinate != Node.mCoordinate)
		{
			PositionDeliveryNodeWidget(NodeWidget);
		}
	}
	else
	{
		NodeWidget = CreateDeliveryNodeWidget(Node);
	}

	RefreshAffectedResearchEdges(Task, Node);
	return IsValid(NodeWidget);
}

void UKPCLDeliveryTreeWidget::FinalizePendingDeliveryTreeUpdates()
{
	TArray<TObjectPtr<UKPCLDeliverNodeWidget>> OrderedNodeWidgets;
	OrderedNodeWidgets.Reserve(mResearchNodes.Num());
	for (const FKPCLResearchTreeNode& Node : mResearchNodes)
	{
		if (UKPCLDeliverNodeWidget* NodeWidget = GetDeliveryNodeWidget(Node.mTask))
		{
			OrderedNodeWidgets.Add(NodeWidget);
		}
	}
	mDeliveryNodeWidgets = MoveTemp(OrderedNodeWidgets);
	RebuildDeliveryNodeWidgetLookup();
	EnsureConnectionLayer();
	RebuildDependencyHighlight();
	InvalidateConnectionLayer();
	InvalidateLayoutAndVolatility();

	const bool bBroadcastRefresh = bRefreshCompletionPending;
	bRefreshCompletionPending = false;
	bIsTreeUpdateInProgress = false;
	mCompletedTreeUpdateOperations = FMath::Max(mCompletedTreeUpdateOperations, mTotalTreeUpdateOperations);
	OnDeliveryTreeUpdateProgress(mCompletedTreeUpdateOperations, mTotalTreeUpdateOperations);
	if (bBroadcastRefresh)
	{
		OnDeliveryTreeRefreshed();
	}
	OnDeliveryTreeUpdateFinished();
}

void UKPCLDeliveryTreeWidget::RebuildResearchLookupCaches()
{
	mResearchNodeIndexByTask.Reset();
	mResearchNodeIndexByCoordinate.Reset();
	mResearchParentsByTask.Reset();
	mParentEdgeIndicesByTask.Reset();
	mChildEdgeIndicesByTask.Reset();
	mResearchNodeIndexByTask.Reserve(mResearchNodes.Num());
	mResearchNodeIndexByCoordinate.Reserve(mResearchNodes.Num());
	for (int32 NodeIndex = 0; NodeIndex < mResearchNodes.Num(); ++NodeIndex)
	{
		const FKPCLResearchTreeNode& Node = mResearchNodes[NodeIndex];
		if (!IsValid(Node.mTask))
		{
			continue;
		}
		mResearchNodeIndexByTask.Add(Node.mTask, NodeIndex);
		mResearchNodeIndexByCoordinate.Add(Node.mCoordinate, NodeIndex);
		TArray<UKAPIDeliveryTask*>& Parents = mResearchParentsByTask.FindOrAdd(Node.mTask);
		Parents.Reserve(Node.mParents.Num());
		for (UKAPIDeliveryTask* Parent : Node.mParents)
		{
			if (IsValid(Parent))
			{
				Parents.Add(Parent);
			}
		}
	}
	for (int32 EdgeIndex = 0; EdgeIndex < mResearchEdges.Num(); ++EdgeIndex)
	{
		const FKPCLResearchTreeEdge& Edge = mResearchEdges[EdgeIndex];
		if (IsValid(Edge.mParent))
		{
			mParentEdgeIndicesByTask.FindOrAdd(Edge.mParent).Add(EdgeIndex);
		}
		if (IsValid(Edge.mChild))
		{
			mChildEdgeIndicesByTask.FindOrAdd(Edge.mChild).Add(EdgeIndex);
		}
	}
}

void UKPCLDeliveryTreeWidget::RebuildDeliveryNodeWidgetLookup()
{
	mDeliveryNodeWidgetByTask.Reset();
	mDeliveryNodeWidgetByTask.Reserve(mDeliveryNodeWidgets.Num());
	for (UKPCLDeliverNodeWidget* NodeWidget : mDeliveryNodeWidgets)
	{
		if (IsValid(NodeWidget) && IsValid(NodeWidget->mNode.mTask))
		{
			mDeliveryNodeWidgetByTask.Add(NodeWidget->mNode.mTask, NodeWidget);
		}
	}
}

void UKPCLDeliveryTreeWidget::RebuildResearchEdges()
{
	mResearchEdges.Reset();
	for (const FKPCLResearchTreeNode& ChildNode : mResearchNodes)
	{
		for (UKAPIDeliveryTask* Parent : ChildNode.mParents)
		{
			if (!IsValid(Parent) || !IsValid(ChildNode.mTask))
			{
				continue;
			}
			FKPCLResearchTreeEdge& Edge = mResearchEdges.AddDefaulted_GetRef();
			Edge.mParent = Parent;
			Edge.mChild = ChildNode.mTask;
			Edge.mParentCoordinate = Parent->mCoordinate;
			Edge.mChildCoordinate = ChildNode.mCoordinate;
			Edge.bParentCompleted = mDeliveryTaskSubsystem->IsTaskCompleted(Parent);
			Edge.bChildUnlocked = ChildNode.bPrerequisitesMet;
		}
	}
}

void UKPCLDeliveryTreeWidget::RecalculateTreeBounds()
{
	if (mResearchNodes.IsEmpty())
	{
		mTreeMinCoordinate = FIntVector2::ZeroValue;
		mTreeMaxCoordinate = FIntVector2::ZeroValue;
		bHasTreeBounds = false;
		return;
	}

	mTreeMinCoordinate = mResearchNodes[0].mCoordinate;
	mTreeMaxCoordinate = mResearchNodes[0].mCoordinate;
	for (const FKPCLResearchTreeNode& Node : mResearchNodes)
	{
		mTreeMinCoordinate.X = FMath::Min(mTreeMinCoordinate.X, Node.mCoordinate.X);
		mTreeMinCoordinate.Y = FMath::Min(mTreeMinCoordinate.Y, Node.mCoordinate.Y);
		mTreeMaxCoordinate.X = FMath::Max(mTreeMaxCoordinate.X, Node.mCoordinate.X);
		mTreeMaxCoordinate.Y = FMath::Max(mTreeMaxCoordinate.Y, Node.mCoordinate.Y);
	}
	bHasTreeBounds = true;
}

void UKPCLDeliveryTreeWidget::RefreshAffectedResearchEdges(UKAPIDeliveryTask* Task, const FKPCLResearchTreeNode& Node)
{
	bool bEdgeChanged = false;
	if (const TArray<int32>* ParentEdges = mParentEdgeIndicesByTask.Find(Task))
	{
		const bool bParentCompleted = mDeliveryTaskSubsystem->IsTaskCompleted(Task);
		for (int32 EdgeIndex : *ParentEdges)
		{
			if (mResearchEdges.IsValidIndex(EdgeIndex) &&
				mResearchEdges[EdgeIndex].bParentCompleted != bParentCompleted)
			{
				mResearchEdges[EdgeIndex].bParentCompleted = bParentCompleted;
				bEdgeChanged = true;
			}
		}
	}
	if (const TArray<int32>* ChildEdges = mChildEdgeIndicesByTask.Find(Task))
	{
		for (int32 EdgeIndex : *ChildEdges)
		{
			if (mResearchEdges.IsValidIndex(EdgeIndex) &&
				mResearchEdges[EdgeIndex].bChildUnlocked != Node.bPrerequisitesMet)
			{
				mResearchEdges[EdgeIndex].bChildUnlocked = Node.bPrerequisitesMet;
				bEdgeChanged = true;
			}
		}
	}
	if (bEdgeChanged)
	{
		InvalidateConnectionLayer();
	}
}

void UKPCLDeliveryTreeWidget::InvalidateConnectionLayer()
{
	MarkDirty(EKPCLDeliveryWidgetDirty::Connections);
	if (IsValid(mConnectionLayerWidget))
	{
		mConnectionLayerWidget->InvalidateLayoutAndVolatility();
	}
}

void UKPCLDeliveryTreeWidget::ScheduleDetachedDeliveryTreeUpdate()
{
	if (!bIncrementalTreeUpdates || !bProcessTreeUpdatesWhileDetached || !HasPendingDeliveryTreeUpdates())
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetTimerManager().IsTimerActive(mDetachedUpdateTimerHandle))
	{
		return;
	}
	mDetachedUpdateTimerHandle = World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &UKPCLDeliveryTreeWidget::HandleDetachedDeliveryTreeUpdate));
}

void UKPCLDeliveryTreeWidget::HandleDetachedDeliveryTreeUpdate()
{
	mDetachedUpdateTimerHandle.Invalidate();
	ProcessIncrementalDeliveryTreeTick();
	ScheduleDetachedDeliveryTreeUpdate();
}

void UKPCLDeliveryTreeWidget::ProcessIncrementalDeliveryTreeTick()
{
	if (mLastIncrementalUpdateFrame == GFrameCounter || !HasPendingDeliveryTreeUpdates())
	{
		return;
	}
	mLastIncrementalUpdateFrame = GFrameCounter;
	if (bIncrementalTreeUpdates)
	{
		ProcessPendingDeliveryTreeUpdates();
	}
	else
	{
		FlushPendingDeliveryTreeUpdates();
	}
}

bool UKPCLDeliveryTreeWidget::HasSameNodeTopology(const FKPCLResearchTreeNode& Left, const FKPCLResearchTreeNode& Right)
{
	if (Left.mTask != Right.mTask || Left.mCoordinate != Right.mCoordinate ||
		Left.bRequireAllParents != Right.bRequireAllParents || Left.mParents.Num() != Right.mParents.Num() ||
		Left.mChildren.Num() != Right.mChildren.Num())
	{
		return false;
	}
	for (int32 ParentIndex = 0; ParentIndex < Left.mParents.Num(); ++ParentIndex)
	{
		if (Left.mParents[ParentIndex] != Right.mParents[ParentIndex])
		{
			return false;
		}
	}
	for (int32 ChildIndex = 0; ChildIndex < Left.mChildren.Num(); ++ChildIndex)
	{
		if (Left.mChildren[ChildIndex] != Right.mChildren[ChildIndex])
		{
			return false;
		}
	}
	return true;
}

bool UKPCLDeliveryTreeWidget::HasSameNodeData(const FKPCLResearchTreeNode& Left, const FKPCLResearchTreeNode& Right)
{
	return FKPCLResearchTreeNode::StaticStruct()->CompareScriptStruct(&Left, &Right, 0);
}

void UKPCLDeliveryTreeWidget::RebuildDependencyHighlight()
{
	UKAPIDeliveryTask* PreviousHoveredTask = mHoveredTask;
	UKAPIDeliveryTask* PreviousSelectedTask = mSelectedTask;
	if (!IsCachedTask(mHoveredTask))
	{
		mHoveredTask = nullptr;
	}

	if (!CanSelectDeliveryTask(mSelectedTask))
	{
		mSelectedTask = nullptr;
	}

	TSet<UKAPIDeliveryTask*> ParentTasks;
	UKAPIDeliveryTask* InteractionTask = GetDependencyInteractionTask();
	CollectDependencyParents(InteractionTask, ParentTasks);
	mDependencyHighlightedTasks.Reset(ParentTasks.Num());
	for (UKAPIDeliveryTask* ParentTask : ParentTasks)
	{
		mDependencyHighlightedTasks.Add(ParentTask);
	}
	mDependencyHighlightedTasks.Sort(&UKAPIDeliveryTask::SortByCoordinate);

	UpdateDeliveryNodeInteractionStates();
	if (IsValid(mDeliveryPreviewWidget))
	{

		if (IsValid(mSelectedTask))
		{
			mDeliveryPreviewWidget->SetPreviewedDeliveryTask(mSelectedTask);
		}
		else
		{
			mDeliveryPreviewWidget->ClearDeliveryPreview();
		}
	}
	InvalidateConnectionLayer();
	if (PreviousHoveredTask != mHoveredTask)
	{
		OnHoveredDeliveryTaskChanged(PreviousHoveredTask, mHoveredTask);
	}
	if (PreviousSelectedTask != mSelectedTask)
	{
		OnSelectedDeliveryTaskChanged(PreviousSelectedTask, mSelectedTask);
		BroadcastSelectedNodeChange();
	}
	OnDeliveryDependencyHighlightChanged(InteractionTask);
}

void UKPCLDeliveryTreeWidget::UpdateDeliveryNodeInteractionStates()
{
	TSet<UKAPIDeliveryTask*> TasksToUpdate;
	if (mAppliedHoveredTask.IsValid())
	{
		TasksToUpdate.Add(mAppliedHoveredTask.Get());
	}
	if (IsValid(mHoveredTask))
	{
		TasksToUpdate.Add(mHoveredTask);
	}
	if (mAppliedSelectedTask.IsValid())
	{
		TasksToUpdate.Add(mAppliedSelectedTask.Get());
	}
	if (IsValid(mSelectedTask))
	{
		TasksToUpdate.Add(mSelectedTask);
	}
	for (const TWeakObjectPtr<UKAPIDeliveryTask>& Task : mAppliedDependencyHighlightedTasks)
	{
		if (Task.IsValid())
		{
			TasksToUpdate.Add(Task.Get());
		}
	}
	for (UKAPIDeliveryTask* Task : mDependencyHighlightedTasks)
	{
		if (IsValid(Task))
		{
			TasksToUpdate.Add(Task);
		}
	}

	for (UKAPIDeliveryTask* Task : TasksToUpdate)
	{
		UKPCLDeliverNodeWidget* NodeWidget = GetDeliveryNodeWidget(Task);
		if (!IsValid(NodeWidget))
		{
			continue;
		}
		NodeWidget->UpdateDeliveryNodeInteractionState(IsDeliveryTaskHovered(Task), IsDeliveryTaskSelected(Task),
													   IsDeliveryTaskDependencyHighlighted(Task));
	}

	mAppliedHoveredTask = mHoveredTask;
	mAppliedSelectedTask = mSelectedTask;
	mAppliedDependencyHighlightedTasks.Reset();
	for (UKAPIDeliveryTask* Task : mDependencyHighlightedTasks)
	{
		if (IsValid(Task))
		{
			mAppliedDependencyHighlightedTasks.Add(Task);
		}
	}
}

void UKPCLDeliveryTreeWidget::CollectDependencyParents(UKAPIDeliveryTask* Task,
													   TSet<UKAPIDeliveryTask*>& OutParents) const
{
	if (!IsCachedTask(Task))
	{
		return;
	}

	TArray<UKAPIDeliveryTask*> PendingTasks = {Task};
	while (!PendingTasks.IsEmpty())
	{
		UKAPIDeliveryTask* CurrentTask = PendingTasks.Pop(EAllowShrinking::No);
		const TArray<UKAPIDeliveryTask*>* Parents = mResearchParentsByTask.Find(CurrentTask);
		if (Parents == nullptr)
		{
			continue;
		}
		for (UKAPIDeliveryTask* ParentTask : *Parents)
		{
			if (!IsCachedTask(ParentTask) || ParentTask == Task || OutParents.Contains(ParentTask))
			{
				continue;
			}
			OutParents.Add(ParentTask);
			PendingTasks.Add(ParentTask);
		}
	}
}

bool UKPCLDeliveryTreeWidget::IsCachedTask(UKAPIDeliveryTask* Task) const
{
	return IsValid(Task) && mResearchNodeIndexByTask.Contains(Task);
}

void UKPCLDeliveryTreeWidget::TeardownDeliveryAuxiliaryWidgets()
{
	if (IsValid(mDeliveryQueueWidget))
	{
		mDeliveryQueueWidget->TeardownDeliveryQueue();
	}
	if (IsValid(mDeliveryPreviewWidget))
	{
		mDeliveryPreviewWidget->TeardownDeliveryPreview();
	}
}

bool UKPCLDeliveryTreeWidget::IsDeliveryNodeMeasured(FIntVector2 Coordinate) const
{
	if (!bAutoSizeNodeSlots)
	{
		return true;
	}

	FKPCLResearchTreeNode Node;
	if (!GetCachedNodeAtCoordinate(Coordinate, Node))
	{

		return true;
	}

	const UKPCLDeliverNodeWidget* NodeWidget = GetDeliveryNodeWidget(Node.mTask.Get());
	return IsValid(NodeWidget) && !NodeWidget->GetDesiredSize().IsNearlyZero();
}

void UKPCLDeliveryTreeWidget::InitializeDeliveryTreeNavigationOffset()
{
	if (bNavigationOffsetInitialized)
	{
		return;
	}

	mZoomLevel = ClampDeliveryTreeZoom(mInitialZoomLevel);
	if (bFocusInitialCoordinateOnSetup)
	{
		if (GetDeliveryTreeViewSize().IsNearlyZero() || !IsDeliveryNodeMeasured(mInitialFocusCoordinate))
		{

			ApplyDeliveryTreeNavigationOffset();
			return;
		}
		mNavigationOffset = ClampDeliveryTreeNavigationOffset(GetDeliveryTreeFocusOffset(mInitialFocusCoordinate));
	}
	else
	{
		mNavigationOffset = ClampDeliveryTreeNavigationOffset(mInitialNavigationOffset);
	}
	bNavigationOffsetInitialized = true;
	ApplyDeliveryTreeNavigationOffset();
}

void UKPCLDeliveryTreeWidget::ApplyDeliveryTreeNavigationOffset()
{
	if (IsValid(mNodeCanvas))
	{

		mNodeCanvas->SetRenderTransformPivot(FVector2D::ZeroVector);
		mNodeCanvas->SetRenderScale(FVector2D(mZoomLevel, mZoomLevel));
		mNodeCanvas->SetRenderTranslation(mNavigationOffset);
	}
	InvalidateLayoutAndVolatility();
}

bool UKPCLDeliveryTreeWidget::EnsureConnectionLayer()
{
	if (!IsValid(mNodeCanvas))
	{
		return false;
	}

	if (!IsValid(mConnectionLayerWidget))
	{
		APlayerController* PlayerController = GetOwningPlayer();
		mConnectionLayerWidget = IsValid(PlayerController)
			? CreateWidget<UKPCLDeliveryTreeConnectionLayer>(PlayerController,
															 UKPCLDeliveryTreeConnectionLayer::StaticClass())
			: CreateWidget<UKPCLDeliveryTreeConnectionLayer>(GetWorld(),
															 UKPCLDeliveryTreeConnectionLayer::StaticClass());
		if (!IsValid(mConnectionLayerWidget))
		{
			return false;
		}
		mConnectionLayerWidget->mDeliveryTreeWidget = this;
		mConnectionLayerWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		mConnectionLayerWidget->ForceVolatile(false);
	}

	if (mConnectionLayerWidget->GetParent() != mNodeCanvas)
	{
		mConnectionLayerWidget->RemoveFromParent();
		if (!IsValid(mNodeCanvas->AddChildToCanvas(mConnectionLayerWidget)))
		{
			return false;
		}
	}

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(mConnectionLayerWidget->Slot);
	if (!IsValid(CanvasSlot))
	{
		return false;
	}
	CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	CanvasSlot->SetOffsets(FMargin(0.0f));
	CanvasSlot->SetAlignment(FVector2D::ZeroVector);
	CanvasSlot->SetAutoSize(false);
	CanvasSlot->SetZOrder(mConnectionZOrder);
	return true;
}

void UKPCLDeliveryTreeWidget::DestroyConnectionLayer()
{
	if (!IsValid(mConnectionLayerWidget))
	{
		mConnectionLayerWidget = nullptr;
		return;
	}
	mConnectionLayerWidget->mDeliveryTreeWidget = nullptr;
	mConnectionLayerWidget->RemoveFromParent();
	mConnectionLayerWidget = nullptr;
}

void UKPCLDeliveryTreeWidget::HandleResearchTreeChanged() { RequestDeliveryTreeRefresh(); }

void UKPCLDeliveryTreeWidget::HandleResearchNodeChanged(const FKPCLResearchTreeNode& Node)
{
	const int32* NodeIndex = mResearchNodeIndexByTask.Find(Node.mTask);
	if (NodeIndex == nullptr || !mResearchNodes.IsValidIndex(*NodeIndex))
	{
		RequestDeliveryTreeRefresh();
	}
	else if (!HasSameNodeTopology(mResearchNodes[*NodeIndex], Node))
	{
		RequestDeliveryTreeRefresh();
	}
	else
	{
		mResearchNodes[*NodeIndex] = Node;
		QueueDeliveryNodeUpdate(Node.mTask, true);
	}
	MarkDirty(EKPCLDeliveryWidgetDirty::Data | EKPCLDeliveryWidgetDirty::State);
	if (!bIncrementalTreeUpdates)
	{
		FlushPendingDeliveryTreeUpdates();
	}
	OnResearchNodeChanged(Node);
}

void UKPCLDeliveryTreeWidget::HandleDeliveryTaskCompleted(UKAPIDeliveryTask* Task, int32 CompletionCount)
{
	MarkDirty(EKPCLDeliveryWidgetDirty::State | EKPCLDeliveryWidgetDirty::Progress);
	OnDeliveryTaskCompleted(Task, CompletionCount);
}

void UKPCLDeliveryTreeWidget::HandleActiveResearchChanged(UKAPIDeliveryTask* PreviousTask,
														  UKAPIDeliveryTask* CurrentTask)
{
	mActiveTask = CurrentTask;
	MarkDirty(EKPCLDeliveryWidgetDirty::State | EKPCLDeliveryWidgetDirty::Queue);
	if (IsCachedTask(PreviousTask))
	{
		QueueDeliveryNodeUpdate(PreviousTask, false);
	}
	if (IsCachedTask(CurrentTask))
	{
		QueueDeliveryNodeUpdate(CurrentTask, false);
	}
	if (!bIncrementalTreeUpdates)
	{
		FlushPendingDeliveryTreeUpdates();
	}
	OnActiveResearchChanged(PreviousTask, CurrentTask);
}

void UKPCLDeliveryTreeWidget::HandleResearchProgressChanged(UKAPIDeliveryTask* Task, float Progress)
{
	MarkDirty(EKPCLDeliveryWidgetDirty::Progress);
	RequestDeliveryNodeRefresh(Task);
	OnResearchProgressChanged(Task, Progress);
}

void UKPCLDeliveryTreeWidget::HandleResearchAvailabilityChanged(UKAPIDeliveryTask* Task, bool bIsAvailable)
{
	MarkDirty(EKPCLDeliveryWidgetDirty::State | EKPCLDeliveryWidgetDirty::Connections);
	RequestDeliveryNodeRefresh(Task);
	OnResearchAvailabilityChanged(Task, bIsAvailable);
}

void UKPCLDeliveryTreeWidget::HandleResearchQueueOrderChanged()
{
	MarkDirty(EKPCLDeliveryWidgetDirty::Queue);
	const TArray<TObjectPtr<UKAPIDeliveryTask>> PreviousQueuedTasks = mQueuedTasks;
	RefreshQueuedTasks();
	TSet<UKAPIDeliveryTask*> QueueTasks;
	for (UKAPIDeliveryTask* Task : PreviousQueuedTasks)
	{
		if (IsCachedTask(Task))
		{
			QueueTasks.Add(Task);
		}
	}
	for (UKAPIDeliveryTask* Task : mQueuedTasks)
	{
		if (IsCachedTask(Task))
		{
			QueueTasks.Add(Task);
		}
	}
	for (UKAPIDeliveryTask* Task : QueueTasks)
	{
		QueueDeliveryNodeUpdate(Task, false);
	}
	if (!bIncrementalTreeUpdates)
	{
		FlushPendingDeliveryTreeUpdates();
	}

	if (IsValid(mSelectedTask) && !CanSelectDeliveryTask(mSelectedTask))
	{
		RebuildDependencyHighlight();
	}
	OnResearchQueueOrderChanged();
}
