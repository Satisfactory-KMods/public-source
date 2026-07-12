#pragma once

#include "Modules/ModuleInterface.h"

/**
 * In-game editor UI module (Phase 4 — browser, inspector, property widget factory, export).
 * Kept as a separate module so dedicated servers and headless targets stay lean.
 */
class FKDataForgeEditorUIModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
