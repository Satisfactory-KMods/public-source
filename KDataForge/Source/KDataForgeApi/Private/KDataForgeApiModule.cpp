#include "KDataForgeApiModule.h"

#include "KDataForgeApiLogging.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogKDataForgeApi);

void FKDataForgeApiModule::StartupModule() {}

void FKDataForgeApiModule::ShutdownModule() {}

IMPLEMENT_MODULE(FKDataForgeApiModule, KDataForgeApi)
