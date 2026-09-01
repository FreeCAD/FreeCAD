# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import ast
from pathlib import Path
import sys
import unittest

TYPING_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TYPING_DIR))

from stubgen.init_exports import (  # noqa: E402
    ModuleExport,
    _render_function_stub,
    load_init_exports,
    render_init_exports,
)
from stubgen.module_merge import (  # noqa: E402
    GENERATED_STUB_HEADER,
    merge_module_support_nodes,
)

ROOT_DIR = Path(__file__).resolve().parents[4]


class InitExportsTests(unittest.TestCase):
    def test_units_tables_produce_public_exports(self):
        exports = load_init_exports(ROOT_DIR)
        unit_exports = {
            export.name: export for export in exports if export.module == "FreeCAD.Units"
        }
        quantity_exports = {
            name for name, export in unit_exports.items() if export.type_expression == "Quantity"
        }
        dimensional_exports = {
            name for name, export in unit_exports.items() if export.type_expression == "Unit"
        }
        enum_exports = {
            name
            for name, export in unit_exports.items()
            if export.class_definition and f"class {name}(IntEnum):" in export.class_definition
        }

        self.assertEqual(170, len(unit_exports))
        self.assertEqual(117, len(quantity_exports))
        self.assertEqual(51, len(dimensional_exports))
        self.assertEqual({"Scheme", "NumberFormat"}, enum_exports)
        self.assertIn("NanoMetre", quantity_exports)
        self.assertIn("YoungsModulus", dimensional_exports)
        self.assertNotIn("Oersted", unit_exports)

        app_exports = {export.name: export for export in exports if export.module == "FreeCAD"}
        self.assertEqual(
            {"Logger", "ScaleType", "PropertyType", "ReturnType"},
            set(app_exports),
        )
        self.assertEqual("type[FCADLogger]", app_exports["Logger"].type_expression)
        self.assertEqual("type[ScaleType]", app_exports["ScaleType"].type_expression)
        self.assertTrue(all(export.doc for export in exports))
        self.assertFalse(any(export.doc.startswith("Named ") for export in exports))
        self.assertIn("class FCADLogger:", app_exports["Logger"].class_definition)
        self.assertIn(
            "def __init__(self, tag: str, **kwargs: object) -> None:",
            app_exports["Logger"].class_definition,
        )
        self.assertIn(
            "def isEnabledFor(self, level: int | str) -> bool:",
            app_exports["Logger"].class_definition,
        )
        self.assertIn("def report(", app_exports["Logger"].class_definition)
        self.assertIn(
            "Catch any exception report it with a message box.",
            app_exports["Logger"].class_definition,
        )
        self.assertIn("def error(", app_exports["Logger"].class_definition)
        self.assertIn("Log an error-level message.", app_exports["Logger"].class_definition)
        self.assertIn(
            "Call *func* and catch exceptions at the error level.",
            app_exports["Logger"].class_definition,
        )
        self.assertEqual(1, app_exports["Logger"].class_definition.count("def __init__("))
        self.assertEqual(1, app_exports["Logger"].class_definition.count("def isEnabledFor("))
        self.assertIn("class Scheme(IntEnum):", unit_exports["Scheme"].class_definition)

    def test_bootstrap_class_stubs_are_not_manually_duplicated(self):
        source = (ROOT_DIR / "src/App/FreeCAD.module.pyi").read_text(encoding="utf-8")
        self.assertNotIn("class FCADLogger", source)
        self.assertNotIn("class ScaleType", source)
        self.assertFalse((ROOT_DIR / "src/App/FreeCAD.Units.module.pyi").exists())

    def test_rendered_fragment_is_valid_stub_source(self):
        source = render_init_exports(load_init_exports(ROOT_DIR))
        tree = ast.parse(source)
        names = {
            node.target.id
            for node in tree.body
            if isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name)
        }
        self.assertIn("NanoMetre", names)
        self.assertIn("YoungsModulus", names)
        self.assertIn("Logger", names)
        self.assertIn("ScaleType", names)
        self.assertIn("Scheme", names)
        self.assertIn("from FreeCAD.Base import Quantity, Unit", source)
        self.assertIn("Logger: type[FCADLogger]", source)

    def test_attribute_documentation_survives_module_merge(self):
        source = render_init_exports(
            (ModuleExport("FreeCAD.Units", "NanoMetre", "Quantity", "One nanometre."),)
        )
        merged = merge_module_support_nodes("from FreeCAD.Base import Quantity\n", source)
        tree = ast.parse(merged)
        declaration_index = next(
            index
            for index, node in enumerate(tree.body)
            if isinstance(node, ast.AnnAssign)
            and isinstance(node.target, ast.Name)
            and node.target.id == "NanoMetre"
        )
        documentation = tree.body[declaration_index + 1]
        self.assertIsInstance(documentation, ast.Expr)
        self.assertEqual("One nanometre.", documentation.value.value)

    def test_generated_module_has_canonical_provenance_header(self):
        source = render_init_exports(
            (ModuleExport("FreeCAD", "Logger", "type[FCADLogger]", "Logger."),)
        )
        merged = merge_module_support_nodes("from FreeCAD import Document\n", source)
        self.assertTrue(merged.startswith("\n".join(GENERATED_STUB_HEADER)))

    def test_bootstrap_function_stubs_preserve_typing_decorators(self):
        source = """\
class Decorated:
    @staticmethod
    def static(value: str) -> str:
        return value

    @classmethod
    def create(cls) -> 'Decorated':
        return cls()

    @property
    def value(self) -> str:
        return 'value'

    @value.setter
    def value(self, value: str) -> None:
        pass
"""
        class_node = ast.parse(source).body[0]
        self.assertIsInstance(class_node, ast.ClassDef)
        rendered = "\n".join(
            _render_function_stub(member)
            for member in class_node.body
            if isinstance(member, ast.FunctionDef)
        )
        self.assertIn("@staticmethod\ndef static", rendered)
        self.assertIn("@classmethod\ndef create", rendered)
        self.assertIn("@property\ndef value", rendered)
        self.assertIn("@value.setter\ndef value", rendered)


if __name__ == "__main__":
    unittest.main()
