"""Local compatibility fixture, not the contract of the rebuilt production backend.

Run with a real .sbp/.sbpcfg pair and launch the game with -SBSLocalTestPort=17891.
The client uses a dummy account in this explicit test mode. No private keys needed.
"""

import argparse
import base64
import json
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


PREFIX = "/api/v1/sbs/"
PREVIEW = base64.b64decode(
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+jG1sAAAAASUVORK5CYII="
)
TAGS = [{"_id": "fixture", "DisplayName": "Local fixture"}]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sbp", required=True, type=Path)
    parser.add_argument("--sbpcfg", required=True, type=Path)
    parser.add_argument("--port", default=17891, type=int)
    args = parser.parse_args()
    if not 1024 <= args.port <= 65535:
        parser.error("port must be 1024..65535")
    files = {"sbp": args.sbp.read_bytes(), "sbpcfg": args.sbpcfg.read_bytes()}
    blueprints = [
        {
            "_id": f"fixture-{index:02d}",
            "name": f"Fixture {index:02d}: {args.sbp.stem}",
            "originalName": args.sbp.stem,
            "owner": "fixture-user",
            "DesignerSize": "Mk.1",
            "createdAt": "2026-09-13T10:00:00.000Z",
            "updatedAt": "2026-09-13T10:00:00.000Z",
            "totalRating": 4,
            "totalRatingCount": 1,
            "downloads": index,
            "tags": TAGS,
            "mods": [],
            "images": ["preview.png"],
            "iconData": {"iconID": 0, "color": {"r": 0.2, "g": 0.7, "b": 1, "a": 1}},
        }
        for index in range(1, 26)
    ]
    packs = [
        {"_id": "fixture-pack", "name": "Local fixture pack", "owner": "fixture-user", "tags": TAGS,
         "mods": [], "blueprints": blueprints[:2], "image": "preview.png"},
        {"_id": "empty-pack", "name": "Empty fixture pack", "tags": [], "mods": [], "blueprints": []},
    ]

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_args):
            pass

        def send(self, status, content, content_type="application/json"):
            body = content if isinstance(content, bytes) else json.dumps(content).encode()
            self.send_response(status)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            try:
                self.wfile.write(body)
            except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
                pass  # An obsolete search can be cancelled by the client.

        def do_POST(self):
            if self.headers.get("x-account-key") not in (None, "", "local-fixture"):
                self.send(400, {"error": "Only dummy fixture credentials are accepted"})
                return
            try:
                length = int(self.headers.get("Content-Length", "0"))
                if not 0 <= length <= 128 * 1024:
                    raise ValueError()
                payload = json.loads(self.rfile.read(length) or b"{}")
                if not isinstance(payload, dict):
                    raise ValueError()
            except (ValueError, UnicodeError):
                self.send(400, {"error": "Invalid fixture request"})
                return
            route = self.path.removeprefix(PREFIX)
            print(json.dumps({"method": "POST", "route": route, "payload": payload}), flush=True)
            if self.path == PREFIX + "mod/authcheck":
                self.send(200, {"schemaVersion": 1, "user": {"id": "fixture-user", "username": "Local fixture", "permissions": []},
                                "scopes": ["sbs:read", "sbs:publish", "sbs:rate"]})
            elif self.path == PREFIX + "mod/gettags":
                self.send(200, {"tags": TAGS})
            elif self.path == PREFIX + "mod/rateblueprint":
                valid = isinstance(payload.get("blueprintId"), str) and payload.get("rating") in range(1, 6)
                self.send(200 if valid else 400, {"success": True} if valid else {"error": "Rating fields missing"})
            elif self.path in (PREFIX + "mod/getblueprints", PREFIX + "mod/getblueprintpacks"):
                options = payload.get("filterOptions", {})
                name = str(options.get("name", "")).casefold()
                if name == "!401":
                    self.send(401, {"error": "Fixture auth rejection", "authMode": "oidc"})
                    return
                if name == "!malformed":
                    self.send(200, b'{"blueprints":')
                    return
                if name == "!slow":
                    time.sleep(2)
                    name = ""
                items = packs if self.path.endswith("getblueprintpacks") else blueprints
                if name == "!badfile":
                    items = [dict(blueprints[0], _id="bad-file", originalName="SBS Invalid Fixture")]
                elif name == "!unsafe":
                    items = [dict(blueprints[0], _id="../escape")]
                else:
                    items = [item for item in items if name in item["name"].casefold()]
                skip = max(0, int(payload.get("skip", 0)))
                limit = max(1, min(200, int(payload.get("limit", 20))))
                self.send(200, {"blueprints": items[skip:skip + limit], "totalBlueprints": len(items)})
            else:
                self.send(404, {"error": "Unknown fixture route"})

        def do_GET(self):
            route = self.path.removeprefix(PREFIX)
            print(json.dumps({"method": "GET", "route": route}), flush=True)
            parts = route.split("/")
            if len(parts) == 3 and parts[0] == "download" and parts[2] in files:
                content = b"intentionally invalid blueprint" if parts[1] == "bad-file" else files[parts[2]]
                self.send(200, content, "application/octet-stream")
            elif route.startswith("image/"):
                self.send(200, PREVIEW, "image/png")
            else:
                self.send(404, {"error": "Unknown fixture route"})

    print(f"SBS_FIXTURE_READY http://127.0.0.1:{args.port}{PREFIX}", flush=True)
    ThreadingHTTPServer(("127.0.0.1", args.port), Handler).serve_forever()


if __name__ == "__main__":
    main()
