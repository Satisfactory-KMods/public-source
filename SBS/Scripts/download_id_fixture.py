"""Metadata-only download-ID fixture; no production lookup, file writes or credentials."""

import json
import re


def blueprint(identifier, name):
    return {"_id": identifier, "name": name, "originalName": name,
            "iconData": {"iconID": 0, "color": {"r": 1, "g": 1, "b": 1, "a": 1}}}


BLUEPRINTS = {
    "fixture-blueprint": blueprint("fixture-blueprint", "Fixture Copper"),
    "fixture-second": blueprint("fixture-second", "Fixture Iron"),
    "fixture-ambiguous": blueprint("fixture-ambiguous", "Fixture Ambiguous"),
}
PACKS = {
    "fixture-pack": {"_id": "fixture-pack", "name": "Fixture Pack",
                     "blueprints": [BLUEPRINTS["fixture-blueprint"], BLUEPRINTS["fixture-second"]]},
    "fixture-ambiguous": {"_id": "fixture-ambiguous", "name": "Fixture Ambiguous Pack",
                          "blueprints": [BLUEPRINTS["fixture-blueprint"]]},
    "fixture-empty": {"_id": "fixture-empty", "name": "Fixture Empty Pack", "blueprints": []},
}


def resolve_download_id(body):
    def error(status, code):
        return status, {"schemaVersion": 1, "error": {"code": code}}

    def unique_fields(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError("duplicate_field")
            result[key] = value
        return result

    if not 1 <= len(body) <= 1024:
        return error(413, "payload_too_large")
    try:
        payload = json.loads(body.decode("utf-8"), object_pairs_hook=unique_fields)
        if not isinstance(payload, dict) or set(payload) != {"schemaVersion", "id", "kind"}:
            raise ValueError("invalid_request")
        identifier, kind = payload["id"], payload["kind"]
        if (type(payload["schemaVersion"]) is not int or payload["schemaVersion"] != 1
                or not isinstance(identifier, str) or not re.fullmatch(r"[A-Za-z0-9_-]{1,128}", identifier)
                or kind not in ("auto", "blueprint", "pack")):
            raise ValueError("invalid_request")
    except (ValueError, UnicodeError, TypeError):
        return error(400, "invalid_request")
    matches = [(label, items[identifier]) for label, items in (("blueprint", BLUEPRINTS), ("pack", PACKS))
               if identifier in items and kind in ("auto", label)]
    if not matches:
        return error(404, "download_not_found")
    if len(matches) != 1:
        return error(409, "ambiguous_download_id")
    resolved_kind, item = matches[0]
    if resolved_kind == "pack" and not item["blueprints"]:
        return error(422, "empty_pack")
    return 200, {"schemaVersion": 1, "kind": resolved_kind, "id": identifier, resolved_kind: item}
