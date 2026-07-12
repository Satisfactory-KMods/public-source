#include "KDataForgeYamlModule.h"

#include "Modules/ModuleManager.h"

// Defined in KDFYamlParser.cpp; routes ryml errors into contained C++ exceptions.
extern void KDFYamlInstallErrorCallbacks();

void FKDataForgeYamlModule::StartupModule() { KDFYamlInstallErrorCallbacks(); }

void FKDataForgeYamlModule::ShutdownModule() {}

IMPLEMENT_MODULE(FKDataForgeYamlModule, KDataForgeYaml)
