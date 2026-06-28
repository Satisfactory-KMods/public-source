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
	UE_LOGFMT(ResourceNodeSubsystem, Log,
			  "[KBFLResourceNodeSubsystem] Initialize: Editor detected, skipping execution");
#else
	if (GetWorld()->GetMapName().Contains("Untitled"))
	{
		UE_LOGFMT(ResourceNodeSubsystem, Log,
				  "[KBFLResourceNodeSubsystem] Initialize: Map name contains 'Untitled', skipping custom spawning");
		Super::Initialize(Collection);
		return;
	}


	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	if (AssetRegistry.IsLoadingAssets())
	{
		UE_LOGFMT(
			ResourceNodeSubsystem, Log,
			"[KBFLResourceNodeSubsystem] Initialize: Assets still loading, subscribing to OnFilesLoaded event...");
		AssetRegistry.OnFilesLoaded().AddUObject(this, &UKBFLResourceNodeSubsystem::SpawnSubLevel);
	}
	else
	{
		UE_LOGFMT(ResourceNodeSubsystem, Log,
				  "[KBFLResourceNodeSubsystem] Initialize: Assets ready, triggering SpawnSubLevel immediately");
		SpawnSubLevel();
	}

	UE_LOGFMT(ResourceNodeSubsystem, Log, "[KBFLResourceNodeSubsystem] Initialize Subsystem complete");

	mCalledDescriptors.Empty();
#endif

	Super::Initialize(Collection);
}

void UKBFLResourceNodeSubsystem::Deinitialize()
{
	UE_LOGFMT(ResourceNodeSubsystem, Log, "[KBFLResourceNodeSubsystem] Deinitializing Subsystem...");
	mCalledDescriptors.Empty();
	for (UKBFLSubLevelSpawning* SubLevelSpawning : mCalledSubLevelSpawning)
	{
		if (SubLevelSpawning)
		{
			SubLevelSpawning->Reset();
		}
	}
	mCalledSubLevelSpawning.Empty();
	Initialized = false;

	UE_LOGFMT(ResourceNodeSubsystem, Log, "[KBFLResourceNodeSubsystem] Deinitialized cleanly.");
	Super::Deinitialize();
}


void UKBFLResourceNodeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	UE_LOGFMT(ResourceNodeSubsystem, Log,
			  "[KBFLResourceNodeSubsystem] OnWorldBeginPlay called for world: {0}, Initialized: {1}", InWorld.GetName(),
			  Initialized);

	if (!Initialized)
	{
		UE_LOGFMT(ResourceNodeSubsystem, Log, "[KBFLResourceNodeSubsystem] Starting descriptor spawning process...");
		SpawnDescriptors();

		Initialized = true;
		UE_LOGFMT(ResourceNodeSubsystem, Log, "[KBFLResourceNodeSubsystem] Subsystem initialized successfully");
	}
	else
	{
		UE_LOGFMT(ResourceNodeSubsystem, Warning,
				  "[KBFLResourceNodeSubsystem] OnWorldBeginPlay called but subsystem already initialized");
	}

	UE_LOGFMT(ResourceNodeSubsystem, Log, "[KBFLResourceNodeSubsystem] Triggering Server_FinishedSpawningNodes");
	Server_FinishedSpawningNodes();
}

void UKBFLResourceNodeSubsystem::SpawnSubLevel()
{
	UE_LOGFMT(ResourceNodeSubsystem, Log,
			  "[KBFLResourceNodeSubsystem] SpawnSubLevel: Starting sub-level spawning search...");

	if (!GetWorld() || !GetWorld()->GetGameInstance())
	{
		UE_LOGFMT(ResourceNodeSubsystem, Error,
				  "[KBFLResourceNodeSubsystem] SpawnSubLevel: World or GameInstance is invalid, aborting");
		return;
	}

	TSet<UKBFLSubLevelSpawning*> SpawnerClasses;
	UKBFLAssetDataSubsystem* AssetSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());
	if (!AssetSubsystem)
	{
		UE_LOGFMT(ResourceNodeSubsystem, Error,
				  "[KBFLResourceNodeSubsystem] SpawnSubLevel: Failed to get AssetDataSubsystem");
		return;
	}

	AssetSubsystem->FindAllDataAssetsOfClass(SpawnerClasses);
	UE_LOGFMT(ResourceNodeSubsystem, Log,
			  "[KBFLResourceNodeSubsystem] SpawnSubLevel: Found {0} sub-level spawning asset(s)", SpawnerClasses.Num());

	for (UKBFLSubLevelSpawning* Spawner : SpawnerClasses)
	{
		if (IsValid(Spawner))
		{
			// Safe check for authority since GetAuthGameMode() can be null during early subsystem initialize phase.
			const bool bHasAuthority = !GetWorld()->IsNetMode(NM_Client) || (GetWorld()->GetAuthGameMode() != nullptr);
			if (!Spawner->mNeedAuth || bHasAuthority)
			{
				if (Spawner->ExecuteAllowed())
				{
					UE_LOGFMT(ResourceNodeSubsystem, Log,
							  "[KBFLResourceNodeSubsystem] SpawnSubLevel: InitSpawning called on '{0}'",
							  Spawner->GetName());
					Spawner->mSubsystem = this;
					Spawner->InitSpawning();
					mCalledSubLevelSpawning.Add(Spawner);
				}
				else
				{
					UE_LOGFMT(
						ResourceNodeSubsystem, Log,
						"[KBFLResourceNodeSubsystem] SpawnSubLevel: Spawner '{0}' execution not allowed, skipping",
						Spawner->GetName());
				}
			}
			else
			{
				UE_LOGFMT(ResourceNodeSubsystem, Log,
						  "[KBFLResourceNodeSubsystem] SpawnSubLevel: Skipping spawner '{0}' - requires authority but "
						  "not auth mode",
						  Spawner->GetName());
			}
		}
		else
		{
			UE_LOGFMT(ResourceNodeSubsystem, Warning,
					  "[KBFLResourceNodeSubsystem] SpawnSubLevel: Null spawner encountered, skipping");
		}
	}

	UE_LOGFMT(ResourceNodeSubsystem, Log,
			  "[KBFLResourceNodeSubsystem] SpawnSubLevel: Finished spawning sub-levels. Called count: {0}",
			  mCalledSubLevelSpawning.Num());
	Server_FinishedSpawningNodes();
}
void UKBFLResourceNodeSubsystem::SpawnDescriptors()
{
	UE_LOGFMT(ResourceNodeSubsystem, Log,
			  "[KBFLResourceNodeSubsystem] SpawnDescriptors: Starting descriptor spawning process");

	if (!GetWorld() || !GetWorld()->GetGameInstance())
	{
		UE_LOGFMT(ResourceNodeSubsystem, Error,
				  "[KBFLResourceNodeSubsystem] SpawnDescriptors: World or GameInstance is invalid, aborting");
		return;
	}

	TSet<UKBFLActorSpawnDescriptorBase*> SpawnerClasses;
	UKBFLAssetDataSubsystem* AssetSubsystem = UKBFLAssetDataSubsystem::Get(GetWorld());

	if (!AssetSubsystem)
	{
		UE_LOGFMT(ResourceNodeSubsystem, Error,
				  "[KBFLResourceNodeSubsystem] SpawnDescriptors: Failed to get AssetDataSubsystem");
		return;
	}

	AssetSubsystem->FindAllDataAssetsOfClass(SpawnerClasses);
	UE_LOGFMT(ResourceNodeSubsystem, Log,
			  "[KBFLResourceNodeSubsystem] SpawnDescriptors: Found {0} spawner descriptor(s)", SpawnerClasses.Num());

	int32 SpawnedCount = 0;
	int32 SkippedAuthCount = 0;

	for (UKBFLActorSpawnDescriptorBase* Spawner : SpawnerClasses)
	{
		if (!Spawner)
		{
			UE_LOGFMT(ResourceNodeSubsystem, Warning,
					  "[KBFLResourceNodeSubsystem] SpawnDescriptors: Null spawner encountered, skipping");
			continue;
		}

		const FString SpawnerName = Spawner->GetName();

		const bool bHasAuthority =
			(GetWorld()->GetNetMode() != NM_Client) || (GetWorld()->GetAuthGameMode() != nullptr);
		if (!Spawner->mNeedAuth || bHasAuthority)
		{
			UE_LOGFMT(ResourceNodeSubsystem, Log,
					  "[KBFLResourceNodeSubsystem] SpawnDescriptors: Processing spawner '{0}'", SpawnerName);
			Spawner->mSubsystem = this;
			Spawner->BeginSpawning();
			mCalledDescriptors.Add(Spawner);
			SpawnedCount++;
		}
		else
		{
			UE_LOGFMT(ResourceNodeSubsystem, Log,
					  "[KBFLResourceNodeSubsystem] SpawnDescriptors: Skipping spawner '{0}' - requires authority but "
					  "not auth mode",
					  SpawnerName);
			SkippedAuthCount++;
		}
	}

	UE_LOGFMT(ResourceNodeSubsystem, Log,
			  "[KBFLResourceNodeSubsystem] SpawnDescriptors: Completed - Spawned: {0}, Skipped (Auth): {1}, Total "
			  "Descriptors: {2}",
			  SpawnedCount, SkippedAuthCount, mCalledDescriptors.Num());

	Server_FinishedSpawningNodes();
}

void UKBFLResourceNodeSubsystem::Server_FinishedSpawningNodes()
{
	UE_LOGFMT(
		ResourceNodeSubsystem, Log,
		"[KBFLResourceNodeSubsystem] Server_FinishedSpawningNodes called. Rebuilding scanner and radar clusters...");

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Iterator->Get();
		if (PC)
		{
			AFGCharacterPlayer* Player = Cast<AFGCharacterPlayer>(PC->GetPawn());
			if (Player)
			{
				UE_LOGFMT(ResourceNodeSubsystem, Log,
						  "[KBFLResourceNodeSubsystem] Regenerating scanner cluster for player PC: {0}", PC->GetName());
				AFGResourceScanner* scanner = Player->GetResourceScanner();
				if (scanner)
				{
					scanner->mNodeClusters.Empty();
					scanner->GenerateNodeClusters();
				}
			}
		}
	}

	// Radar tower scanning is server-authoritative; skip on clients to avoid redundant work
	// on data that is replicated from the server anyway.
	if (GetWorld()->GetNetMode() != NM_Client)
	{
		int32 RadarTowerCount = 0;
		for (TActorIterator<AFGBuildableRadarTower> It(GetWorld()); It; ++It)
		{
			AFGBuildableRadarTower* tower = *It;
			if (!IsValid(tower))
			{
				continue;
			}
			UE_LOGFMT(ResourceNodeSubsystem, Log,
					  "[KBFLResourceNodeSubsystem] Rescanned resources for radar tower: {0}", tower->GetName());
			tower->ClearScannedResources();
			tower->ScanForResources();
			RadarTowerCount++;
		}

		UE_LOGFMT(ResourceNodeSubsystem, Log,
				  "[KBFLResourceNodeSubsystem] Finished rebuilding node clusters. Scanned radar towers count: {0}",
				  RadarTowerCount);
	}
}
