// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "RSSModule.h"

#include "Buildable/RSSBuildableSign.h"
#include "Buildable/RSSSignRCO.h"
#include "FGGameMode.h"
#include "Hologram/FGPoleHologram.h"
#include "Patching/NativeHookManager.h"

DEFINE_LOG_CATEGORY(LogRSS);

void FRSSModule::StartupModule() {}

IMPLEMENT_GAME_MODULE(FRSSModule, RSS);
