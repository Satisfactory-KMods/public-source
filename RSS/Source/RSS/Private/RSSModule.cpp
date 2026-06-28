#include "RSSModule.h"

#include "Buildable/RSSBuildableSign.h"
#include "Buildable/RSSSignRCO.h"
#include "FGGameMode.h"
#include "Hologram/FGPoleHologram.h"
#include "Patching/NativeHookManager.h"

// Define RSS Log Category
DEFINE_LOG_CATEGORY(LogRSS);

void FRSSModule::StartupModule() {}

IMPLEMENT_GAME_MODULE(FRSSModule, RSS);
