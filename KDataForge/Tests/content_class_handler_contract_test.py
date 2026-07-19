"""Static contract guard for the KDataForge content-class handler.

This deliberately has no Unreal runtime dependency: it guards the public YAML surface and the
required mutation paths until the plugin's Unreal automation suite covers these flows directly.
Run with the repository Python interpreter.
"""

from pathlib import Path

SOURCE = Path(__file__).parents[1] / 'Source/KDataForge/Private/Handlers/KDFContentClassHandler.cpp'
HEADER = Path(__file__).parents[1] / 'Source/KDataForge/Public/Handlers/KDFContentClassHandler.h'
WEB_DOCS = Path(r'C:\Users\Administrator\Desktop\claude-workspace\web-applications\frontend\nuxt\layers\documentation\content\en\3.kdataforge')


def require(text: str, expected: str) -> None:
    assert expected in text, f'Missing required content-handler contract: {expected}'


def test_schematic_shortcuts_are_first_class_and_keep_generic_unlocks() -> None:
    source = SOURCE.read_text(encoding='utf-8')
    header = HEADER.read_text(encoding='utf-8')

    for shortcut in ('recipes', 'schematics', 'handSlots', 'inventorySlots', 'scannableResources'):
        require(source, f'TEXT("{shortcut}")')

    require(source, 'ApplyClassListShortcutUnlock')
    require(source, 'ApplyCountShortcutUnlock')
    require(source, 'ApplyScannableResourcesShortcutUnlock')
    require(source, 'ApplyInstancedObjects(TargetCDO, *Unlocks, TEXT("mUnlocks")')
    require(header, 'ApplyClassListShortcutUnlock')


def test_research_nodes_can_link_to_existing_parents_and_remove_all_references() -> None:
    source = SOURCE.read_text(encoding='utf-8')
    header = HEADER.read_text(encoding='utf-8')

    require(source, 'IndexExistingResearchNodes')
    require(source, 'RemoveNodesBySchematic')
    require(source, 'TEXT("addNodes")')
    require(source, 'TEXT("removeNodes")')
    require(source, 'Entry.GetBool(TEXT("autoPath"), false)')
    require(source, 'TEXT("Parents")')
    require(source, 'TEXT("NodesToUnhide")')
    require(source, 'TEXT("UnhiddenBy")')
    require(source, 'TEXT("ChildrenAndRoads")')
    require(header, 'RemoveNodesBySchematic')

    research_block = source[source.index('if (bSupportsNodes)'):source.index('EKDFContentRegKind RegistrationKind')]
    assert research_block.index('RemoveNodesBySchematic') < research_block.index('Entry.Find(TEXT("addNodes"))')
    require(research_block, 'ApplyNodes(TargetCDO, *AddNodes, Entry.GetBool(TEXT("autoPath"), false), Context)')
    require(research_block, "Both 'addNodes' and legacy 'nodes' are present; ignoring 'nodes'")


def test_auto_research_links_follow_vanilla_grid_contract() -> None:
    source = SOURCE.read_text(encoding='utf-8')

    require(source, 'BuildAutoRoadPoints')
    road_builder = source[source.index('BuildAutoRoadPoints'):source.index('WriteChildrenAndRoadsMap')]
    require(road_builder, 'while (Cursor.X != Child.X)')
    require(road_builder, 'while (Cursor.Y != Child.Y)')
    assert 'Points.Add(Parent)' not in road_builder

    require(source, 'AutoUnhideEdgesByTarget')
    require(source, 'AppendUniqueGridPoints')


def test_documentation_exposes_the_new_yaml_contract() -> None:
    schematics = (WEB_DOCS / '3.features/14.schematics.md').read_text(encoding='utf-8')
    research = (WEB_DOCS / '3.features/15.research-trees.md').read_text(encoding='utf-8')
    schema = (WEB_DOCS / '4.reference/2.yaml-schema.md').read_text(encoding='utf-8')

    for shortcut in ('recipes:', 'schematics:', 'handSlots:', 'inventorySlots:', 'scannableResources:'):
        require(schematics, shortcut)
    for reference in ('addNodes:', 'removeNodes:', 'autoPath:', 'already present', 'ChildrenAndRoads'):
        require(research, reference)
    require(schema, 'Schematic shortcut unlocks')


if __name__ == '__main__':
    test_schematic_shortcuts_are_first_class_and_keep_generic_unlocks()
    test_research_nodes_can_link_to_existing_parents_and_remove_all_references()
    test_auto_research_links_follow_vanilla_grid_contract()
    test_documentation_exposes_the_new_yaml_contract()
