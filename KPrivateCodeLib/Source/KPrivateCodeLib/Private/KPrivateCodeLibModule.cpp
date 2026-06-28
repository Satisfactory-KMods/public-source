#include "KPrivateCodeLibModule.h"

#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Equipment/FGHoverPack.h"
#include "FGCharacterPlayer.h"
#include "FGConstructDisqualifier.h"
#include "FGPlayerState.h"
#include "FGSaveSession.h"
#include "Hologram/FGPowerPoleHologram.h"
#include "Hologram/FGPowerPoleWallHologram.h"
#include "HttpModule.h"
#include "Network/Buildings/KPCLNetworkPole.h"
#include "Network/Buildings/KPCLNetworkTower.h"
#include "Network/KPCLNetworkCable.h"
#include "Network/KPCLNetworkCableHologram.h"
#include "Network/KPCLNetworkConnectionComponent.h"
#include "Patching/NativeHookManager.h"
#include "Replication/FGReplicationGraph.h"
#include "Replication/KPCLDefaultRCO.h"

DEFINE_LOG_CATEGORY(LogKPCL);
DEFINE_LOG_CATEGORY(LogFaxit);

struct FRepGraphConnState
{
	FString ViewerName;
	FString ViewTargetName;
	int32 BaseListNum = -1;
};
static TMap<UNetConnection*, FRepGraphConnState> GRepGraphLastState;

void AddHiddenConnection(TCallScope<void (*)(UFGCircuitConnectionComponent*, UFGCircuitConnectionComponent*)>& Scope,
						 UFGCircuitConnectionComponent* Component, UFGCircuitConnectionComponent* other)
{
	if (Component->GetCircuitType() != other->GetCircuitType())
	{
		UE_LOG(LogKPCL, Warning, TEXT("Trying to add hidden connection between different circuit types: %s and %s"),
			   *Component->GetCircuitType()->GetName(), *other->GetCircuitType()->GetName());
		Scope.Cancel();
	}
}

void FKPrivateCodeLib::StartupModule()
{
#if !WITH_EDITOR
	// Disable SSL certificate verification for HTTP requests
	// Set both settings to ensure compatibility across UE versions
	GConfig->SetBool(TEXT("HTTP"), TEXT("bEnableServerCertificateVerification"), false, GEngineIni);
	GConfig->SetBool(TEXT("HTTP"), TEXT("EnableServerCertificateVerification"), false, GEngineIni);
	GConfig->SetBool(TEXT("HTTP"), TEXT("bVerifyPeer"), false, GEngineIni);
	GConfig->SetBool(TEXT("HTTP"), TEXT("VerifyPeer"), false, GEngineIni);

	// Force the HTTP module to re-read its configuration
	if (FModuleManager::Get().IsModuleLoaded("HTTP"))
	{
		FHttpModule& HttpModule = FHttpModule::Get();
		// Toggling NullHttp forces config re-read in some UE versions
		HttpModule.ToggleNullHttp(true);
		HttpModule.ToggleNullHttp(false);
	}

	// Config
	const TArray<FString> ModModuleNames = {"KBFL",
											"RSS",
											"AwesomeSinkStorage",
											"KDecoLib",
											"KLib",
											"KPrivateCodeLib",
											"KUI",
											"MicrowavePower",
											"PimpMyFactory",
											"SatisfactoryPlus",
											"PneumaticFrackingMachine"};

	// Logic
	int32 Added = 0;

	TArray<FString> NewLocalizationPaths;
	GConfig->GetArray(TEXT("Internationalization"), TEXT("LocalizationPaths"), NewLocalizationPaths, GGameIni);

	for (FString ModModuleName : ModModuleNames)
	{
		FString LocPath = "../../../FactoryGame/Mods/{ModuleName}/Localization/{ModuleName}";
		LocPath.ReplaceInline(*FString("{ModuleName}"), *ModModuleName);
		if (NewLocalizationPaths.AddUnique(LocPath) > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("Module: %s ; Added LocalizationPath: %s"), *FString("PrivateCodeLib"), *LocPath);
			Added++;
		}
	}

	GConfig->SetArray(TEXT("Internationalization"), TEXT("LocalizationPaths"), NewLocalizationPaths, GGameIni);
	GConfig->SetArray(TEXT("Internationalization"), TEXT("LocalizationPaths"), NewLocalizationPaths, GEngineIni);
#endif

	if (!WITH_EDITOR)
	{
		SUBSCRIBE_METHOD_VIRTUAL(AFGPowerPoleHologram::CheckValidPlacement, GetMutableDefault<AFGPowerPoleHologram>(),
								 [](auto& Scope, AFGPowerPoleHologram* This)
								 {
									 AKPCLNetworkPole* PoleUpgrade = Cast<AKPCLNetworkPole>(This->mUpgradeTarget);
									 AKPCLNetworkCable* Cable = Cast<AKPCLNetworkCable>(This->mSnapWire);
									 AKPCLNetworkPole* Pole =
										 Cast<AKPCLNetworkPole>(This->GetBuildClass()->GetDefaultObject());

									 if (IsValid(This->mUpgradeTarget) || IsValid(This->mSnapWire))
									 {
										 if (IsValid(Pole))
										 {
											 if (!IsValid(Cable) && !IsValid(PoleUpgrade))
											 {
												 This->AddConstructDisqualifier(UFGCDInvalidAimLocation::StaticClass());
											 }
										 }
										 else
										 {
											 if (IsValid(Cable) || IsValid(PoleUpgrade))
											 {
												 This->AddConstructDisqualifier(UFGCDInvalidAimLocation::StaticClass());
											 }
										 }
									 }
								 });

		SUBSCRIBE_METHOD_VIRTUAL(
			AFGPowerPoleWallHologram::CheckValidPlacement, GetMutableDefault<AFGPowerPoleWallHologram>(),
			[](auto& Scope, AFGPowerPoleWallHologram* This)
			{
				AKPCLNetworkPole* PoleUpgrade = Cast<AKPCLNetworkPole>(This->mUpgradeTarget);
				AKPCLNetworkCable* Cable = Cast<AKPCLNetworkCable>(This->mSnapWire);
				AKPCLNetworkPole* Pole = Cast<AKPCLNetworkPole>(This->GetBuildClass()->GetDefaultObject());

				if (IsValid(This->mUpgradeTarget) || IsValid(This->mSnapWire))
				{
					if (IsValid(Pole))
					{
						if (!IsValid(Cable) && !IsValid(PoleUpgrade))
						{
							This->AddConstructDisqualifier(UFGCDInvalidAimLocation::StaticClass());
						}
					}
					else
					{
						if (IsValid(Cable) || IsValid(PoleUpgrade))
						{
							This->AddConstructDisqualifier(UFGCDInvalidAimLocation::StaticClass());
						}
					}
				}
			});

		SUBSCRIBE_METHOD_VIRTUAL(AFGWireHologram::TryUpgrade, GetMutableDefault<AFGWireHologram>(),
								 [](auto& Scope, AFGWireHologram* This, const FHitResult& hitResult)
								 {
									 AKPCLNetworkCableHologram* CableHologram = Cast<AKPCLNetworkCableHologram>(This);
									 bool IsFaxitCableHolo = IsValid(CableHologram);

									 AKPCLNetworkCable* FaxitCable = Cast<AKPCLNetworkCable>(hitResult.GetActor());
									 AFGBuildableWire* Cable = Cast<AFGBuildableWire>(hitResult.GetActor());

									 bool IsFaxitCable = IsValid(FaxitCable);
									 bool IsFGCable = IsValid(Cable);

									 if (IsFaxitCable || IsFGCable)
									 {
										 if (IsFaxitCableHolo && !IsFaxitCable)
										 {
											 This->AddConstructDisqualifier(UFGCDInvalidAimLocation::StaticClass());
											 Scope.Override(false);
											 return;
										 }

										 if (!IsFaxitCableHolo && IsFaxitCable)
										 {
											 This->AddConstructDisqualifier(UFGCDInvalidAimLocation::StaticClass());
											 Scope.Override(false);
											 return;
										 }
									 }
								 });

		SUBSCRIBE_METHOD(UFGCircuitConnectionComponent::AddHiddenConnection, &AddHiddenConnection);

		// Guard against the <FL> cross-platform player name crash (Satisfactory Experimental).
		// AFGCharacterPlayer::GetOnlinePlayerName() calls UOnlineIntegrationState::GetBackendByOnlineServiceType()
		// which asserts on the base-class stub when EOS is not configured (Steam-only listen server).
		// Return the standard UE player name instead, which is always available.
		// SUBSCRIBE_METHOD(AFGCharacterPlayer::GetOnlinePlayerName,
		// 				 [](auto& Scope, AFGCharacterPlayer* Self, const AFGPlayerState* PlayerState)
		// 				 { Scope.Override(IsValid(PlayerState) ? PlayerState->GetPlayerName() : FString()); });

		if (!IsRunningDedicatedServer())
		{
			return;
		}

		// Safety hook: guard GatherActorListsForConnection against SF's vanilla crash.
		//
		// Root cause (FGReplicationGraph.cpp:1195):
		//   UFGReplicationGraph::mAlwaysRelevantStreamingLevelActors is a non-UPROPERTY
		//   TMap<FName, FActorRepListRefView> that stores raw AActor* pointers with NO GC
		//   tracking. When streaming-level actors are garbage-collected their raw pointers
		//   become dangling. SF's GatherActorListsForConnection dereferences them → AV.
		//
		// Fix strategy:
		//   Before delegating to the original function we sanitize BOTH:
		//   Stale entries are removed in-place via RemoveAtSwap so the original code only
		//   ever sees valid actor pointers.
		SUBSCRIBE_METHOD_VIRTUAL(
			UFGReplicationGraphNode_AlwaysRelevant_ForConnection::GatherActorListsForConnection,
			GetMutableDefault<UFGReplicationGraphNode_AlwaysRelevant_ForConnection>(),
			[](auto& Scope, UFGReplicationGraphNode_AlwaysRelevant_ForConnection* Self,
			   const FConnectionGatherActorListParameters& Params)
			{
				UNetConnection* NetConn = Params.ConnectionManager.NetConnection;
				AActor* Viewer = Params.Viewers.Num() > 0 ? Params.Viewers[0].InViewer.Get() : nullptr;
				AActor* ViewTarget = Params.Viewers.Num() > 0 ? Params.Viewers[0].ViewTarget.Get() : nullptr;

				FString NewViewer = IsValid(Viewer) ? Viewer->GetName() : TEXT("NULL");
				FString NewViewTarget = IsValid(ViewTarget) ? ViewTarget->GetName() : TEXT("NULL");

				// --- 1. Sanitize ReplicationActorList (UE base-class, rebuilt each frame) ---
				// Iterate backwards so RemoveAtSwap doesn't skip entries.
				for (int32 i = Self->ReplicationActorList.Num() - 1; i >= 0; --i)
				{
					AActor* ListActor = Self->ReplicationActorList[i];
					if (!IsValid(ListActor))
					{
						UE_LOG(LogKPCL, Error,
							   TEXT("[RepGraph] STALE ACTOR in ReplicationActorList[%d] (ptr=%p)"
									" — removed. Self=%s NetConn=%s"),
							   i, ListActor, *GetNameSafe(Self), IsValid(NetConn) ? *NetConn->GetName() : TEXT("NULL"));
						Self->ReplicationActorList.RemoveAtSwap(i);
					}
				}

				// --- 2. Sanitize mAlwaysRelevantStreamingLevelActors (SF, non-UPROPERTY) ---
				// This is the actual source of the crash at FGReplicationGraph.cpp:1195.
				// Actors in streaming levels can be GC'd while SF still holds raw pointers
				// to them in this map. We remove stale entries before SF iterates the map.
				if (UWorld* HookWorld = Self->GetWorld())
				{
					if (UNetDriver* HookNetDriver = HookWorld->GetNetDriver())
					{
						if (UFGReplicationGraph* FGGraph =
								Cast<UFGReplicationGraph>(HookNetDriver->GetReplicationDriver()))
						{
							for (auto& Pair : FGGraph->mAlwaysRelevantStreamingLevelActors)
							{
								FActorRepListRefView& RepView = Pair.Value;
								for (int32 i = RepView.Num() - 1; i >= 0; --i)
								{
									AActor* StreamActor = RepView[i];
									if (!IsValid(StreamActor))
									{
										UE_LOG(LogKPCL, Error,
											   TEXT("[RepGraph] STALE STREAMING ACTOR in"
													" mAlwaysRelevantStreamingLevelActors"
													" level='%s'[%d] (ptr=%p) — removed"
													" to prevent crash"),
											   *Pair.Key.ToString(), i, StreamActor);
										RepView.RemoveAtSwap(i);
									}
								}
							}
						}
					}
				}

				// --- 3. Change-detection diagnostic log ---
				int32 NewListNum = Self->ReplicationActorList.Num();
				bool bChanged;
				if (FRepGraphConnState* Prev = GRepGraphLastState.Find(NetConn))
				{
					bChanged = Prev->ViewerName != NewViewer || Prev->ViewTargetName != NewViewTarget ||
						Prev->BaseListNum != NewListNum;
					if (bChanged)
					{
						Prev->ViewerName = NewViewer;
						Prev->ViewTargetName = NewViewTarget;
						Prev->BaseListNum = NewListNum;
					}
				}
				else
				{
					GRepGraphLastState.Add(NetConn, {NewViewer, NewViewTarget, NewListNum});
					bChanged = true;
				}

				if (bChanged)
				{
					// UE_LOG(LogKPCL, Warning,
					//		TEXT("[RepGraph Debug] GatherActorListsForConnection:"
					//		     " Self=%s NetConn=%s ViewerCount=%d Viewer=%s(%s)"
					//		     " ViewTarget=%s(%s) BaseListNum=%d"),
					//		*GetNameSafe(Self),
					//		IsValid(NetConn) ? *NetConn->GetName() : TEXT("NULL"),
					//		Params.Viewers.Num(),
					//		*NewViewer,
					//		IsValid(Viewer) ? *Viewer->GetClass()->GetName() : TEXT("-"),
					//		*NewViewTarget,
					//		IsValid(ViewTarget) ? *ViewTarget->GetClass()->GetName() : TEXT("-"),
					//		NewListNum);
				}

				// Lists are now sanitized — let the original function run normally.
			});
	}
}

IMPLEMENT_GAME_MODULE(FKPrivateCodeLib, KPrivateCodeLib);
