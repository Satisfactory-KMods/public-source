"""Offline/loopback tests for the proposed sharing wire contract; never publish remotely."""

import http.client
import json
import threading
import unittest
import uuid
from pathlib import Path

from sharing_api_fixture import DUMMY_KEY, PREFIX, SharingServer, decode_upload


def multipart(name="Copper Factory", boundary="test_boundary"):
    sbp, config = bytes(range(256)), bytes(reversed(range(256)))
    metadata = {"schemaVersion": 1, "name": name, "description": "line\nü", "originalName": "Copper Factory",
                "visibility": "public", "tagIds": ["copper"], "files": {"sbpBytes": len(sbp), "sbpcfgBytes": len(config)}}
    body = bytearray()
    for part, kind, data in [("metadata", "application/json; charset=utf-8", json.dumps(metadata).encode()),
                             ("sbp", "application/octet-stream", sbp), ("sbpcfg", "application/octet-stream", config)]:
        body.extend(f'--{boundary}\r\nContent-Disposition: form-data; name="{part}"\r\nContent-Type: {kind}\r\n\r\n'.encode())
        body.extend(data)
        body.extend(b"\r\n")
    body.extend(f"--{boundary}--\r\n".encode())
    return f"multipart/form-data; boundary={boundary}", bytes(body)


class SharingFixtureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server = SharingServer(("127.0.0.1", 0))
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.server.server_close()
        cls.thread.join(timeout=2)

    def request(self, method, route, body=None, headers=None):
        connection = http.client.HTTPConnection("127.0.0.1", self.server.server_port, timeout=5)
        connection.request(method, PREFIX + route, body=body, headers=headers or {})
        response = connection.getresponse()
        result = response.status, json.loads(response.read())
        connection.close()
        return result

    def test_missing_and_revoked_login(self):
        self.assertEqual(self.request("GET", "auth/me")[0], 401)
        self.assertEqual(self.request("GET", "auth/me", headers={"Authorization": "Bearer invalid"})[0], 401)
        headers = {"Authorization": "Bearer " + DUMMY_KEY}
        self.assertEqual(self.request("GET", "auth/me", headers=headers)[0], 200)
        self.server.auth_mode = "unauthorized"
        try:
            self.assertEqual(self.request("GET", "auth/me", headers=headers)[0], 401)
        finally:
            self.server.auth_mode = "valid"

    def test_publication_retry_and_conflict(self):
        kind, body = multipart()
        headers = {"Authorization": "Bearer " + DUMMY_KEY, "Content-Type": kind, "Idempotency-Key": str(uuid.uuid4())}
        status, published = self.request("POST", "blueprints", body, headers)
        self.assertEqual(status, 201)
        # MIME boundary changes must not defeat idempotency.
        kind2, body2 = multipart(boundary="different_boundary")
        headers["Content-Type"] = kind2
        self.assertEqual(self.request("POST", "blueprints", body2, headers), (200, published))
        _, changed = multipart(name="Changed name", boundary="different_boundary")
        self.assertEqual(self.request("POST", "blueprints", changed, headers)[0], 409)

    def test_scope_and_legacy_headers(self):
        kind, body = multipart()
        headers = {"Authorization": "Bearer " + DUMMY_KEY, "Content-Type": kind, "Idempotency-Key": str(uuid.uuid4())}
        self.server.auth_mode = "read_only"
        try:
            self.assertEqual(self.request("POST", "blueprints", body, headers)[0], 403)
        finally:
            self.server.auth_mode = "valid"
        headers["x-account-key"] = "forbidden-dummy"
        self.assertEqual(self.request("POST", "blueprints", body, headers)[0], 400)

    def test_binary_and_malformed_parts(self):
        kind, body = multipart()
        metadata, parts, _ = decode_upload(kind, body)
        self.assertEqual(parts["sbp"], bytes(range(256)))
        self.assertEqual(parts["sbpcfg"], bytes(reversed(range(256))))
        self.assertEqual(metadata["description"], "line\nü")
        for malformed in [body.replace(b'name="sbpcfg"', b'name="sbp"'), body.replace(b'"copper"]', b'"copper", "copper"]'),
                          body.replace(b'"schemaVersion": 1', b'"schemaVersion": true'), body.replace(b'"sbpBytes": 256', b'"sbpBytes": 257')]:
            with self.assertRaises(ValueError):
                decode_upload(kind, malformed)

    def test_native_multipart_when_available(self):
        path = Path(__file__).resolve().parents[1] / ".validation/sharing/multipart.bin"
        if not path.exists():
            self.skipTest("Run ValidateSharingData in the Editor commandlet first")
        metadata, parts, _ = decode_upload("multipart/form-data; boundary=SBS_native_fixture_boundary", path.read_bytes())
        self.assertEqual(parts["sbp"], bytes(range(256)))
        self.assertEqual(parts["sbpcfg"], bytes(reversed(range(256))))
        self.assertEqual(metadata["name"], 'Copper "Factory" \\ line')
        self.assertEqual(metadata["tagIds"], ["factory", "copper"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
