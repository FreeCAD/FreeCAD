# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import email.message
import io
import json
from pathlib import Path
import sys
import unittest
from unittest import mock
import urllib.error

TYPING_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TYPING_DIR))

from stubgen.index import next_patch_number, published_versions  # noqa: E402


def http_error(code: int, message: str) -> urllib.error.HTTPError:
    return urllib.error.HTTPError("url", code, message, email.message.Message(), io.BytesIO(b""))


def fake_response(payload: dict):
    response = io.BytesIO(json.dumps(payload).encode("utf-8"))
    response.__enter__ = lambda self=response: self
    response.__exit__ = lambda *args: None
    return response


class NextPatchNumberTests(unittest.TestCase):
    def test_an_unpublished_line_starts_at_zero(self):
        self.assertEqual(next_patch_number(1, 0, []), 0)

    def test_the_next_patch_is_one_past_the_highest(self):
        self.assertEqual(next_patch_number(1, 0, ["1.0.0", "1.0.1", "1.0.2"]), 3)

    def test_other_lines_do_not_count(self):
        versions = ["1.0.0", "1.1.0", "1.1.1", "2.0.0"]
        self.assertEqual(next_patch_number(1, 1, versions), 2)
        self.assertEqual(next_patch_number(1, 0, versions), 1)

    def test_a_pre_release_does_not_advance_the_line_it_previews(self):
        self.assertEqual(next_patch_number(1, 0, ["1.0.0rc1", "1.0.0a2", "1.0.0.dev9"]), 0)
        self.assertEqual(next_patch_number(1, 0, ["1.0.0", "1.0.1rc1"]), 1)

    def test_a_deleted_release_leaves_a_skipped_gap(self):
        self.assertEqual(next_patch_number(1, 0, ["1.0.0", "1.0.3"]), 4)

    def test_unsorted_input_is_handled(self):
        self.assertEqual(next_patch_number(1, 0, ["1.0.10", "1.0.2", "1.0.9"]), 11)


class PublishedVersionsTests(unittest.TestCase):
    def test_versions_are_returned(self):
        payload = {"meta": {"api-version": "1.4"}, "versions": ["1.0.0", "1.0.1"]}
        with mock.patch("urllib.request.urlopen", return_value=fake_response(payload)):
            self.assertEqual(published_versions("freecad-typings"), ["1.0.0", "1.0.1"])

    def test_an_unknown_package_is_not_an_error(self):
        with mock.patch("urllib.request.urlopen", side_effect=http_error(404, "Not Found")):
            self.assertEqual(published_versions("freecad-typings"), [])

    def test_a_server_error_propagates(self):
        with mock.patch("urllib.request.urlopen", side_effect=http_error(503, "Server Error")):
            with self.assertRaises(urllib.error.HTTPError):
                published_versions("freecad-typings")

    def test_an_index_without_the_versions_key_is_refused(self):
        payload = {"meta": {"api-version": "1.0"}, "files": []}
        with mock.patch("urllib.request.urlopen", return_value=fake_response(payload)):
            with self.assertRaises(ValueError) as caught:
                published_versions("freecad-typings")
        self.assertIn("1.0", str(caught.exception))


if __name__ == "__main__":
    unittest.main()
