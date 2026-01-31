// Fill out your copyright notice in the Description page of Project Settings.


#include "KBFLMenuModule.h"

#include "Blueprint/UserWidget.h"
#include "Patching/BlueprintHookManager.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"

UKBFLMenuModule::UKBFLMenuModule() { bRootModule = false; }

void UKBFLMenuModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
	if (Phase == ELifecyclePhase::CONSTRUCTION)
	{
		ConstructionPhase();
	}

	if (Phase == ELifecyclePhase::INITIALIZATION)
	{
		InitPhase();
	}

	if (Phase == ELifecyclePhase::POST_INITIALIZATION)
	{
		PostInitPhase();
	}

	Super::DispatchLifecycleEvent(Phase);
}

void UKBFLMenuModule::InitPhase_Implementation() {}

void UKBFLMenuModule::ConstructionPhase_Implementation() {}

void UKBFLMenuModule::PostInitPhase_Implementation() {}
