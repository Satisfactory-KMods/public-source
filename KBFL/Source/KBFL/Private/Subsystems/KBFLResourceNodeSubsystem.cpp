#include "Subsystems/KBFLResourceNodeSubsystem.h"

#include "Buildables/FGBuildableRadarTower.h"
#include "EngineUtils.h"
#include "Equipment/FGResourceScanner.h"
#include "FGGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Logging/StructuredLog.h"
#include "Module/WorldModuleManager.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Subsystems/KBFLAssetDataSubsystem.h"
#include "Subsystems/ResourceNodes/KBFLSubLevelSpawning.h"

DECLARE_LOG_CATEGORY_EXTERN(ResourceNodeSubsystem, Log, All)

DEFINE_LOG_CATEGORY(ResourceNodeSubsystem)

void UKBFLResourceNodeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency(USubsystemActorManager::StaticClass());

#if WITH_EDITOR
	return;
#endif

	if (GetWorld()->GetMapName().Contains("Untitled"))
	{
		Super::Initialize(Collection);
		return;
	}


	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	if (AssetRegistry.IsLoadingAssets())
	{
		AssetRegistry.OnFilesLoaded().AddUObject(this, &UKBFLResourceNodeSubsystem::SpawnSubLevel);
	}
	else
	{
		SpawnSubLevel();
	}

	UE_LOG(ResourceNodeSubsystem, Log, TEXT("Initialize Subsystem"));

	mCalledDescriptors.Empty();

	Super::Initialize(Collection);
}

void UKBFLResourceNodeSubsystem::Deinitialize()
{
	mCalledDescriptors.Empty();
	for (UKBFLSubLevelSpawning* SubLevelSpawning : mCalledSubLevelSpawning)
	{
		SubLevelSpawning->Reset();
	}
	mCalledSubLevelSpawning.Empty();
	Initialized = false;

	Super::Deinitialize();
}


void UKBFLResourceNodeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	UE_LOGFMT(ResourceNodeSubsystem, Log, "OnWorldBeginPlay called for world: {0}, Initialized: {1}", InWorld.GetName(),
			  Initialized);

	if (!Initialized)
	{
		UE_LOGFMT(ResourceNodeSubsystem, Log, "Starting descriptor spawning process...");
		SpawnDescriptors();

		Initialized = true;
		UE_LOGFMT(ResourceNodeSubsystem, Log, "Subsystem initialized successfully");
	}
	else
	{
		UE_LOGFMT(ResourceNodeSubsystem, Warning, "OnWorldBeginPlay called but subsystem already initialized");
	}

	UE_LOGFMT(ResourceNodeSubsystem, Log, "Triggering Server_FinishedSpawningNodes");
	Server_FinishedSpawningNodes();
}

void UKBFLResourceNodeSubsystem::SpawnSubLevel()
{
	if (!GetWorld() || !GetWorld()->GetGameInstance())
	{
		return;
	}

	TSet<UKBFLSubLevelSpawning*> SpawnerClasses;
	UKBFLAssetDataSubsystem* AssetSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());
	AssetSubsystem->FindAllDataAssetsOfClass(SpawnerClasses);

	for (UKBFLSubLevelSpawning* Spawner : SpawnerClasses)
	{
		if (!Spawner->mNeedAuth || GetWorld()->GetAuthGameMode())
		{
			if (Spawner->ExecuteAllowed())
			{
				Spawner->mSubsystem = this;
				Spawner->InitSpawning();
				mCalledSubLevelSpawning.Add(Spawner);
			}
		}
	}

	Server_FinishedSpawningNodes();
}
void UKBFLResourceNodeSubsystem::SpawnDescriptors()
{
	UE_LOGFMT(ResourceNodeSubsystem, Log, "SpawnDescriptors: Starting descriptor spawning process");

	if (!GetWorld() || !GetWorld()->GetGameInstance())
	{
		UE_LOGFMT(ResourceNodeSubsystem, Error, "SpawnDescriptors: World or GameInstance is invalid, aborting");
		return;
	}

	TSet<UKBFLActorSpawnDescriptorBase*> SpawnerClasses;
	UKBFLAssetDataSubsystem* AssetSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

	if (!AssetSubsystem)
	{
		UE_LOGFMT(ResourceNodeSubsystem, Error, "SpawnDescriptors: Failed to get AssetDataSubsystem");
		return;
	}

	AssetSubsystem->FindAllDataAssetsOfClass(SpawnerClasses);
	UE_LOGFMT(ResourceNodeSubsystem, Log, "SpawnDescriptors: Found {0} spawner descriptor(s)", SpawnerClasses.Num());

	int32 SpawnedCount = 0;
	int32 SkippedAuthCount = 0;

	for (UKBFLActorSpawnDescriptorBase* Spawner : SpawnerClasses)
	{
		if (!Spawner)
		{
			UE_LOGFMT(ResourceNodeSubsystem, Warning, "SpawnDescriptors: Null spawner encountered, skipping");
			continue;
		}

		const FString SpawnerName = Spawner->GetName();

		if (!Spawner->mNeedAuth || GetWorld()->GetAuthGameMode())
		{
			UE_LOGFMT(ResourceNodeSubsystem, Log, "SpawnDescriptors: Processing spawner '{0}'", SpawnerName);
			Spawner->mSubsystem = this;
			Spawner->BeginSpawning();
			mCalledDescriptors.Add(Spawner);
			SpawnedCount++;
		}
		else
		{
			UE_LOGFMT(ResourceNodeSubsystem, Log,
					  "SpawnDescriptors: Skipping spawner '{0}' - requires authority but not auth mode", SpawnerName);
			SkippedAuthCount++;
		}
	}

	UE_LOGFMT(ResourceNodeSubsystem, Log,
			  "SpawnDescriptors: Completed - Spawned: {0}, Skipped (Auth): {1}, Total Descriptors: {2}", SpawnedCount,
			  SkippedAuthCount, mCalledDescriptors.Num());

	Server_FinishedSpawningNodes();
}

void UKBFLResourceNodeSubsystem::Server_FinishedSpawningNodes_Implementation()
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Iterator->Get();
		if (PC)
		{
			AFGCharacterPlayer* Player = Cast<AFGCharacterPlayer>(PC->GetPawn());
			if (Player)
			{
				AFGResourceScanner* scanner = Player->GetResourceScanner();
				scanner->mNodeClusters.Empty();
				scanner->GenerateNodeClusters();
			}
		}
	}

	for (TActorIterator<AFGBuildableRadarTower> It(GetWorld()); It; ++It)
	{
		AFGBuildableRadarTower* tower = *It;
		if (!IsValid(tower))
		{
			continue;
		}
		tower->ClearScannedResources();
		tower->ScanForResources();
	}
}

bool UKBFLResourceNodeSubsystem::Server_FinishedSpawningNodes_Validate() { return true; }
