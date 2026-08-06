#!/usr/bin/env python3

# SPDX-License-Identifier: LGPL-2.1-or-later

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import preserve_eol


class PreserveEolTest(unittest.TestCase):
    def test_skips_lf_only_files(self):
        self.assertFalse(preserve_eol.needs_eol_preservation(b"one\ntwo\nthree\n"))

    def test_processes_crlf_and_mixed_files(self):
        self.assertTrue(preserve_eol.needs_eol_preservation(b"one\r\ntwo\r\n"))
        self.assertTrue(preserve_eol.needs_eol_preservation(b"one\r\ntwo\nthree\r\n"))

    def test_preserves_crlf_replacements(self):
        original = b"one\r\ntwo\r\nthree\r\n"
        modified = b"one\ntwo changed\nthree\n"
        expected = b"one\r\ntwo changed\r\nthree\r\n"
        self.assertEqual(preserve_eol.rewrite_with_preserved_eol(original, modified), expected)

    def test_preserves_mixed_replacements_and_insertions(self):
        original = b"one\r\ntwo\nthree\r\n"
        modified = b"one\ntwo changed\ninserted\nthree\n"
        expected = b"one\r\ntwo changed\ninserted\nthree\r\n"
        self.assertEqual(preserve_eol.rewrite_with_preserved_eol(original, modified), expected)

    def test_insert_before_first_line_uses_next_context(self):
        original = b"one\r\ntwo\r\n"
        modified = b"zero\none\ntwo\n"
        expected = b"zero\r\none\r\ntwo\r\n"
        self.assertEqual(preserve_eol.rewrite_with_preserved_eol(original, modified), expected)

    def test_final_line_without_newline_is_preserved(self):
        original = b"one\r\ntwo"
        modified = b"one\ntwo changed"
        expected = b"one\r\ntwo changed"
        self.assertEqual(preserve_eol.rewrite_with_preserved_eol(original, modified), expected)

    def test_added_final_newline_is_preserved_with_crlf(self):
        original = b"one\r\ntwo"
        modified = b"one\ntwo\n"
        expected = b"one\r\ntwo\r\n"
        self.assertEqual(preserve_eol.rewrite_with_preserved_eol(original, modified), expected)


class PreserveEolGitIntegrationTest(unittest.TestCase):
    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()
        self.repo = Path(self.tempdir.name)
        self.git("init", "-q")
        self.git("config", "user.email", "test@example.invalid")
        self.git("config", "user.name", "Test User")

    def tearDown(self):
        self.tempdir.cleanup()

    def git(self, *args: str) -> str:
        proc = subprocess.run(
            ["git", "-C", str(self.repo), *args],
            check=True,
            capture_output=True,
            text=True,
        )
        return proc.stdout.strip()

    def write_file(self, path: str, data: bytes):
        file_path = self.repo / path
        file_path.parent.mkdir(parents=True, exist_ok=True)
        file_path.write_bytes(data)

    def test_auto_revision_uses_merge_base_for_committed_changes(self):
        self.write_file("f.txt", b"one\r\ntwo\r\n")
        self.git("add", "f.txt")
        self.git("commit", "-q", "-m", "base")
        self.git("checkout", "-q", "-b", "feature")
        self.git("branch", "--set-upstream-to=master", "feature")

        self.write_file("f.txt", b"one\ntwo changed\n")
        self.git("add", "f.txt")
        self.git("commit", "-q", "-m", "bad-eol")

        file_path, status = preserve_eol.process_file(self.repo, "f.txt", "auto", write=False)
        self.assertEqual(file_path, "f.txt")
        self.assertEqual(status, "rewritten")

    def test_auto_revision_follows_staged_rename(self):
        self.write_file("old.txt", b"one\r\ntwo\r\n")
        self.git("add", "old.txt")
        self.git("commit", "-q", "-m", "base")

        self.git("mv", "old.txt", "new.txt")
        self.write_file("new.txt", b"one\ntwo changed\n")
        self.git("add", "new.txt")

        file_path, status = preserve_eol.process_file(self.repo, "new.txt", "auto", write=True)
        self.assertEqual(file_path, "new.txt")
        self.assertEqual(status, "rewritten")
        self.assertEqual((self.repo / "new.txt").read_bytes(), b"one\r\ntwo changed\r\n")


if __name__ == "__main__":
    unittest.main()
