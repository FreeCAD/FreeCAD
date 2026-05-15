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

            from Base.Metadata import bootstrap_export, module, no_args

            \"\"\"Example module doc.\"\"\"

            module(Name="Example", Namespace="TestBindings", Include="ExampleBinding.h")

            def ping(value: int, /, *, retry: bool = ...) -> object:
                \"\"\"
                Ping the module.
                \"\"\"
                ...

            @bootstrap_export
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
        self.assertEqual(export.Include, "ExampleBinding.h")
        self.assertEqual(export.ModuleName, Path(temp_dir).name)
        self.assertTrue(export.IsExplicitlyExported)
        self.assertEqual([method.Name for method in export.Method], ["ping", "reset"])
        self.assertTrue(export.Method[0].Keyword)
        self.assertTrue(export.Method[1].NoArgs)
        self.assertFalse(export.Method[0].Bootstrap)
        self.assertTrue(export.Method[1].Bootstrap)
        self.assertEqual([method.Name for method in export.BootstrapMethods], ["reset"])
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

    def test_module_stub_allows_helper_classes_and_enums(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from enum import IntEnum
            from typing import Protocol

            class Mode(IntEnum):
                One = 1

            class Gate(Protocol):
                def allow(self, /) -> bool: ...

            def ping() -> None:
                ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            path = app_dir / "Example.module.pyi"
            path.write_text(source, encoding="utf-8")
            model = parse(str(path))

        export = model.PythonModule[0]
        self.assertEqual([method.Name for method in export.Method], ["ping"])

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

    def test_module_stub_rejects_bootstrap_export_on_class_methods(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import bootstrap_export, export
            from Base.PyObjectBase import PyObjectBase


            @export
            class Example(PyObjectBase):
                @bootstrap_export
                def ping(self) -> None:
                    ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR) as temp_dir:
            path = Path(temp_dir) / "Example.pyi"
            path.write_text(source, encoding="utf-8")

            with self.assertRaisesRegex(
                ValueError, "Class method 'ping' cannot use bootstrap_export decorator"
            ):
                parse_python_code(str(path))

    def test_module_stub_supports_callback_backed_overloads(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from typing import overload

            from Base.Metadata import callback

            @callback("TestBindings::sListSchemas")
            @overload
            def listSchemas() -> tuple[str, ...]:
                \"\"\"Return all schemas.\"\"\"
                ...

            @overload
            def listSchemas(index: int, /) -> str:
                \"\"\"Return one schema.\"\"\"
                ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            path = app_dir / "Example.module.pyi"
            path.write_text(source, encoding="utf-8")
            model = parse(str(path))

        export = model.PythonModule[0]
        self.assertEqual([method.Name for method in export.Method], ["listSchemas"])
        self.assertEqual(export.Method[0].Callback, "TestBindings::sListSchemas")
        self.assertIn("listSchemas()", export.Method[0].Documentation.UserDocu)
        self.assertIn("listSchemas(index, /)", export.Method[0].Documentation.UserDocu)

    def test_module_stub_infers_callback_symbols_from_module_metadata(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import callback, module

            module(CallbackOwner="TestBindings", CallbackPrefix="s")

            def loadFile(path: str, /) -> None:
                ...

            @callback("TestBindings::sOpenDocument")
            def open(path: str, /) -> None:
                ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            path = app_dir / "Example.module.pyi"
            path.write_text(source, encoding="utf-8")
            model = parse(str(path))

        export = model.PythonModule[0]
        self.assertEqual(export.Method[0].Callback, "TestBindings::sLoadFile")
        self.assertEqual(export.Method[1].Callback, "TestBindings::sOpenDocument")

    def test_module_stub_rejects_callback_prefix_without_owner(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import module

            module(CallbackPrefix="s")

            def ping() -> None:
                ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            path = app_dir / "Example.module.pyi"
            path.write_text(source, encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "CallbackPrefix requires CallbackOwner"):
                parse(str(path))

    def test_extension_module_stub_requires_module_class(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import module

            module(Runtime="ExtensionModule")

            def ping() -> None:
                ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            path = app_dir / "Example.module.pyi"
            path.write_text(source, encoding="utf-8")

            with self.assertRaisesRegex(
                ValueError, "Runtime='ExtensionModule' requires ModuleClass"
            ):
                parse(str(path))

    def test_module_stub_generation_writes_module_wrapper_files(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import bootstrap_export, no_args

            \"\"\"Example module doc.\"\"\"

            def ping(value: int, /, *, retry: bool = ...) -> object:
                \"\"\"
                Ping the module.
                \"\"\"
                ...

            @bootstrap_export
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
        self.assertIn("static PyMethodDef BootstrapMethods[];", header)
        self.assertIn("PyModule_AddFunctions(module, Methods)", module_cpp)
        self.assertIn('return "Example module doc.";', module_cpp)
        self.assertIn('#include "ExampleModulePy.cpp"', module_imp)
        self.assertIn("METH_VARARGS|METH_KEYWORDS", module_cpp)
        self.assertIn("METH_NOARGS", module_cpp)
        self.assertIn('PyErr_SetString(PyExc_NotImplementedError, "Not implemented")', module_imp)

        bootstrap_table = module_cpp.split(
            "PyMethodDef ExampleModulePy::BootstrapMethods[] = {", 1
        )[1]
        bootstrap_table = bootstrap_table.split("};", 1)[0]
        self.assertIn('{"reset"', bootstrap_table)
        self.assertNotIn('{"ping"', bootstrap_table)

    def test_module_stub_generation_uses_existing_callback_symbols(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from typing import overload

            from Base.Metadata import callback, module

            module(Include="ExampleBinding.h")

            @callback("TestBindings::sListSchemas")
            @overload
            def listSchemas() -> tuple[str, ...]:
                ...

            @overload
            def listSchemas(index: int, /) -> str:
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

        self.assertIn("TestBindings::sListSchemas", module_cpp)
        self.assertIn('#include "ExampleBinding.h"', module_cpp)
        self.assertNotIn("staticCallback_listSchemas", header)
        self.assertNotIn("staticCallback_listSchemas", module_cpp)
        self.assertNotIn("PyObject* ExampleModulePy::listSchemas", module_imp)

    def test_module_stub_generation_uses_inferred_callback_symbols(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import module

            module(Include="ExampleBinding.h", CallbackOwner="TestBindings", CallbackPrefix="s")

            def loadFile(path: str, /) -> None:
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

        self.assertIn("TestBindings::sLoadFile", module_cpp)
        self.assertIn('#include "ExampleBinding.h"', module_cpp)
        self.assertNotIn("staticCallback_loadFile", header)
        self.assertNotIn("staticCallback_loadFile", module_cpp)
        self.assertNotIn("PyObject* ExampleModulePy::loadFile", module_imp)

    def test_extension_module_stub_generation_writes_extension_helper_files(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import module

            \"\"\"Example helper module.\"\"\"

            module(
                Name="Example",
                Namespace="TestBindings",
                Include="ExampleBinding.h",
                Runtime="ExtensionModule",
                ModuleClass="ExampleBinding",
            )

            def ping(value: int, /) -> object:
                \"\"\"
                Ping the module.
                \"\"\"
                ...

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

        self.assertIn("static void initialize(ExampleBinding& module);", header)
        self.assertNotIn("PyMethodDef Methods[]", header)
        self.assertIn('#include "ExampleBinding.h"', module_cpp)
        self.assertIn("module.add_varargs_method(", module_cpp)
        self.assertIn("&ExampleBinding::ping", module_cpp)
        self.assertIn("&ExampleBinding::reset", module_cpp)
        self.assertIn("module.initialize(moduleDocumentation())", module_cpp)
        self.assertIn('return "Example helper module.";', module_cpp)
        self.assertNotIn("PyErr_SetString(PyExc_NotImplementedError", module_imp)


if __name__ == "__main__":
    unittest.main()
