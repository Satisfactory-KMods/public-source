#include "KDataForgeYamlModule.h"

#include "Modules/ModuleManager.h"

extern void KDFYamlInstallErrorCallbacks();

void FKDataForgeYamlModule::StartupModule() { KDFYamlInstallErrorCallbacks(); }

void FKDataForgeYamlModule::ShutdownModule() {}

IMPLEMENT_MODULE(FKDataForgeYamlModule, KDataForgeYaml)
