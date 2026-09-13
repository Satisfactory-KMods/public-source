// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"

class FKDataForgeEditorUIModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
