# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest

TYPING_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TYPING_DIR))

from stubgen.project import (  # noqa: E402
    API_VERSION_FILE,
    FREECAD_VERSION_FILE,
    Project,
)


class ProjectVersionTests(unittest.TestCase):
    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.addCleanup(self._temp.cleanup)
        self.root = Path(self._temp.name)

    def _write_root(self, *, suffix: str, major=1, minor=0) -> Path:
        api_version_path = self.root / API_VERSION_FILE
        api_version_path.parent.mkdir(parents=True, exist_ok=True)
        api_version_path.write_text(
            json.dumps(
                {
                    "api_version_major": major,
                    "api_version_minor": minor,
                    "package_name": "freecad-typings",
                }
            ),
            encoding="utf-8",
        )
        (self.root / FREECAD_VERSION_FILE).write_text(
            json.dumps(
                {
                    "version_major": 26,
                    "version_minor": 3,
                    "version_patch": 0,
                    "version_suffix": suffix,
                }
            ),
            encoding="utf-8",
        )
        return self.root

    def test_empty_suffix_is_a_plain_release(self):
        root = self._write_root(suffix="")
        self.assertEqual(Project(root, patch_number=4).version, "1.0.4")

    def test_patch_number_defaults_to_zero(self):
        root = self._write_root(suffix="")
        self.assertEqual(Project(root).version, "1.0.0")

    def test_dev_suffix_alone_does_not_mark_the_package(self):
        root = self._write_root(suffix="dev")
        self.assertIsNone(Project(root).pre_release)
        self.assertEqual(Project(root, patch_number=2).version, "1.0.2")

    def test_dev_build_appends_a_development_release_segment(self):
        root = self._write_root(suffix="dev")
        self.assertEqual(Project(root, patch_number=2, dev_build=42).version, "1.0.2.dev42")

    def test_release_candidate_suffix_becomes_a_pre_release(self):
        for suffix, expected in (
            ("RC1", "1.0.3rc1"),
            ("rc2", "1.0.3rc2"),
            (".RC3", "1.0.3rc3"),
            ("rc01", "1.0.3rc1"),
            ("alpha1", "1.0.3a1"),
            ("BETA2", "1.0.3b2"),
        ):
            with self.subTest(suffix=suffix):
                root = self._write_root(suffix=suffix)
                self.assertEqual(Project(root, patch_number=3).version, expected)

    def test_dev_build_wins_over_a_pre_release_suffix(self):
        root = self._write_root(suffix="RC1")
        self.assertEqual(Project(root, dev_build=7).version, "1.0.0.dev7")

    def test_version_override_wins_over_everything(self):
        root = self._write_root(suffix="RC1")
        project = Project(root, version_override="9.9.9", patch_number=3, dev_build=7)
        self.assertEqual(project.version, "9.9.9")

    def test_unpublishable_suffix_is_refused(self):
        for suffix in ("git", "1", "rc", "snapshot", "rc1x"):
            with self.subTest(suffix=suffix):
                root = self._write_root(suffix=suffix)
                with self.assertRaises(ValueError) as caught:
                    Project(root).version
                self.assertIn(suffix, str(caught.exception))

    def test_api_version_is_coerced_to_integers(self):
        root = self._write_root(suffix="", major="2", minor="11")
        self.assertEqual(Project(root).api_version, (2, 11))
        self.assertEqual(Project(root, patch_number=1).version, "2.11.1")

    def test_freecad_version_records_the_suffix_for_provenance(self):
        self.assertEqual(Project(self._write_root(suffix="RC1")).freecad_version, "26.3.0.RC1")
        self.assertEqual(Project(self._write_root(suffix="")).freecad_version, "26.3.0")


if __name__ == "__main__":
    unittest.main()
