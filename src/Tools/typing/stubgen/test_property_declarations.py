# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import ast
from pathlib import Path
import sys
import unittest

TYPING_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TYPING_DIR))

from stubgen.property_declarations import (  # noqa: E402
    BIM_PROPERTY_SOURCES,
    BIM_PROTOCOL_CLASSES,
    parse_property_declarations,
    validate_protocol_property_contracts,
)
from stubgen.bim_protocols import (
    discover_generated_bim_protocols,
    render_bim_protocols,
)  # noqa: E402
from stubgen.property_contracts import load_property_catalog  # noqa: E402
from stubgen.property_hierarchy import discover_property_hierarchy  # noqa: E402

ROOT_DIR = Path(__file__).resolve().parents[4]


class PropertyDeclarationTests(unittest.TestCase):
    def test_parser_records_literal_declarations_and_owners(self):
        source = """
class ExampleObject:
    pass

class Example:
    def setup(self, obj):
        obj.addProperty("App::PropertyLength", "Radius", "Draft")
        obj.addProperty(type_id, "DynamicName", "Draft")
        obj.addProperty("App::PropertyVector", name, "Draft")
"""
        declarations = parse_property_declarations(source, "example.py")

        self.assertEqual(len(declarations), 1)
        self.assertEqual(
            declarations[0],
            declarations[0].__class__(
                source="example.py",
                line=7,
                owner_class="Example",
                protocol_class="ExampleObject",
                property_name="Radius",
                type_id="App::PropertyLength",
            ),
        )

    def test_draft_protocols_match_cataloged_property_contracts(self):
        issues = validate_protocol_property_contracts(ROOT_DIR, require_complete=True)
        self.assertEqual([], list(issues), "\n".join(issue.format() for issue in issues))

    def test_bim_protocols_match_cataloged_property_contracts(self):
        issues = validate_protocol_property_contracts(
            ROOT_DIR,
            paths=BIM_PROPERTY_SOURCES,
            protocol_classes=BIM_PROTOCOL_CLASSES,
            inherited_source_paths=(Path("src/Mod/BIM/ArchTypeHints.py"),),
        )
        self.assertEqual([], list(issues), "\n".join(issue.format() for issue in issues))

    def test_arch_equipment_protocol_is_generated_from_core_contracts(self):
        protocols = discover_generated_bim_protocols(
            ROOT_DIR,
            discover_property_hierarchy(ROOT_DIR),
            load_property_catalog(ROOT_DIR),
        )

        self.assertEqual(1, len(protocols))
        protocol = protocols[0]
        self.assertEqual("ArchEquipmentObject", protocol.protocol_name)
        self.assertEqual(("ArchComponentObject",), protocol.base_protocols)
        self.assertEqual(
            ["Model", "ProductURL", "StandardCode", "SnapPoints", "EquipmentPower"],
            [property_name for property_name, _, _ in protocol.properties],
        )
        self.assertEqual("list[Base.Vector]", protocol.properties[3][1])
        self.assertEqual(
            "Sequence[Base.Vector | tuple[float, float, float]]",
            protocol.properties[3][2],
        )

        source = render_bim_protocols(
            ROOT_DIR,
            discover_property_hierarchy(ROOT_DIR),
            load_property_catalog(ROOT_DIR),
        )
        ast.parse(source)
        self.assertIn("from ArchTypeHints import ArchComponentObject", source)
        self.assertIn("class ArchEquipmentObject(ArchComponentObject, Protocol):", source)


if __name__ == "__main__":
    unittest.main()
