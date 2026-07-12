//

#include "Subsystems/HelperClasses/KBFLCDOCallRequirement.h"
#include "Engine/World.h"
#include "Subsystems/HelperClasses/KBFLCDOOverwriteBase.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"
#include "TimerManager.h"

void UKBFLCDOCallRequirement::OnFinishedAll_Implementation(UKBFLContentCDOHelperSubsystem* Subsystem,
														   UKBFLCDOOverwriteBase* From)
{
}

void UKBFLCDOCallRequirement::OnModified_Implementation(UKBFLContentCDOHelperSubsystem* Subsystem,
														UKBFLCDOOverwriteBase* From, UObject* Target)
{
}

void UKBFLCDOCallRequirement::OnModify_Implementation(UKBFLContentCDOHelperSubsystem* Subsystem,
													  UKBFLCDOOverwriteBase* From, UObject* Target)
{
}

bool UKBFLCDOCallRequirement::IsRequirementMet_Implementation(UKBFLContentCDOHelperSubsystem* Subsystem,
															  UKBFLCDOOverwriteBase* From, UObject* Target)
{
	return true;
}

void UKBFLCDOCallRequirement::OnInit_Implementation() {}

void UKBFLCDOCallRequirement::DeferedCall_Implementation(UKBFLContentCDOHelperSubsystem* Subsystem,
														 UKBFLCDOOverwriteBase* From, UObject* Target)
{
}

bool UKBFLCDOCallRequirement::ShouldCallDefered_Implementation(UKBFLContentCDOHelperSubsystem* Subsystem,
															   UKBFLCDOOverwriteBase* From, UObject* Target)
{
	return false;
}

void UKBFLCDOCallRequirement::DispatchDeferedCall(UKBFLContentCDOHelperSubsystem* Subsystem,
												  UKBFLCDOOverwriteBase* From, UObject* Target)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || !ShouldCallDefered(Subsystem, From, Target))
	{
		// Deferral not requested (or no world to schedule on) - skip DeferedCall entirely.
		return;
	}

	TWeakObjectPtr WeakThis(this);
	TWeakObjectPtr WeakSubsystem(Subsystem);
	TWeakObjectPtr WeakFrom(From);
	TWeakObjectPtr WeakTarget(Target);

	World->GetTimerManager().SetTimerForNextTick(
		[WeakThis, WeakSubsystem, WeakFrom, WeakTarget]()
		{
			if (!WeakSubsystem.IsValid() || !WeakFrom.IsValid() || !WeakTarget.IsValid())
			{
				UE_LOGFMT(
					LogKBFLCDOOverwrite, Warning,
					"DispatchDeferedCall: Deferred call skipped - Subsystem, From or Target became invalid before "
					"next tick.");
				return;
			}

			if (UKBFLCDOCallRequirement* StrongThis = WeakThis.Get())
			{
				StrongThis->DeferedCall(WeakSubsystem.Get(), WeakFrom.Get(), WeakTarget.Get());
			}
		});
}
UWorld* UKBFLCDOCallRequirement::GetWorld() const
{
	if (!IsValid(mSubsystem))
	{
		return nullptr;
	}

	return mSubsystem->GetWorld();
}
