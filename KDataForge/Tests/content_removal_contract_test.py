"""Static regression guards for the document-level `remove:` on recipe/schematic/research.

In-game smoke testing remains required (every FactoryGame body involved is an auto-generated stub),
but these checks pin the decisions that would otherwise rot silently: the three kinds share one rule
struct and one parse path, the two entries-key early-outs stay relaxed, the schematic/research path
never touches a SaveGame container, and the recipe path — the only one that writes the save — stays
server-gated with its hook installed exactly once.
"""

import re
from pathlib import Path

NEWLINE = chr(10)


ROOT = Path(__file__).parents[1]
HANDLER = ROOT / "Source/KDataForge/Private/Handlers/KDFContentClassHandler.cpp"
HANDLER_HEADER = ROOT / "Source/KDataForge/Public/Handlers/KDFContentClassHandler.h"
REMOVE_SUBSYSTEM = ROOT / "Source/KDataForge/Private/Content/KDFRecipeRemoval.cpp"
REMOVE_SUBSYSTEM_HEADER = ROOT / "Source/KDataForge/Public/Content/KDFRecipeRemoval.h"
LOADER_TYPES = ROOT / "Source/KDataForge/Public/Loader/KDFLoaderTypes.h"
KDF_SUBSYSTEM = ROOT / "Source/KDataForge/Private/Subsystems/KDFSubsystem.cpp"
KDF_SUBSYSTEM_HEADER = ROOT / "Source/KDataForge/Public/Subsystems/KDFSubsystem.h"
MODULE = ROOT / "Source/KDataForge/Private/KDataForgeModule.cpp"
WORLD_MODULE = ROOT / "Source/KDataForge/Private/Module/KDFWorldModule.cpp"
REMOVE_ORCHESTRATOR_HEADER = ROOT / "Source/KDataForge/Public/Subsystems/KDFContentRemoveSubsystem.h"
REMOVE_ORCHESTRATOR = ROOT / "Source/KDataForge/Private/Subsystems/KDFContentRemoveSubsystem.cpp"
BUILD_CS = ROOT / "Source/KDataForge/KDataForge.Build.cs"
UPLUGIN = ROOT / "KDataForge.uplugin"
ACCESS_TRANSFORMERS = ROOT / "Config/AccessTransformers.ini"
# The live dev-machine pack at the repo root — this repo root IS <GameFolder>/FactoryGame/.
PACK = Path(__file__).parents[4] / "DataForge"
SAMPLES = {
    "recipe": PACK / "remove-vanilla-recipes.recipe.yml",
    "schematic": PACK / "remove-vanilla-schematics.schematic.yml",
    "research": PACK / "remove-vanilla-research.research.yml",
}

# Everything AFGSchematicManager / AFGResearchManager persists. The schematic and research half of
# this feature is a registry filter and must not name any of these, ever.
SAVEGAME_CONTAINERS = (
    "mPurchasedSchematics",
    "ResetPurchasedSchematics",
    "mActiveSchematic",
    "mLastActiveSchematic",
    "mPaidOffSchematic",
    "mUnlockedResearchTrees",
    "mCompletedResearch",
    "mSavedOngoingResearch",
)


def code_only(text: str) -> str:
    """Strip C++ comments. KDataForge documents its timing/authority decisions at length, so a bare
    substring ban over a whole file is unsatisfiable - the prose naming a thing is exactly how these
    files explain why they do NOT do it. Ban it in the code, not in the explanation."""
    text = re.sub(r"/[*].*?[*]/", "", text, flags=re.DOTALL)
    return NEWLINE.join(line.split("//")[0] for line in text.split(NEWLINE))


def require(text: str, expected: str) -> None:
    assert expected in text, f"Missing content-removal contract: {expected}"


def _drain_loop() -> str:
    """The removal half of UKDFSubsystem::RegisterQueuedContentForWorld."""
    subsystem = KDF_SUBSYSTEM.read_text(encoding="utf-8")
    start = subsystem.index("for (const FKDFContentRemoval& Removal : mContentRemovals)")
    return subsystem[start:subsystem.index("void UKDFSubsystem::NotifyDataAssetConsumers")]


def test_one_rule_struct_and_one_storage_triple() -> None:
    loader_types = LOADER_TYPES.read_text(encoding="utf-8")
    header = KDF_SUBSYSTEM_HEADER.read_text(encoding="utf-8")
    subsystem = KDF_SUBSYSTEM.read_text(encoding="utf-8")

    require(loader_types, "struct KDATAFORGE_API FKDFContentRemoval")
    require(loader_types, "TWeakObjectPtr<UClass> mContentClass;")
    require(loader_types, "EKDFContentRegKind mKind = EKDFContentRegKind::None;")
    require(header, "void RegisterContentRemoval(const FKDFContentRemoval& Removal);")
    require(header, "const TArray<FKDFContentRemoval>& GetContentRemovals() const")
    require(header, "void ClearContentRemovals()")
    require(header, "TArray<FKDFContentRemoval> mContentRemovals;")
    # De-dup on (class, source file) so a live reload replaces rather than accumulates.
    require(subsystem, "Existing.mContentClass == Removal.mContentClass && Existing.mSourceFile == Removal.mSourceFile")


def test_grammar_lives_on_the_abstract_base_and_gates_on_the_existing_kind_field() -> None:
    """No new flag: mRegistrationKind is already Recipe/Schematic/ResearchTree for exactly these three."""
    handler = HANDLER.read_text(encoding="utf-8")
    header = HANDLER_HEADER.read_text(encoding="utf-8")

    require(header, "void CollectRemovals(")
    require(header, "bool ApplyRemovals(")
    require(handler, "void UKDFContentClassHandler::CollectRemovals(")
    require(handler, "bool UKDFContentClassHandler::ApplyRemovals(")
    require(handler, 'Document.Find(TEXT("remove"))')
    require(handler, "if (mRegistrationKind == EKDFContentRegKind::None)")
    # The three concrete handlers stay ctor-only — no per-kind override may creep in.
    for kind in ("UKDFRecipeHandler", "UKDFSchematicHandler", "UKDFResearchHandler"):
        assert f"{kind}::CollectRemovals" not in handler
        assert f"{kind}::ApplyRemovals" not in handler
        assert f"{kind}::ApplyDocument" not in handler


def test_removal_only_documents_survive_both_early_outs() -> None:
    """BOTH ValidateDocument and ApplyDocument used to bail when the entries key was absent."""
    handler = HANDLER.read_text(encoding="utf-8")

    validate = handler[handler.index("bool UKDFContentClassHandler::ValidateDocument"):]
    validate = validate[:validate.index("bool UKDFContentClassHandler::ApplyClassListShortcutUnlock")]
    require(validate, "CollectRemovals(Document, Context, /*bReportDiagnostics=*/true, Removals)")
    require(validate, "if (!bHasRemovals)")
    # Returning false here skips ApplyDocument entirely, so the error must be conditional.
    assert validate.index("if (!bHasRemovals)") < validate.index("Context.AddError(FString::Printf(TEXT(\"%s document")

    apply = handler[handler.index("bool UKDFContentClassHandler::ApplyDocument"):]
    apply = apply[:apply.index("// --- Concrete document types.")]
    require(apply, "bool bAppliedAny = ApplyRemovals(Document, Context);")
    # Removals must be handled ABOVE the entries guard, and the guard must no longer return early.
    assert apply.index("ApplyRemovals") < apply.index("Document.Find(mEntriesKey)")
    assert "return false;" not in apply, "the entries guard must not early-out any more"


def test_apply_honours_dry_run_records_ops_and_retains_the_class() -> None:
    handler = HANDLER.read_text(encoding="utf-8")
    apply = handler[handler.index("bool UKDFContentClassHandler::ApplyRemovals"):]
    apply = apply[:apply.index("bool UKDFContentClassHandler::ValidateDocument")]

    require(apply, "if (Context.bDryRun)")
    require(apply, "Subsystem->RegisterContentRemoval(Removal)")
    require(apply, "Context.mPatchRecord->mOps.AddDefaulted_GetRef()")
    require(apply, "OpRecord.mOp = EKDFOp::Remove;")
    # mContentClass is weak and mContentRemovals is not a UPROPERTY — nothing else holds the class
    # between UGameInstance::Init() and the first world, so a GC would silently void every rule.
    require(apply, "Subsystem->RetainObject(Removal.mContentClass.Get())")
    assert apply.index("Context.bDryRun") < apply.index("RegisterContentRemoval"), (
        "a dry run must not register anything"
    )


def test_schematic_and_research_use_smls_filter_and_touch_no_savegame_container() -> None:
    """The good path: zero save mutation, zero AccessTransformers, zero hooks."""
    drain = _drain_loop()
    require(drain, "case EKDFContentRegKind::Schematic:")
    require(drain, "ContentRegistry->RemoveSchematic(ContentClass)")
    require(drain, "case EKDFContentRegKind::ResearchTree:")
    require(drain, "ContentRegistry->RemoveResearchTree(ContentClass)")
    # Recipe deliberately falls through — SML ships no RemoveRecipe.
    require(drain, "default:")

    handler = HANDLER.read_text(encoding="utf-8")
    for text, name in ((drain, "the removal drain loop"), (handler, "the content-class handler")):
        for container in SAVEGAME_CONTAINERS:
            assert container not in text, f"{name} must never touch the SaveGame container {container}"
        # KBFL's remover reaches into the managers directly; this design must not.
        for manager in ("AFGSchematicManager", "AFGResearchManager"):
            assert manager not in text, f"{name} must go through SML's registry, not {manager}"

    # SML's Remove* pair is public — no friendship is needed for either manager.
    access_transformers = ACCESS_TRANSFORMERS.read_text(encoding="utf-8")
    assert 'Class="AFGSchematicManager"' not in access_transformers
    assert 'Class="AFGResearchManager"' not in access_transformers

    # Removals drain after registrations so an explicit remove beats a same-class register.
    subsystem = KDF_SUBSYSTEM.read_text(encoding="utf-8")
    assert subsystem.index("ContentRegistry->RegisterRecipe(") < subsystem.index("ContentRegistry->RemoveSchematic(")


def test_recipe_sweep_runs_between_save_load_and_world_begin_play() -> None:
    """UWorldSubsystem::OnWorldBeginPlay is dispatched by UWorld::BeginPlay only AFTER
    GameMode->StartPlay() has begun play on every actor — behind AFGRecipeManager::BeginPlay (which
    rebuilds the derived lists) and behind UFGRecipeManagerReplicationComponent::BeginPlay (which
    snapshots the whole available list to a joining client).

    SML binds ELifecyclePhase::INITIALIZATION to UWorld::OnActorsInitialized, which is after
    AFGGameMode::InitGameState ran UFGSaveSession::RoutePostLoadGame and before UWorld::BeginPlay.
    """
    world_module = WORLD_MODULE.read_text(encoding="utf-8")
    require(world_module, "if (Phase == ELifecyclePhase::INITIALIZATION)")
    # The world module drives removal through the orchestrator, which drives the recipe sweep. What
    # matters is that an explicit INITIALIZATION dispatch is the ONLY thing that starts removal work.
    require(world_module, "ApplyWorldRemovals()")
    # Removals must still drain after the registry registrations, so an explicit remove wins.
    assert world_module.index("RegisterQueuedContentForWorld") < world_module.index("ApplyWorldRemovals")

    # A class whose only members are statics has no business being a world subsystem.
    header = REMOVE_SUBSYSTEM_HEADER.read_text(encoding="utf-8")
    require(header, "class KDATAFORGE_API FKDFRecipeRemoval")
    for stale in (
        ": public UWorldSubsystem",
        "virtual void OnWorldBeginPlay",
        "DoesSupportWorldType",
        "UCLASS()",
        "GENERATED_BODY",
    ):
        assert stale not in header, f"the recipe sweep must not carry {stale} any more"

    # The orchestrator IS a world subsystem — that is the point of it — but the sweep must never be
    # self-scheduled from world begin play, which is the timing bug that killed the first version.
    # Guard the behaviour, not the file's existence.
    orchestrator_header = REMOVE_ORCHESTRATOR_HEADER.read_text(encoding="utf-8")
    orchestrator = REMOVE_ORCHESTRATOR.read_text(encoding="utf-8")
    require(orchestrator_header, "class KDATAFORGE_API UKDFContentRemoveSubsystem : public UWorldSubsystem")
    header_code = code_only(orchestrator_header)
    code = code_only(orchestrator)
    # Never self-scheduled: OnWorldBeginPlay is the timing bug that killed v1, and a timer or an actor
    # delegate is the same bug wearing a different hat.
    for self_schedule in (
        "OnWorldBeginPlay",
        "SetTimerForNextTick",
        "SetTimer",
        "AddOnActorSpawnedHandler",
        "OnWorldInitializedActors",
        "LevelAddedToWorld",
    ):
        assert self_schedule not in header_code and self_schedule not in code, (
            f"removal must be driven by the INITIALIZATION dispatch, never self-scheduled via {self_schedule}"
        )
    # The private-member access stays behind the per-FriendClass friendship: delegate, never reach in.
    require(orchestrator, "FKDFRecipeRemoval::SweepSaveRestoredRecipes(*World)")
    assert "AFGRecipeManager" not in code and "FGRecipeManager.h" not in code, (
        "AccessTransformers friendship is per-FriendClass — the orchestrator must delegate, not reach in"
    )
    # One process-wide hook, installed in the module. A per-world install would stack handlers.
    assert "SUBSCRIBE_METHOD" not in code


def test_recipe_sweep_is_server_only_before_it_mutates_anything() -> None:
    """mAvailableRecipes is UPROPERTY(SaveGame) — the sweep writes the save and must never run on a client."""
    source = REMOVE_SUBSYSTEM.read_text(encoding="utf-8")
    begin_play = source[source.index("void FKDFRecipeRemoval::SweepSaveRestoredRecipes"):]

    require(begin_play, "if (World.GetNetMode() == NM_Client)")
    gate = begin_play.index("if (World.GetNetMode() == NM_Client)")
    for mutation in (
        "Manager->RemoveAvailableRecipes(Matched)",
        "mAvailableCustomizationRecipes.Remove(",
        "mAvailableCustomizationRecipesLookup.Remove(",
        "Manager->RebuildDerivedAvailableRecipesData()",
        "Manager->RebuildAvailableItemDescriptorLookup()",
    ):
        require(begin_play, mutation)
        assert gate < begin_play.index(mutation), f"{mutation} runs before the NM_Client gate"
    assert begin_play.index("return;", gate) < begin_play.index("Manager->RemoveAvailableRecipes"), (
        "the NM_Client branch must return, not just log"
    )

    # Never mutate a container while iterating it, and never touch the Transient mAllRecipes.
    require(begin_play, "TArray<TSubclassOf<UFGRecipe>> Matched;")
    assert "->mAllRecipes" not in source, "mAllRecipes is Transient and does not gate availability"
    assert "PopulateAllRecipesList()" not in source
    # Purchases are never rolled back.
    for container in SAVEGAME_CONTAINERS:
        assert f"->{container}" not in source, f"recipe removal must not touch {container}"

    # Friendship is per-FriendClass, so the sweep has to stay a static member of this named class.
    access_transformers = ACCESS_TRANSFORMERS.read_text(encoding="utf-8")
    require(access_transformers, 'Friend = (Class="AFGRecipeManager", FriendClass="FKDFRecipeRemoval")')


def test_recipe_hook_is_installed_once_and_only_in_a_packaged_build() -> None:
    module = MODULE.read_text(encoding="utf-8")
    handler = HANDLER.read_text(encoding="utf-8")

    require(module, "#if !WITH_EDITOR")
    require(module, "SUBSCRIBE_METHOD(AFGRecipeManager::AddAvailableRecipe,")
    require(module, "Scope.Cancel();")
    require(module, "FKDFRecipeRemoval::IsRecipeRemoved(Self, Recipe)")

    # AFGRecipeManager has TWO add entry points and they are separate symbols — hooking the singular
    # does not hook the batch, and an unhooked batch would put a `remove:`d recipe straight into the
    # SaveGame-backed mAvailableRecipes. Both must be covered.
    require(module, "SUBSCRIBE_METHOD(AFGRecipeManager::AddAvailableRecipes,")
    # SML passes handler args by value, so the batch cannot be filtered in place: it is cancelled and
    # re-issued with the survivors. Re-issuing the SAME list would not terminate.
    require(module, "if (Kept.Num() == Recipes.Num())")
    require(module, "Self->AddAvailableRecipes(Kept);")
    # The batch hook fires for every bulk grant in the game — it needs the O(1) gate, not a per-element
    # subsystem lookup.
    require(module, "FKDFRecipeRemoval::HasAnyRemovalRules(Self)")
    assert module.count("SUBSCRIBE_METHOD") == 2, (
        "exactly one hook per AFGRecipeManager add entry point, each installed once per process"
    )
    # Both hooks live in the one packaged-build block; a hook outside it would fire in PIE.
    packaged = module[module.index("#if !WITH_EDITOR"):module.index("#endif")]
    assert packaged.count("SUBSCRIBE_METHOD") == 2, "every hook must sit inside the #if !WITH_EDITOR gate"
    # Per-document installation would stack a handler per YAML file.
    assert "SUBSCRIBE_METHOD" not in handler
    assert "SUBSCRIBE_METHOD" not in REMOVE_SUBSYSTEM.read_text(encoding="utf-8")
    # A per-world install would stack one handler per world load, not per YAML file.
    assert "SUBSCRIBE_METHOD" not in REMOVE_ORCHESTRATOR.read_text(encoding="utf-8")

    # The hook fires for every recipe add in the game — it has to be cheap when nothing was removed.
    source = REMOVE_SUBSYSTEM.read_text(encoding="utf-8")
    is_removed = source[source.index("bool FKDFRecipeRemoval::IsRecipeRemoved"):]
    is_removed = is_removed[:is_removed.index("void FKDFRecipeRemoval::SweepSaveRestoredRecipes")]
    require(is_removed, "Subsystem->GetContentRemovals().IsEmpty()")
    assert is_removed.index("GetContentRemovals().IsEmpty()") < is_removed.index("for (")


def test_no_hard_dependency_was_added() -> None:
    """KDataForge.uplugin depends on SML alone; KAPI/KBFL/RefinedRDLib/SFP stay reflection-only."""
    uplugin = UPLUGIN.read_text(encoding="utf-8")
    build_cs = BUILD_CS.read_text(encoding="utf-8")
    for optional in ("KAPI", "KBFL", "RefinedRDLib", "SatisfactoryPlus"):
        assert f'"Name": "{optional}"' not in uplugin, f"{optional} must stay an optional dependency"
        assert f'"{optional}"' not in build_cs, f"{optional} must not be a Build.cs entry"
    for source in (REMOVE_SUBSYSTEM, REMOVE_SUBSYSTEM_HEADER, MODULE, HANDLER):
        text = source.read_text(encoding="utf-8")
        for optional in ("KAPI/", "KBFL/", "RefinedRDLib/", "SatisfactoryPlus/"):
            assert f'#include "{optional}' not in text


def test_the_header_documents_the_recipe_asymmetry_where_a_pack_author_will_read_it() -> None:
    """The single most important thing to know about this feature is that one of three kinds differs."""
    for source in (LOADER_TYPES, REMOVE_SUBSYSTEM_HEADER):
        text = source.read_text(encoding="utf-8")
        assert "SaveGame" in text
        assert "RemoveSchematic" in text or "registry filter" in text
        assert "self-heal" in text.lower()
    # Removal never revokes a purchase — pack authors will expect otherwise.
    require(LOADER_TYPES.read_text(encoding="utf-8"), "NEVER REVOKES A PURCHASE")


def test_samples_are_opt_in_and_state_which_kind_writes_the_save() -> None:
    for kind, path in SAMPLES.items():
        sample = path.read_text(encoding="utf-8")
        require(sample, "conditions: { hasMod: KDFContentRemoveOptIn }")
        require(sample, "MUST BE OPTED INTO")
        require(sample, "remove:")
        require(sample, f"type: {kind}")
    recipe = SAMPLES["recipe"].read_text(encoding="utf-8")
    require(recipe, "WRITES YOUR SAVE")
    for save_clean in ("schematic", "research"):
        require(SAMPLES[save_clean].read_text(encoding="utf-8"), "SAVE-CLEAN")


if __name__ == "__main__":
    test_one_rule_struct_and_one_storage_triple()
    test_grammar_lives_on_the_abstract_base_and_gates_on_the_existing_kind_field()
    test_removal_only_documents_survive_both_early_outs()
    test_apply_honours_dry_run_records_ops_and_retains_the_class()
    test_schematic_and_research_use_smls_filter_and_touch_no_savegame_container()
    test_recipe_sweep_runs_between_save_load_and_world_begin_play()
    test_recipe_sweep_is_server_only_before_it_mutates_anything()
    test_recipe_hook_is_installed_once_and_only_in_a_packaged_build()
    test_no_hard_dependency_was_added()
    test_the_header_documents_the_recipe_asymmetry_where_a_pack_author_will_read_it()
    test_samples_are_opt_in_and_state_which_kind_writes_the_save()
    print("content removal contract: OK")
