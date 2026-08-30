#include "Subsystems/KDFResourceNodeSubsystem.h"

#include "Buildables/FGBuildableRadarTower.h"
#include "Buildables/FGBuildableResourceExtractorBase.h"
#include "Buildables/FGBuildableWaterPump.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Equipment/FGResourceScanner.h"
#include "FGActorRepresentationManager.h"
#include "FGCharacterPlayer.h"
#include "FGSchematicManager.h"
#include "FGUnlockSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "KDFLogging.h"
#include "KDFNode.h"
#include "Reflection/KDFOpEngine.h"
#include "Reflection/KDFPropertyPath.h"
#include "Representation/FGResourceNodeRepresentation.h"
#include "Resources/FGResourceNodeBase.h"
#include "Resources/FGResourceNodeFrackingCore.h"
#include "Resources/FGResourceNodeFrackingSatellite.h"
#include "Subsystems/KDFSubsystem.h"
#include "TimerManager.h"
#include "UObject/UObjectHash.h"
#include "Unlocks/FGUnlockScannableResource.h"

namespace
{

	constexpr float MeshActorMatchToleranceCm = 100.0f;

	void ApplyClassOpOnContainer(UObject* Owner, const TCHAR* PropertyName, const FString& ClassPath, EKDFOp Op)
	{
		FKDFPropertyPath Path;
		FString Error;
		if (!IsValid(Owner) || !FKDFPropertyPath::Parse(PropertyName, Path, Error))
		{
			return;
		}
		const TSharedRef<FKDFNode> Value = FKDFNode::MakeScalar(ClassPath);
		FKDFOpArgs Args;
		Args.mValue = &Value.Get();
		FKDFOpEngine::ApplyOp(Owner, Path, Op, Args, Error);
	}
}

bool UKDFResourceNodeSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UKDFResourceNodeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UWorld* World = GetWorld(); World != nullptr)
	{
		mSpawnHandle = World->AddOnActorSpawnedHandler(
			FOnActorSpawned::FDelegate::CreateUObject(this, &UKDFResourceNodeSubsystem::OnActorSpawned));

		mDestroyHandle = World->AddOnActorDestroyedHandler(
			FOnActorDestroyed::FDelegate::CreateUObject(this, &UKDFResourceNodeSubsystem::OnActorDestroyed));
	}

	mActorsInitializedHandle = FWorldDelegates::OnWorldInitializedActors.AddUObject(
		this, &UKDFResourceNodeSubsystem::OnWorldActorsInitialized);
	mLevelAddedHandle =
		FWorldDelegates::LevelAddedToWorld.AddUObject(this, &UKDFResourceNodeSubsystem::OnLevelAddedToWorld);
}

void UKDFResourceNodeSubsystem::OnWorldActorsInitialized(const UWorld::FActorsInitializedParams& Params)
{

	if (Params.World == nullptr || Params.World != GetWorld())
	{
		return;
	}
	SweepWorld(*Params.World);
}

void UKDFResourceNodeSubsystem::OnLevelAddedToWorld(ULevel* Level, UWorld* World)
{

	if (Level == nullptr || World == nullptr || World != GetWorld())
	{
		return;
	}
	const UKDFSubsystem* Subsystem = UKDFSubsystem::Get(World);
	if (Subsystem == nullptr || Subsystem->GetNodePurges().IsEmpty())
	{
		return;
	}

	BuildSatelliteIndex(*World);
	BuildOccupiedNodeIndex(*World);
	BuildMeshIndex(*World);
	for (AActor* Actor : Level->Actors)
	{
		if (AFGResourceNodeBase* Node = Cast<AFGResourceNodeBase>(Actor); Node != nullptr)
		{
			TryPurgeNode(Node);
		}
	}
}

void UKDFResourceNodeSubsystem::SweepWorld(UWorld& World)
{
	const UKDFSubsystem* Subsystem = UKDFSubsystem::Get(&World);
	if (Subsystem == nullptr || Subsystem->GetNodePurges().IsEmpty())
	{
		return;
	}

	BuildSatelliteIndex(World);
	BuildOccupiedNodeIndex(World);
	BuildMeshIndex(World);

	int32 DestroyedCount = 0;
	for (TActorIterator<AFGResourceNodeBase> It(&World); It; ++It)
	{
		DestroyedCount += TryPurgeNode(*It) ? 1 : 0;
	}
	if (DestroyedCount > 0)
	{
		UE_LOG(LogKDataForge, Log, TEXT("Resource node purge: destroyed %d level-placed node(s)"), DestroyedCount);
	}
}

void UKDFResourceNodeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const UKDFSubsystem* Subsystem = UKDFSubsystem::Get(&InWorld);
	if (Subsystem == nullptr || Subsystem->GetNodePurges().IsEmpty())
	{
		return;
	}

	InWorld.GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &UKDFResourceNodeSubsystem::SweepDeferred));

	if (AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(&InWorld); SchematicManager != nullptr)
	{
		SchematicManager->PurchasedSchematicDelegate.AddDynamic(this, &UKDFResourceNodeSubsystem::OnSchematicPurchased);
	}
}

void UKDFResourceNodeSubsystem::SweepDeferred()
{

	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown)
	{
		return;
	}
	SweepWorld(*World);

	PurgeScannerResources();

	RefreshScannersAndRadarTowers();
}

void UKDFResourceNodeSubsystem::Deinitialize()
{

	FWorldDelegates::OnWorldInitializedActors.Remove(mActorsInitializedHandle);
	mActorsInitializedHandle.Reset();
	FWorldDelegates::LevelAddedToWorld.Remove(mLevelAddedHandle);
	mLevelAddedHandle.Reset();

	if (UWorld* World = GetWorld(); World != nullptr)
	{
		if (mSpawnHandle.IsValid())
		{
			World->RemoveOnActorSpawnedHandler(mSpawnHandle);
			mSpawnHandle.Reset();
		}
		if (mDestroyHandle.IsValid())
		{
			World->RemoveOnActorDestroyedHandler(mDestroyHandle);
			mDestroyHandle.Reset();
		}
		if (AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(World); SchematicManager != nullptr)
		{
			SchematicManager->PurchasedSchematicDelegate.RemoveDynamic(
				this, &UKDFResourceNodeSubsystem::OnSchematicPurchased);
		}
	}
	Super::Deinitialize();
}

void UKDFResourceNodeSubsystem::OnActorSpawned(AActor* Actor)
{

	if (AFGNodeMeshActor* Mesh = Cast<AFGNodeMeshActor>(Actor); Mesh != nullptr)
	{
		IndexMeshActor(Mesh);
		return;
	}
	if (AFGResourceNodeFrackingSatellite* Satellite = Cast<AFGResourceNodeFrackingSatellite>(Actor);
		Satellite != nullptr)
	{
		IndexSatellite(Satellite);
	}
	if (AFGResourceNodeBase* Node = Cast<AFGResourceNodeBase>(Actor); Node != nullptr)
	{
		TryPurgeNode(Node);
	}
}

void UKDFResourceNodeSubsystem::OnActorDestroyed(AActor* Actor)
{

	const AFGResourceNodeBase* Node = Cast<AFGResourceNodeBase>(Actor);
	if (Node == nullptr || FindMatchingPurge(Node) == nullptr)
	{
		return;
	}
	DestroyMeshActorFor(Node);
}

void UKDFResourceNodeSubsystem::OnSchematicPurchased(TSubclassOf<UFGSchematic>  )
{
	PurgeScannerResources();
}

const FKDFNodePurge* UKDFResourceNodeSubsystem::FindMatchingPurge(const AFGResourceNodeBase* Node)
{
	const UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Node);
	if (Subsystem == nullptr)
	{
		return nullptr;
	}

	const UClass* ResourceClass = Node->GetResourceClass().Get();
	if (ResourceClass == nullptr)
	{
		return nullptr;
	}

	for (const FKDFNodePurge& Purge : Subsystem->GetNodePurges())
	{
		if (Purge.mResourceClass.Get() != ResourceClass)
		{
			continue;
		}
		if (!Purge.mNodeTypes.IsEmpty() && !Purge.mNodeTypes.Contains(static_cast<uint8>(Node->GetResourceNodeType())))
		{
			continue;
		}
		return &Purge;
	}
	return nullptr;
}

bool UKDFResourceNodeSubsystem::TryPurgeNode(AFGResourceNodeBase* Node)
{

	if (!IsValid(Node) || !Node->HasAuthority())
	{
		return false;
	}
	const FKDFNodePurge* Purge = FindMatchingPurge(Node);
	if (Purge == nullptr)
	{
		return false;
	}
	if (!Purge->bAllowOccupied)
	{
		if (IsNodeGroupOccupied(Node))
		{

			UE_LOG(LogKDataForge, Warning,
				   TEXT("Resource node purge (%s): skipped occupied node %s — set 'allowOccupied: true' to destroy "
						"it anyway"),
				   *Purge->mSourceFile, *Node->GetName());
			return false;
		}

		if (const UWorld* World = Node->GetWorld();
			bHasUnresolvedExtractors && World != nullptr && !World->HasBegunPlay())
		{
			UE_LOG(LogKDataForge, Warning,
				   TEXT("Resource node purge (%s): deferred node %s to the post-begin-play pass — an extractor "
						"has not resolved its resource yet, so occupancy cannot be decided here"),
				   *Purge->mSourceFile, *Node->GetName());
			return false;
		}
	}

	if (AFGResourceNodeFrackingCore* Core = Cast<AFGResourceNodeFrackingCore>(Node); Core != nullptr)
	{

		TArray<AFGResourceNodeFrackingSatellite*> Satellites;
		GetSatellitesOf(Core, Satellites);
		for (AFGResourceNodeFrackingSatellite* SatelliteNode : Satellites)
		{
			DestroyNode(SatelliteNode);
		}
	}
	DestroyNode(Node);
	return true;
}

void UKDFResourceNodeSubsystem::BuildSatelliteIndex(UWorld& World)
{

	mSatellitesByCore.Reset();
	for (TActorIterator<AFGResourceNodeFrackingSatellite> It(&World); It; ++It)
	{
		IndexSatellite(*It);
	}
}

void UKDFResourceNodeSubsystem::IndexSatellite(AFGResourceNodeFrackingSatellite* Satellite)
{
	if (!IsValid(Satellite))
	{
		return;
	}
	if (AFGResourceNodeFrackingCore* Core = Satellite->GetCore().Get(); Core != nullptr)
	{
		mSatellitesByCore.AddUnique(Core, Satellite);
	}
}

void UKDFResourceNodeSubsystem::BuildOccupiedNodeIndex(UWorld& World)
{

	mOccupiedNodes.Reset();
	bHasUnresolvedExtractors = false;
	for (TActorIterator<AFGBuildableResourceExtractorBase> It(&World); It; ++It)
	{
		const AFGBuildableResourceExtractorBase* Extractor = *It;
		if (!IsValid(Extractor))
		{
			continue;
		}

		if (Extractor->IsA<AFGBuildableWaterPump>())
		{
			continue;
		}

		const AActor* OccupiedNode = Cast<AActor>(Extractor->GetExtractableResource().GetObject());
		if (OccupiedNode == nullptr)
		{
			OccupiedNode = Extractor->GetResourceNode();
		}
		if (OccupiedNode != nullptr)
		{
			mOccupiedNodes.Add(OccupiedNode);
		}
		else
		{
			bHasUnresolvedExtractors = true;
		}
	}
}

void UKDFResourceNodeSubsystem::BuildMeshIndex(UWorld& World)
{
	mMeshByNode.Reset();
	mUnlinkedMeshes.Reset();
	bMeshIndexBuilt = true;
	for (TActorIterator<AFGNodeMeshActor> It(&World); It; ++It)
	{
		IndexMeshActor(*It);
	}

	if (mMeshByNode.IsEmpty() && !mUnlinkedMeshes.IsEmpty())
	{
		UE_LOG(LogKDataForge, Warning,
			   TEXT("Resource node purge: none of the %d node mesh actor(s) in '%s' exposed a readable "
					"mNodeActor link — falling back to proximity matching for all of them"),
			   mUnlinkedMeshes.Num(), *World.GetName());
	}
}

void UKDFResourceNodeSubsystem::IndexMeshActor(AFGNodeMeshActor* Mesh)
{
	if (!IsValid(Mesh))
	{
		return;
	}

	if (AFGResourceNodeBase* LinkedNode = Mesh->mNodeActor.Get(); LinkedNode != nullptr)
	{
		mMeshByNode.Add(LinkedNode, Mesh);
		return;
	}
	mUnlinkedMeshes.AddUnique(Mesh);
}

void UKDFResourceNodeSubsystem::GetSatellitesOf(AFGResourceNodeFrackingCore* Core,
												TArray<AFGResourceNodeFrackingSatellite*>& OutSatellites) const
{
	TArray<TWeakObjectPtr<AFGResourceNodeFrackingSatellite>> Indexed;
	mSatellitesByCore.MultiFind(Core, Indexed);

	Indexed.Append(Core->Native_GetSatellites());
	for (const TWeakObjectPtr<AFGResourceNodeFrackingSatellite>& Satellite : Indexed)
	{
		if (AFGResourceNodeFrackingSatellite* Resolved = Satellite.Get(); Resolved != nullptr)
		{
			OutSatellites.AddUnique(Resolved);
		}
	}
}

bool UKDFResourceNodeSubsystem::IsNodeGroupOccupied(AFGResourceNodeBase* Node) const
{

	if (AFGResourceNodeFrackingSatellite* AsSatellite = Cast<AFGResourceNodeFrackingSatellite>(Node);
		AsSatellite != nullptr)
	{
		if (AFGResourceNodeFrackingCore* OwningCore = AsSatellite->GetCore().Get(); OwningCore != nullptr)
		{
			Node = OwningCore;
		}
	}

	if (Node->IsOccupied() || mOccupiedNodes.Contains(Node))
	{
		return true;
	}
	if (AFGResourceNodeFrackingCore* Core = Cast<AFGResourceNodeFrackingCore>(Node); Core != nullptr)
	{

		TArray<AFGResourceNodeFrackingSatellite*> Satellites;
		GetSatellitesOf(Core, Satellites);
		for (const AFGResourceNodeFrackingSatellite* Satellite : Satellites)
		{
			if (Satellite->IsOccupied() || mOccupiedNodes.Contains(Satellite))
			{
				return true;
			}
		}
	}
	return false;
}

void UKDFResourceNodeSubsystem::DestroyNode(AFGResourceNodeBase* Node)
{
	if (AFGActorRepresentationManager* Representations = AFGActorRepresentationManager::Get(Node->GetWorld());
		Representations != nullptr)
	{

		if (UFGResourceNodeRepresentation* Representation = Representations->FindResourceNodeRepresentation(Node);
			Representation != nullptr)
		{
			Representations->RemoveRepresentation(Representation);
		}
	}

	Node->Destroy();
}

void UKDFResourceNodeSubsystem::DestroyMeshActorFor(const AFGResourceNodeBase* Node)
{
	UWorld* World = Node->GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (!bMeshIndexBuilt)
	{

		BuildMeshIndex(*World);
	}

	AFGNodeMeshActor* MeshActor = nullptr;
	if (const TWeakObjectPtr<AFGNodeMeshActor>* Linked = mMeshByNode.Find(Node); Linked != nullptr)
	{
		MeshActor = Linked->Get();
	}
	if (MeshActor == nullptr)
	{

		const FVector NodeLocation = Node->GetActorLocation();
		float NearestDistSq = MeshActorMatchToleranceCm * MeshActorMatchToleranceCm;
		for (const TWeakObjectPtr<AFGNodeMeshActor>& Candidate : mUnlinkedMeshes)
		{
			AFGNodeMeshActor* Mesh = Candidate.Get();
			if (Mesh == nullptr)
			{
				continue;
			}
			if (const float DistSq = FVector::DistSquared(NodeLocation, Mesh->GetActorLocation());
				DistSq <= NearestDistSq)
			{
				NearestDistSq = DistSq;
				MeshActor = Mesh;
			}
		}
	}
	if (MeshActor != nullptr)
	{
		mMeshByNode.Remove(Node);
		MeshActor->Destroy();
	}
}

void UKDFResourceNodeSubsystem::PurgeScannerResources()
{
	UWorld* World = GetWorld();
	if (World == nullptr || World->GetNetMode() == NM_Client)
	{
		return;
	}
	const UKDFSubsystem* Subsystem = UKDFSubsystem::Get(World);
	if (Subsystem == nullptr || Subsystem->GetNodePurges().IsEmpty())
	{
		return;
	}

	AFGUnlockSubsystem* Unlocks = AFGUnlockSubsystem::Get(World);
	int32 RemovedPairCount = 0;
	for (const FKDFNodePurge& Purge : Subsystem->GetNodePurges())
	{
		const UClass* ResourceClass = Purge.mResourceClass.Get();
		if (!Purge.bRemoveFromScanner || ResourceClass == nullptr)
		{
			continue;
		}

		if (Unlocks != nullptr)
		{

			RemovedPairCount +=
				Unlocks->mScannableResourcesPairs.RemoveAll([ResourceClass](const FScannableResourcePair& Pair)
															{ return Pair.ResourceDescriptor.Get() == ResourceClass; });
		}
		PurgeKAPIAllowLists(ResourceClass);
	}
	if (RemovedPairCount > 0)
	{
		UE_LOG(LogKDataForge, Log, TEXT("Resource node purge: removed %d scannable resource pair(s)"),
			   RemovedPairCount);
	}
}

void UKDFResourceNodeSubsystem::PurgeKAPIAllowLists(const UClass* ResourceClass)
{

	const FString ResourcePath = ResourceClass->GetPathName();

	if (UClass* AllowListClass = FindObject<UClass>(nullptr, TEXT("/Script/KAPI.KAPIExtractorAllowList"));
		AllowListClass != nullptr)
	{
		TArray<UObject*> AllowLists;
		GetObjectsOfClass(AllowListClass, AllowLists,  true);
		for (UObject* AllowList : AllowLists)
		{
			ApplyClassOpOnContainer(AllowList, TEXT("mAllowedResources"), ResourcePath, EKDFOp::Remove);

			ApplyClassOpOnContainer(AllowList, TEXT("mDisallowResources"), ResourcePath, EKDFOp::Remove);
			ApplyClassOpOnContainer(AllowList, TEXT("mDisallowResources"), ResourcePath, EKDFOp::Append);
		}
	}

	if (UClass* KAPIClass = FindObject<UClass>(nullptr, TEXT("/Script/KAPI.KAPIDataAssetSubsystem"));
		KAPIClass != nullptr)
	{
		if (UGameInstance* GameInstance = GetWorld()->GetGameInstance(); GameInstance != nullptr)
		{
			if (UGameInstanceSubsystem* KAPISubsystem = GameInstance->GetSubsystemBase(KAPIClass);
				KAPISubsystem != nullptr)
			{

				ApplyClassOpOnContainer(KAPISubsystem, TEXT("mAllowedScannableResources"), ResourcePath,
										EKDFOp::Remove);
			}
		}
	}
}

void UKDFResourceNodeSubsystem::RefreshScannersAndRadarTowers()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		const APlayerController* PlayerController = Iterator->Get();
		if (PlayerController == nullptr)
		{
			continue;
		}
		const AFGCharacterPlayer* Player = Cast<AFGCharacterPlayer>(PlayerController->GetPawn());
		AFGResourceScanner* Scanner = Player != nullptr ? Player->GetResourceScanner() : nullptr;
		if (Scanner == nullptr)
		{
			continue;
		}
		Scanner->mNodeClusters.Empty();
		Scanner->mNodeClustersUpToDate = false;
		Scanner->GenerateNodeClusters();
		Scanner->mNodeClustersUpToDate = false;
	}

	if (World->GetNetMode() == NM_Client)
	{
		return;
	}
	for (TActorIterator<AFGBuildableRadarTower> It(World); It; ++It)
	{
		AFGBuildableRadarTower* Tower = *It;
		if (!IsValid(Tower))
		{
			continue;
		}
		Tower->ClearScannedResources();
		Tower->ScanForResources();
	}
}
