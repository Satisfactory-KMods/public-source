"""Static regression guards for lazy CDO patch lifecycle handling.

Unreal runtime smoke testing remains required, but these checks prevent reintroducing eager CDO
creation or dropping initial-pass deduplication during refactors.
"""

from pathlib import Path


ROOT = Path(__file__).parents[1]
SUBSYSTEM_SOURCE = ROOT / "Source/KDataForge/Private/Subsystems/KDFSubsystem.cpp"
SUBSYSTEM_HEADER = ROOT / "Source/KDataForge/Public/Subsystems/KDFSubsystem.h"
CDO_HANDLER = ROOT / "Source/KDataForge/Private/Handlers/KDFCdoHandler.cpp"
LOADER_TYPES = ROOT / "Source/KDataForge/Public/Loader/KDFLoaderTypes.h"


def require(text: str, expected: str) -> None:
    assert expected in text, f"Missing lazy-watch safety contract: {expected}"


def test_class_created_callback_never_forces_an_unlinked_cdo() -> None:
    source = SUBSYSTEM_SOURCE.read_text(encoding="utf-8")
    callback = source[source.index("void UKDFSubsystem::NotifyUObjectCreated"):]
    callback = callback[:callback.index("void UKDFSubsystem::OnUObjectArrayShutdown")]
    require(callback, "QueueLazyClassForProcessing")
    assert "ApplyLazyWatchesToClass" not in callback
    assert "GetDefaultObject" not in callback


def test_pending_classes_wait_for_full_class_and_cdo_load() -> None:
    source = SUBSYSTEM_SOURCE.read_text(encoding="utf-8")
    header = SUBSYSTEM_HEADER.read_text(encoding="utf-8")
    require(source, "RF_NeedInitialization | RF_NeedLoad | RF_NeedPostLoad | RF_NeedPostLoadSubobjects")
    require(source, "EInternalObjectFlags::PendingConstruction")
    require(source, "NewClass->ClassWithin == nullptr")
    require(source, "NewClass->ClassConstructor == nullptr")
    require(source, "NewClass->GetDefaultObject(false)")
    require(source, "!bHasLazyClassWatches.load(std::memory_order_acquire) || !IsValid(NewClass)")
    require(header, "mPendingLazyClasses")
    require(header, "std::atomic<bool> bHasLazyClassWatches")


def test_initial_matches_are_deduplicated_per_watch() -> None:
    handler = CDO_HANDLER.read_text(encoding="utf-8")
    loader_types = LOADER_TYPES.read_text(encoding="utf-8")
    subsystem = SUBSYSTEM_SOURCE.read_text(encoding="utf-8")
    require(loader_types, "mAppliedClasses")
    require(handler, "Watch.mAppliedClasses.Add(TargetClass)")
    require(handler, "Watch.mAppliedClasses.Add(Derived)")
    require(handler, "Watch.mAppliedClasses.Add(Candidate)")
    require(subsystem, "Watch.mAppliedClasses.Contains(NewClass)")
    require(subsystem, "Watch.mAppliedClasses.Add(NewClass)")


if __name__ == "__main__":
    test_class_created_callback_never_forces_an_unlinked_cdo()
    test_pending_classes_wait_for_full_class_and_cdo_load()
    test_initial_matches_are_deduplicated_per_watch()
