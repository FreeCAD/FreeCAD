# pyright: strict

"""Contract tests for Python API MDX rendering."""

from __future__ import annotations

import ast
from pathlib import Path
import unittest

from stubgen.python_api.markdown import render_class_page, render_callable_group
from stubgen.python_api.model import ApiCallableGroup, ApiClass
from stubgen.signature_parser import group_callable_definitions, parse_callable_group


class PythonApiMarkdownTests(unittest.TestCase):
    def test_docstring_braces_are_escaped_for_mdx(self) -> None:
        tree = ast.parse('''
def setMaterial(material: object) -> None:
    """Accept {Int:Material,...} material mappings."""
    ...
''')
        node = tree.body[0]
        assert isinstance(node, ast.FunctionDef)
        group = ApiCallableGroup(
            name="setMaterial",
            signatures=parse_callable_group(group_callable_definitions(tree.body)["setMaterial"]),
            doc=ast.get_docstring(node),
        )

        output = "\n".join(render_callable_group(group, source_base_url=None))

        self.assertIn(r"Accept \{Int:Material,...\} material mappings.", output)

    def test_class_docstring_braces_are_escaped_for_mdx(self) -> None:
        page = render_class_page(
            Path("."),
            ApiClass(name="Example", module_name="FreeCAD", doc="Use {name} here."),
            source_base_url=None,
        )

        self.assertIn(r"Use \{name\} here.", page)

    def test_docstring_import_examples_are_not_treated_as_mdx_imports(self) -> None:
        tree = ast.parse('''
def example() -> None:
    """Example usage:

    import Mesh
    for item in mesh:
        Mesh.show(item)
    """
    ...
''')
        node = tree.body[0]
        assert isinstance(node, ast.FunctionDef)
        group = ApiCallableGroup(
            name="example",
            signatures=parse_callable_group(group_callable_definitions(tree.body)["example"]),
            doc=ast.get_docstring(node),
        )

        output = "\n".join(render_callable_group(group, source_base_url=None))

        self.assertIn("&#105;mport Mesh", output)

    def test_docstring_angle_brackets_are_not_treated_as_jsx(self) -> None:
        tree = ast.parse('''
def contact() -> None:
    """Contact <maintainer@example.com> for details."""
    ...
''')
        node = tree.body[0]
        assert isinstance(node, ast.FunctionDef)
        group = ApiCallableGroup(
            name="contact",
            signatures=parse_callable_group(group_callable_definitions(tree.body)["contact"]),
            doc=ast.get_docstring(node),
        )

        output = "\n".join(render_callable_group(group, source_base_url=None))

        self.assertIn("&lt;maintainer@example.com&gt;", output)


if __name__ == "__main__":
    unittest.main()
