#include "Module/KDFWorldModule.h"

#include "Commands/KDFChatCommand.h"
#include "Subsystems/KDFStateSubsystem.h"
#include "Subsystems/KDFSubsystem.h"

UKDFWorldModule::UKDFWorldModule()
{
	bRootModule = true;
	mChatCommands.Add(AKDFChatCommand::StaticClass());
	ModSubsystems.Add(AKDFStateSubsystem::StaticClass());
}

void UKDFWorldModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
	Super::DispatchLifecycleEvent(Phase);

	if (Phase == ELifecyclePhase::INITIALIZATION)
	{
		if (UKDFSubsystem* Subsystem = UKDFSubsystem::Get(GetWorld()))
		{
			Subsystem->RegisterQueuedContentForWorld(GetWorld());
		}
	}
}
