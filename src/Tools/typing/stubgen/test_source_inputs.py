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
from stubgen.source_inputs import (
    parse_module_stub_signature_overrides,
    parse_source_type_stub_signature_overrides,
    supplement_module_methods_from_stub_signatures,
)

ROOT_DIR = Path(__file__).resolve().parents[4]


class SourceInputsTests(unittest.TestCase):
    def _parse_module_stub(self, source: str):
        with tempfile.TemporaryDirectory(dir=ROOT_DIR) as temp_dir:
            root = Path(temp_dir)
            source_dir = root / "src"
            app_dir = source_dir / "App"
            app_dir.mkdir(parents=True)
            (app_dir / "FreeCAD.module.pyi").write_text(source, encoding="utf-8")
            return parse_module_stub_signature_overrides(root, source_dir)

    def _parse_source_stub(self, source: str):
        with tempfile.TemporaryDirectory(dir=ROOT_DIR) as temp_dir:
            root = Path(temp_dir)
            source_dir = root / "src"
            app_dir = source_dir / "App"
            app_dir.mkdir(parents=True)
            (app_dir / "FreeCAD.Test.pyi").write_text(source, encoding="utf-8")
            return parse_source_type_stub_signature_overrides(root, source_dir)

    def test_overload_groups_are_preserved_for_module_and_type_stubs(self):
        module_source = textwrap.dedent("""
            from typing import overload

            @overload
            def ping(value: int, /) -> str: ...

            @overload
            def ping(value: str, /) -> str: ...
            """)
        source_stub = textwrap.dedent("""
            from typing import overload

            class Test:
                @overload
                def ping(self, value: int, /) -> str: ...

                @overload
                def ping(self, value: str, /) -> str: ...
            """)

        module_signatures = self._parse_module_stub(module_source)
        source_signatures = self._parse_source_stub(source_stub)
        self.assertEqual(2, len(module_signatures[("FreeCAD", "ping")][0]))
        self.assertEqual(2, len(source_signatures[("FreeCAD", "Test", "ping")][0]))

    def test_mixed_overload_groups_are_rejected(self):
        module_source = textwrap.dedent("""
            from typing import overload

            @overload
            def ping(value: int, /) -> str: ...

            def ping(value: str, /) -> str: ...
            """)
        source_stub = textwrap.dedent("""
            from typing import overload

            class Test:
                @overload
                def ping(self, value: int, /) -> str: ...

                def ping(self, value: str, /) -> str: ...
            """)

        for parser, source in (
            (self._parse_module_stub, module_source),
            (self._parse_source_stub, source_stub),
        ):
            with self.subTest(parser=parser.__name__):
                with self.assertRaisesRegex(ValueError, "must all use @overload"):
                    parser(source)

    def test_duplicate_non_overload_declarations_are_rejected(self):
        module_source = textwrap.dedent("""
            def ping(value: int, /) -> str: ...
            def ping(value: str, /) -> str: ...
            """)
        source_stub = textwrap.dedent("""
            class Test:
                def ping(self, value: int, /) -> str: ...
                def ping(self, value: str, /) -> str: ...
            """)

        for parser, source in (
            (self._parse_module_stub, module_source),
            (self._parse_source_stub, source_stub),
        ):
            with self.subTest(parser=parser.__name__):
                with self.assertRaisesRegex(ValueError, "must all use @overload"):
                    parser(source)

    def test_single_overload_declarations_are_rejected(self):
        module_source = textwrap.dedent("""
            from typing import overload

            @overload
            def ping(value: int, /) -> str: ...
            """)
        source_stub = textwrap.dedent("""
            from typing import overload

            class Test:
                @overload
                def ping(self, value: int, /) -> str: ...
            """)

        for parser, source in (
            (self._parse_module_stub, module_source),
            (self._parse_source_stub, source_stub),
        ):
            with self.subTest(parser=parser.__name__):
                with self.assertRaisesRegex(ValueError, "at least two"):
                    parser(source)

    def test_duplicate_overload_signatures_are_rejected(self):
        module_source = textwrap.dedent("""
            from typing import overload

            @overload
            def ping(value: int, /) -> str: ...

            @overload
            def ping(value: int, /) -> str: ...
            """)
        source_stub = textwrap.dedent("""
            from typing import overload

            class Test:
                @overload
                def ping(self, value: int, /) -> str: ...

                @overload
                def ping(self, value: int, /) -> str: ...
            """)

        for parser, source in (
            (self._parse_module_stub, module_source),
            (self._parse_source_stub, source_stub),
        ):
            with self.subTest(parser=parser.__name__):
                with self.assertRaisesRegex(ValueError, "duplicate overload signatures"):
                    parser(source)

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

    def test_return_annotations_are_read_from_the_python_ast(self):
        source = textwrap.dedent("""
            from typing import Literal

            def ping() -> Literal["a:b"]: ...
            """)

        signatures = self._parse_module_stub(source)
        self.assertEqual("Literal['a:b']", signatures[("FreeCAD", "ping")][0][0].returns)


if __name__ == "__main__":
    unittest.main()
