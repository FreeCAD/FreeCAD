# SPDX-License-Identifier: LGPL-2.1-or-later

import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

BINDINGS_DIR = Path(__file__).resolve().parents[1]
SRC_DIR = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(BINDINGS_DIR))

from generate import generate
from model.generateModel_Python import parse, parse_python_code


class GenerateModelPythonTests(unittest.TestCase):
    def test_overload_only_constructor_docs_are_merged_into_class_doc(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from typing import overload

            from Base.Metadata import export
            from Base.PyObjectBase import PyObjectBase


            @export(Constructor=True)
            class Example(PyObjectBase):
                \"\"\"Example class doc.\"\"\"

                @overload
                def __init__(self) -> None: ...

                @overload
                def __init__(self, value: int) -> None:
                    \"\"\"
                    Create example objects.
                    \"\"\"
                    ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR) as temp_dir:
            path = Path(temp_dir) / "Example.pyi"
            path.write_text(source, encoding="utf-8")
            model = parse_python_code(str(path))

        export = model.PythonExport[0]
        self.assertEqual([method.Name for method in export.Methode], [])
        self.assertIn("Example class doc.", export.Documentation.UserDocu)
        self.assertIn("Example()", export.Documentation.UserDocu)
        self.assertIn("Example(value)", export.Documentation.UserDocu)
        self.assertIn("Example(value: int) -> None", export.Documentation.UserDocu)
        self.assertIn("Create example objects.", export.Documentation.UserDocu)

    def test_existing_class_constructor_docs_are_not_duplicated(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from typing import overload

            from Base.Metadata import export
            from Base.PyObjectBase import PyObjectBase


            @export(Constructor=True)
            class Example(PyObjectBase):
                \"\"\"
                Example class doc.

                The following constructors are supported:

                Example()
                Empty constructor.

                Example(value)
                Value constructor.
                \"\"\"

                @overload
                def __init__(self) -> None: ...

                @overload
                def __init__(self, value: int) -> None: ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR) as temp_dir:
            path = Path(temp_dir) / "Example.pyi"
            path.write_text(source, encoding="utf-8")
            model = parse_python_code(str(path))

        export = model.PythonExport[0]
        user_doc = export.Documentation.UserDocu
        self.assertIn("The following constructors are supported:", user_doc)
        self.assertEqual(user_doc.count("Example()"), 1)
        self.assertEqual(user_doc.count("Example(value)"), 1)
        self.assertNotIn("--\n", user_doc)

    def test_module_stub_parses_to_python_module_export(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import module, no_args

            \"\"\"Example module doc.\"\"\"

            module(Name="Example", Namespace="TestBindings")

            def ping(value: int, /, *, retry: bool = ...) -> object:
                \"\"\"
                Ping the module.
                \"\"\"
                ...

            @no_args
            def reset() -> None:
                \"\"\"
                Reset the module state.
                \"\"\"
                ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            path = app_dir / "Example.module.pyi"
            path.write_text(source, encoding="utf-8")
            model = parse(str(path))

        export = model.PythonModule[0]
        self.assertEqual(export.Name, "Example")
        self.assertEqual(export.Namespace, "TestBindings")
        self.assertEqual(export.ModuleName, Path(temp_dir).name)
        self.assertTrue(export.IsExplicitlyExported)
        self.assertEqual([method.Name for method in export.Method], ["ping", "reset"])
        self.assertTrue(export.Method[0].Keyword)
        self.assertTrue(export.Method[1].NoArgs)
        self.assertIn("Example module doc.", export.Documentation.UserDocu)
        self.assertIn("ping(value, /, *, retry=...)", export.Method[0].Documentation.UserDocu)
        self.assertIn("reset()", export.Method[1].Documentation.UserDocu)

    def test_module_stub_uses_filename_defaults_without_module_metadata(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            \"\"\"Example module doc.\"\"\"

            def ping() -> None:
                \"\"\"
                Ping the module.
                \"\"\"
                ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            path = app_dir / "Example.module.pyi"
            path.write_text(source, encoding="utf-8")
            model = parse(str(path))

        export = model.PythonModule[0]
        self.assertEqual(export.Name, "Example")
        self.assertEqual(export.Namespace, "Example")
        self.assertEqual(export.ModuleName, Path(temp_dir).name)
        self.assertFalse(export.IsExplicitlyExported)
        self.assertEqual([method.Name for method in export.Method], ["ping"])
        self.assertIn("Example module doc.", export.Documentation.UserDocu)

    def test_module_stub_rejects_bound_method_decorators(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            @staticmethod
            def ping() -> None:
                ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            path = app_dir / "Example.module.pyi"
            path.write_text(source, encoding="utf-8")

            with self.assertRaisesRegex(
                ValueError, "Module-level function 'ping' cannot use bound-method decorators"
            ):
                parse(str(path))

    def test_module_stub_generation_writes_module_wrapper_files(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import no_args

            \"\"\"Example module doc.\"\"\"

            def ping(value: int, /, *, retry: bool = ...) -> object:
                \"\"\"
                Ping the module.
                \"\"\"
                ...

            @no_args
            def reset() -> None:
                \"\"\"
                Reset the module state.
                \"\"\"
                ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            output_dir = Path(temp_dir) / "generated"
            output_dir.mkdir()
            path = app_dir / "Example.module.pyi"
            path.write_text(source, encoding="utf-8")

            generate(str(path), str(output_dir))

            header = (output_dir / "ExampleModulePy.h").read_text(encoding="utf-8")
            module_cpp = (output_dir / "ExampleModulePy.cpp").read_text(encoding="utf-8")
            module_imp = (output_dir / "ExampleModulePyImp.cpp").read_text(encoding="utf-8")

        self.assertIn("class ExampleModulePy", header)
        self.assertIn("static const char* moduleDocumentation();", header)
        self.assertIn("PyModule_AddFunctions(module, Methods)", module_cpp)
        self.assertIn('return "Example module doc.";', module_cpp)
        self.assertIn('#include "ExampleModulePy.cpp"', module_imp)
        self.assertIn("METH_VARARGS|METH_KEYWORDS", module_cpp)
        self.assertIn("METH_NOARGS", module_cpp)
        self.assertIn('PyErr_SetString(PyExc_NotImplementedError, "Not implemented")', module_imp)


if __name__ == "__main__":
    unittest.main()
