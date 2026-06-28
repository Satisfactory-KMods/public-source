// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/RssSignWidget.h"

#include "Interface/RssSignInterface.h"
#include "Kismet/KismetSystemLibrary.h"

void URssSignWidget::SetBuildable(AActor* Buildable)
{
	if (Buildable)
	{
		if (UKismetSystemLibrary::DoesImplementInterface(Buildable, URssSignInterface::StaticClass()))
		{
			mBuildable = Buildable;
			mSignWidgetData = IRssSignInterface::Execute_GetSignData(mBuildable);
			mUIData = IRssSignInterface::Execute_GetSignUiIData(mBuildable);
			OnBuildableSet(mBuildable);
		}
	}
}

void URssSignWidget::UpdateSignData(FRssSignData SignData)
{
	mSignWidgetData = SignData;
	OnSignDataUpdated(mSignWidgetData);
}

void URssSignWidget::OnRequestUpdateSign_Implementation() {}
