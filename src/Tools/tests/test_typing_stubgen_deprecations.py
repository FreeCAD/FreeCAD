# SPDX-License-Identifier: LGPL-2.1-or-later

import ast
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

from src.Tools.typing.stubgen.class_merge import (  # noqa: E402
    class_public_symbol,
    known_stub_module_roots,
    module_symbol_renames,
    public_class_stub_source,
    public_import_target_index,
)
from src.Tools.typing.stubgen.deprecation import structured_deprecation_message  # noqa: E402
from src.Tools.typing.stubgen.model import BindingClass, BindingMethod  # noqa: E402
from src.Tools.typing.stubgen.render import write_stub_file  # noqa: E402
from src.Tools.typing.stubgen.source_inputs import (  # noqa: E402
    parse_module_stub_signature_overrides,
    parse_source_type_stub_signature_overrides,
)


class TestTypingStubgenDeprecations(unittest.TestCase):
    def _public_class_stub(self, source: str):
        source = textwrap.dedent(source).lstrip()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            stub_path = root / "src" / "App" / "Example.pyi"
            stub_path.parent.mkdir(parents=True)
            stub_path.write_text(source, encoding="utf-8")

            tree = ast.parse(source, filename=str(stub_path))
            class_node = next(node for node in tree.body if isinstance(node, ast.ClassDef))
            klass = BindingClass(
                source="src/App/Example.pyi",
                line=class_node.lineno,
                class_name=class_node.name,
                export_name=f"{class_node.name}Py",
                python_name=f"FreeCAD.{class_node.name}",
                public_names=[f"FreeCAD.{class_node.name}"],
                base_class=None,
                explicit_export=True,
            )
            module_classes = [klass]
            module_name = "FreeCAD"
            module_symbols = {
                symbol
                for item in module_classes
                if (symbol := class_public_symbol(item, module_name)) is not None
            }
            return public_class_stub_source(
                root,
                klass,
                module_name,
                module_symbol_renames(module_classes, module_name),
                module_symbols,
                public_import_target_index(module_classes),
                known_stub_module_roots(module_classes),
            )

    def test_structured_metadata_validates_releases_and_punctuation(self) -> None:
        with self.assertRaisesRegex(ValueError, "later"):
            structured_deprecation_message({"deprecated_in": "27.2", "removed_in": "26.3"})
        self.assertEqual(
            structured_deprecation_message(
                {
                    "deprecated_in": "26.3",
                    "removed_in": "27.2",
                    "details": "Legacy compatibility API.",
                }
            ),
            "since FreeCAD 26.3 and will be removed in FreeCAD 27.2; Legacy compatibility API.",
        )

    def test_public_class_stub_preserves_method_and_attribute_deprecations(self) -> None:
        source = textwrap.dedent("""
            from Base.Metadata import deprecated, deprecated_attributes, export
            from typing import Final

            @export()
            @deprecated_attributes(
                old_attr={
                    "deprecated_in": "26.3",
                    "removed_in": "27.2",
                    "replacement": "new_attr",
                },
                mutable_old={
                    "deprecated_in": "26.3",
                    "removed_in": "27.2",
                    "replacement": "mutable_new",
                },
            )
            class Example:
                old_attr: Final[int] = ...
                \"\"\"Deprecated read-only attribute.\"\"\"

                mutable_old: str = ...
                \"\"\"Deprecated mutable attribute.\"\"\"

                new_attr: Final[int] = ...
                mutable_new: str = ...

                @deprecated(
                    deprecated_in="26.3",
                    removed_in="27.2",
                    replacement="new_method()",
                )
                def old_method(self) -> None:
                    \"\"\"Deprecated method.\"\"\"
                    ...
            """).lstrip()

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            stub_path = root / "src" / "App" / "Example.pyi"
            stub_path.parent.mkdir(parents=True)
            stub_path.write_text(source, encoding="utf-8")

            tree = ast.parse(source, filename=str(stub_path))
            class_node = next(node for node in tree.body if isinstance(node, ast.ClassDef))
            klass = BindingClass(
                source="src/App/Example.pyi",
                line=class_node.lineno,
                class_name="Example",
                export_name="ExamplePy",
                python_name="FreeCAD.Example",
                public_names=["FreeCAD.Example"],
                base_class=None,
                explicit_export=True,
            )
            module_name = "FreeCAD"
            module_classes = [klass]
            module_symbols = {
                symbol
                for klass in module_classes
                if (symbol := class_public_symbol(klass, module_name)) is not None
            }
            stub = public_class_stub_source(
                root,
                klass,
                module_name,
                module_symbol_renames(module_classes, module_name),
                module_symbols,
                public_import_target_index(module_classes),
                known_stub_module_roots(module_classes),
            )

        self.assertIsNotNone(stub)
        assert stub is not None
        self.assertIn("from typing_extensions import deprecated", stub.import_lines)

        public_tree = ast.parse(stub.source)
        public_class = next(node for node in public_tree.body if isinstance(node, ast.ClassDef))
        self.assertEqual(public_class.decorator_list, [])
        self.assertFalse(
            any(
                isinstance(node, ast.AnnAssign)
                and isinstance(node.target, ast.Name)
                and node.target.id in {"old_attr", "mutable_old"}
                for node in public_class.body
            )
        )

        old_method = self._function_with_decorator(public_class.body, "old_method", "deprecated")
        self.assertIsNotNone(old_method)
        assert old_method is not None
        self.assertEqual(
            self._deprecated_message(old_method),
            "since FreeCAD 26.3 and will be removed in FreeCAD 27.2; use new_method() instead.",
        )

        old_attr_getter = self._function_with_decorator(public_class.body, "old_attr", "property")
        self.assertIsNotNone(old_attr_getter)
        self.assertTrue(self._has_decorator(old_attr_getter, "deprecated"))
        self.assertEqual(ast.unparse(old_attr_getter.returns), "int")
        self.assertEqual(ast.get_docstring(old_attr_getter), "Deprecated read-only attribute.")
        self.assertEqual(
            self._deprecated_message(old_attr_getter),
            "since FreeCAD 26.3 and will be removed in FreeCAD 27.2; use new_attr instead.",
        )
        self.assertIsNone(self._function_with_decorator(public_class.body, "old_attr", "setter"))

        mutable_getter = self._function_with_decorator(public_class.body, "mutable_old", "property")
        mutable_setter = self._function_with_decorator(public_class.body, "mutable_old", "setter")
        self.assertIsNotNone(mutable_getter)
        self.assertIsNotNone(mutable_setter)
        self.assertTrue(self._has_decorator(mutable_getter, "deprecated"))
        self.assertTrue(self._has_decorator(mutable_setter, "deprecated"))
        self.assertEqual(ast.unparse(mutable_getter.returns), "str")
        self.assertEqual(
            ast.unparse(mutable_setter.args.args[1].annotation),
            "str",
        )

    def test_method_only_deprecation_adds_public_import(self) -> None:
        stub = self._public_class_stub("""
            from Base.Metadata import deprecated, export

            @export()
            class Example:
                @deprecated(deprecated_in="26.3", removed_in="27.2")
                def old_method(self) -> None: ...
            """)

        self.assertIsNotNone(stub)
        assert stub is not None
        self.assertIn("from typing_extensions import deprecated", stub.import_lines)
        public_class = next(
            node for node in ast.parse(stub.source).body if isinstance(node, ast.ClassDef)
        )
        self.assertIsNotNone(
            self._function_with_decorator(public_class.body, "old_method", "deprecated")
        )

    def test_class_only_deprecation_adds_public_import(self) -> None:
        stub = self._public_class_stub("""
            from Base.Metadata import deprecated, export

            @export()
            @deprecated(deprecated_in="26.3", removed_in="27.2")
            class Example:
                def active_method(self) -> None: ...
            """)

        self.assertIsNotNone(stub)
        assert stub is not None
        self.assertIn("from typing_extensions import deprecated", stub.import_lines)
        public_class = next(
            node for node in ast.parse(stub.source).body if isinstance(node, ast.ClassDef)
        )
        self.assertTrue(self._has_decorator(public_class, "deprecated"))

    def test_module_stub_preserves_deprecated_function_decorators(self) -> None:
        source = textwrap.dedent("""
            from Base.Metadata import deprecated

            @deprecated(
                deprecated_in="26.3",
                removed_in="27.2",
                replacement="replacement()",
            )
            def old_function(value: int, /) -> str:
                \"\"\"Deprecated module function.\"\"\"
                ...
            """).lstrip()

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source_dir = root / "src"
            stub_path = source_dir / "App" / "Example.module.pyi"
            stub_path.parent.mkdir(parents=True)
            stub_path.write_text(source, encoding="utf-8")

            parsed = parse_module_stub_signature_overrides(root, source_dir)
            signature_group, _ = parsed[("Example", "old_function")]
            self.assertEqual(
                signature_group[0].deprecated_message,
                "since FreeCAD 26.3 and will be removed in FreeCAD 27.2; use replacement() instead.",
            )

            method = self._binding_method(
                source="src/App/Example.cpp",
                context_kind="pycxx_module",
                context_name="Example",
                inferred_module="Example",
                python_name="old_function",
            )
            out_path = root / "out" / "Example.pyi"
            write_stub_file(
                out_path,
                [method],
                stub_signature_overrides={
                    (method.source, method.context_name, method.python_name): signature_group
                },
            )

            public_tree = ast.parse(out_path.read_text(encoding="utf-8"), filename=str(out_path))

        self.assertTrue(
            any(
                isinstance(node, ast.ImportFrom)
                and node.module == "typing_extensions"
                and any(alias.name == "deprecated" for alias in node.names)
                for node in public_tree.body
            )
        )
        public_function = next(
            node for node in public_tree.body if isinstance(node, ast.FunctionDef)
        )
        self.assertEqual(
            self._deprecated_message(public_function),
            "since FreeCAD 26.3 and will be removed in FreeCAD 27.2; use replacement() instead.",
        )
        self.assertEqual(ast.get_docstring(public_function), "Deprecated module function.")

    def test_type_stub_preserves_deprecated_method_decorators(self) -> None:
        source = textwrap.dedent("""
            from Base.Metadata import deprecated

            class _Example:
                @deprecated(
                    deprecated_in="26.3",
                    removed_in="27.2",
                    replacement="newMethod",
                )
                def oldMethod(self, value: int, /) -> object:
                    \"\"\"Deprecated typed method.\"\"\"
                    ...
            """).lstrip()

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source_dir = root / "src"
            stub_path = source_dir / "Gui" / "FreeCADGui._Example.pyi"
            stub_path.parent.mkdir(parents=True)
            stub_path.write_text(source, encoding="utf-8")

            parsed = parse_source_type_stub_signature_overrides(root, source_dir)
            signature_group, _ = parsed[("FreeCADGui", "_Example", "oldMethod")]
            self.assertEqual(
                signature_group[0].deprecated_message,
                "since FreeCAD 26.3 and will be removed in FreeCAD 27.2; use newMethod instead.",
            )

            method = self._binding_method(
                source="src/Gui/ExamplePy.cpp",
                context_kind="python_type",
                context_name="_Example",
                inferred_module="FreeCADGui",
                python_name="oldMethod",
            )
            out_path = root / "out" / "FreeCADGui._Example.pyi"
            write_stub_file(
                out_path,
                [method],
                class_name="_Example",
                stub_signature_overrides={
                    (method.source, method.context_name, method.python_name): signature_group
                },
            )

            public_tree = ast.parse(out_path.read_text(encoding="utf-8"), filename=str(out_path))

        self.assertTrue(
            any(
                isinstance(node, ast.ImportFrom)
                and node.module == "typing_extensions"
                and any(alias.name == "deprecated" for alias in node.names)
                for node in public_tree.body
            )
        )
        public_class = next(node for node in public_tree.body if isinstance(node, ast.ClassDef))
        public_method = next(
            node
            for node in public_class.body
            if isinstance(node, ast.FunctionDef) and node.name == "oldMethod"
        )
        self.assertEqual(
            self._deprecated_message(public_method),
            "since FreeCAD 26.3 and will be removed in FreeCAD 27.2; use newMethod instead.",
        )
        self.assertEqual(ast.get_docstring(public_method), "Deprecated typed method.")

    @staticmethod
    def _binding_method(
        *,
        source: str,
        context_kind: str,
        context_name: str,
        inferred_module: str | None,
        python_name: str,
    ) -> BindingMethod:
        return BindingMethod(
            family="pycxx_add_method",
            source=source,
            line=1,
            table=None,
            context_kind=context_kind,
            context_name=context_name,
            inferred_module=inferred_module,
            method_kind="varargs",
            python_name=python_name,
            cxx_callable=f"dummy::{python_name}",
            flags="",
            doc="",
            generated_source=False,
        )

    @staticmethod
    def _function_with_decorator(
        body: list[ast.stmt],
        name: str,
        decorator_name: str,
    ) -> ast.FunctionDef | None:
        for node in body:
            if not isinstance(node, ast.FunctionDef) or node.name != name:
                continue
            if any(
                TestTypingStubgenDeprecations._decorator_name(decorator) == decorator_name
                for decorator in node.decorator_list
            ):
                return node
        return None

    @staticmethod
    def _has_decorator(node: ast.FunctionDef | ast.ClassDef | None, decorator_name: str) -> bool:
        if node is None:
            return False
        return any(
            TestTypingStubgenDeprecations._decorator_name(decorator) == decorator_name
            for decorator in node.decorator_list
        )

    @staticmethod
    def _decorator_name(decorator: ast.expr) -> str:
        if isinstance(decorator, ast.Name):
            return decorator.id
        if isinstance(decorator, ast.Call):
            return TestTypingStubgenDeprecations._decorator_name(decorator.func)
        if isinstance(decorator, ast.Attribute):
            return decorator.attr
        return ast.unparse(decorator)

    @staticmethod
    def _deprecated_message(node: ast.FunctionDef) -> str | None:
        for decorator in node.decorator_list:
            if TestTypingStubgenDeprecations._decorator_name(decorator) != "deprecated":
                continue
            if isinstance(decorator, ast.Call) and decorator.args:
                value = decorator.args[0]
                if isinstance(value, ast.Constant) and isinstance(value.value, str):
                    return value.value
            return ""
        return None


if __name__ == "__main__":
    unittest.main()
