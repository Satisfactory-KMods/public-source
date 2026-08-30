"""Export FGUnlock and FGAvailabilityDependency descendants from Unreal Asset Registry.

Run inside Unreal Python, not CPython:

    UnrealEditor-Cmd.exe FactoryGame.uproject -run=pythonscript \
      -script=Mods/GameFeatures/KDataForge/Tools/scan_instanced_class_catalog.py

Set ``KDF_CLASS_CATALOG_OUTPUT`` to override output. Default:
``Saved/KDataForge/instanced-class-catalog.json``.
"""

from __future__ import annotations

import json
import os
import re
from pathlib import Path

import unreal

FAMILIES = {
    "unlock": "/Script/FactoryGame.FGUnlock",
    "dependency": "/Script/FactoryGame.FGAvailabilityDependency",
}
SKIP_DIRS = {
    ".git",
    "Binaries",
    "DerivedDataCache",
    "Intermediate",
    "node_modules",
    "OLD",
    "Saved",
}
CLASS_DECL = re.compile(
    r"(?P<uclass>UCLASS\s*\([^\r\n]*\))\s*"
    r"class\s+(?:[A-Z0-9_]+_API\s+)?(?P<child>[UA]\w+)\s*:\s*public\s+(?P<parent>[UA]\w+)",
    re.MULTILINE,
)


def _class_path(value: unreal.TopLevelAssetPath) -> str:
    return f"{value.package_name}.{value.asset_name}"


def _export_path(value: str | None) -> str | None:
    if value and "'" in value:
        return value.split("'", 2)[1]
    return value


def _module_for_header(path: Path) -> str:
    for marker in ("Public", "Private"):
        if marker in path.parts:
            index = path.parts.index(marker)
            if index > 0:
                return path.parts[index - 1]
    return "Unknown"


def _native_inventory(project_root: Path) -> dict[str, dict[str, object]]:
    declarations: dict[str, dict[str, object]] = {}
    for source_root in (project_root / "Source", project_root / "Mods"):
        for dirpath, dirnames, filenames in os.walk(source_root):
            dirnames[:] = [name for name in dirnames if name not in SKIP_DIRS]
            for filename in filenames:
                if not filename.endswith(".h"):
                    continue
                path = Path(dirpath) / filename
                text = path.read_text(encoding="utf-8", errors="ignore")
                module = _module_for_header(path)
                for match in CLASS_DECL.finditer(text):
                    cpp_name = match.group("child")
                    declarations[cpp_name] = {
                        "cppName": cpp_name,
                        "name": cpp_name[1:],
                        "path": f"/Script/{module}.{cpp_name[1:]}",
                        "parentCppName": match.group("parent"),
                        "abstract": bool(re.search(r"\babstract\b", match.group("uclass"), re.IGNORECASE)),
                        "source": module,
                        "sourceFile": path.relative_to(project_root).as_posix(),
                    }
    for declaration in declarations.values():
        parent = declarations.get(str(declaration.pop("parentCppName")))
        declaration["parent"] = parent["path"] if parent else None
    return declarations


def _asset_registry_inventory() -> tuple[dict[str, set[str]], dict[str, dict[str, str]]]:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    derived: dict[str, set[str]] = {}
    for family, base_path in FAMILIES.items():
        base = unreal.load_class(None, base_path)
        if base is None:
            raise RuntimeError(f"Could not load {base_path}")
        derived[family] = {
            _class_path(value)
            for value in registry.get_derived_class_names({base.get_class_path_name()}, set())
        }

    blueprints: dict[str, dict[str, str]] = {}
    wanted = set().union(*derived.values())
    for asset in registry.get_all_assets():
        generated = _export_path(asset.get_tag_value("GeneratedClass"))
        if not generated or generated not in wanted:
            continue
        blueprints[generated] = {
            "path": generated,
            "parent": _export_path(asset.get_tag_value("ParentClass")) or "",
            "source": str(asset.package_name).strip("/").split("/", 1)[0],
            "asset": str(asset.package_name),
        }
    return derived, blueprints


def build_catalog(project_root: Path) -> dict[str, object]:
    native_by_cpp = _native_inventory(project_root)
    native_by_path = {str(item["path"]): item for item in native_by_cpp.values()}
    derived, blueprints = _asset_registry_inventory()

    families: dict[str, object] = {}
    for family, base_path in FAMILIES.items():
        paths = derived[family]
        blueprint_by_parent: dict[str, list[str]] = {}
        for path, blueprint in blueprints.items():
            if path in paths and blueprint["parent"]:
                blueprint_by_parent.setdefault(blueprint["parent"], []).append(path)

        entries: list[dict[str, object]] = []
        for path in sorted(paths):
            native = native_by_path.get(path)
            if native is not None:
                variants = sorted(blueprint_by_parent.get(path, []))
                entries.append(
                    {
                        **native,
                        "family": family,
                        # A direct Blueprint of the family root is a standalone implementation,
                        # not a safe default for every reference to the abstract root.
                        "blueprint": variants[0] if variants and path != base_path else None,
                        "blueprints": variants,
                    }
                )
                continue
            blueprint = blueprints.get(path)
            if blueprint is not None:
                entries.append(
                    {
                        "family": family,
                        "name": path.rsplit(".", 1)[-1],
                        "path": path,
                        "parent": blueprint["parent"],
                        "abstract": False,
                        "blueprint": None,
                        "blueprints": [],
                        "source": blueprint["source"],
                        "sourceFile": blueprint["asset"],
                    }
                )
                continue
            unreal.log_warning(f"Catalog path lacks native header and Blueprint AssetData: {path}")

        families[family] = {"base": base_path, "entries": entries}

    return {
        "schemaVersion": 1,
        "engineVersion": unreal.SystemLibrary.get_engine_version(),
        "families": families,
    }


project_root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
default_output = project_root / "Saved" / "KDataForge" / "instanced-class-catalog.json"
output = Path(os.environ.get("KDF_CLASS_CATALOG_OUTPUT", str(default_output)))
output.parent.mkdir(parents=True, exist_ok=True)
catalog = build_catalog(project_root)
output.write_text(json.dumps(catalog, indent=2) + "\n", encoding="utf-8")
unreal.log(f"KDataForge class catalog: {output}")
for family, value in catalog["families"].items():
    unreal.log(f"KDataForge class catalog: {family}={len(value['entries'])}")
