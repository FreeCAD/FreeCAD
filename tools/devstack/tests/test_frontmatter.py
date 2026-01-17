from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from tools.devstack.core.frontmatter import (
    strip_body_frontmatter,
    title_from_body_frontmatter,
    title_with_number,
)


class TestFrontmatter(unittest.TestCase):
    def test_strip_body_frontmatter(self) -> None:
        text = "---\n" "title: \"Gui: Hello\"\n" "---\n" "\n" "Body\n"
        self.assertEqual(strip_body_frontmatter(text), "Body\n")

    def test_title_from_body_frontmatter(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "body.md"
            path.write_text("---\n" "title: 'Tests: Foo'\n" "---\n" "\n" "Body\n", encoding="utf-8")
            self.assertEqual(title_from_body_frontmatter(path), "Tests: Foo")

    def test_title_with_number(self) -> None:
        self.assertEqual(title_with_number("Gui: Thing", "001"), "Gui: [001] Thing")
        self.assertEqual(title_with_number("Thing", "001"), "[001] Thing")
        self.assertEqual(title_with_number("Thing", ""), "Thing")

