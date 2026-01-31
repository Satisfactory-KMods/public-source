#include "KBFLModule.h"

void FKBFLModule::StartupModule()
{
#if !WITH_EDITOR
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
}

IMPLEMENT_GAME_MODULE(FKBFLModule, KBFL);
