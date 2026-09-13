// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Widget/KPCLDeliveryQueueWidget.h"

#include "Widget/KPCLDeliveryTreeWidget.h"

void UKPCLDeliveryQueueWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (IsValid(mDeliveryTreeWidget))
	{
		SetupDeliveryQueue(mDeliveryTreeWidget);
	}
}

void UKPCLDeliveryQueueWidget::NativeDestruct()
{
	TeardownDeliveryQueue();
	Super::NativeDestruct();
}

bool UKPCLDeliveryQueueWidget::SetupDeliveryQueue(UKPCLDeliveryTreeWidget* DeliveryTreeWidget)
{
	if (!IsValid(DeliveryTreeWidget) || !IsValid(DeliveryTreeWidget->mDeliveryTaskSubsystem))
	{
		return false;
	}
	if (bIsSetup && mDeliveryTreeWidget == DeliveryTreeWidget)
	{
		return RefreshDeliveryQueue();
	}

	TeardownDeliveryQueue();
	mDeliveryTreeWidget = DeliveryTreeWidget;
	mDeliveryTaskSubsystem = DeliveryTreeWidget->mDeliveryTaskSubsystem;
	bIsSetup = true;
	BindSubsystemEvents();
	OnDeliveryQueueSetup(mDeliveryTreeWidget);
	return RefreshDeliveryQueue();
}

void UKPCLDeliveryQueueWidget::TeardownDeliveryQueue()
{
	const bool bWasSetup = bIsSetup;
	UnbindSubsystemEvents();
	mQueueEntries.Reset();
	mDeliveryTaskSubsystem = nullptr;
	mDeliveryTreeWidget = nullptr;
	bIsSetup = false;
	if (bWasSetup)
	{
		OnDeliveryQueueDestroyed();
	}
}

bool UKPCLDeliveryQueueWidget::RefreshDeliveryQueue()
{
	if (!IsValid(mDeliveryTaskSubsystem))
	{
		return false;
	}

	const TArray<UKAPIDeliveryTask*> QueuedTasks = mDeliveryTaskSubsystem->GetQueuedTasks();
	TArray<FKPCLDeliveryQueueEntry> RefreshedEntries;
	RefreshedEntries.Reserve(QueuedTasks.Num());
	for (int32 QueueIndex = 0; QueueIndex < QueuedTasks.Num(); ++QueueIndex)
	{
		FKPCLResearchTreeNode Node;
		if (!mDeliveryTaskSubsystem->GetResearchNode(QueuedTasks[QueueIndex], Node))
		{
			continue;
		}

		FKPCLDeliveryQueueEntry& Entry = RefreshedEntries.AddDefaulted_GetRef();
		Entry.mQueueIndex = QueueIndex;
		Entry.mTask = Node.mTask;
		Entry.mNode = MoveTemp(Node);
		Entry.bIsActive = QueueIndex == 0;
		Entry.bCanRemove = mDeliveryTaskSubsystem->CanRemoveFromQueueAt(QueueIndex);
	}

	mQueueEntries = MoveTemp(RefreshedEntries);
	OnDeliveryQueueChanged(mQueueEntries);
	MarkDirty(EKPCLDeliveryWidgetDirty::Data | EKPCLDeliveryWidgetDirty::Queue);
	return true;
}

bool UKPCLDeliveryQueueWidget::GetDeliveryQueueEntry(int32 QueueIndex, FKPCLDeliveryQueueEntry& OutEntry) const
{
	if (!mQueueEntries.IsValidIndex(QueueIndex))
	{
		OutEntry = FKPCLDeliveryQueueEntry();
		return false;
	}
	OutEntry = mQueueEntries[QueueIndex];
	return true;
}

bool UKPCLDeliveryQueueWidget::CanRemoveQueueEntry(int32 QueueIndex) const
{
	return IsValid(mDeliveryTaskSubsystem) && mDeliveryTaskSubsystem->CanRemoveFromQueueAt(QueueIndex);
}

bool UKPCLDeliveryQueueWidget::TryRemoveQueueEntry(int32 QueueIndex)
{
	return IsValid(mDeliveryTreeWidget) && mDeliveryTreeWidget->TryRemoveLastQueueEntry(QueueIndex);
}

bool UKPCLDeliveryQueueWidget::TryMoveQueueEntry(int32 FromIndex, int32 ToIndex)
{
	return IsValid(mDeliveryTreeWidget) && mDeliveryTreeWidget->TryMoveQueueEntry(FromIndex, ToIndex);
}

bool UKPCLDeliveryQueueWidget::ContainsQueuedTask(UKAPIDeliveryTask* Task) const
{
	return IsValid(Task) &&
		mQueueEntries.ContainsByPredicate([Task](const FKPCLDeliveryQueueEntry& Entry) { return Entry.mTask == Task; });
}

void UKPCLDeliveryQueueWidget::HandleResearchQueueOrderChanged() { RefreshDeliveryQueue(); }

void UKPCLDeliveryQueueWidget::HandleResearchNodeChanged(const FKPCLResearchTreeNode& Node)
{
	if (ContainsQueuedTask(Node.mTask))
	{
		MarkDirty(EKPCLDeliveryWidgetDirty::State);
		RefreshDeliveryQueue();
	}
}

void UKPCLDeliveryQueueWidget::HandleActiveResearchChanged(UKAPIDeliveryTask* PreviousTask,
														   UKAPIDeliveryTask* CurrentTask)
{
	MarkDirty(EKPCLDeliveryWidgetDirty::State);
	RefreshDeliveryQueue();
}

void UKPCLDeliveryQueueWidget::HandleResearchProgressChanged(UKAPIDeliveryTask* Task, float Progress)
{
	if (ContainsQueuedTask(Task))
	{
		MarkDirty(EKPCLDeliveryWidgetDirty::Progress);
		RefreshDeliveryQueue();
	}
}

void UKPCLDeliveryQueueWidget::HandleResearchAvailabilityChanged(UKAPIDeliveryTask* Task, bool bIsAvailable)
{
	if (ContainsQueuedTask(Task))
	{
		MarkDirty(EKPCLDeliveryWidgetDirty::State);
		RefreshDeliveryQueue();
	}
}
