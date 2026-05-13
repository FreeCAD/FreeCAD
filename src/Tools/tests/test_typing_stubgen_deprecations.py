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
from src.Tools.typing.stubgen.model import BindingClass  # noqa: E402


class TestTypingStubgenDeprecations(unittest.TestCase):
    def test_public_class_stub_preserves_method_and_attribute_deprecations(self) -> None:
        source = textwrap.dedent("""
            from Base.Metadata import export, deprecated_attributes
            from typing import Final
            from typing_extensions import deprecated

            @export()
            @deprecated_attributes(
                old_attr="Use new_attr instead.",
                mutable_old="Use mutable_new instead.",
            )
            class Example:
                old_attr: Final[int] = ...
                \"\"\"Deprecated read-only attribute.\"\"\"

                mutable_old: str = ...
                \"\"\"Deprecated mutable attribute.\"\"\"

                new_attr: Final[int] = ...
                mutable_new: str = ...

                @deprecated("Use new_method instead.")
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

        old_attr_getter = self._function_with_decorator(public_class.body, "old_attr", "property")
        self.assertIsNotNone(old_attr_getter)
        self.assertTrue(self._has_decorator(old_attr_getter, "deprecated"))
        self.assertEqual(ast.unparse(old_attr_getter.returns), "int")
        self.assertEqual(ast.get_docstring(old_attr_getter), "Deprecated read-only attribute.")
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
    def _has_decorator(node: ast.FunctionDef | None, decorator_name: str) -> bool:
        if node is None:
            return False
        return any(
            TestTypingStubgenDeprecations._decorator_name(decorator) == decorator_name
            for decorator in node.decorator_list
        )

    @staticmethod
    def _decorator_name(decorator: ast.expr) -> str:
        match decorator:
            case ast.Name(id=name):
                return name
            case ast.Call(func=func):
                return TestTypingStubgenDeprecations._decorator_name(func)
            case ast.Attribute(attr=attr):
                return attr
            case _:
                return ast.unparse(decorator)


if __name__ == "__main__":
    unittest.main()
