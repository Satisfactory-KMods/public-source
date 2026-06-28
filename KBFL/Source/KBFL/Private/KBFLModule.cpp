#include "KBFLModule.h"

#if !WITH_EDITOR
#include "Engine/NetDriver.h"
#include "Patching/NativeHookManager.h"
#include "Replication/FGReplicationGraph.h"
#endif

#if WITH_EDITOR
#include "ContentBrowserMenuContexts.h"
#include "Engine/Blueprint.h"
#include "Framework/Commands/UIAction.h"
#include "KBFLGameInstanceModule.h"
#include "KBFLWorldModule.h"
#include "ToolMenus.h"
#endif

void FKBFLModule::StartupModule()
{
#if !WITH_EDITOR
	// Filter null/unresolvable soft-class entries before they enter CustomClassRepPolicies.
	// Accessing the map directly is impossible (symbol not exported by FactoryGame.dll), so we
	// intercept at registration time instead: any entry that LoadSynchronous() can't resolve is dropped.
	SUBSCRIBE_METHOD(UFGReplicationGraph::RegisterCustomClassRepPolicy,
					 [](auto& scope, TSoftClassPtr<AActor> InActor, EClassRepPolicy InRepPolicy)
					 {
						 if (!InActor.IsNull() && InActor.LoadSynchronous() != nullptr)
						 {
							 scope(InActor, InRepPolicy);
						 }
					 });

	SUBSCRIBE_METHOD_VIRTUAL(UFGReplicationGraphNode_AlwaysRelevant_ForConnection::GatherActorListsForConnection,
							 GetMutableDefault<UFGReplicationGraphNode_AlwaysRelevant_ForConnection>(),
							 [](auto& scope, UFGReplicationGraphNode_AlwaysRelevant_ForConnection* self,
								const FConnectionGatherActorListParameters& Params)
							 {
								 UWorld* World = self->GetWorld();
								 UNetDriver* NetDriver = World ? World->GetNetDriver() : nullptr;
								 if (!NetDriver || !NetDriver->GetReplicationDriver<UFGReplicationGraph>())
								 {
									 return;
								 }
								 scope(self, Params);
							 });
/*
//SUBSCRIBE_METHOD_VIRTUAL_AFTER( AFGBuildableWire::BeginPlay, GetMutableDefault<AFGBuildableWire>(),
[&](AFGBuildableWire* Wire) { if(ensure(Wire)) { FTimerHandle TimerHandle;
Wire->GetWorldTimerManager().SetTimer(TimerHandle, Wire, &AFGBuildableWire::UpdateWireMesh, 1.f, false); } } )

//SUBSCRIBE_METHOD( AFGBuildableWire::UpdateWireMesh, [&](auto& Call, AFGBuildableWire* Wire) { if(ensure(Wire) &&
ensure(Wire->mWireMesh)) { Call(Wire); Wire->mWireMesh->SetCustomPrimitiveDataFloat(0, Wire->GetLength() / 100);
Wire->mWireMesh->SetCustomPrimitiveDataFloat(1, 0.0f); Wire->mWireMesh->SetCustomPrimitiveDataFloat(2, 0.0f);
Wire->mWireMesh->SetCustomPrimitiveDataFloat(3, -48.f); Wire->mWireMesh->SetCustomPrimitiveDataFloat(4, 0.0f);
Wire->mWireMesh->SetCustomPrimitiveDataFloat(5, 0.703125); Wire->mWireMesh->SetCustomPrimitiveDataFloat(5, 0.0f);
Wire->mWireMesh->SetCustomPrimitiveDataFloat(6, 0.0f); Wire->mWireMesh->SetCustomPrimitiveDataFloat(7, 0.0f);
Wire->mWireMesh->SetCustomPrimitiveDataFloat(8, 0.0f); Wire->mWireMesh->SetCustomPrimitiveDataFloat(9, 0.0f);
Wire->mWireMesh->SetCustomPrimitiveDataFloat(10, 0.0f); } } )

//SUBSCRIBE_METHOD( AFGGameState::GetBuildingColorDataForSlot, &GetBuildingColorDataForSlot );

SUBSCRIBE_METHOD( UWorldModuleManager::DispatchLifecycleEvent, [](auto& Call, UWorldModuleManager* Manager,
ELifecyclePhase Phase) { if(Manager->RootModuleList.Num() > 0) { UKBFLContentCDOHelperSubsystem* CDOHelperSubsystem =
Manager->GetWorld()->GetGameInstance()->GetSubsystem<UKBFLContentCDOHelperSubsystem>(); UKBFLCustomizerSubsystem*
CustomizerSubsystem = Manager->GetWorld()->GetSubsystem<UKBFLCustomizerSubsystem>(); UKBFLResourceNodeSubsystem*
NodeSubsystem = Manager->GetWorld()->GetSubsystem<UKBFLResourceNodeSubsystem>();

			  // going save that it called not in the registry phase to avoid crashed while CDO!
			  for (UWorldModule* RootModule : Manager->RootModuleList) {
			  if(UKBFLWorldModule* WorldModule = Cast<UKBFLWorldModule>(RootModule)) {
if(!WorldModule->bScanForCDOsDone) { WorldModule->FindAllCDOs(); } if(CDOHelperSubsystem)
CDOHelperSubsystem->BeginCDOForModule(RootModule, Phase); if(CustomizerSubsystem && WorldModule->mCallCustomizerInPhase
== Phase) { UE_LOG(LogKBFLModule, Log, TEXT("Starting CustomizerSubsystem for Mod: %s"),
*WorldModule->GetOwnerModReference().ToString()); CustomizerSubsystem->BeginForModule(RootModule); } if(NodeSubsystem &&
WorldModule->mCallNodesInPhase == Phase) { UE_LOG(LogKBFLModule, Log, TEXT("Starting NodeSubsystem for Mod: %s"),
*WorldModule->GetOwnerModReference().ToString()); NodeSubsystem->BeginSpawningForModule(RootModule); } } } for
(UWorldModule* RootModule : Manager->RootModuleList) { if(ensureAlways(RootModule)) { UE_LOG(LogKBFLModule, Log,
TEXT("Dispatching lifecycle event %s to Mod %s"), *UModModule::LifecyclePhaseToString(Phase),
*RootModule->GetOwnerModReference().ToString()); RootModule->DispatchLifecycleEvent(Phase); } } Call.Cancel(); } } );
*/
#endif

#if WITH_EDITOR
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateStatic(&FKBFLModule::RegisterContentBrowserMenu));
#endif
}

void FKBFLModule::ShutdownModule()
{
#if WITH_EDITOR
	if (UObjectInitialized())
	{
		UToolMenus::UnregisterOwner(FName("KBFLModuleScan"));
	}
#endif
}

#if WITH_EDITOR
#define LOCTEXT_NAMESPACE "KBFLModuleScan"

void FKBFLModule::RegisterContentBrowserMenu()
{
	FToolMenuOwnerScoped OwnerScoped(FName("KBFLModuleScan"));

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu");
	if (!Menu)
	{
		return;
	}

	FToolMenuSection& Section = Menu->FindOrAddSection("KMods", LOCTEXT("KModsSection", "KMods"));

	Section.AddDynamicEntry(
		"KBFLModuleScan",
		FNewToolMenuSectionDelegate::CreateLambda(
			[](FToolMenuSection& InSection)
			{
				const UContentBrowserAssetContextMenuContext* Ctx =
					InSection.Context.FindContext<UContentBrowserAssetContextMenuContext>();
				if (!Ctx)
				{
					return;
				}

				// Only show when at least one selected asset is a Blueprint (module assets are blueprints).
				bool bHasBlueprint = false;
				for (const FAssetData& Asset : Ctx->SelectedAssets)
				{
					if (Asset.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName())
					{
						bHasBlueprint = true;
						break;
					}
				}
				if (!bHasBlueprint)
				{
					return;
				}

				InSection.AddMenuEntry(
					"KBFLScanWorldModule", LOCTEXT("KBFLScanWorld", "Scan World Module Content"),
					LOCTEXT("KBFLScanWorldTip",
							"Fill schematics / research trees / chat commands on selected world module blueprints"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda(
						[Ctx]()
						{
							for (UBlueprint* BP : Ctx->LoadSelectedObjects<UBlueprint>())
							{
								UClass* Cls = BP ? BP->GeneratedClass.Get() : nullptr;
								if (UKBFLWorldModule* Module =
										Cls ? Cast<UKBFLWorldModule>(Cls->GetDefaultObject()) : nullptr)
								{
									Module->ScanSchematics();
									Module->ScanResearchTrees();
									Module->ScanChatCommands();
								}
							}
						})));

				InSection.AddMenuEntry(
					"KBFLScanInstanceModule", LOCTEXT("KBFLScanInstance", "Scan Instance Module Content"),
					LOCTEXT("KBFLScanInstanceTip",
							"Fill mod configs / RCOs / session settings on selected instance module blueprints"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda(
						[Ctx]()
						{
							for (UBlueprint* BP : Ctx->LoadSelectedObjects<UBlueprint>())
							{
								UClass* Cls = BP ? BP->GeneratedClass.Get() : nullptr;
								if (UKBFLGameInstanceModule* Module =
										Cls ? Cast<UKBFLGameInstanceModule>(Cls->GetDefaultObject()) : nullptr)
								{
									Module->ScanModConfigurations();
									Module->ScanRemoteCallObjects();
									Module->ScanSessionSettings();
								}
							}
						})));
			}));
}

#undef LOCTEXT_NAMESPACE
#endif

IMPLEMENT_GAME_MODULE(FKBFLModule, KBFL);
