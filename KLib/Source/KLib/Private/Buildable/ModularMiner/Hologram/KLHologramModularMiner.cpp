// Copyright Coffee Stain Studios. All Rights Reserved.


#include "Buildable/ModularMiner/Hologram/KLHologramModularMiner.h"

#include "FGConstructDisqualifier.h"
#include "Resources/FGExtractableResourceInterface.h"
#include "Subsystem/KLUnlockSubsystem.h"

void AKLHologramModularMiner::CheckValidPlacement()
{
	Super::CheckValidPlacement();

	if (mSnappedExtractableResource)
	{
		if (AKLUnlockSubsystem* Subsystem = AKLUnlockSubsystem::Get(GetWorld()))
		{
			if (TSubclassOf<UFGResourceDescriptor> Resource = mSnappedExtractableResource->GetResourceClass())
			{
				if (!Subsystem->HasInformationAboutOre(Resource) && mInvalidKnowledge)
				{
					AddConstructDisqualifier(mInvalidKnowledge);
				}
			}
		}
	}

	if (mConstructDisqualifiers.Contains(UFGCDInvalidFloor::StaticClass()))
	{
		mConstructDisqualifiers.Remove(UFGCDInvalidFloor::StaticClass());
	}
}
