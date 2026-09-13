"""Tests for the common blueprint/pack lookup DTO and its loopback HTTP route."""

import http.client
import json
import threading
import unittest

from download_id_fixture import resolve_download_id
from sharing_api_fixture import PREFIX, SharingServer


class DownloadIdFixtureTests(unittest.TestCase):
    def resolve(self, identifier, kind="auto"):
        return resolve_download_id(json.dumps({"schemaVersion": 1, "id": identifier, "kind": kind}).encode())

    def test_both_types_through_one_request(self):
        for identifier, kind in [("fixture-blueprint", "blueprint"), ("fixture-pack", "pack")]:
            for requested in ["auto", kind]:
                status, value = self.resolve(identifier, requested)
                self.assertEqual(status, 200)
                self.assertEqual(value["kind"], kind)
                self.assertEqual(value[kind]["_id"], identifier)
                self.assertNotIn("pack" if kind == "blueprint" else "blueprint", value)
        pack = self.resolve("fixture-pack")[1]["pack"]
        self.assertEqual(len(pack["blueprints"]), 2)
        self.assertEqual(len({item["originalName"].casefold() for item in pack["blueprints"]}), 2)

    def test_no_search_fallback_and_unambiguous_typed_lookup(self):
        self.assertEqual(self.resolve("fixture")[0], 404)
        self.assertEqual(self.resolve("fixture-pack", "blueprint")[0], 404)
        self.assertEqual(self.resolve("fixture-ambiguous")[0], 409)
        self.assertEqual(self.resolve("fixture-ambiguous", "blueprint")[1]["kind"], "blueprint")
        self.assertEqual(self.resolve("fixture-ambiguous", "pack")[1]["kind"], "pack")
        self.assertEqual(self.resolve("fixture-empty")[0], 422)

    def test_strict_request_fields_and_limits(self):
        valid = b'{"schemaVersion":1,"id":"fixture-pack","kind":"auto"}'
        for body in [valid.replace(b'"schemaVersion":1', b'"schemaVersion":true'),
                     valid.replace(b'"kind":"auto"', b'"kind":null'),
                     valid.replace(b'"kind":"auto"', b'"kind":"auto","kind":"pack"'),
                     valid.replace(b'"id":"fixture-pack"', b'"id":"../fixture-pack"'),
                     valid.replace(b'"id":"fixture-pack"', b'"id":123'),
                     valid[:-1] + b',"url":"https://invalid.test"}', b'[]', b'\xff', b'{}']:
            with self.subTest(body=body):
                self.assertEqual(resolve_download_id(body)[0], 400)
        self.assertEqual(resolve_download_id(valid + b" " * 1024)[0], 413)
        self.assertEqual(self.resolve("a" * 129)[0], 400)

    def test_public_http_route_without_account_or_mod_key(self):
        server = SharingServer(("127.0.0.1", 0))
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            for identifier, expected in [("fixture-blueprint", 200), ("fixture-pack", 200), ("fixture-ambiguous", 409)]:
                connection = http.client.HTTPConnection("127.0.0.1", server.server_port, timeout=3)
                try:
                    connection.request("POST", PREFIX + "mod/resolveid", json.dumps(
                        {"schemaVersion": 1, "kind": "auto", "id": identifier}), {"Content-Type": "application/json"})
                    response = connection.getresponse()
                    self.assertEqual(response.status, expected)
                    self.assertIsNone(response.getheader("Location"))
                    self.assertEqual(json.loads(response.read())["schemaVersion"], 1)
                finally:
                    connection.close()
        finally:
            server.shutdown()
            server.server_close()
            thread.join(timeout=2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
