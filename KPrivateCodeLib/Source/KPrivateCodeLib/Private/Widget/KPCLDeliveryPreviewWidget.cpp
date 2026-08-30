#include "Widget/KPCLDeliveryPreviewWidget.h"

#include "Widget/KPCLDeliveryTreeWidget.h"

void UKPCLDeliveryPreviewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (IsValid(mDeliveryTreeWidget))
	{
		SetupDeliveryPreview(mDeliveryTreeWidget);
	}
}

void UKPCLDeliveryPreviewWidget::NativeDestruct()
{
	TeardownDeliveryPreview();
	Super::NativeDestruct();
}

bool UKPCLDeliveryPreviewWidget::SetupDeliveryPreview(UKPCLDeliveryTreeWidget* DeliveryTreeWidget)
{
	if (!IsValid(DeliveryTreeWidget) || !IsValid(DeliveryTreeWidget->mDeliveryTaskSubsystem))
	{
		return false;
	}
	if (bIsSetup && mDeliveryTreeWidget == DeliveryTreeWidget)
	{
		if (IsValid(DeliveryTreeWidget->mSelectedTask))
		{
			return SetPreviewedDeliveryTask(DeliveryTreeWidget->mSelectedTask);
		}
		ClearDeliveryPreviewInternal(false);
		return true;
	}

	TeardownDeliveryPreview();
	mDeliveryTreeWidget = DeliveryTreeWidget;
	mDeliveryTaskSubsystem = DeliveryTreeWidget->mDeliveryTaskSubsystem;
	bIsSetup = true;
	BindSubsystemEvents();
	OnDeliveryPreviewSetup(mDeliveryTreeWidget);
	if (IsValid(DeliveryTreeWidget->mHoveredTask))
	{
		return SetPreviewedDeliveryTask(DeliveryTreeWidget->mHoveredTask);
	}
	ClearDeliveryPreviewInternal(false);
	return true;
}

void UKPCLDeliveryPreviewWidget::TeardownDeliveryPreview()
{
	const bool bWasSetup = bIsSetup;
	UnbindSubsystemEvents();

	ClearDeliveryPreviewInternal(false);
	mDeliveryTaskSubsystem = nullptr;
	mDeliveryTreeWidget = nullptr;
	bIsSetup = false;
	if (bWasSetup)
	{
		OnDeliveryPreviewDestroyed();
	}
}

bool UKPCLDeliveryPreviewWidget::SetPreviewedDeliveryTask(UKAPIDeliveryTask* Task)
{
	if (!IsValid(Task))
	{
		ClearDeliveryPreview();
		return false;
	}
	if (!IsValid(mDeliveryTreeWidget) || !IsValid(mDeliveryTaskSubsystem))
	{
		return false;
	}

	FKPCLResearchTreeNode PreviewNode;
	if (!mDeliveryTreeWidget->GetCachedNode(Task, PreviewNode))
	{
		ClearDeliveryPreviewInternal(false);
		return false;
	}

	mPreviewTask = Task;
	mPreviewNode = MoveTemp(PreviewNode);
	bHasPreview = true;
	OnDeliveryPreviewChanged(mPreviewNode);
	MarkDirty(EKPCLDeliveryWidgetDirty::Data | EKPCLDeliveryWidgetDirty::State | EKPCLDeliveryWidgetDirty::Progress |
			  EKPCLDeliveryWidgetDirty::Queue);
	return true;
}

void UKPCLDeliveryPreviewWidget::ClearDeliveryPreview() { ClearDeliveryPreviewInternal(true); }

void UKPCLDeliveryPreviewWidget::ClearDeliveryPreviewInternal(bool bClearTreeSelection)
{
	UKAPIDeliveryTask* PreviousTask = mPreviewTask;
	const bool bHadPreview = bHasPreview || IsValid(PreviousTask);
	mPreviewTask = nullptr;
	mPreviewNode = FKPCLResearchTreeNode();
	bHasPreview = false;
	if (bHadPreview)
	{
		OnDeliveryPreviewCleared(PreviousTask);
		MarkDirty(EKPCLDeliveryWidgetDirty::Data);
	}

	if (bClearTreeSelection && IsValid(mDeliveryTreeWidget))
	{
		mDeliveryTreeWidget->ClearDeliveryTaskSelection();
	}
}

bool UKPCLDeliveryPreviewWidget::RefreshDeliveryPreview()
{
	if (!IsValid(mDeliveryTaskSubsystem) || !IsValid(mPreviewTask))
	{
		return false;
	}

	FKPCLResearchTreeNode PreviewNode;
	if (!mDeliveryTaskSubsystem->GetResearchNode(mPreviewTask, PreviewNode))
	{
		ClearDeliveryPreviewInternal(false);
		return false;
	}
	mPreviewNode = MoveTemp(PreviewNode);
	bHasPreview = true;
	OnDeliveryPreviewChanged(mPreviewNode);
	MarkDirty(EKPCLDeliveryWidgetDirty::Data);
	return true;
}

bool UKPCLDeliveryPreviewWidget::TryQueuePreviewedTask(TArray<FText>& ErrorMessages)
{
	UKAPIDeliveryTask* PreviewTask = mPreviewTask;
	bool bWasQueued = false;
	if (!IsValid(mDeliveryTreeWidget) || !IsValid(mPreviewTask))
	{
		ErrorMessages.Reset();
	}
	else
	{
		bWasQueued = mDeliveryTreeWidget->TryQueueTask(mPreviewTask, ErrorMessages);
	}

	OnDeliveryPreviewQueueAttempted(PreviewTask, bWasQueued, ErrorMessages);
	return bWasQueued;
}

UFGInventoryComponent* UKPCLDeliveryPreviewWidget::GetManualDeliveryInventory() const
{
	return IsValid(mDeliveryTaskSubsystem) ? mDeliveryTaskSubsystem->GetManualDeliveryInventory() : nullptr;
}

UFGInventoryComponent* UKPCLDeliveryPreviewWidget::GetRewardInventory() const
{
	return IsValid(mDeliveryTaskSubsystem) ? mDeliveryTaskSubsystem->GetRewardInventory() : nullptr;
}

TArray<UFGAvailabilityDependency*> UKPCLDeliveryPreviewWidget::GetTaskNotMetDependencies() const
{
	return IsValid(mDeliveryTaskSubsystem)
			   ? mDeliveryTaskSubsystem->GetTaskNotMetDependencies(mPreviewTask)
			   : TArray<UFGAvailabilityDependency*>();
}

TArray<FKPCLParentTaskNotMetReason> UKPCLDeliveryPreviewWidget::GetNotMetParentTask() const
{
	return IsValid(mDeliveryTaskSubsystem)
			   ? mDeliveryTaskSubsystem->GetNotMetParentTask(mPreviewTask)
			   : TArray<FKPCLParentTaskNotMetReason>();
}

bool UKPCLDeliveryPreviewWidget::ShouldShowUnlocks() const
{
	return GetTaskNotMetDependencies().IsEmpty() && GetNotMetParentTask().IsEmpty();
}

void UKPCLDeliveryPreviewWidget::HandleResearchQueueOrderChanged()
{
	if (bHasPreview)
	{
		MarkDirty(EKPCLDeliveryWidgetDirty::Queue);
		RefreshDeliveryPreview();
	}
}

void UKPCLDeliveryPreviewWidget::HandleResearchNodeChanged(const FKPCLResearchTreeNode& Node)
{
	if (Node.mTask == mPreviewTask)
	{
		MarkDirty(EKPCLDeliveryWidgetDirty::State);
		RefreshDeliveryPreview();
	}
}

void UKPCLDeliveryPreviewWidget::HandleDeliveryTaskCompleted(UKAPIDeliveryTask* Task, int32 CompletionCount)
{
	if (Task == mPreviewTask)
	{
		OnDeliveryPreviewTaskCompleted(Task, CompletionCount);
	}
}

void UKPCLDeliveryPreviewWidget::HandleActiveResearchChanged(UKAPIDeliveryTask* PreviousTask,
															 UKAPIDeliveryTask* CurrentTask)
{
	if (PreviousTask == mPreviewTask || CurrentTask == mPreviewTask)
	{
		MarkDirty(EKPCLDeliveryWidgetDirty::State | EKPCLDeliveryWidgetDirty::Queue);
		RefreshDeliveryPreview();
	}
}

void UKPCLDeliveryPreviewWidget::HandleResearchProgressChanged(UKAPIDeliveryTask* Task, float Progress)
{
	if (Task == mPreviewTask)
	{
		MarkDirty(EKPCLDeliveryWidgetDirty::Progress);
		RefreshDeliveryPreview();
	}
}

void UKPCLDeliveryPreviewWidget::HandleResearchAvailabilityChanged(UKAPIDeliveryTask* Task, bool bIsAvailable)
{
	if (Task == mPreviewTask)
	{
		MarkDirty(EKPCLDeliveryWidgetDirty::State);
		RefreshDeliveryPreview();
	}
}
