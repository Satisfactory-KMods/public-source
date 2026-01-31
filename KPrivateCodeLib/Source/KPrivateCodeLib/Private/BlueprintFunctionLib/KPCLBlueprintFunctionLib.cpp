// Copyright Coffee Stain Studios. All Rights Reserved.
#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"

#include "AbstractInstanceManager.h"
#include "BFL/KBFL_ConfigTools.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Buildables/FGBuildableRailroadTrack.h"
#include "Components/ListViewBase.h"
#include "Components/NamedSlotInterface.h"
#include "Components/PanelWidget.h"
#include "Configuration/ModConfiguration.h"
#include "Configuration/Properties/ConfigPropertyBool.h"
#include "FGPowerConnectionComponent.h"
#include "KPrivateCodeLibModule.h"
#include "Logging/StructuredLog.h"
#include "ModLoading/ModLoadingLibrary.h"
#include "Resources/FGBuildingDescriptor.h"
#include "Resources/FGEquipmentDescriptor.h"
#include "Resources/FGVehicleDescriptor.h"
#include "Subsystems/KAPIDataAssetSubsystem.h"

void UKPCLBlueprintFunctionLib::SetAllowOnIndex_ThreadSafe(UFGInventoryComponent* Component, int32 Index,
														   TSubclassOf<UFGItemDescriptor> ItemClass)
{
	if (Component)
	{
		if (Component->GetAllowedItemOnIndex(Index) != ItemClass)
		{
			// Einfach ausführen, Buildable lokaler Variablen war ungenutzt
			if (IsInGameThread())
			{
				Component->SetAllowedItemOnIndex(Index, ItemClass);
			}
			else
			{
				FFunctionGraphTask::CreateAndDispatchWhenReady(
					[Component, Index, ItemClass]() { Component->SetAllowedItemOnIndex(Index, ItemClass); },
					GET_STATID(STAT_TaskGraph_OtherTasks), nullptr, ENamedThreads::GameThread);
			}
		}
	}
}

UPanelSlot* UKPCLBlueprintFunctionLib::InjectWidgetAt(UPanelWidget* TargetPanel, UUserWidget* WidgetToInject,
													  FMargin Padding, int32 Index, bool bRequireTFITCheck)
{
	if (!IsValid(TargetPanel) || !IsValid(WidgetToInject))
	{
		return nullptr;
	}

	if (bRequireTFITCheck)
	{
		UModLoadingLibrary* ModLoadingLibrary = UKAPIDataAssetSubsystem::GetModLoadingLibraryWithContext(TargetPanel);
		if (ModLoadingLibrary && ModLoadingLibrary->IsModLoaded("TFIT"))
		{
			TSoftClassPtr<UModConfiguration> TFITConfigClass(
				FSoftClassPath(TEXT("/TFIT/TFITModConfiguration.TFITModConfiguration_C")));
			if (!TFITConfigClass.IsNull() && TFITConfigClass.IsValid())
			{
				TSubclassOf<UModConfiguration> LoadedClass = TFITConfigClass.LoadSynchronous();
				if (IsValid(LoadedClass))
				{
					UConfigPropertySection* Section = UKBFL_ConfigTools::GetPropertySection(LoadedClass, TargetPanel);
					if (IsValid(Section))
					{
						UConfigProperty** Property = Section->SectionProperties.Find(FString("Tooltips"));
						if (Property && IsValid(*Property))
						{
							UConfigPropertySection* TooltipsSection = Cast<UConfigPropertySection>(*Property);
							if (IsValid(TooltipsSection))
							{
								UConfigProperty** EnabledProperty =
									TooltipsSection->SectionProperties.Find(FString("Enabled"));
								if (EnabledProperty && IsValid(*EnabledProperty))
								{
									UConfigPropertyBool* BoolProperty = Cast<UConfigPropertyBool>(*EnabledProperty);
									if (IsValid(BoolProperty) && BoolProperty->Value)
									{
										Index = INDEX_NONE;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (Index == INDEX_NONE)
	{
		UPanelSlot* AddedSlot = TargetPanel->AddChild(WidgetToInject);
		UKAPTooltipWidgetInjector::SetPadding(AddedSlot, Padding);
		return AddedSlot;
	}

	TArray<UWidget*> Childrens = TargetPanel->GetAllChildren();
	TArray<FMargin> Margins;
	for (UWidget* Child : Childrens)
	{
		Margins.Add(UKAPTooltipWidgetInjector::GetPadding(Child->Slot));
	}

	Childrens.Insert(WidgetToInject, Index);
	Margins.Insert(Padding, Index);

	TargetPanel->ClearChildren();
	for (int32 i = 0; i < Childrens.Num(); ++i)
	{
		UPanelSlot* AddedSlot = TargetPanel->AddChild(Childrens[i]);
		UKAPTooltipWidgetInjector::SetPadding(AddedSlot, Margins[i]);
	}
	return WidgetToInject->Slot;
}

void UKPCLBlueprintFunctionLib::UpdateIconLib(UFGIconLibrary* IconLib, TArray<TSubclassOf<UObject>> InObjects,
											  bool bRemoveList)
{
	if (!IsValid(IconLib))
	{
		return;
	}

	for (TSubclassOf<UObject> Object : InObjects)
	{
		if (TSubclassOf<UFGItemDescriptor> ItemClass = TSubclassOf<UFGItemDescriptor>(Object))
		{
			FIconData* ExistingIconData = IconLib->mIconData.FindByPredicate(
				[ItemClass](const FIconData& IconData)
				{
					if (!IsValid(IconData.ItemDescriptor.LoadSynchronous()))
					{
						return false;
					}

					return IconData.ItemDescriptor.LoadSynchronous() == ItemClass;
				});
			UTexture* Icon = UFGItemDescriptor::GetBigIcon(ItemClass);

			EIconType IconType = EIconType::ESIT_Part;
			if (ItemClass->IsChildOf(UFGEquipmentDescriptor::StaticClass()))
			{
				IconType = EIconType::ESIT_Equipment;
			}
			if (ItemClass->IsChildOf(UFGVehicleDescriptor::StaticClass()) ||
				ItemClass->IsChildOf(UFGBuildingDescriptor::StaticClass()))
			{
				IconType = EIconType::ESIT_Building;
			}

			if (ExistingIconData)
			{
				ExistingIconData->IconName = FText::GetEmpty();
				ExistingIconData->IconType = IconType;
				ExistingIconData->Texture = bRemoveList ? nullptr : Icon;
				ExistingIconData->Hidden = !IsValid(ItemClass) || bRemoveList;

				continue;
			}

			int32 ID = 0;
			while (IconLib->mIconData.FindByPredicate([ID](const FIconData& IconData) { return IconData.ID == ID; }))
			{
				ID++;
			}

			FIconData NewIconData;
			NewIconData.Hidden = bRemoveList;
			NewIconData.ID = ID;
			NewIconData.ItemDescriptor = ItemClass;
			NewIconData.IconName = FText::GetEmpty();
			NewIconData.IconType = IconType;
			NewIconData.Texture = Icon;
			if (!bRemoveList)
			{
				IconLib->mIconData.Add(NewIconData);
			}
		}
	}

	int32 Index = 0;
	while (Index < IconLib->mIconData.Num())
	{
		FIconData& Data = IconLib->mIconData[Index];
		Data.IconName = FText::GetEmpty();

		TSubclassOf<UFGItemDescriptor> ItemClass = Data.ItemDescriptor.LoadSynchronous();
		UObject* Texture = UFGItemDescriptor::GetBigIcon(ItemClass);

		UE_LOGFMT(
			LogKPCL, Error,
			"UKPCLBlueprintFunctionLib::UpdateIconLib: Processing IconData IDX={0} ID={1}, ItemClass={2}, Texture={3}",
			Index, Data.ID, IsValid(ItemClass) ? *ItemClass->GetName() : TEXT("Invalid"),
			IsValid(Cast<UTexture>(Texture)) ? *Texture->GetName() : TEXT("Invalid"));

		if (!IsValid(ItemClass) || !IsValid(Texture))
		{
			Data.Hidden = true;
			Data.DisplayNameOverride = true;
			Data.IconName = FText::GetEmpty();
			Data.Texture = nullptr;
			Index++;
			continue;
		}

		Data.Texture = Texture;
		Data.DisplayNameOverride = false;
		Index++;
	}

	IconLib->MarkPackageDirty();
}

UObject* UKPCLBlueprintFunctionLib::GetDefaultSilent(TSubclassOf<UObject> InClass)
{
	if (IsValid(InClass))
	{
		return InClass.GetDefaultObject();
	}
	return nullptr;
}

AActor* UKPCLBlueprintFunctionLib::ResolveHitResult(const FHitResult& InHitResult)
{
	AActor* ActorHit = InHitResult.GetActor();
	if (!IsValid(ActorHit))
	{
		return nullptr;
	}
	FInstanceHandle Handle;
	AAbstractInstanceManager* Manager = AAbstractInstanceManager::GetInstanceManager(ActorHit->GetWorld());
	if (IsValid(Manager))
	{
		Manager->ResolveHit(InHitResult, Handle);
	}

	if (IsValid(Handle.GetOwner()))
	{
		return Handle.GetOwner();
	}
	return InHitResult.GetActor();
}

AActor* UKPCLBlueprintFunctionLib::ResolveOverlapResult(const FOverlapResult& InOverlapResult)
{
	AActor* ActorHit = InOverlapResult.GetActor();
	if (!IsValid(ActorHit))
	{
		return nullptr;
	}
	FInstanceHandle Handle;
	AAbstractInstanceManager* Manager = AAbstractInstanceManager::GetInstanceManager(ActorHit->GetWorld());
	if (IsValid(Manager))
	{
		Manager->ResolveOverlap(InOverlapResult, Handle);
	}

	if (IsValid(Handle.GetOwner()))
	{
		return Handle.GetOwner();
	}
	return InOverlapResult.GetActor();
}

TArray<UWidget*> UKPCLBlueprintFunctionLib::FindChildWidgetsOfClassDeep(UUserWidget* CurrentWidget,
																		TSubclassOf<UWidget> WidgetClass)
{
	TSet<UWidget*> ResultWidgets;

	if (!CurrentWidget || !WidgetClass)
	{
		return ResultWidgets.Array();
	}

	if (CurrentWidget->WidgetTree)
	{
		UWidget* RootWidget = CurrentWidget->WidgetTree->RootWidget;
		if (!RootWidget)
		{
			return ResultWidgets.Array();
		}

		TArray<UWidget*> WidgetsToVisit;
		WidgetsToVisit.Add(RootWidget);

		while (WidgetsToVisit.Num() > 0)
		{
			UWidget* VisitWidget = WidgetsToVisit.Pop();
			if (!VisitWidget)
			{
				continue;
			}

			if (VisitWidget->IsA(WidgetClass))
			{
				ResultWidgets.Add(VisitWidget);
			}

			if (UUserWidget* NestedUserWidget = Cast<UUserWidget>(VisitWidget))
			{
				ResultWidgets.Append(FindChildWidgetsOfClassDeep(NestedUserWidget, WidgetClass));
			}

			if (UPanelWidget* Panel = Cast<UPanelWidget>(VisitWidget))
			{
				int32 ChildrenCount = Panel->GetChildrenCount();
				for (int32 i = ChildrenCount - 1; i >= 0; --i)
				{
					if (UWidget* Child = Panel->GetChildAt(i))
					{
						WidgetsToVisit.Add(Child);
					}
				}
			}

			if (UListViewBase* ListView = Cast<UListViewBase>(VisitWidget))
			{
				for (UUserWidget* Entry : ListView->GetDisplayedEntryWidgets())
				{
					if (Entry)
					{
						if (Entry->IsA(WidgetClass))
						{
							ResultWidgets.Add(Entry);
						}
						ResultWidgets.Append(FindChildWidgetsOfClassDeep(Entry, WidgetClass));
					}
				}
			}
		}
	}
	return ResultWidgets.Array();
}
