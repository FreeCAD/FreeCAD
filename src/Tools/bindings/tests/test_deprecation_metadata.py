"""Tests for explicit and legacy deprecation metadata in Python binding stubs."""

from __future__ import annotations

import ast
from pathlib import Path
import re
import sys
import tempfile
import textwrap
import unittest

BINDINGS_ROOT = Path(__file__).resolve().parents[1]
SRC_ROOT = Path(__file__).resolve().parents[3]
if str(BINDINGS_ROOT) not in sys.path:
    sys.path.insert(0, str(BINDINGS_ROOT))


from model.generateModel_Python import parse  # noqa: E402
from model.typedModel import DeprecationLifecycle  # noqa: E402
from templates.templateClassPyExport import TemplateClassPyExport  # noqa: E402
from templates.templateModulePyExport import TemplateModulePyExport  # noqa: E402


class DeprecationMetadataTest(unittest.TestCase):
    def _write_stub(self, tempdir: str, relative_path: str, contents: str) -> Path:
        path = Path(tempdir, relative_path)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(textwrap.dedent(contents).strip() + "\n", encoding="utf-8")
        return path

    def _parse_export(self, relative_path: str, contents: str):
        with tempfile.TemporaryDirectory() as tempdir:
            stub_path = self._write_stub(tempdir, relative_path, contents)
            model = parse(str(stub_path))
            self.assertEqual(len(model.PythonExport), 1)
            return model.PythonExport[0]

    def _generate_cpp(self, relative_path: str, contents: str) -> str:
        with tempfile.TemporaryDirectory() as tempdir:
            stub_path = self._write_stub(tempdir, relative_path, contents)
            output_dir = Path(tempdir, "generated")
            output_dir.mkdir()

            model = parse(str(stub_path))
            export = model.PythonExport[0]

            template = TemplateClassPyExport()
            template.outputDir = str(output_dir) + "/"
            template.inputDir = str(stub_path.parent) + "/"
            template.export = export
            template.is_python = True
            template.Generate()

            return (output_dir / f"{export.Name}.cpp").read_text(encoding="utf-8")

    def _generate_module_cpp(self, relative_path: str, contents: str) -> str:
        with tempfile.TemporaryDirectory() as tempdir:
            stub_path = self._write_stub(tempdir, relative_path, contents)
            output_dir = Path(tempdir, "generated")
            output_dir.mkdir()

            model = parse(str(stub_path))
            export = model.PythonModule[0]

            template = TemplateModulePyExport()
            template.outputDir = str(output_dir) + "/"
            template.inputDir = str(stub_path.parent) + "/"
            template.export = export
            template.Generate()

            return (output_dir / f"{export.Name}ModulePy.cpp").read_text(encoding="utf-8")

    def test_rejects_positional_deprecated_decorator(self) -> None:
        with self.assertRaisesRegex(ValueError, "keyword arguments"):
            self._parse_export(
                "src/Base/TestThing.pyi",
                """
                from Metadata import deprecated, export
                from PyObjectBase import PyObjectBase

                @export()
                class TestThing(PyObjectBase):
                    @deprecated("Use newName instead.")
                    def oldName(self) -> None: ...
                """,
            )

    def test_deprecated_part_bindings_declare_registered_python_names(self) -> None:
        registrations = (SRC_ROOT / "Mod/Part/App/AppPart.cpp").read_text(encoding="utf-8")
        expected = {
            "Mod/Part/App/TopoShape.pyi": ("TopoShape", "Part.Shape"),
            "Mod/Part/App/TopoShapeFace.pyi": ("TopoShapeFace", "Part.Face"),
            "Mod/Part/App/TopoShapeWire.pyi": ("TopoShapeWire", "Part.Wire"),
        }

        for relative_path, (native_name, python_name) in expected.items():
            model = parse(str(SRC_ROOT / relative_path))
            self.assertEqual(model.PythonExport[0].PythonName, python_name)
            public_leaf = python_name.rsplit(".", 1)[-1]
            self.assertRegex(
                registrations,
                rf"addType\(&Part::{native_name}Py\s*::Type,partModule,\"{public_leaf}\"\)",
            )

    def test_parses_structured_deprecation_lifecycle(self) -> None:
        export = self._parse_export(
            "src/Base/TestThing.pyi",
            """
            from __future__ import annotations

            from Metadata import deprecated, export
            from PyObjectBase import PyObjectBase

            @export()
            class TestThing(PyObjectBase):
                @deprecated(
                    deprecated_in="26.3",
                    removed_in="27.2",
                    replacement="newName()",
                )
                def oldName(self) -> None: ...
            """,
        )

        methods = {method.Name: method for method in export.Methode}
        self.assertEqual(
            methods["oldName"].Deprecated,
            DeprecationLifecycle("26.3", "27.2", "newName()"),
        )

    def test_structured_deprecation_requires_both_releases(self) -> None:
        with self.assertRaisesRegex(ValueError, "removed_in"):
            self._parse_export(
                "src/Base/TestThing.pyi",
                """
                from Metadata import deprecated, export
                from PyObjectBase import PyObjectBase

                @export()
                class TestThing(PyObjectBase):
                    @deprecated(deprecated_in="26.3")
                    def oldName(self) -> None: ...
                """,
            )

    def test_structured_deprecation_rejects_unknown_fields(self) -> None:
        with self.assertRaisesRegex(ValueError, "unknown keyword 'deadline'"):
            self._parse_export(
                "src/Base/TestThing.pyi",
                """
                from Metadata import deprecated, export
                from PyObjectBase import PyObjectBase

                @export()
                class TestThing(PyObjectBase):
                    @deprecated(
                        deprecated_in="26.3",
                        removed_in="27.2",
                        deadline="27.2",
                    )
                    def oldName(self) -> None: ...
                """,
            )

    def test_structured_deprecation_rejects_invalid_release_order(self) -> None:
        with self.assertRaisesRegex(ValueError, "later"):
            self._parse_export(
                "src/Base/TestThing.pyi",
                """
                from Metadata import deprecated, export
                from PyObjectBase import PyObjectBase

                @export()
                class TestThing(PyObjectBase):
                    @deprecated(deprecated_in="27.2", removed_in="26.3")
                    def oldName(self) -> None: ...
                """,
            )

    def test_parses_structured_deprecated_attribute(self) -> None:
        export = self._parse_export(
            "src/Base/TestThing.pyi",
            """
            from Metadata import deprecated_attributes, export
            from PyObjectBase import PyObjectBase

            @export()
            @deprecated_attributes(
                oldAttr={
                    "deprecated_in": "26.3",
                    "removed_in": "27.2",
                    "replacement": "newAttr",
                    "details": "Legacy compatibility API.",
                },
            )
            class TestThing(PyObjectBase):
                oldAttr: object = ...
            """,
        )

        attributes = {attribute.Name: attribute for attribute in export.Attribute}
        self.assertEqual(
            attributes["oldAttr"].Deprecated,
            DeprecationLifecycle(
                "26.3",
                "27.2",
                "newAttr",
                "Legacy compatibility API.",
            ),
        )

    def test_structured_deprecated_attributes_ignore_docstring_markers(self) -> None:
        export = self._parse_export(
            "src/Base/TestThing.pyi",
            """
            from __future__ import annotations

            from Metadata import deprecated_attributes, export
            from PyObjectBase import PyObjectBase
            from typing import Final

            @export()
            @deprecated_attributes(
                oldAttr={
                    "deprecated_in": "26.3",
                    "removed_in": "27.2",
                    "replacement": "newAttr",
                },
            )
            class TestThing(PyObjectBase):
                oldAttr: Final[object] = ...
                \"\"\"Deprecated -- old docstring fallback.\"\"\"
            """,
        )

        attributes = {attribute.Name: attribute for attribute in export.Attribute}
        self.assertEqual(
            attributes["oldAttr"].Deprecated,
            DeprecationLifecycle("26.3", "27.2", "newAttr"),
        )

    def test_docstring_markers_do_not_deprecate_bindings(self) -> None:
        export = self._parse_export(
            "src/Base/TestThing.pyi",
            """
            from __future__ import annotations

            from Metadata import export
            from PyObjectBase import PyObjectBase
            from typing import Final

            @export()
            class TestThing(PyObjectBase):
                legacyAttr: Final[object] = ...
                \"\"\"Deprecated -- use replacementAttr.\"\"\"

                def legacyMethod(self) -> None:
                    \"\"\"Deprecated: use replacementMethod.\"\"\"
                    ...
            """,
        )

        attributes = {attribute.Name: attribute for attribute in export.Attribute}
        methods = {method.Name: method for method in export.Methode}
        self.assertIsNone(attributes["legacyAttr"].Deprecated)
        self.assertIsNone(methods["legacyMethod"].Deprecated)

    def test_unknown_deprecated_attribute_metadata_raises_clean_error(self) -> None:
        with tempfile.TemporaryDirectory() as tempdir:
            stub_path = self._write_stub(
                tempdir,
                "src/Base/TestThing.pyi",
                """
                from __future__ import annotations

                from Metadata import export, deprecated_attributes
                from PyObjectBase import PyObjectBase
                from typing import Final

                @export()
                @deprecated_attributes(
                    Missing={"deprecated_in": "26.3", "removed_in": "27.2"},
                )
                class TestThing(PyObjectBase):
                    Present: Final[object] = ...
                    \"\"\"A valid attribute.\"\"\"
                """,
            )

            with self.assertRaisesRegex(
                Exception,
                "Unknown deprecated attribute metadata for class 'TestThing': Missing",
            ):
                parse(str(stub_path))

    def test_generation_emits_runtime_warnings_for_methods_and_attributes(self) -> None:
        generated_cpp = self._generate_cpp(
            "src/Base/TestThing.pyi",
            """
            from __future__ import annotations

            from Metadata import deprecated, deprecated_attributes, export
            from PyObjectBase import PyObjectBase
            from typing import Final

            @export()
            @deprecated_attributes(
                oldAttr={
                    "deprecated_in": "26.3",
                    "removed_in": "27.2",
                    "replacement": "newAttr",
                },
            )
            class TestThing(PyObjectBase):
                oldAttr: Final[object] = ...
                \"\"\"Legacy alias for newAttr.\"\"\"

                @deprecated(
                    deprecated_in="26.3",
                    removed_in="27.2",
                    replacement="newMethod",
                    details="Compatibility method.",
                )
                def oldMethod(self) -> None:
                    \"\"\"Legacy alias for newMethod.\"\"\"
                    ...
            """,
        )

        self.assertIn(
            "#include <Base/Interpreter.h>",
            generated_cpp,
        )
        self.assertIn(
            '"Base.TestThing.oldMethod"',
            generated_cpp,
        )
        self.assertIn(
            '.replacement = "newMethod"',
            generated_cpp,
        )
        self.assertIn(
            '.details = "Compatibility method."',
            generated_cpp,
        )
        self.assertIn(
            '"Base.TestThing.oldAttr"',
            generated_cpp,
        )
        self.assertIn(
            '.replacement = "newAttr"',
            generated_cpp,
        )

    def test_generation_skips_warning_helper_without_deprecations(self) -> None:
        generated_cpp = self._generate_cpp(
            "src/Base/TestThing.pyi",
            """
            from __future__ import annotations

            from Metadata import export
            from PyObjectBase import PyObjectBase
            from typing import Final

            @export()
            class TestThing(PyObjectBase):
                activeAttr: Final[object] = ...
                \"\"\"Normal attribute.\"\"\"

                def activeMethod(self) -> None:
                    \"\"\"Normal method.\"\"\"
                    ...
            """,
        )

        self.assertNotIn("Base::warnDeprecatedPythonApi(", generated_cpp)
        self.assertNotIn("#include <Base/Interpreter.h>", generated_cpp)

    def test_deprecated_module_function_generates_runtime_warning(self) -> None:
        generated_cpp = self._generate_module_cpp(
            "src/Mod/Example/App/Example.module.pyi",
            """
            from Base.Metadata import deprecated

            @deprecated(
                deprecated_in="26.3",
                removed_in="27.2",
                replacement="newFunction",
            )
            def oldFunction(value: int, /) -> None: ...
            """,
        )

        self.assertIn("#include <Base/Interpreter.h>", generated_cpp)
        self.assertIn(
            '"Example.oldFunction"',
            generated_cpp,
        )
        self.assertIn(
            '.deprecatedIn = "26.3"',
            generated_cpp,
        )
        self.assertIn(
            '.removedIn = "27.2"',
            generated_cpp,
        )
        self.assertIn(
            '.replacement = "newFunction"',
            generated_cpp,
        )

    def test_repo_stubs_do_not_use_legacy_deprecation_markers(self) -> None:
        legacy_marker = re.compile(r"Deprecated:|deprecated --|Deprecated --|deprecated -")
        offenders = []

        for path in SRC_ROOT.rglob("*.pyi"):
            text = path.read_text(encoding="utf-8")
            tree = ast.parse(text, filename=str(path))

            for node in tree.body:
                if isinstance(node, ast.ClassDef):
                    deprecated_attributes = self._deprecated_attributes(node)
                    for stmt, docstring in self._attribute_docstrings(node):
                        if (
                            legacy_marker.search(docstring)
                            and stmt.target.id not in deprecated_attributes
                        ):
                            offenders.append(
                                f"{path.relative_to(SRC_ROOT).as_posix()}:{stmt.lineno}:{stmt.target.id}"
                            )

                    for func in self._function_defs(node.body):
                        docstring = ast.get_docstring(func) or ""
                        if legacy_marker.search(docstring) and not self._has_deprecated_decorator(
                            func
                        ):
                            offenders.append(
                                f"{path.relative_to(SRC_ROOT).as_posix()}:{func.lineno}:{func.name}"
                            )

                elif isinstance(node, ast.FunctionDef):
                    docstring = ast.get_docstring(node) or ""
                    if legacy_marker.search(docstring) and not self._has_deprecated_decorator(node):
                        offenders.append(
                            f"{path.relative_to(SRC_ROOT).as_posix()}:{node.lineno}:{node.name}"
                        )

        self.assertEqual(offenders, [])

    def _has_deprecated_decorator(self, node: ast.AST) -> bool:
        decorators = getattr(node, "decorator_list", [])
        return any(self._decorator_name(decorator) == "deprecated" for decorator in decorators)

    def _decorator_name(self, decorator: ast.expr) -> str | None:
        match decorator:
            case ast.Name(id=name):
                return name
            case ast.Attribute(attr=attr):
                return attr
            case ast.Call(func=func):
                return self._decorator_name(func)
        return None

    def _deprecated_attributes(self, node: ast.ClassDef) -> set[str]:
        names = set()
        for decorator in node.decorator_list:
            if not isinstance(decorator, ast.Call):
                continue
            if self._decorator_name(decorator) != "deprecated_attributes":
                continue
            for keyword in decorator.keywords:
                if keyword.arg:
                    names.add(keyword.arg)
        return names

    def _attribute_docstrings(self, node: ast.ClassDef):
        for index, stmt in enumerate(node.body):
            if not isinstance(stmt, ast.AnnAssign) or not isinstance(stmt.target, ast.Name):
                continue
            if index + 1 >= len(node.body):
                continue
            next_stmt = node.body[index + 1]
            if (
                isinstance(next_stmt, ast.Expr)
                and isinstance(next_stmt.value, ast.Constant)
                and isinstance(next_stmt.value.value, str)
            ):
                yield stmt, next_stmt.value.value

    def _function_defs(self, nodes: list[ast.stmt]) -> list[ast.FunctionDef]:
        funcs = []
        for node in nodes:
            if isinstance(node, ast.FunctionDef):
                funcs.append(node)
            elif isinstance(node, ast.If):
                funcs.extend(self._function_defs(node.body))
                funcs.extend(self._function_defs(node.orelse))
        return funcs


if __name__ == "__main__":
    unittest.main()
