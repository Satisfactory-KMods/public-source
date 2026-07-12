#include "Module/KDFGameInstanceModule.h"

#include "KDFLogging.h"
#include "Module/GameInstanceModuleManager.h"
#include "Net/KDFNetHandler.h"
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

	// SML dispatches module phases during its own subsystem initialization — custom subsystems may
	// not exist yet at that point. SML provides exactly this hook to force-create them first.
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
	case ELifecyclePhase::CONSTRUCTION:
		FKDFNetHandler::Register(); // MP patchset checksum handshake (idempotent per engine run)
		break;
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
