"""Check native UI text identities; optionally verify UE actually gathered every key."""

import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LITERAL = r'"(?:[^"\\]|\\.)*"'
ENTRY = re.compile(r'NSLOCTEXT\(\s*("SBS")\s*,\s*(' + LITERAL + r')\s*,\s*(' + LITERAL + r')\s*\)')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", action="store_true", help="Require current native keys in the generated manifest")
    args = parser.parse_args()
    expected, failures = {}, []
    for path in (ROOT / "Source/SBS").rglob("*"):
        if path.suffix not in (".h", ".cpp"):
            continue
        source = path.read_text(encoding="utf-8-sig")
        for _, key, text in ENTRY.findall(source):
            key, text = json.loads(key), json.loads(text)
            if key in expected and expected[key] != text:
                failures.append("Conflicting native localization source: " + key)
            expected[key] = text
        if "FText::FromString" in source:
            failures.append("Review runtime-created nonlocalizable FText: " + str(path.relative_to(ROOT)))
        if re.search(r'FSBSOperationResult::Make\(\s*TEXT\([^)]*\),\s*TEXT\(', source):
            failures.append("Operation message bypasses localization: " + str(path.relative_to(ROOT)))
    if not any(key.startswith("Sharing.") for key in expected) or not any(key.startswith("Api.") for key in expected):
        failures.append("Missing sharing/query localization identities")
    if args.manifest:
        data = (ROOT / "Content/Localization/SBS/SBS.manifest").read_bytes()
        manifest = json.loads(data.decode("utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"))
        gathered = {}

        def collect(node, parent=""):
            namespace = node.get("Namespace", parent)
            for child in node.get("Children", []):
                if namespace == "SBS":
                    for key in child.get("Keys", []):
                        gathered[key["Key"]] = child["Source"]["Text"]
            for child in node.get("Subnamespaces", []):
                collect(child, namespace)

        collect(manifest)
        for key, value in expected.items():
            if gathered.get(key) != value:
                failures.append("Missing/stale gathered native text: " + key)
    if failures:
        raise SystemExit("\n".join(failures))
    print(f"SBS localization verified: {len(expected)} native identities; manifest={args.manifest}")


if __name__ == "__main__":
    main()
