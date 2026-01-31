#include "KPrivateCodeLibModule.h"

#include "FGConstructDisqualifier.h"
#include "FGSaveSession.h"
#include "HttpModule.h"
#include "BlueprintFunctionLib/KPCLBlueprintFunctionLib.h"
#include "Equipment/FGHoverPack.h"
#include "Hologram/FGPowerPoleHologram.h"
#include "Hologram/FGPowerPoleWallHologram.h"
#include "Network/KPCLNetworkCable.h"
#include "Network/KPCLNetworkCableHologram.h"
#include "Network/KPCLNetworkConnectionComponent.h"
#include "Network/Buildings/KPCLNetworkPole.h"
#include "Network/Buildings/KPCLNetworkTower.h"
#include "Patching/NativeHookManager.h"
#include "Replication/KPCLDefaultRCO.h"
#include "Subsystem/KPCLUnlockSubsystem.h"

DEFINE_LOG_CATEGORY(LogKPCL);
DEFINE_LOG_CATEGORY(LogFaxit);

void PlayerStateBeginPlayer(CallScope<void(*)(AFGPlayerState*)>& scope, AFGPlayerState* State)
{
	if (State->GetWorld())
	{
		if (AKPCLUnlockSubsystem* Sub = AKPCLUnlockSubsystem::Get(State->GetWorld()))
		{
			Sub->RegisterPlayerState(State);
		}
	}
}

void AddHiddenConnection(TCallScope<void(*)(UFGCircuitConnectionComponent*, UFGCircuitConnectionComponent*)>& Scope,
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
	const TArray<FString> ModModuleNames = {
		"KBFL",
		"RSS",
		"AwesomeSinkStorage",
		"KDecoLib",
		"KLib",
		"KPrivateCodeLib",
		"KUI",
		"MicrowavePower",
		"PimpMyFactory",
		"SatisfactoryPlus",
		"PneumaticFrackingMachine"
	};

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
		SUBSCRIBE_METHOD_VIRTUAL(AFGPowerPoleHologram::CheckValidPlacement,
		                         GetMutableDefault<AFGPowerPoleHologram>(),
		                         [](auto& Scope, AFGPowerPoleHologram* This)
		                         {
		                         AKPCLNetworkPole* PoleUpgrade = Cast<AKPCLNetworkPole>(This->mUpgradeTarget);
		                         AKPCLNetworkCable* Cable = Cast<AKPCLNetworkCable>(This->mSnapWire);
		                         AKPCLNetworkPole* Pole = Cast<AKPCLNetworkPole>(This->GetBuildClass()->GetDefaultObject
			                         ());

		                         if (IsValid(This->mUpgradeTarget) || IsValid(This->mSnapWire)){
		                         if (IsValid(Pole))
		                         {
		                         if (!IsValid(Cable) && !IsValid(PoleUpgrade))
		                         {
		                         This->AddConstructDisqualifier(UFGCDInvalidAimLocation::StaticClass());
		                         }
		                         } else {
		                         if (IsValid(Cable) || IsValid(PoleUpgrade))
		                         {
		                         This->AddConstructDisqualifier(UFGCDInvalidAimLocation::StaticClass());
		                         }
		                         }
		                         }
		                         });

		SUBSCRIBE_METHOD_VIRTUAL(AFGPowerPoleWallHologram::CheckValidPlacement,
		                         GetMutableDefault<AFGPowerPoleWallHologram>(),
		                         [](auto& Scope, AFGPowerPoleWallHologram* This)
		                         {

		                         AKPCLNetworkPole* PoleUpgrade = Cast<AKPCLNetworkPole>(This->mUpgradeTarget);
		                         AKPCLNetworkCable* Cable = Cast<AKPCLNetworkCable>(This->mSnapWire);
		                         AKPCLNetworkPole* Pole = Cast<AKPCLNetworkPole>(This->GetBuildClass()->GetDefaultObject
			                         ());

		                         if (IsValid(This->mUpgradeTarget) || IsValid(This->mSnapWire)){
		                         if (IsValid(Pole))
		                         {
		                         if (!IsValid(Cable) && !IsValid(PoleUpgrade))
		                         {
		                         This->AddConstructDisqualifier(UFGCDInvalidAimLocation::StaticClass());
		                         }
		                         } else {
		                         if (IsValid(Cable) || IsValid(PoleUpgrade))
		                         {
		                         This->AddConstructDisqualifier(UFGCDInvalidAimLocation::StaticClass());
		                         }
		                         }
		                         }
		                         });


		SUBSCRIBE_METHOD_VIRTUAL(AFGWireHologram::TryUpgrade,
		                         GetMutableDefault<AFGWireHologram>(),
		                         [](auto& Scope, AFGWireHologram* This,const FHitResult& hitResult)
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

		SUBSCRIBE_METHOD_VIRTUAL(AFGPlayerState::BeginPlay, GetMutableDefault<AFGPlayerState>(),
		                         &PlayerStateBeginPlayer);
		SUBSCRIBE_METHOD(UFGSaveSession::SaveGame,
		                 [](auto &scope, UFGSaveSession* This, const FString& fileName, FOnSaveGameComplete
			                 completeDelegate, void* userData)
		                 {
		                 if (This && This->GetWorld())
		                 {
		                 for (FObjectReferenceDisc Ref : This->mUnresolvedWorldSaveData.DestroyedActors)
		                 {
		                 UE_LOG(LogKPCL, Log, TEXT("mUnresolvedWorldSaveData DestroyedActors: %s"), *Ref.ToString());
		                 }

		                 for (FObjectReferenceDisc Ref : This->mPersistentAndRuntimeData.DestroyedActors)
		                 {
		                 UE_LOG(LogKPCL, Log, TEXT("mPersistentAndRuntimeData DestroyedActors: %s"), *Ref.ToString());
		                 }
		                 }
		                 });
		SUBSCRIBE_METHOD(UFGSaveSession::LoadDestroyActors, [](auto &scope, UFGSaveSession* This, ULevel* level)
		                 {
		                 if (This && This->GetWorld())
		                 {
		                 for (auto& Elem : This->mPerLevelDataMap)
		                 {
		                 const FString& LevelName = Elem.Key;
		                 TUniquePtr<FPerStreamingLevelSaveData>& SaveDataPtr = Elem.Value;

		                 if (SaveDataPtr.IsValid())
		                 {
		                 if (LevelName == FString("Persistent_Level"))
		                 {
		                 SaveDataPtr->DestroyedActors = SaveDataPtr->DestroyedActors.FilterByPredicate([LevelName](const
				                 FObjectReferenceDisc& Elem)
			                 {
			                 if (Elem.ToString().Contains(FString("BP_ResourceNode")) || Elem.ToString().Contains(
				                 FString("Geyser")))
			                 {
			                 UE_LOG(LogKPCL, Log, TEXT("Removed DestroyedActors: %s in level %s"), *Elem.ToString(), *
				                 LevelName);
			                 return false;
			                 }
			                 return true;
			                 });
		                 }
		                 }
		                 }
		                 }
		                 });
	}
}

IMPLEMENT_GAME_MODULE(FKPrivateCodeLib, KPrivateCodeLib);
