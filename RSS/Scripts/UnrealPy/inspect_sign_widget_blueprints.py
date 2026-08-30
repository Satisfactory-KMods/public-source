"""Dump RSS sign WidgetBlueprint structure and K2 graphs for native migration work.

Run against a live editor:
    python run_via_remote.py --script C:/Modding/Mods/GameFeatures/RSS/Scripts/UnrealPy/inspect_sign_widget_blueprints.py

Output is diagnostic only and lands in ``C:/Modding/exports``.
"""

import json
import os

import unreal


ASSET_PATHS = (
    "/RSS/Buildable/-Shared/BP_Bases/BP_SignWidget_General",
    "/RSS/Buildable/-Shared/BP_Bases/BP_SignComponentWidget_Base",
    "/RSS/Buildable/-Shared/BP_Bases/BP_WC_SignElement_Base",
    "/RSS/Buildable/-Shared/BP_Bases/BP_WC_SignWidget_Text",
    "/RSS/Interface/WidgetComp/Rss_WC_ElementBuilder",
    "/RSS/Interface/WidgetComp/Rss_WC_SignSettings",
    "/RSS/Interface/Widget_Rss2",
)
OUTPUT_PATH = "C:/Modding/exports/rss-sign-widget-blueprints.json"
T3D_OUTPUT_DIRECTORY = "C:/Modding/exports/rss-sign-widget-t3d"
EXPORT_ONLY_ASSET_PATHS = (
    "/RSS/Buildable/-Shared/BP_Bases/Materials/WidgetEffects/MM_MainEffect",
    "/RSS/Buildable/-Shared/BP_Bases/Materials/WidgetEffects/MI_GridEffect_Tile",
)

GRAPH_CANDIDATES = (
    "EventGraph",
    "OnBuildableSet",
    "OnSignDataUpdated",
    "OnRequestUpdateSign",
    "Create",
    "CreateSelection",
    "RemoveAllSelect",
    "CreateTextElement",
    "CreateImageElement",
    "CreateEffectElement",
    "UpdateElementByIndex",
    "UpdateTextElement",
    "UpdateImageElement",
    "UpdateEffectElement",
    "GetPosition",
    "GetDrawSize",
)

WIDGET_CANDIDATES = (
    "MOverlay",
    "mElementCanv",
    "mSelectCanvas",
    "mPreviewImage",
    "mPreviewMask",
    "mMainCanv",
    "mOutline",
    "mOutlineBorder",
    "mBackground",
    "mText",
    "SizeBox",
    "CanvasPanel",
    "Image",
    "Text",
)


def _string(value):
    if value is None:
        return None
    try:
        return str(value)
    except Exception:
        return repr(value)


def _property(obj, name, default=None):
    try:
        return obj.get_editor_property(name)
    except Exception:
        return default


def _object_path(obj):
    if obj is None:
        return None
    try:
        return obj.get_path_name()
    except Exception:
        return _string(obj)


def _pin_data(pin):
    linked_to = _property(pin, "linked_to", []) or []
    pin_type = _property(pin, "pin_type")
    return {
        "name": _string(_property(pin, "pin_name")),
        "direction": _string(_property(pin, "direction")),
        "default_value": _string(_property(pin, "default_value")),
        "default_object": _object_path(_property(pin, "default_object")),
        "category": _string(_property(pin_type, "pin_category")) if pin_type else None,
        "subcategory": _string(_property(pin_type, "pin_sub_category")) if pin_type else None,
        "subcategory_object": _object_path(_property(pin_type, "pin_sub_category_object")) if pin_type else None,
        "container_type": _string(_property(pin_type, "container_type")) if pin_type else None,
        "linked_to": [
            {
                "node": linked.get_outer().get_name() if linked and linked.get_outer() else None,
                "pin": _string(_property(linked, "pin_name")),
            }
            for linked in linked_to
        ],
    }


def _node_data(node):
    pins = _property(node, "pins", []) or []
    result = {
        "name": node.get_name(),
        "class": node.get_class().get_name(),
        "comment": _string(_property(node, "node_comment")),
        "x": _property(node, "node_pos_x"),
        "y": _property(node, "node_pos_y"),
        "pins": [_pin_data(pin) for pin in pins],
    }
    for property_name in (
        "function_reference",
        "event_reference",
        "custom_function_name",
        "delegate_reference",
        "variable_reference",
        "proxy_factory_function_name",
        "proxy_factory_class",
        "proxy_class",
    ):
        value = _property(node, property_name)
        if value is not None:
            result[property_name] = _object_path(value) if hasattr(value, "get_path_name") else _string(value)
    return result


def _graph_data(graph, graph_kind):
    try:
        nodes = graph.get_nodes()
    except Exception:
        nodes = _property(graph, "nodes", []) or []
    return {
        "name": graph.get_name(),
        "kind": graph_kind,
        "nodes": [_node_data(node) for node in nodes],
        "python_attributes": sorted(name for name in dir(graph) if not name.startswith("__")),
    }


def _slot_data(slot):
    if slot is None:
        return None
    result = {"class": slot.get_class().get_name()}
    for name in (
        "layout_data",
        "position",
        "size",
        "anchors",
        "alignment",
        "auto_size",
        "padding",
        "horizontal_alignment",
        "vertical_alignment",
        "z_order",
    ):
        value = _property(slot, name)
        if value is not None:
            result[name] = _string(value)
    return result


def _widget_data(widget):
    result = {
        "name": widget.get_name(),
        "class": widget.get_class().get_name(),
        "path": widget.get_path_name(),
        "slot": _slot_data(_property(widget, "slot")),
    }
    for name in (
        "visibility",
        "render_opacity",
        "render_transform",
        "desired_size_scale",
        "brush",
        "color_and_opacity",
        "text",
        "font",
        "justification",
        "line_height_percentage",
        "margin",
        "min_desired_width",
        "auto_wrap_text",
        "wrap_text_at",
        "width_override",
        "height_override",
    ):
        value = _property(widget, name)
        if value is not None:
            result[name] = _string(value)
    try:
        children_count = widget.get_children_count()
    except Exception:
        children_count = 0
    result["children"] = [_widget_data(widget.get_child_at(index)) for index in range(children_count)]
    return result


def _widget_tree_data(blueprint):
    widget_tree = _property(blueprint, "widget_tree")
    if widget_tree is None:
        return None
    root = _property(widget_tree, "root_widget")
    return _widget_data(root) if root else None


def _source_widgets_data(blueprint):
    widgets = []
    seen = set()
    for name in WIDGET_CANDIDATES:
        try:
            widget = unreal.EditorUtilityLibrary.find_source_widget_by_name(blueprint, unreal.Name(name))
        except Exception:
            widget = None
        if widget is None or widget.get_path_name() in seen:
            continue
        seen.add(widget.get_path_name())
        widgets.append(_widget_data(widget))
    return widgets


def _variable_data(variable):
    result = {}
    for name in ("var_name", "friendly_name", "category", "default_value", "property_flags"):
        value = _property(variable, name)
        if value is not None:
            result[name] = _string(value)
    var_type = _property(variable, "var_type")
    if var_type is not None:
        result["type"] = {
            "category": _string(_property(var_type, "pin_category")),
            "subcategory": _string(_property(var_type, "pin_sub_category")),
            "subcategory_object": _object_path(_property(var_type, "pin_sub_category_object")),
            "container_type": _string(_property(var_type, "container_type")),
        }
    return result


def _blueprint_data(asset_path):
    blueprint = unreal.load_asset(asset_path)
    if blueprint is None:
        raise RuntimeError(f"Missing asset: {asset_path}")
    generated_class = blueprint.generated_class()
    asset_data = unreal.EditorAssetLibrary.find_asset_data(asset_path)
    try:
        parent_class = generated_class.get_super_class()
    except Exception:
        parent_class = _property(blueprint, "parent_class")
    graphs = []
    for property_name, graph_kind in (
        ("ubergraph_pages", "ubergraph"),
        ("function_graphs", "function"),
        ("delegate_signature_graphs", "delegate"),
        ("macro_graphs", "macro"),
    ):
        for graph in (_property(blueprint, property_name, []) or []):
            graphs.append(_graph_data(graph, graph_kind))
    try:
        event_graph = unreal.BlueprintEditorLibrary.find_event_graph(blueprint)
    except Exception:
        event_graph = None
    if event_graph is not None:
        graphs.append(_graph_data(event_graph, "event"))
    known_graph_names = {graph["name"] for graph in graphs}
    for graph_name in GRAPH_CANDIDATES:
        try:
            graph = unreal.BlueprintEditorLibrary.find_graph(blueprint, unreal.Name(graph_name))
        except Exception:
            graph = None
        if graph is not None and graph.get_name() not in known_graph_names:
            graphs.append(_graph_data(graph, "found"))
            known_graph_names.add(graph.get_name())
    return {
        "asset_path": asset_path,
        "asset_class": blueprint.get_class().get_name(),
        "parent_class": _object_path(parent_class),
        "generated_class": _object_path(generated_class),
        "asset_registry": {
            "attributes": sorted(name for name in dir(asset_data) if not name.startswith("__")),
            "tags": {
                tag: _string(asset_data.get_tag_value(tag))
                for tag in ("ParentClass", "GeneratedClass", "NativeParentClass")
                if hasattr(asset_data, "get_tag_value")
            },
        },
        "variables": [_variable_data(variable) for variable in (_property(blueprint, "new_variables", []) or [])],
        "widget_tree": _widget_tree_data(blueprint),
        "source_widgets": _source_widgets_data(blueprint),
        "graphs": graphs,
        "compile_status": _string(_property(blueprint, "status")),
        "python_attributes": sorted(name for name in dir(blueprint) if not name.startswith("__")),
        "generated_class_attributes": sorted(name for name in dir(generated_class) if not name.startswith("__")),
        "cdo_attributes": sorted(
            name for name in dir(unreal.get_default_object(generated_class)) if not name.startswith("__")
        ),
    }


def main():
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    os.makedirs(T3D_OUTPUT_DIRECTORY, exist_ok=True)
    payload = {
        "assets": [_blueprint_data(path) for path in ASSET_PATHS],
        "editor_utility_library_attributes": sorted(
            name for name in dir(unreal.EditorUtilityLibrary) if not name.startswith("__")
        ),
        "blueprint_editor_library_attributes": sorted(
            name for name in dir(unreal.BlueprintEditorLibrary) if not name.startswith("__")
        ),
        "api_docs": {
            "find_event_graph": unreal.BlueprintEditorLibrary.find_event_graph.__doc__,
            "find_graph": unreal.BlueprintEditorLibrary.find_graph.__doc__,
            "find_source_widget_by_name": unreal.EditorUtilityLibrary.find_source_widget_by_name.__doc__,
        },
    }
    with open(OUTPUT_PATH, "w", encoding="utf-8") as output_file:
        json.dump(payload, output_file, indent=2, ensure_ascii=False)
    for asset_path in (*ASSET_PATHS, *EXPORT_ONLY_ASSET_PATHS):
        blueprint = unreal.load_asset(asset_path)
        export_task = unreal.AssetExportTask()
        export_task.object = blueprint
        export_task.filename = os.path.join(T3D_OUTPUT_DIRECTORY, f"{blueprint.get_name()}.t3d")
        export_task.automated = True
        export_task.prompt = False
        export_task.replace_identical = True
        if not unreal.Exporter.run_asset_export_task(export_task):
            raise RuntimeError(f"Failed to export {asset_path}")
    unreal.log(f"[RSS Sign Widget Inspect] wrote {OUTPUT_PATH}")
    for asset in payload["assets"]:
        node_count = sum(len(graph["nodes"]) for graph in asset["graphs"])
        unreal.log(
            f"[RSS Sign Widget Inspect] {asset['asset_path']}: "
            f"parent={asset['parent_class']} graphs={len(asset['graphs'])} nodes={node_count}"
        )
    unreal.log("[RSS Sign Widget Inspect] BYTECODE BEGIN")
    for class_name in ("BP_SignWidget_General_C", "BP_SignComponentWidget_Base_C", "Rss_WC_ElementBuilder_C"):
        unreal.SystemLibrary.execute_console_command(None, f"DISASMSCRIPT {class_name}")
    unreal.log("[RSS Sign Widget Inspect] BYTECODE END")


main()
