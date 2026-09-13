"""Run with UE 5.6.1-CSS -run=pythonscript -script=<this file>."""

import json
from pathlib import Path

import unreal


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / ".validation" / "blueprints.json"


def main():
    migrate_widget_urls = "-sbsmigratelegacyimageurls" in unreal.SystemLibrary.get_command_line().lower().split()
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(["/SBS"], force_rescan=True)

    feature = unreal.load_asset("/SBS/SBS")
    if feature is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.FGGameFeatureData)
        feature = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "SBS", "/SBS", unreal.FGGameFeatureData, factory
        )
        if feature is None:
            raise RuntimeError("Could not create /SBS/SBS GameFeature data")
        if not unreal.EditorAssetLibrary.save_loaded_asset(feature):
            raise RuntimeError("Could not save SBS GameFeature data")

    if not isinstance(feature, unreal.FGGameFeatureData):
        raise RuntimeError("/SBS/SBS must be FGGameFeatureData")

    report = []
    failures = []
    # Load the complete graph before compiling any root module to avoid nested compile requests.
    assets = [(data, data.get_asset()) for data in registry.get_assets_by_path("/SBS", recursive=True)]
    for data, asset in assets:
        path = str(data.package_name)
        if asset is None:
            failures.append(path + ": could not load")
            continue
        if isinstance(asset, unreal.Blueprint):
            migrated_urls = unreal.SBSBlueprintMigrationLibrary.migrate_image_urls(asset) if migrate_widget_urls else 0
            if migrated_urls < 0:
                raise RuntimeError("Image URL migration failed: " + path)
            result = json.loads(unreal.SBSBlueprintMigrationLibrary.inspect_blueprint(asset))
            status = result["status"]
            report.append(result)
            unreal.log("SBS_BLUEPRINT " + path + " " + status)
            if result["errors"] or status == "error":
                failures.append(path + ": " + status)
            elif migrated_urls:
                if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
                    raise RuntimeError("Could not save migrated image URL graph: " + path)
                unreal.log(f"SBS_IMAGE_URL_MIGRATED {path} pins={migrated_urls}")

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps({"blueprints": report, "failures": failures}, indent=2), encoding="utf-8")
    native = json.loads(unreal.SBSBlueprintMigrationLibrary.validate_native_data())
    (REPORT.parent / "native-data.json").write_text(json.dumps(native, indent=2), encoding="utf-8")
    if native["failures"]:
        raise RuntimeError("SBS data validation failed: " + "; ".join(native["failures"]))
    unreal.log(f"SBS_NATIVE_DATA_VALIDATION_PASSED checks={native['checks']}")
    sharing = json.loads(unreal.SBSBlueprintMigrationLibrary.validate_sharing_data())
    (REPORT.parent / "sharing-data.json").write_text(json.dumps(sharing, indent=2), encoding="utf-8")
    if sharing["failures"]:
        raise RuntimeError("SBS sharing validation failed: " + "; ".join(sharing["failures"]))
    unreal.log(f"SBS_SHARING_VALIDATION_PASSED checks={sharing['checks']} http={sharing['loopbackHttpTested']}")
    download_ids = json.loads(unreal.SBSBlueprintMigrationLibrary.validate_download_id_data())
    (REPORT.parent / "download-id-data.json").write_text(json.dumps(download_ids, indent=2), encoding="utf-8")
    if download_ids["failures"]:
        raise RuntimeError("SBS download ID validation failed: " + "; ".join(download_ids["failures"]))
    unreal.log(f"SBS_DOWNLOAD_ID_VALIDATION_PASSED checks={download_ids['checks']} http={download_ids['loopbackHttpTested']}")
    if failures:
        raise RuntimeError("SBS Blueprint failures: " + "; ".join(failures))
    if len(report) < 17:
        raise RuntimeError(f"Only {len(report)} SBS Blueprints loaded; inventory incomplete")
    unreal.log(f"SBS_BLUEPRINT_VALIDATION_PASSED count={len(report)}")


main()
