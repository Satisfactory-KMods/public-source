// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Widget/KPCLDeliverNodeWidget.h"

#include "Components/Button.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Widget/KPCLDeliveryTreeWidget.h"

UKPCLDeliverNodeWidget::UKPCLDeliverNodeWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UKPCLDeliverNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (IsValid(mButton))
	{
		mButton->OnPressed.AddUniqueDynamic(this, &UKPCLDeliverNodeWidget::HandleDeliveryNodeButtonPressed);
	}
	if (bIsSetup)
	{
		RefreshDeliveryNode();
	}
	else if (IsValid(mDeliveryTreeWidget))
	{
		SetupDeliveryNode(mDeliveryTreeWidget, mNode);
	}
	if (IsValid(mDeliveryTreeWidget) && IsValid(mNode.mTask))
	{
		UpdateDeliveryNodeInteractionState(mDeliveryTreeWidget->IsDeliveryTaskHovered(mNode.mTask),
										   mDeliveryTreeWidget->IsDeliveryTaskSelected(mNode.mTask),
										   mDeliveryTreeWidget->IsDeliveryTaskDependencyHighlighted(mNode.mTask));
	}
}

void UKPCLDeliverNodeWidget::NativeDestruct()
{
	if (IsValid(mButton))
	{
		mButton->OnPressed.RemoveDynamic(this, &UKPCLDeliverNodeWidget::HandleDeliveryNodeButtonPressed);
	}
	SetDeliveryNodeHovered(false);
	Super::NativeDestruct();
}

FReply UKPCLDeliverNodeWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

FReply UKPCLDeliverNodeWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry,
															  const FPointerEvent& InMouseEvent)
{
	if (ShouldLockSelectionFromClick(InMouseEvent.GetEffectingButton()))
	{

		bClickHasPreviewMouseDown = true;
		mLastPreviewMouseDownTime = FPlatformTime::Seconds();

		SelectDeliveryNode();
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UKPCLDeliverNodeWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry,
															  const FPointerEvent& InMouseEvent)
{
	if (ShouldQueueTaskFromDoubleClick(InMouseEvent.GetEffectingButton()) && IsValid(mDeliveryTreeWidget) &&
		mDeliveryTreeWidget->bQueueTaskOnNodeDoubleClick)
	{

		bClickHasPreviewMouseDown = false;
		TryQueueTaskFromDoubleClick();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UKPCLDeliverNodeWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	SetDeliveryNodeHovered(true);
}

void UKPCLDeliverNodeWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	SetDeliveryNodeHovered(false);
	Super::NativeOnMouseLeave(InMouseEvent);
}

bool UKPCLDeliverNodeWidget::SetupDeliveryNode(UKPCLDeliveryTreeWidget* DeliveryTreeWidget,
											   const FKPCLResearchTreeNode& Node)
{
	if (!IsValid(DeliveryTreeWidget) || !IsValid(Node.mTask))
	{
		return false;
	}
	if (bIsSetup && mDeliveryTreeWidget == DeliveryTreeWidget)
	{
		return UpdateDeliveryNode(Node);
	}

	TeardownDeliveryNode();
	mDeliveryTreeWidget = DeliveryTreeWidget;
	mDeliveryTaskSubsystem = DeliveryTreeWidget->mDeliveryTaskSubsystem;
	mNode = Node;
	bIsSetup = true;
	MarkDirty(EKPCLDeliveryWidgetDirty::Data | EKPCLDeliveryWidgetDirty::State | EKPCLDeliveryWidgetDirty::Progress |
			  EKPCLDeliveryWidgetDirty::Queue | EKPCLDeliveryWidgetDirty::Layout);
	OnDeliveryNodeSetup(mNode);
	UpdateDeliveryNodeInteractionState(DeliveryTreeWidget->IsDeliveryTaskHovered(mNode.mTask),
									   DeliveryTreeWidget->IsDeliveryTaskSelected(mNode.mTask),
									   DeliveryTreeWidget->IsDeliveryTaskDependencyHighlighted(mNode.mTask));
	return true;
}

void UKPCLDeliverNodeWidget::TeardownDeliveryNode()
{
	const bool bWasSetup = bIsSetup;
	UpdateDeliveryNodeInteractionState(false, false, false);
	mDeliveryTreeWidget = nullptr;
	mDeliveryTaskSubsystem = nullptr;
	mNode = FKPCLResearchTreeNode();
	bIsSetup = false;
	if (bWasSetup)
	{
		OnDeliveryNodeDestroyed();
	}
}

bool UKPCLDeliverNodeWidget::RefreshDeliveryNode()
{
	if (!IsValid(mDeliveryTaskSubsystem) || !IsValid(mNode.mTask))
	{
		return false;
	}

	FKPCLResearchTreeNode RefreshedNode;
	return mDeliveryTaskSubsystem->GetResearchNode(mNode.mTask, RefreshedNode) && UpdateDeliveryNode(RefreshedNode);
}

bool UKPCLDeliverNodeWidget::UpdateDeliveryNode(const FKPCLResearchTreeNode& Node)
{
	if (!IsValid(Node.mTask))
	{
		return false;
	}
	const FKPCLResearchTreeNode PreviousNode = mNode;
	const bool bPreviousUnlocked = IsUnlocked();
	const bool bPreviousCanBeUnlocked = CanBeUnlocked();
	mNode = Node;
	EKPCLDeliveryWidgetDirty DirtyFlags = EKPCLDeliveryWidgetDirty::Data;
	OnDeliveryNodeChanged(mNode);
	if (PreviousNode.mState != mNode.mState)
	{
		DirtyFlags |= EKPCLDeliveryWidgetDirty::State;
		OnDeliveryNodeStateChanged(PreviousNode.mState, mNode.mState);
	}
	if (!FMath::IsNearlyEqual(PreviousNode.mProgress, mNode.mProgress))
	{
		DirtyFlags |= EKPCLDeliveryWidgetDirty::Progress;
		OnDeliveryNodeProgressChanged(PreviousNode.mProgress, mNode.mProgress);
	}
	if (bPreviousUnlocked != IsUnlocked() || bPreviousCanBeUnlocked != CanBeUnlocked() ||
		PreviousNode.bPrerequisitesMet != mNode.bPrerequisitesMet)
	{
		DirtyFlags |= EKPCLDeliveryWidgetDirty::State;
		OnDeliveryNodeAvailabilityChanged(IsUnlocked(), CanBeUnlocked());
	}
	if (PreviousNode.mQueueIndices != mNode.mQueueIndices)
	{
		DirtyFlags |= EKPCLDeliveryWidgetDirty::Queue;
		OnDeliveryNodeQueueChanged(mNode.mQueueIndices);
	}
	if (PreviousNode.mCompletedCount != mNode.mCompletedCount ||
		(PreviousNode.mState == EKPCLResearchNodeState::Maxed) != IsMaxed())
	{
		DirtyFlags |= EKPCLDeliveryWidgetDirty::State;
		OnDeliveryNodeCompletionChanged(mNode.mCompletedCount, IsMaxed());
	}
	if (PreviousNode.mCoordinate != mNode.mCoordinate)
	{
		DirtyFlags |= EKPCLDeliveryWidgetDirty::Layout;
	}
	MarkDirty(DirtyFlags);
	return true;
}

FVector2D UKPCLDeliverNodeWidget::GetDeliveryNodePosition(bool bRelativeToTreeBounds) const
{
	return IsValid(mDeliveryTreeWidget) ? mDeliveryTreeWidget->GetNodePosition(mNode.mCoordinate, bRelativeToTreeBounds)
										: FVector2D::ZeroVector;
}

bool UKPCLDeliverNodeWidget::TryQueueTask(TArray<FText>& ErrorMessages)
{
	bool bWasQueued = false;
	if (!IsValid(mDeliveryTreeWidget) || !IsValid(mNode.mTask))
	{
		ErrorMessages.Reset();
	}
	else
	{
		bWasQueued = mDeliveryTreeWidget->TryQueueTask(mNode.mTask, ErrorMessages);
	}

	OnDeliveryNodeQueueAttempted(bWasQueued, ErrorMessages);
	return bWasQueued;
}

bool UKPCLDeliverNodeWidget::TryQueueTaskFromDoubleClick()
{
	TArray<FText> ErrorMessages;
	return TryQueueTask(ErrorMessages);
}

bool UKPCLDeliverNodeWidget::NotifyDeliveryNodeClicked()
{

	const bool bHadPreviewMouseDown = bClickHasPreviewMouseDown;
	bClickHasPreviewMouseDown = false;
	if (bHadPreviewMouseDown)
	{
		return false;
	}

	const double SecondsSincePreviewMouseDown = FPlatformTime::Seconds() - mLastPreviewMouseDownTime;
	if (SecondsSincePreviewMouseDown > FMath::Max(0.0f, mConsumedDoubleClickMaxDelay))
	{
		return false;
	}

	if (!IsValid(mDeliveryTreeWidget) || !mDeliveryTreeWidget->bQueueTaskOnNodeDoubleClick)
	{
		return false;
	}
	return TryQueueTaskFromDoubleClick();
}

void UKPCLDeliverNodeWidget::HandleDeliveryNodeButtonPressed() { NotifyDeliveryNodeClicked(); }

bool UKPCLDeliverNodeWidget::ShouldQueueTaskFromDoubleClick(const FKey& MouseButton)
{
	return MouseButton == EKeys::LeftMouseButton;
}

bool UKPCLDeliverNodeWidget::ShouldLockSelectionFromClick(const FKey& MouseButton)
{

	return MouseButton == EKeys::LeftMouseButton;
}

bool UKPCLDeliverNodeWidget::IsLocked() const { return mNode.mState == EKPCLResearchNodeState::Locked; }

bool UKPCLDeliverNodeWidget::IsUnlocked() const { return !IsLocked(); }

bool UKPCLDeliverNodeWidget::CanBeUnlocked() const { return mNode.bCanQueue; }

bool UKPCLDeliverNodeWidget::ArePrerequisitesMet() const { return mNode.bPrerequisitesMet; }

bool UKPCLDeliverNodeWidget::CanBeSelected() const
{

	return IsValid(mDeliveryTreeWidget) && mDeliveryTreeWidget->CanSelectDeliveryTask(mNode.mTask);
}

bool UKPCLDeliverNodeWidget::IsAvailable() const
{
	return mNode.mState == EKPCLResearchNodeState::Available ||
		mNode.mState == EKPCLResearchNodeState::RepeatableAvailable;
}

bool UKPCLDeliverNodeWidget::IsQueued() const
{
	return !mNode.mQueueIndices.IsEmpty() || IsActive() || IsWaitingInQueue();
}

bool UKPCLDeliverNodeWidget::IsWaitingInQueue() const
{
	return mNode.mState == EKPCLResearchNodeState::Queued || mNode.mState == EKPCLResearchNodeState::RepeatableQueued;
}

bool UKPCLDeliverNodeWidget::IsActive() const { return mNode.mState == EKPCLResearchNodeState::Active; }

bool UKPCLDeliverNodeWidget::IsCompleted() const { return mNode.mState == EKPCLResearchNodeState::Completed; }

bool UKPCLDeliverNodeWidget::HasBeenCompleted() const
{
	return mNode.mCompletedCount > 0 || IsCompleted() || IsMaxed();
}

bool UKPCLDeliverNodeWidget::IsRepeatable() const { return mNode.bIsRepeatable; }

bool UKPCLDeliverNodeWidget::IsMaxed() const { return mNode.mState == EKPCLResearchNodeState::Maxed; }

int32 UKPCLDeliverNodeWidget::GetQueueCount() const { return mNode.mQueueIndices.Num(); }

int32 UKPCLDeliverNodeWidget::GetFirstQueueIndex() const
{
	return mNode.mQueueIndices.IsEmpty() ? INDEX_NONE : mNode.mQueueIndices[0];
}

void UKPCLDeliverNodeWidget::SetDeliveryNodeHovered(bool bHovered)
{
	if (!IsValid(mDeliveryTreeWidget) || !IsValid(mNode.mTask))
	{
		return;
	}
	if (bHovered)
	{
		mDeliveryTreeWidget->SetHoveredDeliveryTask(mNode.mTask);
	}
	else if (mDeliveryTreeWidget->IsDeliveryTaskHovered(mNode.mTask))
	{
		mDeliveryTreeWidget->SetHoveredDeliveryTask(nullptr);
	}
}

void UKPCLDeliverNodeWidget::SelectDeliveryNode()
{

	const bool bWasSelected = IsValid(mNode.mTask) && CanBeSelected();
	if (bWasSelected)
	{
		mDeliveryTreeWidget->SelectDeliveryTask(mNode.mTask);
	}

	OnDeliveryNodeSelectionAttempted(bWasSelected, IsLocked());
}

void UKPCLDeliverNodeWidget::UpdateDeliveryNodeInteractionState(bool bHovered, bool bSelected,
																bool bDependencyHighlighted)
{
	bool bStateChanged = false;
	if (bIsHovered != bHovered)
	{
		bIsHovered = bHovered;
		bStateChanged = true;
		OnDeliveryNodeHoverChanged(bIsHovered);
	}
	if (bIsSelected != bSelected)
	{
		bIsSelected = bSelected;
		bStateChanged = true;
		OnDeliveryNodeSelectionChanged(bIsSelected);
	}
	if (bIsDependencyHighlighted != bDependencyHighlighted)
	{
		bIsDependencyHighlighted = bDependencyHighlighted;
		bStateChanged = true;
		OnDeliveryNodeDependencyHighlightChanged(bIsDependencyHighlighted);
	}
	if (bStateChanged)
	{
		MarkDirty(EKPCLDeliveryWidgetDirty::Selection);
	}
}
