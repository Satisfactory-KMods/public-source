"""Compatibility entry point for instanced-class catalog scan.

Run inside Unreal Python. New code should invoke
``scan_instanced_class_catalog.py`` directly; this filename remains for existing
documentation and scripts.
"""

from __future__ import annotations

import runpy
from pathlib import Path

try:
    import unreal  # noqa: F401
except ImportError as error:
    raise SystemExit(
        "scan_unlock_catalog.py requires Unreal Python; use UnrealEditor-Cmd with "
        "scan_instanced_class_catalog.py"
    ) from error

runpy.run_path(str(Path(__file__).with_name("scan_instanced_class_catalog.py")), run_name="__main__")
