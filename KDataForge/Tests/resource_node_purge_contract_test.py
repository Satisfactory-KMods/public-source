"""Static regression guards for `type: resourcenode` purges.

In-game smoke testing remains required (none of this is verifiable from the stub headers), but these
checks pin the decisions that would otherwise break silently during a refactor: the handler must not
try to touch the world, the primary sweep must run before world begin play (the vanilla randomizer
and KBFL's spawner both act at or before it), the purge must re-run every world load, the node's
separate mesh actor must be destroyed on every peer without touching the node's crash-prone
mMeshActor soft pointer, occupied nodes must be refused by default, and the de-scan must hit the live
replicated pair list rather than the deprecated one.

Running the primary sweep before BeginPlay means every guard has to read a source that is POPULATED
at that phase — level-authored UPROPERTYs and SaveGame properties, never runtime-built lists. Four of
these tests exist purely to pin that: satellites matched from the satellite's own mCore, occupancy
unioned in from the extractor's SaveGame mExtractableResource, the second pass deferred to a
next-tick timer so it cannot race KBFL's own OnWorldBeginPlay, and the mesh reverse-link match left
unconditional.
"""

from pathlib import Path


ROOT = Path(__file__).parents[1]
HANDLER = ROOT / "Source/KDataForge/Private/Handlers/KDFResourceNodeHandler.cpp"
NODE_SUBSYSTEM = ROOT / "Source/KDataForge/Private/Subsystems/KDFResourceNodeSubsystem.cpp"
NODE_SUBSYSTEM_HEADER = ROOT / "Source/KDataForge/Public/Subsystems/KDFResourceNodeSubsystem.h"
LOADER_TYPES = ROOT / "Source/KDataForge/Public/Loader/KDFLoaderTypes.h"
KDF_SUBSYSTEM = ROOT / "Source/KDataForge/Private/Subsystems/KDFSubsystem.cpp"
ACCESS_TRANSFORMERS = ROOT / "Config/AccessTransformers.ini"
# The live dev-machine pack at the repo root — this repo root IS <GameFolder>/FactoryGame/.
SAMPLE_PACK_DOC = Path(__file__).parents[4] / "DataForge/purge-uranium.resourcenode.yml"


def require(text: str, expected: str) -> None:
    assert expected in text, f"Missing resource-node purge contract: {expected}"


def test_handler_only_registers_rules_and_is_wired_up() -> None:
    handler = HANDLER.read_text(encoding="utf-8")
    subsystem = KDF_SUBSYSTEM.read_text(encoding="utf-8")
    require(handler, 'mRootType = TEXT("resourcenode")')
    require(handler, "Subsystem->RegisterNodePurge(Purge)")
    require(handler, "Context.bDryRun")
    require(handler, "Context.mPatchRecord->mOps.AddDefaulted_GetRef()")
    require(subsystem, "RegisterHandler(NewObject<UKDFResourceNodeHandler>(this))")
    # The loader runs at UGameInstance::Init() with no world — the handler must never destroy anything.
    assert "Destroy()" not in handler
    assert "TActorIterator" not in handler


def test_purge_reruns_every_load_and_catches_streamed_in_nodes() -> None:
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    require(node_subsystem, "TActorIterator<AFGResourceNodeBase>")
    require(node_subsystem, "AddOnActorSpawnedHandler")
    require(node_subsystem, "RemoveOnActorSpawnedHandler")
    # Every hook goes in at Initialize (UWorld::InitWorld), never at OnWorldBeginPlay: KBFL's node
    # spawner also runs at OnWorldBeginPlay and cross-plugin ordering there is not guaranteed.
    initialize = node_subsystem[node_subsystem.index("void UKDFResourceNodeSubsystem::Initialize"):]
    initialize = initialize[:initialize.index("void UKDFResourceNodeSubsystem::OnWorldActorsInitialized")]
    for hook in (
        "AddOnActorSpawnedHandler",
        "AddOnActorDestroyedHandler",
        "FWorldDelegates::OnWorldInitializedActors.AddUObject",
        "FWorldDelegates::LevelAddedToWorld.AddUObject",
    ):
        require(initialize, hook)


def test_primary_sweep_runs_before_begin_play_not_at_it() -> None:
    """OnWorldBeginPlay is the LAST phase of UWorld::BeginPlay.

    By then GameMode->StartPlay() has dispatched BeginPlay on every actor, so the vanilla node
    randomizer has already assigned mResourceClassOverride and KBFL's SpawnDescriptors() has already
    run. FWorldDelegates::OnWorldInitializedActors (end of UWorld::InitializeActorsForPlay) is the
    last seam where every level-placed node exists and nothing has begun play yet.
    """
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    require(node_subsystem, "void UKDFResourceNodeSubsystem::OnWorldActorsInitialized(const UWorld::FActorsInitializedParams& Params)")
    require(node_subsystem, "void UKDFResourceNodeSubsystem::SweepWorld(UWorld& World)")
    # OnWorldInitializedActors and LevelAddedToWorld are process-global — they fire for every world.
    actors_initialized = node_subsystem[node_subsystem.index("void UKDFResourceNodeSubsystem::OnWorldActorsInitialized"):]
    actors_initialized = actors_initialized[:actors_initialized.index("void UKDFResourceNodeSubsystem::OnLevelAddedToWorld")]
    require(actors_initialized, "Params.World != GetWorld()")
    require(actors_initialized, "SweepWorld(*Params.World)")
    level_added = node_subsystem[node_subsystem.index("void UKDFResourceNodeSubsystem::OnLevelAddedToWorld"):]
    level_added = level_added[:level_added.index("void UKDFResourceNodeSubsystem::SweepWorld")]
    require(level_added, "World != GetWorld()")
    require(level_added, "Level->Actors")

    # Both global delegates must be released without a GetWorld() gate — a stale world during
    # teardown would otherwise leak the binding into every later world (KBFL's documented bug).
    deinit = node_subsystem[node_subsystem.index("void UKDFResourceNodeSubsystem::Deinitialize"):]
    deinit = deinit[:deinit.index("void UKDFResourceNodeSubsystem::OnActorSpawned")]
    for removal in (
        "FWorldDelegates::OnWorldInitializedActors.Remove",
        "FWorldDelegates::LevelAddedToWorld.Remove",
        "RemoveOnActorDestroyedHandler",
    ):
        require(deinit, removal)
    assert deinit.index("FWorldDelegates::OnWorldInitializedActors.Remove") < deinit.index("if (UWorld* World = GetWorld()"), (
        "the process-global delegates must be released outside the GetWorld() gate"
    )


def test_second_pass_is_deferred_past_every_subsystems_begin_play() -> None:
    """UKBFLResourceNodeSubsystem rewrites mResourceClass from its OWN OnWorldBeginPlay.

    UWorld::BeginPlay dispatches world subsystems in subsystem-CREATION order, which no plugin
    controls, so a pass that runs inside this subsystem's OnWorldBeginPlay races KBFL's writer. A
    next-tick timer is the first point that is after every subsystem's begin play.
    """
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    header = NODE_SUBSYSTEM_HEADER.read_text(encoding="utf-8")

    begin_play = node_subsystem[node_subsystem.index("void UKDFResourceNodeSubsystem::OnWorldBeginPlay"):]
    begin_play = begin_play[:begin_play.index("void UKDFResourceNodeSubsystem::SweepDeferred")]
    require(begin_play, "InWorld.GetTimerManager().SetTimerForNextTick(")
    require(begin_play, "FTimerDelegate::CreateUObject(this, &UKDFResourceNodeSubsystem::SweepDeferred)")
    assert "SweepWorld(InWorld)" not in begin_play, (
        "pass 4 must not run inside OnWorldBeginPlay — it would race KBFL's own OnWorldBeginPlay"
    )

    deferred = node_subsystem[node_subsystem.index("void UKDFResourceNodeSubsystem::SweepDeferred"):]
    deferred = deferred[:deferred.index("void UKDFResourceNodeSubsystem::Deinitialize")]
    # The world can tear down between the schedule and the tick; the delegate only guards the UObject.
    require(deferred, "IsValid(World)")
    require(deferred, "World->bIsTearingDown")
    require(deferred, "SweepWorld(*World);")
    require(node_subsystem, "void UKDFResourceNodeSubsystem::SweepDeferred()")
    require(header, "void SweepDeferred();")
    # The class comment must not present the node-manager hazard as solved: pass 4 destroys nodes
    # AFTER AFGResourceNodeManager built its non-UPROPERTY raw-pointer lists, and nothing clears them.
    require(header, "KNOWN LIMITATION")
    require(header, "AFGResourceNodeManager")


def test_occupied_nodes_are_refused_by_default() -> None:
    loader_types = LOADER_TYPES.read_text(encoding="utf-8")
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    require(loader_types, "bool bAllowOccupied = false;")
    require(node_subsystem, "if (!Purge->bAllowOccupied)")
    require(node_subsystem, "if (IsNodeGroupOccupied(Node))")


def test_occupancy_resolves_to_the_cluster_root_before_testing() -> None:
    """A satellite must answer for its whole well, not just for itself.

    With no `nodeTypes:` filter every satellite matches a purge in its own right, so SweepWorld
    reaches satellites independently of their core. Testing a satellite alone spared an occupied core
    while still destroying its unoccupied satellites out from under the activator standing on it.
    """
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    require(node_subsystem, "Cast<AFGResourceNodeFrackingSatellite>(Node)")
    require(node_subsystem, "AsSatellite->GetCore().Get()")
    require(node_subsystem, "Node = OwningCore;")


def test_water_pumps_do_not_defer_the_whole_sweep() -> None:
    """A water pump extracts from an AFGWaterVolume, never from a resource node.

    It is one of the families the game expects to have an unresolved link after load — that is what
    its TryFindMissingResource override is for. Counting it raised bHasUnresolvedExtractors on
    ordinary saves, which deferred every purge-matched node in the world.
    """
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    require(node_subsystem, "#include \"Buildables/FGBuildableWaterPump.h\"")
    require(node_subsystem, "Extractor->IsA<AFGBuildableWaterPump>()")


def test_mesh_index_warns_when_it_degrades_to_proximity_only() -> None:
    """Falling back to 100 cm proximity for every mesh cannot tell close-together nodes apart.

    That is a silent quality drop, so it has to be visible in the log or a leftover / wrongly
    destroyed mesh is undiagnosable.
    """
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    require(node_subsystem, "if (mMeshByNode.IsEmpty() && !mUnlinkedMeshes.IsEmpty())")
    require(node_subsystem, "falling back to proximity matching for all of them")


def test_occupancy_is_read_from_the_save_restored_extractor_side() -> None:
    """AFGResourceNodeBase::mIsOccupied is Replicated with its SaveGame meta explicitly removed.

    Deserialization cannot restore it, so before BeginPlay every node reads NOT occupied and an
    occupied node would be destroyed under a live miner. The extractor's mExtractableResource IS
    UPROPERTY(SaveGame) and is written directly by the save — that is the only occupancy source that
    exists at OnWorldInitializedActors, and it must be UNIONED with the node's own flag, not replace
    it (the flag is the truth after begin play, when a miner is built at runtime).
    """
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    header = NODE_SUBSYSTEM_HEADER.read_text(encoding="utf-8")
    access_transformers = ACCESS_TRANSFORMERS.read_text(encoding="utf-8")

    require(node_subsystem, "TActorIterator<AFGBuildableResourceExtractorBase>")
    require(node_subsystem, "mOccupiedNodes.Add(OccupiedNode)")
    require(header, "TSet<const AActor*> mOccupiedNodes;")
    # Public getters only — no new friendship was needed to read the extractor's node link.
    require(node_subsystem, "Extractor->GetExtractableResource().GetObject()")
    require(node_subsystem, "Extractor->GetResourceNode()")
    assert 'Class="AFGBuildableResourceExtractorBase"' not in access_transformers, (
        "GetExtractableResource()/GetResourceNode() are public — no AccessTransformers entry may be added"
    )

    # Built ONCE per sweep, never once per node.
    sweep = node_subsystem[node_subsystem.index("void UKDFResourceNodeSubsystem::SweepWorld"):]
    sweep = sweep[:sweep.index("void UKDFResourceNodeSubsystem::OnWorldBeginPlay")]
    for builder in ("BuildSatelliteIndex(World);", "BuildOccupiedNodeIndex(World);", "BuildMeshIndex(World);"):
        require(sweep, builder)
        assert sweep.index(builder) < sweep.index("TActorIterator<AFGResourceNodeBase>"), (
            f"{builder} must run before the node loop, not inside it"
        )

    occupied = node_subsystem[node_subsystem.index("bool UKDFResourceNodeSubsystem::IsNodeGroupOccupied"):]
    occupied = occupied[:occupied.index("void UKDFResourceNodeSubsystem::DestroyNode")]
    require(occupied, "Node->IsOccupied() || mOccupiedNodes.Contains(Node)")
    require(occupied, "Satellite->IsOccupied() || mOccupiedNodes.Contains(Satellite)")

    # An extractor whose node link has not resolved yet makes "not occupied" a guess — defer, never
    # destroy. Skipping a destroy is recoverable; destroying a live node is not.
    require(node_subsystem, "bHasUnresolvedExtractors")
    defer = node_subsystem[node_subsystem.index("bool UKDFResourceNodeSubsystem::TryPurgeNode"):]
    defer = defer[:defer.index("void UKDFResourceNodeSubsystem::BuildSatelliteIndex")]
    require(defer, "!World->HasBegunPlay()")
    assert defer.index("bHasUnresolvedExtractors") < defer.index("DestroyNode(Node);")


def test_fracking_satellites_are_matched_from_the_satellite_side() -> None:
    """AFGResourceNodeFrackingCore::mSatellites is a plain member, no UPROPERTY and no SaveGame.

    It is filled only by RegisterSatellite(), which the satellite calls from OnConstruction/BeginPlay
    — so it is EMPTY at OnWorldInitializedActors. Purging a core off that list would destroy the core
    alone and leave every satellite holding mCore, a hard TObjectPtr to a freed actor. The satellite's
    own mCore is UPROPERTY(EditInstanceOnly): level-authored and deserialized with the level.
    """
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    header = NODE_SUBSYSTEM_HEADER.read_text(encoding="utf-8")

    require(node_subsystem, "TActorIterator<AFGResourceNodeFrackingSatellite>")
    require(node_subsystem, "Satellite->GetCore().Get()")
    require(header, "mSatellitesByCore")

    # The index is built once per sweep — never an inner TActorIterator per core (O(n*m)).
    build = node_subsystem[node_subsystem.index("void UKDFResourceNodeSubsystem::BuildSatelliteIndex"):]
    build = build[:build.index("void UKDFResourceNodeSubsystem::BuildOccupiedNodeIndex")]
    require(build, "mSatellitesByCore.Reset();")
    purge = node_subsystem[node_subsystem.index("bool UKDFResourceNodeSubsystem::TryPurgeNode"):]
    purge = purge[:purge.index("void UKDFResourceNodeSubsystem::BuildSatelliteIndex")]
    assert "TActorIterator" not in purge, "TryPurgeNode must read the per-sweep index, not iterate per node"
    require(purge, "GetSatellitesOf(Core, Satellites)")

    # The runtime list stays in play for the post-begin-play pass, unioned in — never as the only source.
    satellites_of = node_subsystem[node_subsystem.index("void UKDFResourceNodeSubsystem::GetSatellitesOf"):]
    satellites_of = satellites_of[:satellites_of.index("bool UKDFResourceNodeSubsystem::IsNodeGroupOccupied")]
    require(satellites_of, "mSatellitesByCore.MultiFind(Core, Indexed)")
    require(satellites_of, "Core->Native_GetSatellites()")
    assert satellites_of.index("MultiFind") < satellites_of.index("Native_GetSatellites"), (
        "the satellite-side index is the primary source; the runtime list is only unioned in"
    )


def test_descan_uses_the_live_pair_list_not_the_deprecated_one() -> None:
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    access_transformers = ACCESS_TRANSFORMERS.read_text(encoding="utf-8")
    require(node_subsystem, "mScannableResourcesPairs.RemoveAll")
    # RemoveAllScannableResources() clears only the deprecated, non-replicated list — a silent no-op.
    assert "Unlocks->RemoveAllScannableResources" not in node_subsystem
    require(node_subsystem, "PurchasedSchematicDelegate.AddDynamic")
    require(access_transformers, 'Friend = (Class="AFGUnlockSubsystem", FriendClass="UKDFResourceNodeSubsystem")')
    require(access_transformers, 'Friend = (Class="AFGResourceScanner", FriendClass="UKDFResourceNodeSubsystem")')
    # The AFGResourceNodeBase friendship existed only for the protected GetMeshActor(); the mesh is
    # now found through the public AFGNodeMeshActor::mNodeActor and by proximity, so it must be gone.
    assert 'Class="AFGResourceNodeBase"' not in access_transformers


def test_separate_mesh_actor_and_representation_are_removed_too() -> None:
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    header = NODE_SUBSYSTEM_HEADER.read_text(encoding="utf-8")
    # The node's own mMeshActor soft pointer is unusable on hand-streamed World Partition sublevels:
    # its remapping is never set up, reading the path can crash and Get() never resolves the in-world
    # instance (documented in-tree by RefinedRDLib). Match the mesh actor from the other side instead.
    assert "Node->GetMeshActor()" not in node_subsystem
    require(node_subsystem, "TActorIterator<AFGNodeMeshActor>")
    require(node_subsystem, "MeshActorMatchToleranceCm")
    require(node_subsystem, "MeshActor->Destroy()")

    # AFGNodeMeshActor::mNodeActor is UPROPERTY(EditInstanceOnly) — level-authored and deserialized
    # with the level, so it is readable at both sweep phases. Gating the authoritative match on
    # HasActorBegunPlay() made it unreachable (nothing has begun play at either phase) and left only
    # the 100 cm proximity heuristic live. TSoftObjectPtr::Get() on an unresolved path returns null.
    assert "Mesh->HasActorBegunPlay" not in node_subsystem, (
        "the reverse-link match must be unconditional — nothing has begun play at either sweep phase"
    )
    require(node_subsystem, "Mesh->mNodeActor.Get()")
    require(node_subsystem, "mMeshByNode.Add(LinkedNode, Mesh)")
    require(node_subsystem, "mUnlinkedMeshes.AddUnique(Mesh)")

    # Built ONCE per sweep, and proximity is used ONLY for meshes with no readable reverse link.
    destroy_mesh = node_subsystem[node_subsystem.index("void UKDFResourceNodeSubsystem::DestroyMeshActorFor"):]
    destroy_mesh = destroy_mesh[:destroy_mesh.index("void UKDFResourceNodeSubsystem::PurgeScannerResources")]
    assert "TActorIterator" not in destroy_mesh, (
        "the mesh index is built once per sweep, never re-iterated per destroyed node"
    )
    require(destroy_mesh, "mMeshByNode.Find(Node)")
    require(destroy_mesh, "for (const TWeakObjectPtr<AFGNodeMeshActor>& Candidate : mUnlinkedMeshes)")
    assert destroy_mesh.index("mMeshByNode.Find(Node)") < destroy_mesh.index("mUnlinkedMeshes)"), (
        "the reverse link must be tried before proximity"
    )
    # Never force-load a soft pointer just to destroy what it points at.
    assert "LoadSynchronous" not in node_subsystem
    # AFGResourceNodeBase's IFGActorRepresentationInterface inheritance is commented out in the game
    # headers, so the AActor* overload is unverified — the typed lookup is the supported route.
    require(node_subsystem, "FindResourceNodeRepresentation(Node)")
    assert "Representations->RemoveRepresentationOfActor" not in node_subsystem
    require(header, "class KDATAFORGE_API UKDFResourceNodeSubsystem : public UWorldSubsystem")


def test_mesh_actor_destruction_runs_on_every_peer_not_only_the_server() -> None:
    """AFGNodeMeshActor is an unreplicated, level-placed AStaticMeshActor.

    A server-side Destroy() on it is invisible to clients, so gating it behind the node's
    HasAuthority() check left every client with a floating mesh where the node used to be. Driving it
    off the node's destruction instead runs identically on the server and on a client receiving the
    replicated destruction.
    """
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    destroy_node = node_subsystem[node_subsystem.index("void UKDFResourceNodeSubsystem::DestroyNode"):]
    destroy_node = destroy_node[:destroy_node.index("void UKDFResourceNodeSubsystem::DestroyMeshActorFor")]
    assert "MeshActor" not in destroy_node, (
        "DestroyNode is reached only past the HasAuthority() gate — the mesh must not be destroyed there"
    )
    on_destroyed = node_subsystem[node_subsystem.index("void UKDFResourceNodeSubsystem::OnActorDestroyed"):]
    on_destroyed = on_destroyed[:on_destroyed.index("void UKDFResourceNodeSubsystem::OnSchematicPurchased")]
    require(on_destroyed, "DestroyMeshActorFor(Node)")
    require(on_destroyed, "FindMatchingPurge(Node)")
    assert "HasAuthority" not in on_destroyed, "the client half must not be authority-gated"


def test_kapi_is_reached_by_soft_lookup_only() -> None:
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    require(node_subsystem, 'FindObject<UClass>(nullptr, TEXT("/Script/KAPI.KAPIExtractorAllowList"))')
    require(node_subsystem, 'FindObject<UClass>(nullptr, TEXT("/Script/KAPI.KAPIDataAssetSubsystem"))')
    # KDataForge.uplugin depends only on SML — no KAPI header may be included here.
    assert "#include \"DataAssets/KAPI" not in node_subsystem


def test_kapi_blacklist_is_added_to_not_removed_from() -> None:
    """mDisallowResources is a BLACKLIST, so a purge must ADD to it.

    UKAPIDataAssetSubsystem::ScanForAllowList adds every mAllowedResources entry to
    mAllowedScannableResources and then removes every mDisallowResources entry from it. Removing from
    mDisallowResources therefore makes the resource MORE scannable — the exact inverse of a purge.
    It is also the only durable half: mAllowedScannableResources is emptied and rebuilt on every
    StartScanForDataAssets, but the blacklist is re-read by that same rebuild.
    """
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    allow_op = 'ApplyClassOpOnContainer(AllowList, TEXT("mAllowedResources"), ResourcePath, EKDFOp::Remove)'
    disallow_remove = 'ApplyClassOpOnContainer(AllowList, TEXT("mDisallowResources"), ResourcePath, EKDFOp::Remove)'
    disallow_append = 'ApplyClassOpOnContainer(AllowList, TEXT("mDisallowResources"), ResourcePath, EKDFOp::Append)'
    require(node_subsystem, allow_op)
    require(node_subsystem, disallow_append)
    # Remove-then-append, in that order: the array op engine's Append does not dedupe and
    # PurgeScannerResources re-runs on every schematic purchase.
    require(node_subsystem, disallow_remove)
    assert node_subsystem.index(disallow_remove) < node_subsystem.index(disallow_append), (
        "mDisallowResources must be de-duplicated (Remove) before the Append, or repeated purges "
        "stack duplicate blacklist entries"
    )
    # The derived cache may still be cleared as a same-session fast path, but never on its own.
    require(node_subsystem, 'TEXT("mAllowedScannableResources"), ResourcePath,')
    require(node_subsystem, "EKDFOp::Remove)")


def test_resource_class_is_retained_against_gc() -> None:
    """FKDFNodePurge::mResourceClass is weak and mNodePurges is not a UPROPERTY.

    Purges are resolved at UGameInstance::Init(); nothing else holds the descriptor class between
    there and the first world, so without an explicit retain a GC turns every purge into a no-op.
    """
    handler = HANDLER.read_text(encoding="utf-8")
    loader_types = LOADER_TYPES.read_text(encoding="utf-8")
    require(handler, "Subsystem->RetainObject(Purge.mResourceClass.Get())")
    require(loader_types, "TWeakObjectPtr<UClass> mResourceClass;")


def test_client_scanner_clusters_are_refreshed_unconditionally() -> None:
    """The scanner-cluster rebuild is client-local state and must run on every peer.

    TryPurgeNode returns false without authority, so DestroyedCount is always 0 on a client —
    gating the refresh on it made the client half dead code. The radar-tower half gates itself.
    """
    node_subsystem = NODE_SUBSYSTEM.read_text(encoding="utf-8")
    # It rides along with the deferred pass, so the caches are rebuilt after the LAST destruction.
    deferred = node_subsystem[node_subsystem.index("void UKDFResourceNodeSubsystem::SweepDeferred"):]
    deferred = deferred[:deferred.index("void UKDFResourceNodeSubsystem::Deinitialize")]
    require(deferred, "RefreshScannersAndRadarTowers();")
    # It must sit outside the sweep (whose destroyed count is always 0 on a client) and after it.
    assert deferred.index("RefreshScannersAndRadarTowers();") > deferred.index("SweepWorld(*World);")
    assert "DestroyedCount" not in deferred, (
        "the destroyed count lives in SweepWorld; gating the client-local refresh on it is dead code"
    )
    # Radar towers stay server-only inside the refresh itself.
    require(node_subsystem, "World->GetNetMode() == NM_Client")


def test_bad_node_type_drops_the_whole_entry() -> None:
    """A partial node-type set would purge a different set of nodes than the document asked for."""
    handler = HANDLER.read_text(encoding="utf-8")
    require(handler, "bool bEntryValid = true;")
    require(handler, "bEntryValid = false;")
    require(handler, "if (!bEntryValid)")


def test_sample_pack_purge_is_opt_in_and_documents_persistence() -> None:
    sample = SAMPLE_PACK_DOC.read_text(encoding="utf-8")
    require(sample, "conditions: { hasMod: ")
    require(sample, "MUST BE OPTED INTO")
    # The old claim ("remove the file and the nodes come back") was false without SatisfactoryPlus.
    assert "Remove this file and the nodes come back" not in sample
    for text in (
        sample,
        LOADER_TYPES.read_text(encoding="utf-8"),
        NODE_SUBSYSTEM_HEADER.read_text(encoding="utf-8"),
    ):
        assert "SatisfactoryPlus" in text, "the persistence caveat must name its one exception"
        assert "tombstone" in text.lower()


if __name__ == "__main__":
    test_handler_only_registers_rules_and_is_wired_up()
    test_purge_reruns_every_load_and_catches_streamed_in_nodes()
    test_primary_sweep_runs_before_begin_play_not_at_it()
    test_second_pass_is_deferred_past_every_subsystems_begin_play()
    test_occupied_nodes_are_refused_by_default()
    test_occupancy_is_read_from_the_save_restored_extractor_side()
    test_fracking_satellites_are_matched_from_the_satellite_side()
    test_descan_uses_the_live_pair_list_not_the_deprecated_one()
    test_separate_mesh_actor_and_representation_are_removed_too()
    test_mesh_actor_destruction_runs_on_every_peer_not_only_the_server()
    test_kapi_is_reached_by_soft_lookup_only()
    test_kapi_blacklist_is_added_to_not_removed_from()
    test_resource_class_is_retained_against_gc()
    test_client_scanner_clusters_are_refreshed_unconditionally()
    test_bad_node_type_drops_the_whole_entry()
    test_sample_pack_purge_is_opt_in_and_documents_persistence()
    print("resource node purge contract: OK")
