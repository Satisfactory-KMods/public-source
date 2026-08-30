"""Create and wire the four KPCL delivery widget Blueprint bases.

Run in a live editor through Unreal Python remote execution:
    python run_in_editor.py Mods/GameFeatures/KPrivateCodeLib/Scripts/UnrealPy/configure_delivery_widgets.py

Existing Tree and Node prototypes are preserved. Missing widgets and bindings are
added idempotently so the script can be run again after native class changes.
"""

import unreal


ASSET_DIR = "/KPrivateCodeLib/Hooks/DeliveryWidgets"
TREE_PATH = f"{ASSET_DIR}/BPW_DeliveryTree"
NODE_PATH = f"{ASSET_DIR}/BPW_DeliveryNode"
QUEUE_PATH = f"{ASSET_DIR}/BPW_DeliveryQueue"
PREVIEW_PATH = f"{ASSET_DIR}/BPW_DeliveryPreview"
SUBSYSTEM_PATH = "/KPrivateCodeLib/SubSystem/BP_DeliverTaskSubsystem"

CREATED_ASSETS = []
UPDATED_ASSETS = []


def log(message):
    unreal.log(f"[DeliveryWidgets] {message}")


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def load_widget_blueprint(asset_path):
    asset = unreal.load_asset(asset_path)
    if not asset:
        return None
    if asset.get_class().get_name() != "WidgetBlueprint":
        raise RuntimeError(f"{asset_path} is {asset.get_class().get_name()}, expected WidgetBlueprint")
    return asset


def verify_parent(widget_blueprint, parent_class, asset_path):
    generated_class = require(widget_blueprint.generated_class(), f"{asset_path} has no generated class")
    cdo = unreal.get_default_object(generated_class)
    if not isinstance(cdo, parent_class):
        raise RuntimeError(f"{asset_path} does not inherit {parent_class.__name__}")


def create_or_load_widget_blueprint(asset_path, parent_class):
    widget_blueprint = load_widget_blueprint(asset_path)
    if widget_blueprint:
        verify_parent(widget_blueprint, parent_class, asset_path)
        return widget_blueprint

    asset_name = asset_path.rsplit("/", 1)[-1]
    package_path = asset_path.rsplit("/", 1)[0]
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    factory.set_editor_property("edit_after_new", False)
    widget_blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name,
        package_path,
        unreal.WidgetBlueprint,
        factory,
    )
    require(widget_blueprint, f"Failed to create {asset_path}")
    if widget_blueprint.get_class().get_name() != "WidgetBlueprint":
        raise RuntimeError(
            f"Factory created {widget_blueprint.get_class().get_name()} for {asset_path}, expected WidgetBlueprint"
        )
    CREATED_ASSETS.append(asset_path)
    log(f"created {asset_path} from {parent_class.__name__}")
    return widget_blueprint


def find_widget(widget_blueprint, widget_name):
    return unreal.EditorUtilityLibrary.find_source_widget_by_name(widget_blueprint, unreal.Name(widget_name))


def ensure_widget(widget_blueprint, widget_class, widget_name, parent_name=None):
    widget = find_widget(widget_blueprint, widget_name)
    if widget:
        expected_class_name = widget_class.__name__ if isinstance(widget_class, type) else widget_class.get_name()
        if widget.get_class().get_name() != expected_class_name:
            raise RuntimeError(
                f"{widget_blueprint.get_name()}.{widget_name} is {widget.get_class().get_name()}, "
                f"expected {expected_class_name}"
            )
        return widget

    parent = unreal.Name(parent_name) if parent_name else unreal.Name()
    widget = unreal.EditorUtilityLibrary.add_source_widget(
        widget_blueprint,
        widget_class,
        unreal.Name(widget_name),
        parent,
    )
    require(widget, f"Failed to add {widget_name} to {widget_blueprint.get_name()}")
    log(f"added {widget_blueprint.get_name()}.{widget_name}")
    return widget


def set_text(widget, text):
    try:
        widget.set_text(unreal.Text(text))
    except Exception:
        widget.set_editor_property("text", unreal.Text(text))


def set_overlay_alignment(widget, horizontal, vertical):
    slot = widget.get_editor_property("slot")
    if slot and slot.get_class().get_name() == "OverlaySlot":
        slot.set_editor_property("horizontal_alignment", horizontal)
        slot.set_editor_property("vertical_alignment", vertical)


def compile_and_save(widget_blueprint, asset_path):
    widget_blueprint.modify(True)
    unreal.BlueprintEditorLibrary.compile_blueprint(widget_blueprint)
    require(
        unreal.EditorAssetLibrary.save_loaded_asset(widget_blueprint, only_if_is_dirty=False),
        f"Failed to save {asset_path}",
    )
    if asset_path not in CREATED_ASSETS:
        UPDATED_ASSETS.append(asset_path)


def configure_queue_widget(queue_blueprint):
    root = ensure_widget(queue_blueprint, unreal.Border, "mQueueRoot")
    root.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)

    ensure_widget(queue_blueprint, unreal.VerticalBox, "mQueueContent", "mQueueRoot")
    title = ensure_widget(queue_blueprint, unreal.TextBlock, "mQueueTitle", "mQueueContent")
    set_text(title, "Delivery Queue")
    ensure_widget(queue_blueprint, unreal.VerticalBox, "mQueueEntriesPanel", "mQueueContent")
    empty_text = ensure_widget(queue_blueprint, unreal.TextBlock, "mQueueEmptyText", "mQueueContent")
    set_text(empty_text, "No delivery tasks queued")


def configure_preview_widget(preview_blueprint):
    root = ensure_widget(preview_blueprint, unreal.Border, "mPreviewRoot")
    root.set_editor_property("visibility", unreal.SlateVisibility.VISIBLE)

    ensure_widget(preview_blueprint, unreal.VerticalBox, "mPreviewContent", "mPreviewRoot")
    title = ensure_widget(preview_blueprint, unreal.TextBlock, "mPreviewTitle", "mPreviewContent")
    set_text(title, "Delivery Preview")
    ensure_widget(preview_blueprint, unreal.Image, "mPreviewIcon", "mPreviewContent")
    description = ensure_widget(preview_blueprint, unreal.TextBlock, "mPreviewDescription", "mPreviewContent")
    set_text(description, "Hover a delivery node")
    progress = ensure_widget(preview_blueprint, unreal.ProgressBar, "mPreviewProgress", "mPreviewContent")
    progress.set_editor_property("percent", 0.0)
    ensure_widget(preview_blueprint, unreal.Button, "mQueueButton", "mPreviewContent")
    button_label = ensure_widget(preview_blueprint, unreal.TextBlock, "mQueueButtonLabel", "mQueueButton")
    set_text(button_label, "Add to queue")


def configure_tree_widget(tree_blueprint, node_blueprint, queue_blueprint, preview_blueprint):
    node_canvas = find_widget(tree_blueprint, "mNodeCanvas")
    if not node_canvas:
        ensure_widget(tree_blueprint, unreal.Overlay, "mDeliveryTreeRoot")
        node_canvas = ensure_widget(tree_blueprint, unreal.CanvasPanel, "mNodeCanvas", "mDeliveryTreeRoot")

    parent_widget = node_canvas.get_parent()
    parent_name = parent_widget.get_name() if parent_widget else node_canvas.get_name()

    queue_widget = ensure_widget(
        tree_blueprint,
        queue_blueprint.generated_class(),
        "mDeliveryQueueWidget",
        parent_name,
    )
    preview_widget = ensure_widget(
        tree_blueprint,
        preview_blueprint.generated_class(),
        "mDeliveryPreviewWidget",
        parent_name,
    )
    set_overlay_alignment(
        queue_widget,
        unreal.HorizontalAlignment.H_ALIGN_RIGHT,
        unreal.VerticalAlignment.V_ALIGN_TOP,
    )
    set_overlay_alignment(
        preview_widget,
        unreal.HorizontalAlignment.H_ALIGN_RIGHT,
        unreal.VerticalAlignment.V_ALIGN_BOTTOM,
    )

    tree_blueprint.modify(True)
    unreal.BlueprintEditorLibrary.compile_blueprint(tree_blueprint)
    tree_cdo = unreal.get_default_object(tree_blueprint.generated_class())
    tree_cdo.set_editor_property("m_delivery_node_widget_class", node_blueprint.generated_class())


def configure_subsystem(tree_blueprint):
    subsystem_blueprint = require(unreal.load_asset(SUBSYSTEM_PATH), f"Missing {SUBSYSTEM_PATH}")
    subsystem_blueprint.modify(True)
    unreal.BlueprintEditorLibrary.compile_blueprint(subsystem_blueprint)
    subsystem_cdo = unreal.get_default_object(subsystem_blueprint.generated_class())
    subsystem_cdo.modify(True)
    subsystem_cdo.set_editor_property("m_delivery_tree_widget_class", tree_blueprint.generated_class())
    require(
        unreal.EditorAssetLibrary.save_loaded_asset(subsystem_blueprint, only_if_is_dirty=False),
        f"Failed to save {SUBSYSTEM_PATH}",
    )
    UPDATED_ASSETS.append(SUBSYSTEM_PATH)


def verify_source_widgets(widget_blueprint, expected_widgets):
    for widget_name, expected_class_name in expected_widgets.items():
        widget = require(
            find_widget(widget_blueprint, widget_name),
            f"{widget_blueprint.get_name()} is missing {widget_name}",
        )
        actual_class_name = widget.get_class().get_name()
        if actual_class_name != expected_class_name:
            raise RuntimeError(
                f"{widget_blueprint.get_name()}.{widget_name} is {actual_class_name}, expected {expected_class_name}"
            )


def verify_native_functions(widget_blueprint, function_names):
    cdo = unreal.get_default_object(widget_blueprint.generated_class())
    missing = [function_name for function_name in function_names if not hasattr(cdo, function_name)]
    if missing:
        raise RuntimeError(f"{widget_blueprint.get_name()} is missing native functions {missing}")


def verify_configuration(tree_blueprint, node_blueprint, queue_blueprint, preview_blueprint):
    verify_parent(tree_blueprint, unreal.KPCLDeliveryTreeWidget, TREE_PATH)
    verify_parent(node_blueprint, unreal.KPCLDeliverNodeWidget, NODE_PATH)
    verify_parent(queue_blueprint, unreal.KPCLDeliveryQueueWidget, QUEUE_PATH)
    verify_parent(preview_blueprint, unreal.KPCLDeliveryPreviewWidget, PREVIEW_PATH)

    verify_source_widgets(
        tree_blueprint,
        {
            "mNodeCanvas": "CanvasPanel",
            "mDeliveryQueueWidget": "BPW_DeliveryQueue_C",
            "mDeliveryPreviewWidget": "BPW_DeliveryPreview_C",
        },
    )
    verify_source_widgets(
        node_blueprint,
        {
            "mButton": "Button",
            "mDeliveryIcon": "Image",
            "mProgressBar": "KUI_ProgressBar_V1_C",
            "mLockedOverlay": "Overlay",
            "mFinishedOverlay": "Overlay",
        },
    )
    verify_source_widgets(
        queue_blueprint,
        {
            "mQueueRoot": "Border",
            "mQueueContent": "VerticalBox",
            "mQueueEntriesPanel": "VerticalBox",
            "mQueueEmptyText": "TextBlock",
        },
    )
    verify_source_widgets(
        preview_blueprint,
        {
            "mPreviewRoot": "Border",
            "mPreviewContent": "VerticalBox",
            "mPreviewIcon": "Image",
            "mPreviewDescription": "TextBlock",
            "mPreviewProgress": "ProgressBar",
            "mQueueButton": "Button",
        },
    )

    verify_native_functions(
        tree_blueprint,
        ("setup_delivery_tree", "refresh_delivery_tree", "synchronize_delivery_auxiliary_widgets"),
    )
    verify_native_functions(
        node_blueprint,
        ("setup_delivery_node", "refresh_delivery_node", "try_queue_task"),
    )
    verify_native_functions(
        queue_blueprint,
        ("setup_delivery_queue", "refresh_delivery_queue", "try_remove_queue_entry", "try_move_queue_entry"),
    )
    verify_native_functions(
        preview_blueprint,
        ("setup_delivery_preview", "refresh_delivery_preview", "try_queue_previewed_task"),
    )

    tree_cdo = unreal.get_default_object(tree_blueprint.generated_class())
    if tree_cdo.get_editor_property("m_delivery_node_widget_class") != node_blueprint.generated_class():
        raise RuntimeError("BPW_DeliveryTree does not use BPW_DeliveryNode")

    subsystem_blueprint = require(unreal.load_asset(SUBSYSTEM_PATH), f"Missing {SUBSYSTEM_PATH}")
    subsystem_cdo = unreal.get_default_object(subsystem_blueprint.generated_class())
    if subsystem_cdo.get_editor_property("m_delivery_tree_widget_class") != tree_blueprint.generated_class():
        raise RuntimeError("BP_DeliverTaskSubsystem does not use BPW_DeliveryTree")

    log("verification passed: parents, WidgetTree bindings, native functions, and subsystem class")


def main():
    tree_blueprint = require(load_widget_blueprint(TREE_PATH), f"Missing existing prototype {TREE_PATH}")
    node_blueprint = require(load_widget_blueprint(NODE_PATH), f"Missing existing prototype {NODE_PATH}")
    verify_parent(tree_blueprint, unreal.KPCLDeliveryTreeWidget, TREE_PATH)
    verify_parent(node_blueprint, unreal.KPCLDeliverNodeWidget, NODE_PATH)

    queue_blueprint = create_or_load_widget_blueprint(QUEUE_PATH, unreal.KPCLDeliveryQueueWidget)
    preview_blueprint = create_or_load_widget_blueprint(PREVIEW_PATH, unreal.KPCLDeliveryPreviewWidget)

    configure_queue_widget(queue_blueprint)
    configure_preview_widget(preview_blueprint)
    compile_and_save(queue_blueprint, QUEUE_PATH)
    compile_and_save(preview_blueprint, PREVIEW_PATH)

    configure_tree_widget(tree_blueprint, node_blueprint, queue_blueprint, preview_blueprint)
    compile_and_save(tree_blueprint, TREE_PATH)
    configure_subsystem(tree_blueprint)
    verify_configuration(tree_blueprint, node_blueprint, queue_blueprint, preview_blueprint)

    log(f"created={CREATED_ASSETS}")
    log(f"updated={UPDATED_ASSETS}")
    log("configuration complete")


main()
