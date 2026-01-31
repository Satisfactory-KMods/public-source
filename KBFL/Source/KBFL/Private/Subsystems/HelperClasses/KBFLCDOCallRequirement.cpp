//

#include "Subsystems/HelperClasses/KBFLCDOCallRequirement.h"
#include "Subsystems/HelperClasses/KBFLCDOOverwriteBase.h"
#include "Subsystems/KBFLContentCDOHelperSubsystem.h"

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
UWorld* UKBFLCDOCallRequirement::GetWorld() const
{
	if (!IsValid(mSubsystem))
	{
		return nullptr;
	}

	return mSubsystem->GetWorld();
}
