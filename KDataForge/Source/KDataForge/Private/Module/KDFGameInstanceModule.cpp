// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Module/KDFGameInstanceModule.h"

#include "KDFLogging.h"
#include "Module/GameInstanceModuleManager.h"
#include "Net/KDFRemoteCallObject.h"
#include "Subsystems/KDFSubsystem.h"

UKDFGameInstanceModule::UKDFGameInstanceModule()
{
	bRootModule = true;
	RemoteCallObjects.Add(UKDFRemoteCallObject::StaticClass());
}

void UKDFGameInstanceModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
	Super::DispatchLifecycleEvent(Phase);

	if (UGameInstanceModuleManager* ModuleManager =
			GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UGameInstanceModuleManager>() : nullptr)
	{
		ModuleManager->EnsureSubsystemInitialized(UKDFSubsystem::StaticClass());
	}

	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(GetGameInstance());
	if (Subsystem == nullptr)
	{
		UE_LOG(LogKDataForge, Error, TEXT("KDF subsystem missing during lifecycle phase %s"),
			   *LifecyclePhaseToString(Phase));
		return;
	}

	switch (Phase)
	{
	case ELifecyclePhase::INITIALIZATION:
		Subsystem->RunInitialLoad();
		break;
	case ELifecyclePhase::POST_INITIALIZATION:
		UE_LOG(LogKDataForge, Display, TEXT("%s"), *Subsystem->BuildReportString());
		break;
	default:
		break;
	}
}
