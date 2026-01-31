#include "DataAssets/KAPTooltipWidgetInjector.h"

#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Subsystems/KAPIDataAssetSubsystem.h"


bool UKAPTooltipWidgetInjector::ShouldInjectForItem(TSubclassOf<UFGItemDescriptor> ItemClass) const
{
	if (!IsValid(ItemClass))
	{
		return false;
	}
	if (mClassFilter.IsEmpty())
	{
		return true;
	}

	for (TSubclassOf<UFGItemDescriptor> Class : mClassFilter)
	{
		if (!IsValid(Class))
		{
			continue;
		}
		if (ItemClass->IsChildOf(Class))
		{
			return true;
		}
	}

	return false;
}

void UKAPTooltipWidgetInjector::SetPadding(UPanelSlot* Slot, FMargin Padding)
{
	if (UVerticalBoxSlot* AsVBoxSlot = Cast<UVerticalBoxSlot>(Slot))
	{
		AsVBoxSlot->SetPadding(Padding);
	}
	else if (UHorizontalBoxSlot* AsHBoxSlot = Cast<UHorizontalBoxSlot>(Slot))
	{
		AsHBoxSlot->SetPadding(Padding);
	}
}

FMargin UKAPTooltipWidgetInjector::GetPadding(UPanelSlot* Slot)
{
	if (UVerticalBoxSlot* AsVBoxSlot = Cast<UVerticalBoxSlot>(Slot))
	{
		return AsVBoxSlot->GetPadding();
	}
	if (UHorizontalBoxSlot* AsHBoxSlot = Cast<UHorizontalBoxSlot>(Slot))
	{
		return AsHBoxSlot->GetPadding();
	}
	return FMargin();
}
