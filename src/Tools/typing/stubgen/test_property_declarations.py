# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from pathlib import Path
import sys
import unittest

TYPING_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TYPING_DIR))

from stubgen.property_declarations import (  # noqa: E402
    parse_property_declarations,
    validate_protocol_property_contracts,
)

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


if __name__ == "__main__":
    unittest.main()
