#include "KPrivateCodeLibModule.h"

#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Equipment/FGHoverPack.h"
#include "FGCharacterPlayer.h"
#include "FGConstructDisqualifier.h"
#include "FGGameRulesSubsystem.h"
#include "FGPlayerState.h"
#include "FGSaveSession.h"
#include "FGSchematic.h"
#include "FGSchematicManager.h"
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
#include "Subsystem/KPCLDeliveryTaskSubsystem.h"
#include "Subsystem/KPCLUnlockSubsystem.h"

DEFINE_LOG_CATEGORY(LogKPCL);
DEFINE_LOG_CATEGORY(LogFaxit);

namespace
{
	bool IsRepGraphActorUsable(const AActor* Actor)
	{
		if (Actor == nullptr)
		{
			return false;
		}

		const UPTRINT ActorAddress = reinterpret_cast<UPTRINT>(Actor);
		if (ActorAddress < 0x100 || (ActorAddress & (MIN_ALIGNMENT - 1)) != 0)
		{
			return false;
		}

		if (!Actor->IsValidLowLevel())
		{
			return false;
		}

		return IsValid(Actor) && !Actor->IsUnreachable() && !Actor->IsActorBeingDestroyed();
	}
}

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

	GConfig->SetBool(TEXT("HTTP"), TEXT("bEnableServerCertificateVerification"), false, GEngineIni);
	GConfig->SetBool(TEXT("HTTP"), TEXT("EnableServerCertificateVerification"), false, GEngineIni);
	GConfig->SetBool(TEXT("HTTP"), TEXT("bVerifyPeer"), false, GEngineIni);
	GConfig->SetBool(TEXT("HTTP"), TEXT("VerifyPeer"), false, GEngineIni);

	if (FModuleManager::Get().IsModuleLoaded("HTTP"))
	{
		FHttpModule& HttpModule = FHttpModule::Get();

		HttpModule.ToggleNullHttp(true);
		HttpModule.ToggleNullHttp(false);
	}

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

#if !WITH_EDITOR
	SUBSCRIBE_METHOD(UFGSchematic::GetCost,
					 [](auto& Scope, TSubclassOf<UFGSchematic> SchematicClass)
					 {
						 const TArray<FItemAmount> VanillaCost = Scope(SchematicClass);
						 if (AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::GetFromCurrentWorld())
						 {
							 Scope.Override(UnlockSubsystem->GetScaledSchematicCost(SchematicClass, VanillaCost));
						 }
					 });

	SUBSCRIBE_METHOD(UFGSchematic::CanGiveAccessToSchematic,
					 [](auto& Scope, TSubclassOf<UFGSchematic> SchematicClass, UObject* WorldContext)
					 {
						 if (AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(WorldContext);
							 IsValid(UnlockSubsystem) && !UnlockSubsystem->CanPurchaseRepeatSchematic(SchematicClass))
						 {
							 Scope.Override(false);
						 }
					 });

	SUBSCRIBE_METHOD_AFTER(AFGGameRulesSubsystem::SetNoUnlockCost,
						   [](AFGGameRulesSubsystem* GameRules, bool bEnabled)
						   {
							   if (!bEnabled || !IsValid(GameRules))
							   {
								   return;
							   }
							   if (AKPCLDeliveryTaskSubsystem* DeliverySubsystem =
									   AKPCLDeliveryTaskSubsystem::Get(GameRules))
							   {
								   DeliverySubsystem->ScheduleFreeUnlockScan();
							   }
						   });

	SUBSCRIBE_METHOD(
		AFGSchematicManager::CanGiveAccessToSchematic,
		[](auto& Scope, const AFGSchematicManager* SchematicManager, TSubclassOf<UFGSchematic> SchematicClass)
		{
			if (AKPCLUnlockSubsystem* UnlockSubsystem = AKPCLUnlockSubsystem::Get(SchematicManager->GetWorld());
				IsValid(UnlockSubsystem) && !UnlockSubsystem->CanPurchaseRepeatSchematic(SchematicClass))
			{
				Scope.Override(false);
			}
		});
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

				for (int32 i = Self->ReplicationActorList.Num() - 1; i >= 0; --i)
				{
					AActor* ListActor = Self->ReplicationActorList[i];
					if (!IsRepGraphActorUsable(ListActor))
					{
						UE_LOG(LogKPCL, Error,
							   TEXT("[RepGraph] STALE ACTOR in ReplicationActorList[%d] (ptr=%p)"
									" — removed. Self=%s NetConn=%s"),
							   i, ListActor, *GetNameSafe(Self), IsValid(NetConn) ? *NetConn->GetName() : TEXT("NULL"));
						Self->ReplicationActorList.RemoveAtSwap(i);
					}
				}

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
									if (!IsRepGraphActorUsable(StreamActor))
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

				}

			});

		UE_LOG(LogKPCL, Display, TEXT("[RepGraph] Selective stale-actor sanitizer registered"));
	}
}

IMPLEMENT_GAME_MODULE(FKPrivateCodeLib, KPrivateCodeLib);
