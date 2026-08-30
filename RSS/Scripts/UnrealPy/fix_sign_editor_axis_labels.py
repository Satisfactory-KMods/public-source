"""Fix swapped axis labels in RSS element editor WidgetBlueprint.

Run against live Unreal Editor:
    python run_via_remote.py --script C:/Modding/Mods/GameFeatures/RSS/Scripts/UnrealPy/fix_sign_editor_axis_labels.py

Safety: refuses mutation when target package is already dirty and saves only
``Rss_WC_ElementBuilder``. Sign rendering blueprints are never loaded or saved.
"""

import json
import os
import traceback

import unreal


ASSET_PATH = "/RSS/Interface/WidgetComp/Rss_WC_ElementBuilder"
OUTPUT_PATH = "C:/Modding/exports/rss-axis-label-fix.json"
LABELS = {
    "TextBlock_0": "Horizontal",
    "TextBlock": "Vertical",
}
PROTECTED_ASSETS = (
    "/RSS/Buildable/-Shared/BP_Bases/BP_SignComponentWidget_Base",
    "/RSS/Buildable/-Shared/BP_Bases/BP_SignWidget_General",
)


def _string(value):
    if value is None:
        return None
    try:
        return str(value)
    except Exception:
        return repr(value)


def _object_path(value):
    if value is None:
        return None
    try:
        return value.get_path_name()
    except Exception:
        return _string(value)


def _package_is_dirty(package):
    is_dirty = getattr(package, "is_dirty", None)
    if callable(is_dirty):
        return bool(is_dirty())

    try:
        package_path = _object_path(package)
        dirty_packages = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
        return any(_object_path(candidate) == package_path for candidate in dirty_packages)
    except Exception:
        return None


def _find_text_blocks(blueprint):
    widgets = {}
    for widget_name in LABELS:
        widget = unreal.EditorUtilityLibrary.find_source_widget_by_name(
            blueprint, unreal.Name(widget_name)
        )
        if widget is None:
            raise RuntimeError(f"Missing source widget: {widget_name}")
        if widget.get_class().get_name() != "TextBlock":
            raise RuntimeError(
                f"Unexpected widget class for {widget_name}: {widget.get_class().get_name()}"
            )
        widgets[widget_name] = widget
    return widgets


def _read_labels(widgets):
    return {
        widget_name: _string(widget.get_editor_property("text"))
        for widget_name, widget in widgets.items()
    }


def main():
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    payload = {
        "script": __file__,
        "asset": ASSET_PATH,
        "protected_assets": list(PROTECTED_ASSETS),
        "protected_assets_saved": [],
        "expected": dict(LABELS),
        "mutated": False,
        "compiled": False,
        "saved": False,
        "status": "blocked",
        "errors": [],
    }

    try:
        blueprint = unreal.load_asset(ASSET_PATH)
        if blueprint is None:
            raise RuntimeError(f"Missing WidgetBlueprint: {ASSET_PATH}")

        package = blueprint.get_outermost()
        widgets = _find_text_blocks(blueprint)
        payload["package"] = _object_path(package)
        payload["labels_before"] = _read_labels(widgets)
        payload["package_dirty_before"] = _package_is_dirty(package)

        if payload["package_dirty_before"] is None:
            payload["blocked_reason"] = "Could not verify target package dirty state"
        elif payload["package_dirty_before"]:
            payload["blocked_reason"] = "Target package already dirty; no mutation or save attempted"
        else:
            if payload["labels_before"] != LABELS:
                with unreal.ScopedEditorTransaction("Fix RSS sign editor axis labels"):
                    for widget_name, expected_text in LABELS.items():
                        widgets[widget_name].set_editor_property("text", expected_text)
                payload["mutated"] = True

                unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
                payload["compiled"] = True
                payload["compile_status"] = "compile_blueprint completed without exception"

                payload["saved"] = bool(
                    unreal.EditorAssetLibrary.save_asset(ASSET_PATH, only_if_is_dirty=False)
                )
                if not payload["saved"]:
                    raise RuntimeError(f"Failed to save asset: {ASSET_PATH}")

            payload["labels_after"] = _read_labels(widgets)
            payload["package_dirty_after"] = _package_is_dirty(package)
            payload["verified"] = payload["labels_after"] == LABELS
            if not payload["verified"]:
                raise RuntimeError(
                    f"Saved label verification failed: {payload['labels_after']}"
                )
            if payload["package_dirty_after"] is True:
                raise RuntimeError("Target package remained dirty after save")
            payload["status"] = "passed"
    except Exception as error:
        payload["errors"].append(
            {
                "message": str(error),
                "traceback": traceback.format_exc(),
            }
        )

    with open(OUTPUT_PATH, "w", encoding="utf-8") as output_file:
        json.dump(payload, output_file, indent=2, ensure_ascii=False)

    log = unreal.log if payload["status"] == "passed" else unreal.log_warning
    log(f"[RSS Axis Label Fix] status={payload['status']} wrote {OUTPUT_PATH}")
    if payload.get("blocked_reason"):
        unreal.log_warning(f"[RSS Axis Label Fix] {payload['blocked_reason']}")
    for error in payload["errors"]:
        unreal.log_error(f"[RSS Axis Label Fix] {error['message']}")

    if payload["status"] != "passed":
        detail = payload.get("blocked_reason") or "; ".join(
            error["message"] for error in payload["errors"]
        )
        raise RuntimeError(f"RSS axis-label fix {payload['status']}: {detail}")


main()
