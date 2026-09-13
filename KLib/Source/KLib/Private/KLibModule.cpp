// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "KLibModule.h"

#include <Modules/ModuleManager.h>

#include "Equipment/FGHoverPack.h"
#include "FGGameMode.h"
#include "Network/KPCLNetworkConnectionComponent.h"
#include "Patching/NativeHookManager.h"
#include "Replication/KLDefaultRCO.h"
#include "Logging.h"

void FKLibModule::StartupModule()
{

}

IMPLEMENT_GAME_MODULE(FKLibModule, KLib);
