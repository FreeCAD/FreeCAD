# SPDX-License-Identifier: LGPL-2.1-or-later

import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

BINDINGS_DIR = Path(__file__).resolve().parents[1]
SRC_DIR = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(BINDINGS_DIR))

from generate import generate, source_dependency_map
from model.generateModel_Python import parse, parse_python_code
from model.typedModel import ParameterType


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

    def test_father_include_is_inferred_from_same_directory_parent_pyi(self):
        parent_source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import export
            from Base.PyObjectBase import PyObjectBase


            @export(Name="NativeParentPy")
            class Parent(PyObjectBase):
                ...
            """)
        child_source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import export
            from Parent import Parent


            @export(Constructor=True)
            class Child(Parent):
                ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            (app_dir / "Parent.pyi").write_text(parent_source, encoding="utf-8")
            child_path = app_dir / "Child.pyi"
            child_path.write_text(child_source, encoding="utf-8")

            model = parse_python_code(str(child_path))
            self.assertEqual(
                model.SourceDependencies,
                [(app_dir / "Parent.pyi").resolve().as_posix()],
            )

        export = model.PythonExport[0]
        self.assertEqual(export.Father, "NativeParentPy")
        self.assertEqual(
            export.FatherInclude,
            f"Mod/{Path(temp_dir).name}/App/NativeParentPy.h",
        )

    def test_father_include_is_inferred_from_runtime_style_parent_import(self):
        parent_source = textwrap.dedent("""
            from __future__ import annotations

            from Base.PyObjectBase import PyObjectBase


            class Parent(PyObjectBase):
                ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            module_name = Path(temp_dir).name
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            (app_dir / "Parent.pyi").write_text(parent_source, encoding="utf-8")
            child_path = app_dir / "Child.pyi"
            child_path.write_text(
                textwrap.dedent(f"""
                    from __future__ import annotations

                    from Base.Metadata import export
                    from {module_name}.Parent import Parent


                    @export(Constructor=True)
                    class Child(Parent):
                        ...
                    """),
                encoding="utf-8",
            )

            model = parse_python_code(str(child_path))
            self.assertEqual(
                model.SourceDependencies,
                [(app_dir / "Parent.pyi").resolve().as_posix()],
            )

        export = model.PythonExport[0]
        self.assertEqual(export.Father, "ParentPy")
        self.assertEqual(export.FatherInclude, f"Mod/{module_name}/App/ParentPy.h")

    def test_generation_writes_depfile_for_inferred_parent_pyi(self):
        parent_source = textwrap.dedent("""
            from __future__ import annotations

            from Base.PyObjectBase import PyObjectBase


            class Parent(PyObjectBase):
                ...
            """)
        child_source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import export
            from Parent import Parent


            @export(Constructor=True)
            class Child(Parent):
                ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            parent_path = app_dir / "Parent.pyi"
            child_path = app_dir / "Child.pyi"
            output_dir = Path(temp_dir) / "generated"
            depfile = output_dir / "ChildPy.d"
            parent_path.write_text(parent_source, encoding="utf-8")
            child_path.write_text(child_source, encoding="utf-8")

            generate(str(child_path), str(output_dir), str(depfile))
            depfile_text = depfile.read_text(encoding="utf-8")

        self.assertIn((output_dir / "ChildPy.h").resolve().as_posix(), depfile_text)
        self.assertIn((output_dir / "ChildPy.cpp").resolve().as_posix(), depfile_text)
        self.assertIn(child_path.resolve().as_posix(), depfile_text)
        self.assertIn(parent_path.resolve().as_posix(), depfile_text)

    def test_source_dependency_map_groups_dependencies_by_input(self):
        parent_source = textwrap.dedent("""
            from __future__ import annotations

            from Base.PyObjectBase import PyObjectBase


            class Parent(PyObjectBase):
                ...
            """)
        child_source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import export
            from Parent import Parent


            @export(Constructor=True)
            class Child(Parent):
                ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            parent_path = app_dir / "Parent.pyi"
            first_child_path = app_dir / "FirstChild.pyi"
            second_child_path = app_dir / "SecondChild.pyi"
            parent_path.write_text(parent_source, encoding="utf-8")
            first_child_path.write_text(
                child_source.replace("Child", "FirstChild"), encoding="utf-8"
            )
            second_child_path.write_text(
                child_source.replace("Child", "SecondChild"), encoding="utf-8"
            )

            dependency_map = source_dependency_map([first_child_path, second_child_path])

        self.assertEqual(
            dependency_map,
            [
                {
                    "source": first_child_path.resolve().as_posix(),
                    "dependencies": [parent_path.resolve().as_posix()],
                },
                {
                    "source": second_child_path.resolve().as_posix(),
                    "dependencies": [parent_path.resolve().as_posix()],
                },
            ],
        )

    def test_annotated_vector_attributes_add_geometry_header_include(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from typing import Annotated

            from Base import Vector
            from Base.Metadata import cxx_type, export
            from Base.PyObjectBase import PyObjectBase


            @export(Constructor=True)
            class Example(PyObjectBase):
                Position: Annotated[Vector, cxx_type("Vector")]
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            example_path = app_dir / "Example.pyi"
            output_dir = Path(temp_dir) / "generated"
            example_path.write_text(source, encoding="utf-8")

            model = parse_python_code(str(example_path))
            export = model.PythonExport[0]
            generate(str(example_path), str(output_dir))
            header_text = (output_dir / "ExamplePy.h").read_text(encoding="utf-8")

        self.assertEqual(export.FatherInclude, "Base/PyObjectBase.h")
        self.assertEqual(export.HeaderIncludes, ["Base/GeometryPyCXX.h"])
        self.assertIn("#include <Base/PyObjectBase.h>", header_text)
        self.assertIn("#include <Base/GeometryPyCXX.h>", header_text)
        self.assertIn("ExamplePy : public Base::PyObjectBase", header_text)

    def test_vector_attributes_remain_object_attributes_without_cxx_type(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            import Base
            from Base.Vector import Vector
            from Base.Metadata import export
            from Base.PyObjectBase import PyObjectBase


            @export(Constructor=True)
            class Example(PyObjectBase):
                Position: Vector
                Direction: Base.Vector
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR / "Mod") as temp_dir:
            app_dir = Path(temp_dir) / "App"
            app_dir.mkdir()
            example_path = app_dir / "Example.pyi"
            example_path.write_text(source, encoding="utf-8")
            export = parse_python_code(str(example_path)).PythonExport[0]

        self.assertEqual(export.HeaderIncludes, [])
        self.assertEqual(
            [attr.Parameter.Type for attr in export.Attribute],
            [ParameterType.OBJECT, ParameterType.OBJECT],
        )

    def test_twin_pointer_defaults_to_twin(self):
        source = textwrap.dedent("""
            from __future__ import annotations

            from Base.Metadata import export
            from Base.PyObjectBase import PyObjectBase


            @export(Twin="NativeTwin")
            class Example(PyObjectBase):
                ...
            """)

        with tempfile.TemporaryDirectory(dir=SRC_DIR) as temp_dir:
            path = Path(temp_dir) / "Example.pyi"
            path.write_text(source, encoding="utf-8")
            model = parse_python_code(str(path))

        export = model.PythonExport[0]
        self.assertEqual(export.Twin, "NativeTwin")
        self.assertEqual(export.TwinPointer, "NativeTwin")

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
