"""Compare the original reflected SBS surface and asset inventory with this worktree.

This source-level check complements compilation of all real Blueprint assets. It does
not establish runtime behavior or replace the Unreal compiler.
"""

import re
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BASELINE = "a5da1641aeec7917e07a98f8e2374af962380a7c"
CONTRACT = Path(__file__).with_name("blueprint_contract.json")
# Explicit maintainer-requested API removal; do not preserve or recreate this obsolete pin.
REMOVED_MEMBERS = {("FSBSUserData", "Role")}


def balanced(text, start):
    depth, quoted, escaped = 0, False, False
    for index in range(start, len(text)):
        char = text[index]
        if escaped:
            escaped = False
        elif char == "\\" and quoted:
            escaped = True
        elif char == '"':
            quoted = not quoted
        elif not quoted:
            if char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
                if not depth:
                    return index + 1
    raise ValueError("Unbalanced reflection declaration")


def normalize(text):
    text = re.sub(r"\b(FORCEINLINE|virtual|override)\b", "", text)
    return re.sub(r"\s+", "", text)


def surface(text):
    text = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
    result = {}
    sections = list(re.finditer(r"U(?:CLASS|STRUCT)\s*\([^)]*\)\s*(?:class|struct)\s+(?:\w+_API\s+)?(\w+)[^{]*\{", text))
    for number, section in enumerate(sections):
        name = section[1]
        end = sections[number + 1].start() if number + 1 < len(sections) else len(text)
        body = text[section.end():end]
        result[name] = {"declaration": normalize(section[0]), "members": {}}
        for match in re.finditer(r"U(FUNCTION|PROPERTY)\s*\(", body):
            meta_end = balanced(body, body.index("(", match.start()))
            metadata = body[match.start():meta_end]
            if match[1] == "PROPERTY" and "Blueprint" not in metadata:
                continue
            rest = body[meta_end:].lstrip()
            if match[1] == "FUNCTION":
                signature_end = balanced(rest, rest.index("("))
                declaration = rest[:signature_end]
                const = re.match(r"\s*const\b", rest[signature_end:])
                if const:
                    declaration += " const"
                member = re.search(r"(\w+)\s*\(", declaration)[1]
            else:
                declaration = rest.split(";", 1)[0].split("=", 1)[0].strip()
                member = re.search(r"(\w+)\s*$", declaration)[1]
            result[name]["members"][member] = normalize(metadata + declaration)
    return result


def delegates(text):
    result = {}
    for match in re.finditer(r"DECLARE_DYNAMIC_\w+\s*\(", text):
        start = text.index("(", match.start())
        end = balanced(text, start)
        declaration = text[match.start():end]
        name = text[start + 1:end - 1].split(",", 1)[0].strip()
        result[name] = normalize(declaration)
    return result


def main():
    baseline = json.loads(CONTRACT.read_text(encoding="utf-8"))
    if baseline["commit"] != BASELINE:
        raise SystemExit("Unexpected Blueprint contract baseline")
    failures, member_count = [], 0
    for path in baseline["assets"]:
        if not (ROOT / path).is_file():
            failures.append(f"Asset removed or moved: {path}")
    for path, recorded in baseline["headers"].items():
        before = recorded["types"]
        text = (ROOT / path).read_text(encoding="utf-8-sig") if (ROOT / path).exists() else ""
        after = surface(text)
        for name, original in before.items():
            current = after.get(name)
            if current is None or current["declaration"] != original["declaration"]:
                failures.append(f"Reflected type changed: {path}: {name}")
                continue
            for member, definition in original["members"].items():
                if (name, member) in REMOVED_MEMBERS:
                    if member in current["members"]:
                        failures.append(f"Obsolete member must be removed: {name}.{member}")
                    continue
                member_count += 1
                if current["members"].get(member) != definition:
                    failures.append(f"Blueprint member/pin metadata changed: {path}: {name}.{member}")
        current_delegates = delegates(text)
        for name, definition in recorded["delegates"].items():
            if current_delegates.get(name) != definition:
                failures.append(f"Blueprint delegate pins changed: {path}: {name}")
    for path in (ROOT / "Source").rglob("*"):
        if path.suffix in (".h", ".cpp"):
            required = "Public" if path.suffix == ".h" else "Private"
            if required not in path.relative_to(ROOT / "Source").parts:
                failures.append(f"{path.suffix} must be under {required}: {path.relative_to(ROOT)}")
    if failures:
        raise SystemExit("\n".join(failures))
    print(f"Blueprint contract verified: {member_count} retained members; obsolete FSBSUserData.Role removed; all original assets present.")


if __name__ == "__main__":
    main()
