#pragma once

#include "Modules/ModuleInterface.h"

class FKDataForgeApiModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
