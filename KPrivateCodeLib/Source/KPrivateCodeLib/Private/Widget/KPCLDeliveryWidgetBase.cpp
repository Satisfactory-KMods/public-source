#include "Widget/KPCLDeliveryWidgetBase.h"

#include "Engine/World.h"
#include "TimerManager.h"

void UKPCLDeliveryWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	ScheduleDeliveryWidgetRerender();
}

void UKPCLDeliveryWidgetBase::NativeDestruct()
{
	mDirtyFlags = 0;
	bRerenderScheduled = false;
	Super::NativeDestruct();
}

void UKPCLDeliveryWidgetBase::MarkDeliveryWidgetDirty(int32 DirtyFlags)
{
	if (DirtyFlags == 0)
	{
		return;
	}

	mDirtyFlags |= DirtyFlags;
	ScheduleDeliveryWidgetRerender();
}

bool UKPCLDeliveryWidgetBase::FlushDeliveryWidgetRerender()
{
	if (mDirtyFlags == 0)
	{
		return false;
	}

	const int32 DirtyFlags = mDirtyFlags;
	mDirtyFlags = 0;
	NativeOnDeliveryWidgetRerender(DirtyFlags);
	OnDeliveryWidgetRerender(DirtyFlags);
	return true;
}

bool UKPCLDeliveryWidgetBase::IsDeliveryWidgetDirty(int32 DirtyFlags) const
{
	return DirtyFlags == 0 ? mDirtyFlags != 0 : (mDirtyFlags & DirtyFlags) != 0;
}

void UKPCLDeliveryWidgetBase::NativeOnDeliveryWidgetRerender(int32 DirtyFlags) { InvalidateLayoutAndVolatility(); }

void UKPCLDeliveryWidgetBase::BindSubsystemEvents()
{
	if (!IsValid(mDeliveryTaskSubsystem))
	{
		return;
	}
	mDeliveryTaskSubsystem->mOnResearchQueueOrderChanged.AddUniqueDynamic(
		this, &UKPCLDeliveryWidgetBase::HandleResearchQueueOrderChanged);
	mDeliveryTaskSubsystem->mOnResearchNodeChanged.AddUniqueDynamic(
		this, &UKPCLDeliveryWidgetBase::HandleResearchNodeChanged);
	mDeliveryTaskSubsystem->mOnTaskCompleted.AddUniqueDynamic(this,
															  &UKPCLDeliveryWidgetBase::HandleDeliveryTaskCompleted);
	mDeliveryTaskSubsystem->mOnActiveResearchChanged.AddUniqueDynamic(
		this, &UKPCLDeliveryWidgetBase::HandleActiveResearchChanged);
	mDeliveryTaskSubsystem->mOnResearchProgressChanged.AddUniqueDynamic(
		this, &UKPCLDeliveryWidgetBase::HandleResearchProgressChanged);
	mDeliveryTaskSubsystem->mOnResearchAvailabilityChanged.AddUniqueDynamic(
		this, &UKPCLDeliveryWidgetBase::HandleResearchAvailabilityChanged);
}

void UKPCLDeliveryWidgetBase::UnbindSubsystemEvents()
{
	if (!IsValid(mDeliveryTaskSubsystem))
	{
		return;
	}
	mDeliveryTaskSubsystem->mOnResearchQueueOrderChanged.RemoveAll(this);
	mDeliveryTaskSubsystem->mOnResearchNodeChanged.RemoveAll(this);
	mDeliveryTaskSubsystem->mOnTaskCompleted.RemoveAll(this);
	mDeliveryTaskSubsystem->mOnActiveResearchChanged.RemoveAll(this);
	mDeliveryTaskSubsystem->mOnResearchProgressChanged.RemoveAll(this);
	mDeliveryTaskSubsystem->mOnResearchAvailabilityChanged.RemoveAll(this);
}

void UKPCLDeliveryWidgetBase::ScheduleDeliveryWidgetRerender()
{
	if (bRerenderScheduled || mDirtyFlags == 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{

		return;
	}

	bRerenderScheduled = true;
	TWeakObjectPtr<UKPCLDeliveryWidgetBase> WeakThis(this);
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda(
		[WeakThis]()
		{
			if (UKPCLDeliveryWidgetBase* Widget = WeakThis.Get())
			{
				Widget->bRerenderScheduled = false;
				Widget->FlushDeliveryWidgetRerender();
			}
		}));
}
