from __future__ import annotations

import subprocess
import tempfile
import unittest
from pathlib import Path

from tools.devstack.core.stackconf import base_branch_name, default_body_dir, filtered_mode, key_number, read_conf


def _git(root: Path, argv: list[str]) -> str:
    proc = subprocess.run(["git", *argv], cwd=root, check=True, capture_output=True, text=True)
    return (proc.stdout or "").strip()


class TestStackConf(unittest.TestCase):
    def test_helpers(self) -> None:
        self.assertEqual(base_branch_name("origin/main"), "main")
        self.assertEqual(base_branch_name("main"), "main")
        self.assertEqual(key_number("001-foo"), "001")
        self.assertEqual(key_number("foo"), "")

    def test_read_conf_legacy_entries(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            subprocess.run(["git", "init"], cwd=root, check=True, capture_output=True)
            subprocess.run(["git", "config", "user.email", "test@example.com"], cwd=root, check=True)
            subprocess.run(["git", "config", "user.name", "Test"], cwd=root, check=True)

            (root / "README").write_text("x\n", encoding="utf-8")
            subprocess.run(["git", "add", "README"], cwd=root, check=True)
            subprocess.run(["git", "commit", "-m", "c1"], cwd=root, check=True, capture_output=True)
            sha1 = _git(root, ["rev-parse", "HEAD"])

            (root / "README").write_text("y\n", encoding="utf-8")
            subprocess.run(["git", "add", "README"], cwd=root, check=True)
            subprocess.run(["git", "commit", "-m", "c2"], cwd=root, check=True, capture_output=True)
            sha2 = _git(root, ["rev-parse", "HEAD"])

            (root / ".devstack").mkdir(parents=True, exist_ok=True)
            conf_path = root / ".devstack" / "stack.conf"
            conf_path.write_text(
                "\n".join(
                    [
                        "base origin/main",
                        "pr_prefix pr/my-series/",
                        "body_dir .devstack/pr-bodies/my-series",
                        "ignore deadbeef",
                        "",
                        f"001-one {sha1}",
                        f"002-two {sha2} bodies/002-two.md",
                        "",
                    ]
                ),
                encoding="utf-8",
            )

            conf = read_conf(root)
            self.assertEqual(conf.base_remote_ref, "origin/main")
            self.assertEqual(conf.pr_prefix, "pr/my-series/")
            self.assertEqual(conf.body_dir, ".devstack/pr-bodies/my-series")
            self.assertTrue(filtered_mode(conf))
            self.assertEqual(conf.ignore, ["deadbeef"])
            self.assertEqual(conf.entries[0].key, "001-one")
            self.assertEqual(conf.entries[0].branch, "pr/my-series/001-one")
            self.assertEqual(conf.entries[0].sha, sha1)
            self.assertEqual(conf.entries[1].body, "bodies/002-two.md")

    def test_default_body_dir_from_pr_prefix(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            conf_path = root / ".devstack" / "stack.conf"
            got = default_body_dir(root, conf_path, "pr/gui-refactor/")
            self.assertEqual(got, ".devstack/pr-bodies/gui-refactor")

