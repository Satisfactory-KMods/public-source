// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/RssSignWidget.h"

#include "Interface/RssSignInterface.h"
#include "Kismet/KismetSystemLibrary.h"

void URssSignWidget::SetBuildable(AActor* Buildable)
{
	if (IsValid(Buildable) && UKismetSystemLibrary::DoesImplementInterface(Buildable, URssSignInterface::StaticClass()))
	{
		mBuildable = Buildable;
		mSignWidgetData = IRssSignInterface::Execute_GetSignData(Buildable);
		mUIData = IRssSignInterface::Execute_GetSignUiIData(Buildable);
		OnBuildableSet(Buildable);
		return;
	}

	// Pooled render components may be reassigned after their previous sign was destroyed.
	// Do not leave a stale actor for Blueprint event handlers to cast or dereference.
	mBuildable = nullptr;
}

void URssSignWidget::UpdateSignData(FRssSignData SignData)
{
	mSignWidgetData = SignData;
	if (IsValid(mBuildable) &&
		UKismetSystemLibrary::DoesImplementInterface(mBuildable, URssSignInterface::StaticClass()))
	{
		OnSignDataUpdated(mSignWidgetData);
	}
}

void URssSignWidget::OnRequestUpdateSign_Implementation() {}
