#include "Unlocks/KPCLFaxitFeatureUnlock.h"

#include "Subsystem/KPCLFaxitSubsystem.h"

void UKPCLFaxitFeatureUnlock::ApplyToFaxit(AKPCLFaxitSubsystem* Subsystem) const
{
	if (!IsValid(Subsystem))
	{
		return;
	}

	switch (mFeature)
	{
	case EKPCLFaxitFeature::RemoteAccess:
		Subsystem->SetRemoteAccessUnlocked(true);
		break;
	case EKPCLFaxitFeature::Sink:
		Subsystem->SetSinkUnlocked(true);
		break;
	case EKPCLFaxitFeature::Depot:
		Subsystem->SetDepotUnlocked(true);
		break;
	}
}
