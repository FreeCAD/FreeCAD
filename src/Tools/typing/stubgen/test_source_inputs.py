# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import tempfile
import textwrap
import unittest
from pathlib import Path
import sys

TYPING_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TYPING_DIR))

from stubgen.model import BindingMethod
from stubgen.source_inputs import supplement_module_methods_from_stub_signatures

ROOT_DIR = Path(__file__).resolve().parents[4]


class SourceInputsTests(unittest.TestCase):
    def test_module_stub_signatures_supplement_missing_module_methods(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import bootstrap_export, typing_only

            def ping(value: int, /) -> str: ...

            @bootstrap_export
            def setupWithoutGUI() -> None: ...

            @typing_only
            def listCommands() -> list[str]: ...
            """)

        with tempfile.TemporaryDirectory(dir=ROOT_DIR) as temp_dir:
            root = Path(temp_dir)
            source_dir = root / "src"
            app_dir = source_dir / "App"
            app_dir.mkdir(parents=True)
            (app_dir / "FreeCAD.module.pyi").write_text(source, encoding="utf-8")

            methods = supplement_module_methods_from_stub_signatures(root, source_dir, [])

        keys = {(method.inferred_module, method.python_name) for method in methods}
        self.assertEqual(
            keys,
            {
                ("FreeCAD", "listCommands"),
                ("FreeCAD", "ping"),
                ("FreeCAD", "setupWithoutGUI"),
            },
        )

    def test_module_stub_signatures_do_not_duplicate_existing_module_methods(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            def ping(value: int, /) -> str: ...
            """)

        existing = [
            BindingMethod(
                family="pymethoddef",
                source="src/App/ApplicationPy.cpp",
                line=1,
                table="ApplicationPy::Methods",
                context_kind="pymethoddef_table",
                context_name="ApplicationPy::Methods",
                inferred_module="FreeCAD",
                method_kind="varargs",
                python_name="ping",
                cxx_callable="ApplicationPy::sPing",
                flags="METH_VARARGS",
                doc="",
                generated_source=False,
            )
        ]

        with tempfile.TemporaryDirectory(dir=ROOT_DIR) as temp_dir:
            root = Path(temp_dir)
            source_dir = root / "src"
            app_dir = source_dir / "App"
            app_dir.mkdir(parents=True)
            (app_dir / "FreeCAD.module.pyi").write_text(source, encoding="utf-8")

            methods = supplement_module_methods_from_stub_signatures(root, source_dir, existing)

        self.assertEqual(len(methods), 1)
        self.assertEqual(methods[0].source, "src/App/ApplicationPy.cpp")


if __name__ == "__main__":
    unittest.main()
