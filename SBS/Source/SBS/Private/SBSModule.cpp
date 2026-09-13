// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "SBSModule.h"

#include "BlueprintFunctionLib/SBSWidgetCompatibility.h"
#include "GameFramework/InputSettings.h"
#include "InputCoreTypes.h"
#include "Modules/ModuleManager.h"
#include "UObject/CoreRedirects.h"

void FSBSModule::StartupModule()
{

	TArray<FCoreRedirect> Redirects;
	Redirects.Emplace(ECoreRedirectFlags::Type_Class, TEXT("/Script/KBFL.KBFL_Widgets"),
					  TEXT("/Script/SBS.SBSWidgetCompatibility"));
	Redirects.Emplace(ECoreRedirectFlags::Type_Function, TEXT("/Script/KBFL.OnWidgetCreated__DelegateSignature"),
					  TEXT("/Script/SBS.OnWidgetCreated__DelegateSignature"));
	FCoreRedirects::AddRedirectList(Redirects, TEXT("SBS legacy widget compatibility"));

	UInputSettings* Settings = GetMutableDefault<UInputSettings>();
	TArray<FInputActionKeyMapping> Existing;
	Settings->GetActionMappingByName(TEXT("SBS.OpenBlueprint"), Existing);
	if (Existing.IsEmpty())
	{
		Settings->AddActionMapping(FInputActionKeyMapping(TEXT("SBS.OpenBlueprint"), EKeys::B, false, true), false);
	}
}

void FSBSModule::ShutdownModule() { USBSWidgetCompatibility::ShutdownHooks(); }

IMPLEMENT_GAME_MODULE(FSBSModule, SBS);
