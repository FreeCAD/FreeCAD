# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import ast
from pathlib import Path
import sys
import tempfile
import unittest

TYPING_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TYPING_DIR))

from stubgen.property_contracts import (  # noqa: E402
    conversion_metadata_issues,
    load_property_catalog,
    property_contract,
    render_property_aliases,
)
from stubgen.property_hierarchy import (  # noqa: E402
    _matching_brace,
    discover_conversion_overrides,
    discover_property_hierarchy,
)
from stubgen.type_hierarchy import discover_type_hierarchy  # noqa: E402

ROOT_DIR = Path(__file__).resolve().parents[4]


class PropertyContractsTests(unittest.TestCase):
    def test_catalog_distinguishes_quantity_constraint_from_quantity(self):
        hierarchy = discover_property_hierarchy(ROOT_DIR)
        catalog = load_property_catalog(ROOT_DIR)
        constrained = property_contract("App::PropertyLength", hierarchy, catalog)
        unconstrained = property_contract("App::PropertyQuantity", hierarchy, catalog)

        self.assertEqual(constrained.getter, "Base.Quantity")
        self.assertEqual(constrained.setter, "float | str | Base.Quantity")
        self.assertNotIn("Unit", constrained.setter)
        self.assertIn("Unit", unconstrained.setter)

    def test_catalog_covers_unique_type_ids(self):
        catalog = load_property_catalog(ROOT_DIR)
        type_ids = [contract.type_id for contract in catalog.contracts]
        self.assertEqual(len(type_ids), len(set(type_ids)))

    def test_metadata_contains_roots_and_overrides_only(self):
        catalog = load_property_catalog(ROOT_DIR)
        type_ids = {contract.type_id for contract in catalog.contracts}
        self.assertIn("App::PropertyQuantity", type_ids)
        self.assertIn("App::PropertyQuantityConstraint", type_ids)
        self.assertNotIn("App::PropertyLength", type_ids)
        self.assertNotIn("App::PropertyDistance", type_ids)

    def test_cpp_inheritance_resolves_common_descendants(self):
        hierarchy = discover_property_hierarchy(ROOT_DIR)
        catalog = load_property_catalog(ROOT_DIR)
        full_hierarchy = discover_type_hierarchy(ROOT_DIR)

        self.assertEqual(
            "App::Property",
            hierarchy.nodes["App::PropertyLinkBase"].parent,
        )
        self.assertIn("App::PropertyLinkBase", hierarchy.chain("App::PropertyLink"))

        length = property_contract("App::PropertyLength", hierarchy, catalog)
        distance = property_contract("App::PropertyDistance", hierarchy, catalog)
        direction = property_contract("App::PropertyDirection", hierarchy, catalog)
        hidden_link = property_contract("App::PropertyLinkHidden", hierarchy, catalog)

        self.assertEqual(length.getter, "Base.Quantity")
        self.assertEqual(length.setter, "float | str | Base.Quantity")
        self.assertEqual(distance.setter, "float | str | Base.Quantity | Base.Unit")
        self.assertEqual(direction.getter, "Base.Vector")
        self.assertIn("tuple[float, float, float]", direction.setter)
        self.assertEqual(hidden_link.getter, "DocumentObject | None")
        self.assertEqual(hidden_link.setter, "DocumentObject | None")

        part_shape = property_contract("Part::PropertyPartShape", full_hierarchy, catalog)
        material = property_contract("Materials::PropertyMaterial", full_hierarchy, catalog)
        file_included = property_contract("App::PropertyFileIncluded", full_hierarchy, catalog)
        xlink_list = property_contract("App::PropertyXLinkList", full_hierarchy, catalog)
        expression_engine = property_contract(
            "App::PropertyExpressionEngine", full_hierarchy, catalog
        )
        aliases = {alias.name: alias.expression for alias in catalog.aliases}
        self.assertEqual("Part.Shape", part_shape.getter)
        self.assertEqual("Materials.Material", material.getter)
        self.assertEqual("str", file_included.getter)
        self.assertEqual(
            "_FileInput | dict[str, str] | tuple[str | bytes, str | bytes]",
            file_included.setter,
        )
        self.assertIn("IOBase", aliases["_FileInput"])
        self.assertIn("_DocumentObjectListInput", xlink_list.setter)
        self.assertIn("_DocumentObjectSubLinkList", xlink_list.getter)
        self.assertEqual("list[tuple[str, str | None]]", expression_engine.getter)
        self.assertIsNone(expression_engine.setter)

    def test_core_link_getter_aliases_preserve_nullability(self):
        catalog = load_property_catalog(ROOT_DIR)
        contracts = {contract.type_id: contract for contract in catalog.contracts}

        self.assertEqual(
            "list[DocumentObject | None]",
            contracts["App::PropertyLinkList"].getter.alias.expression,
        )
        self.assertEqual(
            "tuple[DocumentObject, list[str]] | None",
            contracts["App::PropertyLinkSub"].getter.alias.expression,
        )

        hierarchy = discover_property_hierarchy(ROOT_DIR)
        resolved = property_contract("App::PropertyLinkList", hierarchy, catalog)
        self.assertEqual(
            "list[DocumentObject | None]",
            resolved.getter,
        )

    def test_cpp_conversion_overrides_have_metadata(self):
        hierarchy = discover_property_hierarchy(ROOT_DIR)
        catalog = load_property_catalog(ROOT_DIR)
        overrides = discover_conversion_overrides(ROOT_DIR)
        override_keys = {(override.type_id, override.direction) for override in overrides}
        self.assertIn(("App::PropertyQuantityConstraint", "setter"), override_keys)
        self.assertIn(("App::PropertyVector", "setter"), override_keys)
        self.assertIn(("App::PropertyLinkSub", "getter"), override_keys)
        issues = conversion_metadata_issues(ROOT_DIR, hierarchy, catalog)
        self.assertEqual([], list(issues), "\n".join(issue.format() for issue in issues))

    def test_cpp_conversion_override_locations_are_exact(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            app_directory = root / "src/App"
            app_directory.mkdir(parents=True)
            (app_directory / "PropertyTest.h").write_text(
                "// test header\n"
                "class PropertyTest\n"
                "{\n"
                "public:\n"
                "    // void setPyObject() override;\n"
                "    Py::Object getPyObject(\n"
                "        std::function<void(int, std::pair<int, int>)> callback\n"
                "    ) const noexcept(false) & override;\n"
                "    void setPyObject(\n"
                "        std::function<void(int)> callback = nullptr\n"
                "    ) && override;\n"
                "};\n",
                encoding="utf-8",
            )

            overrides = discover_conversion_overrides(root)

        self.assertEqual(
            [(override.type_id, override.direction) for override in overrides],
            [("App::PropertyTest", "getter"), ("App::PropertyTest", "setter")],
        )
        self.assertEqual("src/App/PropertyTest.h", overrides[0].source)
        self.assertEqual(6, overrides[0].line)

    def test_class_body_matching_ignores_comments_and_literals(self):
        source = "{ char brace = '}'; /* } */ const char *text = \"{\"; int value; } tail"
        self.assertEqual(len(source) - 5, _matching_brace(source, 0))

    def test_rendered_aliases_are_self_contained(self):
        catalog = load_property_catalog(ROOT_DIR)
        source = render_property_aliases(catalog)
        tree = ast.parse(source)
        rendered = {
            node.target.id: ast.unparse(node.value)
            for node in tree.body
            if isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name)
        }

        expected = {
            spec.name: ast.unparse(ast.parse(spec.expression, mode="eval").body)
            for spec in catalog.aliases
        }
        self.assertEqual(rendered, expected)
        self.assertIn("from . import Base as Base", source)
        self.assertNotIn("FreeCAD.Base", source)
        imported_names = {
            alias.asname or alias.name.split(".", 1)[0]
            for node in tree.body
            if isinstance(node, (ast.Import, ast.ImportFrom))
            for alias in node.names
        }
        self.assertTrue({"Base", "IOBase", "Sequence", "TypeAlias"}.issubset(imported_names))

    def test_core_module_no_longer_manually_defines_catalog_aliases(self):
        catalog = load_property_catalog(ROOT_DIR)
        source = (ROOT_DIR / "src/App/FreeCAD.module.pyi").read_text(encoding="utf-8")
        tree = ast.parse(source)
        names = {
            node.target.id
            for node in tree.body
            if isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name)
        }
        self.assertTrue(names.isdisjoint({spec.name for spec in catalog.aliases}))


if __name__ == "__main__":
    unittest.main()
