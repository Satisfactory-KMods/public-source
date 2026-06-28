#include "KLibModule.h"

#include <Modules/ModuleManager.h>

#include "Equipment/FGHoverPack.h"
#include "FGGameMode.h"
#include "Network/KPCLNetworkConnectionComponent.h"
#include "Patching/NativeHookManager.h"
#include "Replication/KLDefaultRCO.h"

void FKLibModule::StartupModule()
{
	/*#if !WITH_EDITOR
	// Fix a crash where CSS try to connect to the Network Circuit but there are different classes!
	SUBSCRIBE_METHOD( UFGCircuitConnectionComponent::AddHiddenConnection, [&](
	CallScope<void(*)(UFGCircuitConnectionComponent*, UFGCircuitConnectionComponent*)>& Scope,
	UFGCircuitConnectionComponent* Self, UFGCircuitConnectionComponent* other )
	{
		if( IsValid( Self ) && IsValid( other ) )
		{
			if( Self->GetCircuitType() != other->GetCircuitType() )
			{
				Scope.Cancel();
			}
		}
	} );
	#endif*/
}

IMPLEMENT_GAME_MODULE(FKLibModule, KLib);
