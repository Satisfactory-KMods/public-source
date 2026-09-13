// Copyright Kyri123 / KMods 2026. All Rights Reserved.

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
