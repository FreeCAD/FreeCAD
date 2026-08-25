# SPDX-License-Identifier: LGPL-2.1-or-later

from pathlib import Path
import tempfile
import unittest

from stubgen.api_extract import extract_curated_api_model
from stubgen.stub_support import collect_stub_support


class StubSupportTests(unittest.TestCase):
    def test_support_fragments_are_not_public_api_declarations(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "Demo.module.pyi").write_text(
                """\
from typing import TypeVar

_T = TypeVar("_T")

def ping(value: int) -> bool: ...
""",
                encoding="utf-8",
            )
            (source_dir / "Demo.Widget.pyi").write_text(
                """\
from typing import Final

class Widget:
    value: Final[int]
    _token: object

    def refresh(self) -> None: ...
""",
                encoding="utf-8",
            )

            model = extract_curated_api_model(root, source_dir)
            support = collect_stub_support(root, source_dir, model)

        self.assertIn("_T = TypeVar('_T')", support.module_source("Demo"))
        self.assertNotIn("def ping", support.module_source("Demo"))
        self.assertIn("_token: object", support.class_source("Demo", "Widget"))
        self.assertNotIn("value: Final[int]", support.class_source("Demo", "Widget"))
        self.assertNotIn("def refresh", support.class_source("Demo", "Widget"))


if __name__ == "__main__":
    unittest.main()
