// Fill out your copyright notice in the Description page of Project Settings.

#include "KPCLWorldModule.h"

DECLARE_LOG_CATEGORY_EXTERN(KBFLWorldModuleV2Log, Log, All);

DEFINE_LOG_CATEGORY(KBFLWorldModuleV2Log);

UKPCLWorldModule::UKPCLWorldModule() : Super() {}

void UKPCLWorldModule::InitPhase_Implementation()
{
	Super::InitPhase_Implementation();

	if (IsEasyNodesEnabled())
	{
		ApplyEasyNodes(mResearchTrees);
	}
}

bool UKPCLWorldModule::IsEasyNodesEnabled_Implementation() { return mUseEasyNodes; }

void UKPCLWorldModule::ApplyEasyNodes_Implementation(const TArray<TSubclassOf<UFGResearchTree>>& Nodes) {}
