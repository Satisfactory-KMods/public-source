#include "KDataForgeModule.h"

#include "KDFLogging.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogKDataForge);

void FKDataForgeModule::StartupModule() {}

void FKDataForgeModule::ShutdownModule() {}

IMPLEMENT_MODULE(FKDataForgeModule, KDataForge)
