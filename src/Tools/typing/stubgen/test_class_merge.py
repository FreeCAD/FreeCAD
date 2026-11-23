# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import ast
import tempfile
from pathlib import Path
import sys
import unittest

TYPING_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TYPING_DIR))

from stubgen.class_merge import public_class_stub_source  # noqa: E402
from stubgen.module_merge import merge_type_class_support_nodes  # noqa: E402
from stubgen.model import BindingClass  # noqa: E402


class ClassMergeTests(unittest.TestCase):
    def test_merges_source_adjacent_bases_without_class_members(self):
        target = """\
class Derived:
    ...
"""
        support = """\
class Derived(Base):
    ...
"""
        support_class = ast.parse(support).body[0]
        self.assertIsInstance(support_class, ast.ClassDef)

        merged = merge_type_class_support_nodes(
            target,
            "Derived",
            "",
            class_bases=support_class.bases,
        )

        self.assertIn("class Derived(Base):", merged)

    def test_preserves_property_getters_and_setters(self):
        source = """\
from typing import Sequence

class Binding:
    @property
    def A(self) -> tuple[float, ...]: ...

    @A.setter
    def A(self, value: Sequence[float]) -> None: ...
"""
        klass = BindingClass(
            source="src/Base/Binding.pyi",
            line=3,
            class_name="Binding",
            export_name="BindingPy",
            python_name=None,
            public_names=["FreeCAD.Base.Binding"],
            base_class=None,
            explicit_export=False,
        )

        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            path = root / klass.source
            path.parent.mkdir(parents=True)
            path.write_text(source, encoding="utf-8")

            rendered = public_class_stub_source(
                root,
                klass,
                "FreeCAD.Base",
                {},
                {"Binding"},
                {},
                set(),
            )

        assert rendered is not None
        self.assertIn("@property", rendered.source)
        self.assertIn("def A(self) -> tuple[float, ...]", rendered.source)
        self.assertIn("@A.setter", rendered.source)
        self.assertIn("def A(self, value: Sequence[float]) -> None", rendered.source)


if __name__ == "__main__":
    unittest.main()
