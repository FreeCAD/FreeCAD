# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import ast
import json
from pathlib import Path
import sys
import tempfile
import tomllib
import unittest

TYPING_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TYPING_DIR))

from stubgen.property_declarations import (  # noqa: E402
    BIM_GENERATED_OBJECT_CLASSES,
    BIM_PROPERTY_SOURCES,
    BIM_MANUAL_OBJECT_CLASSES,
    BIM_TYPE_CHECK_SOURCES,
    parse_property_declarations,
    validate_protocol_property_contracts,
)
from stubgen.bim_protocols import (
    discover_generated_bim_objects,
    render_bim_objects,
    write_bim_checker_configs,
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
                object_class="ExampleObject",
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
            protocol_classes=BIM_MANUAL_OBJECT_CLASSES,
            inherited_source_paths=(Path("src/Mod/BIM/ArchTypeHints.py"),),
        )
        self.assertEqual([], list(issues), "\n".join(issue.format() for issue in issues))

    def test_bim_objects_are_generated_from_the_registry(self):
        objects = discover_generated_bim_objects(
            ROOT_DIR,
            discover_property_hierarchy(ROOT_DIR),
            load_property_catalog(ROOT_DIR),
        )

        self.assertEqual(6, len(objects))
        equipment = next(obj for obj in objects if obj.object_name == "ArchEquipmentObject")
        self.assertEqual("ArchEquipmentObject", equipment.object_name)
        self.assertEqual(("Part.Feature", "ArchComponentObject"), equipment.base_types)
        self.assertEqual(
            ["Model", "ProductURL", "StandardCode", "SnapPoints", "EquipmentPower"],
            [property_name for property_name, _, _ in equipment.properties],
        )
        self.assertEqual("list[Base.Vector]", equipment.properties[3][1])
        self.assertEqual(
            "Sequence[Base.Vector | tuple[float, float, float]]",
            equipment.properties[3][2],
        )

        frame = next(obj for obj in objects if obj.object_name == "ArchFrameObject")
        frame_properties = {name: (getter, setter) for name, getter, setter in frame.properties}
        self.assertEqual(("str", "str | list[str]"), frame_properties["Edges"])

        space = next(obj for obj in objects if obj.object_name == "ArchSpaceObject")
        space_properties = {name: (getter, setter) for name, getter, setter in space.properties}
        for name in ("SpaceType", "Conditioning", "AreaCalculationType"):
            self.assertEqual(("str", "str | list[str]"), space_properties[name])

        source = render_bim_objects(
            ROOT_DIR,
            discover_property_hierarchy(ROOT_DIR),
            load_property_catalog(ROOT_DIR),
        )
        ast.parse(source)
        self.assertIn("import Part", source)
        self.assertIn("class ArchIFCRootObject(DocumentObject):", source)
        self.assertIn("class ArchEquipmentObject(Part.Feature, ArchComponentObject):", source)
        self.assertIn("class ArchFrameObject(Part.Feature, ArchComponentObject):", source)
        self.assertIn("class ArchSpaceObject(Part.Feature, ArchComponentObject):", source)

        self.assertEqual(
            {
                "src/Mod/BIM/ArchIFC.py",
                "src/Mod/BIM/ArchComponent.py",
                "src/Mod/BIM/ArchEquipment.py",
                "src/Mod/BIM/ArchFrame.py",
                "src/Mod/BIM/ArchSpace.py",
            },
            set(BIM_GENERATED_OBJECT_CLASSES),
        )

    def test_root_bim_object_uses_document_object_once(self):
        source = render_bim_objects(
            ROOT_DIR,
            discover_property_hierarchy(ROOT_DIR),
            load_property_catalog(ROOT_DIR),
            generated_object_classes={"src/Mod/BIM/ArchEquipment.py": {"_Equipment": "RootObject"}},
            inherited_object_classes={},
            runtime_base_types={},
        )

        self.assertIn("from FreeCAD import DocumentObject", source)
        self.assertIn("class RootObject(DocumentObject):", source)
        self.assertNotIn("DocumentObject, DocumentObject", source)

    def test_checker_configs_include_every_generated_bim_source(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            pyright_path, pyrefly_path = write_bim_checker_configs(Path(temp_dir), ROOT_DIR)
            pyright = json.loads(pyright_path.read_text(encoding="utf-8"))
            pyrefly = tomllib.loads(pyrefly_path.read_text(encoding="utf-8"))
            pyright_sources = {
                (pyright_path.parent / path).resolve() for path in pyright["include"]
            }
            pyrefly_sources = {
                (pyrefly_path.parent / path).resolve() for path in pyrefly["project-includes"]
            }

        expected_sources = {(ROOT_DIR / source).resolve() for source in BIM_TYPE_CHECK_SOURCES}
        bim_root = (ROOT_DIR / "src/Mod/BIM").resolve()
        configured_pyright_sources = {path for path in pyright_sources if bim_root in path.parents}
        configured_pyrefly_sources = {path for path in pyrefly_sources if bim_root in path.parents}
        self.assertEqual(expected_sources, configured_pyright_sources)
        self.assertEqual(expected_sources, configured_pyrefly_sources)


if __name__ == "__main__":
    unittest.main()
