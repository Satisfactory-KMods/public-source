#include "DataAssets/KAPIModularMinerDescription.h"

FKAPIModuleItems UKAPIModularMinerDescription::GetItemsForModule(TSubclassOf<UKAPIWasteProducerType> Module) const
{
	if (!IsValid(Module) || !mModuleInformation.Contains(Module))
	{
		return FKAPIModuleItems();
	}

	return mModuleInformation.FindRef(Module);
}

bool UKAPIModularMinerDescription::IsModuleAllowed(TSubclassOf<AFGBuildable> Module) const
{
	return !mPreventModules.Contains(Module);
}
