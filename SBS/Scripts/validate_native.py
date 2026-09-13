"""Editor commandlet checks without compiling, migrating or saving the maintainer's widgets."""

import json
from pathlib import Path

import unreal


root = Path(__file__).resolve().parents[1] / ".validation"
root.mkdir(parents=True, exist_ok=True)
for name, run in [
    ("native-data", unreal.SBSBlueprintMigrationLibrary.validate_native_data),
    ("sharing-data", unreal.SBSBlueprintMigrationLibrary.validate_sharing_data),
    ("download-id-data", unreal.SBSBlueprintMigrationLibrary.validate_download_id_data),
]:
    result = json.loads(run())
    (root / (name + ".json")).write_text(json.dumps(result, indent=2), encoding="utf-8")
    if result["failures"]:
        raise RuntimeError(name + ": " + "; ".join(result["failures"]))
    unreal.log(f"SBS_VALIDATION_PASSED {name} checks={result['checks']}")
