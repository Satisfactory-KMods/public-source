"""Loopback-only SBS sharing contract fixture. No game, credentials or production uploads required."""

import argparse
import hashlib
import json
import re
import threading
import uuid
from email.parser import BytesParser
from email.policy import default
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from download_id_fixture import resolve_download_id

PREFIX = "/api/v1/sbs/"
DUMMY_KEY = "sbsu_" + "A" * 43
MAX_BODY = 66 * 1024 * 1024


def decode_upload(content_type, body):
    """Validate the wire DTO, not the Satisfactory binary format (fixture only)."""
    message = BytesParser(policy=default).parsebytes(
        f"Content-Type: {content_type}\r\nMIME-Version: 1.0\r\n\r\n".encode("ascii") + body
    )
    if not message.is_multipart() or message.defects:
        raise ValueError("invalid_multipart")
    parts = {}
    for part in message.iter_parts():
        name = part.get_param("name", header="content-disposition")
        if name not in ("metadata", "sbp", "sbpcfg") or name in parts or part.is_multipart() or part.defects:
            raise ValueError("invalid_parts")
        if part.get_content_disposition() != "form-data":
            raise ValueError("invalid_parts")
        parts[name] = part.get_payload(decode=True)
    if set(parts) != {"metadata", "sbp", "sbpcfg"}:
        raise ValueError("missing_parts")
    if not 20 <= len(parts["sbp"]) <= 64 * 1024 * 1024 or not 20 <= len(parts["sbpcfg"]) <= 1024 * 1024:
        raise ValueError("invalid_file_size")
    if len(parts["metadata"]) > 65536:
        raise ValueError("invalid_metadata")

    def unique_object(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError("duplicate_json_field")
            result[key] = value
        return result

    metadata = json.loads(parts["metadata"].decode("utf-8"), object_pairs_hook=unique_object)
    if not isinstance(metadata, dict) or set(metadata) != {
        "schemaVersion", "name", "description", "originalName", "visibility", "tagIds", "files"
    }:
        raise ValueError("invalid_metadata")
    if type(metadata["schemaVersion"]) is not int or metadata["schemaVersion"] != 1 or metadata["visibility"] != "public":
        raise ValueError("invalid_metadata")
    name, description, stem = (metadata[field] for field in ("name", "description", "originalName"))
    if not all(isinstance(value, str) for value in (name, description, stem)):
        raise ValueError("invalid_metadata")
    if not 1 <= len(name.encode("utf-16-le")) // 2 <= 120 or name != name.strip() or any(ord(c) < 32 or ord(c) == 127 for c in name):
        raise ValueError("invalid_name")
    if len(description.encode("utf-16-le")) // 2 > 10000 or any((ord(c) < 32 and c not in "\t\r\n") or ord(c) == 127 for c in description):
        raise ValueError("invalid_description")
    reserved = stem.split(".", 1)[0].rstrip().upper()
    if (not 1 <= len(stem.encode("utf-16-le")) // 2 <= 180 or stem.endswith((".", " "))
            or stem.lower().endswith((".sbp", ".sbpcfg")) or any(c in '/\\:<>"|?*' or ord(c) < 32 or ord(c) == 127 for c in stem)
            or reserved in {"CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$"}
            or re.fullmatch(r"(?:COM|LPT)[0-9¹²³]", reserved)):
        raise ValueError("invalid_filename")
    tags = metadata["tagIds"]
    if not isinstance(tags, list) or len(tags) > 32 or any(not isinstance(tag, str) or not re.fullmatch(r"[A-Za-z0-9_-]{1,128}", tag) for tag in tags) or len(set(tags)) != len(tags):
        raise ValueError("invalid_tags")
    sizes = metadata["files"]
    if not isinstance(sizes, dict) or set(sizes) != {"sbpBytes", "sbpcfgBytes"}:
        raise ValueError("invalid_sizes")
    if any(type(sizes[key]) is not int for key in sizes) or sizes != {"sbpBytes": len(parts["sbp"]), "sbpcfgBytes": len(parts["sbpcfg"])}:
        raise ValueError("invalid_sizes")
    digest = hashlib.sha256(json.dumps(metadata, sort_keys=True, ensure_ascii=False, separators=(",", ":")).encode("utf-8"))
    digest.update(hashlib.sha256(parts["sbp"]).digest())
    digest.update(hashlib.sha256(parts["sbpcfg"]).digest())
    return metadata, parts, digest.hexdigest()


class SharingServer(ThreadingHTTPServer):
    daemon_threads = True

    def __init__(self, address):
        super().__init__(address, SharingHandler)
        self.publications = {}
        self.lock = threading.Lock()
        self.auth_mode = "valid"


class SharingHandler(BaseHTTPRequestHandler):
    def log_message(self, *_args):
        pass  # Do not log headers or credentials, even on failure.

    def send_json(self, status, value):
        data = json.dumps(value).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        try:
            self.wfile.write(data)
        except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
            pass

    def error(self, status, code):
        self.send_json(status, {"schemaVersion": 1, "error": {"code": code}})

    def authenticated(self):
        if self.headers.get("Authorization") != "Bearer " + DUMMY_KEY or self.server.auth_mode == "unauthorized":
            self.error(401, "invalid_credentials")
            return False
        if self.headers.get("x-account-key") or self.headers.get("x-api-key"):
            self.error(400, "legacy_credentials_forbidden")
            return False
        return True

    def do_GET(self):
        if self.path != PREFIX + "auth/me":
            self.error(404, "not_found")
        elif self.authenticated():
            scopes = ["sbs:read"] if self.server.auth_mode == "read_only" else ["sbs:read", "sbs:publish", "sbs:rate"]
            self.send_json(200, {"schemaVersion": 1, "user": {"id": "fixture-user", "username": "Local fixture", "permissions": []}, "scopes": scopes})

    def do_POST(self):
        self.connection.settimeout(10)
        if self.path == PREFIX + "mod/resolveid":
            # Public read route. Reject supplied non-dummy credentials without recording them.
            if self.headers.get("Authorization") and not self.authenticated():
                return
            if self.headers.get("x-account-key") not in (None, "", "local-fixture") or self.headers.get("x-api-key"):
                self.error(400, "fixture_credentials_only")
                return
            if self.headers.get("Content-Type", "").split(";", 1)[0] != "application/json":
                self.error(415, "unsupported_media_type")
                return
            try:
                length = int(self.headers.get("Content-Length", "0"))
                if not 1 <= length <= 1024:
                    self.error(413, "payload_too_large")
                    return
                body = self.rfile.read(length)
                if len(body) != length:
                    raise ValueError("incomplete_body")
                status, value = resolve_download_id(body)
                self.send_json(status, value)
            except (ValueError, TimeoutError):
                self.error(400, "invalid_request")
            return
        if not self.authenticated():
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length <= 0 or length > MAX_BODY:
                self.error(413, "payload_too_large")
                return
            body = self.rfile.read(length)
            if len(body) != length:
                raise ValueError("incomplete_body")
            if self.path == PREFIX + "__test/auth-mode":
                mode = json.loads(body)["mode"]
                if mode not in ("valid", "read_only", "unauthorized"):
                    raise ValueError("invalid_mode")
                self.server.auth_mode = mode
                self.send_json(200, {"success": True})
                return
            if self.path != PREFIX + "blueprints":
                self.error(404, "not_found")
                return
            if self.server.auth_mode == "read_only":
                self.error(403, "insufficient_scope")
                return
            request_id = self.headers.get("Idempotency-Key", "")
            if str(uuid.UUID(request_id)) != request_id:
                raise ValueError("invalid_idempotency_key")
            _, _, digest = decode_upload(self.headers.get("Content-Type", ""), body)
        except (ValueError, UnicodeError, KeyError, TypeError, TimeoutError):
            self.error(400, "invalid_request")
            return
        with self.server.lock:
            previous = self.server.publications.get(request_id)
            if previous:
                if previous[0] != digest:
                    self.error(409, "idempotency_conflict")
                else:
                    self.send_json(200, previous[1])
                return
            blueprint_id = "fixture-upload-" + str(len(self.server.publications) + 1)
            response = {"schemaVersion": 1, "requestId": request_id, "blueprint": {
                "id": blueprint_id, "url": "https://k-mods.com/sbs/blueprints/" + blueprint_id, "visibility": "public"
            }}
            self.server.publications[request_id] = (digest, response)
            self.send_json(201, response)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=17892)
    args = parser.parse_args()
    if not 1024 <= args.port <= 65535:
        parser.error("port must be 1024..65535")
    with SharingServer(("127.0.0.1", args.port)) as server:
        print(f"SBS_SHARING_FIXTURE_READY port={args.port}", flush=True)
        server.serve_forever()


if __name__ == "__main__":
    main()
