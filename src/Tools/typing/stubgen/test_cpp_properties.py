# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import ast
from pathlib import Path
import sys
import tempfile
import unittest

TYPING_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TYPING_DIR))

from stubgen.cpp_properties import (  # noqa: E402
    CppProperty,
    TypedCppProperty,
    discover_cpp_properties,
    typed_cpp_properties,
)
from stubgen.api_extract import module_from_source  # noqa: E402
from stubgen.generated_api import add_cpp_properties_to_model  # noqa: E402
from stubgen.render import render_module  # noqa: E402
from stubgen.stub_support import StubSupport  # noqa: E402
from python_api_model.model import ApiOrigin, PythonApiModel  # noqa: E402
from stubgen.document_object_types import direct_python_types  # noqa: E402
from stubgen.discovery import collect_type_registrations  # noqa: E402
from stubgen.parsing import load_source_files  # noqa: E402
from stubgen.property_contracts import load_property_catalog  # noqa: E402
from stubgen.source_inputs import collect_binding_classes  # noqa: E402
from stubgen.type_hierarchy import (  # noqa: E402
    TypeHierarchy,
    TypeNode,
    discover_type_hierarchy,
)
from stubgen.model import PublicPythonType  # noqa: E402

ROOT_DIR = Path(__file__).resolve().parents[4]


class CppPropertyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hierarchy = discover_type_hierarchy(ROOT_DIR)
        source_files = load_source_files(ROOT_DIR, ROOT_DIR / "src")
        registrations = collect_type_registrations(ROOT_DIR, source_files)
        cls.classes = collect_binding_classes(ROOT_DIR, ROOT_DIR / "src", registrations)
        cls.catalog = load_property_catalog(ROOT_DIR)

    def test_geo_feature_placement_is_derived_from_cpp_and_property_metadata(self):
        properties = discover_cpp_properties(ROOT_DIR, self.hierarchy)
        geo_feature = [
            property_ for property_ in properties if property_.owner_type_id == "App::GeoFeature"
        ]
        self.assertEqual(
            [(property_.property_name, property_.property_type_id) for property_ in geo_feature],
            [("Placement", "App::PropertyPlacement")],
        )

        typed = typed_cpp_properties(
            tuple(geo_feature),
            self.hierarchy,
            self.catalog,
            direct_python_types(self.classes, self.hierarchy),
        )
        self.assertEqual(len(typed), 1)
        self.assertEqual(typed[0].owner.qualified_name, "FreeCAD.GeoFeature")
        self.assertEqual(typed[0].getter, "Base.Placement")
        self.assertEqual(typed[0].setter, "Base.Placement | Base.Matrix")

    def test_read_only_cpp_properties_do_not_get_invented_setters(self):
        properties = discover_cpp_properties(ROOT_DIR, self.hierarchy)
        expression_engine = next(
            property_
            for property_ in properties
            if property_.owner_type_id == "App::DocumentObject"
            and property_.property_name == "ExpressionEngine"
        )
        typed = typed_cpp_properties(
            (expression_engine,),
            self.hierarchy,
            self.catalog,
            direct_python_types(self.classes, self.hierarchy),
        )
        self.assertEqual(len(typed), 1)
        self.assertEqual(typed[0].getter, "list[tuple[str, str | None]]")
        self.assertIsNone(typed[0].setter)

    def test_spreadsheet_property_wrappers_are_resolved(self):
        properties = discover_cpp_properties(ROOT_DIR, self.hierarchy)
        by_name = {
            property_.property_name: property_
            for property_ in properties
            if property_.owner_type_id == "Spreadsheet::Sheet"
        }
        typed = typed_cpp_properties(
            tuple(by_name[name] for name in ("cells", "columnWidths", "rowHeights")),
            self.hierarchy,
            self.catalog,
            direct_python_types(self.classes, self.hierarchy),
        )
        self.assertEqual(
            {property_.name: (property_.getter, property_.setter) for property_ in typed},
            {
                "cells": ("Spreadsheet.PropertySheet", "Spreadsheet.PropertySheet"),
                "columnWidths": ("Spreadsheet.PropertyColumnWidths", None),
                "rowHeights": ("Spreadsheet.PropertyRowHeights", None),
            },
        )

    def test_mesh_kernel_contract_is_resolved(self):
        properties = discover_cpp_properties(ROOT_DIR, self.hierarchy)
        mesh_property = next(
            property_
            for property_ in properties
            if property_.owner_type_id == "Mesh::Feature" and property_.property_name == "Mesh"
        )
        typed = typed_cpp_properties(
            (mesh_property,),
            self.hierarchy,
            self.catalog,
            direct_python_types(self.classes, self.hierarchy),
        )
        self.assertEqual(len(typed), 1)
        self.assertEqual("Mesh.Mesh", typed[0].getter)
        self.assertEqual("Mesh.Mesh | list[list[float]]", typed[0].setter)

    def test_view_provider_properties_use_public_non_document_object_names(self):
        properties = discover_cpp_properties(ROOT_DIR, self.hierarchy)
        visibility = next(
            property_
            for property_ in properties
            if property_.owner_type_id == "Gui::ViewProviderDocumentObject"
            and property_.property_name == "Visibility"
        )
        typed = typed_cpp_properties(
            (visibility,),
            self.hierarchy,
            self.catalog,
            direct_python_types(
                self.classes,
                type_ids={visibility.owner_type_id},
            ),
        )
        self.assertEqual(len(typed), 1)
        self.assertEqual("FreeCADGui.ViewProviderDocumentObject", typed[0].owner.qualified_name)
        self.assertEqual("bool", typed[0].getter)
        self.assertEqual("bool | int", typed[0].setter)

    def test_simple_core_property_contracts_are_not_left_unresolved(self):
        properties = discover_cpp_properties(ROOT_DIR, self.hierarchy)
        part_properties = {
            property_.property_name: property_
            for property_ in properties
            if property_.owner_type_id == "App::Part"
        }
        typed = typed_cpp_properties(
            (part_properties["Uid"], part_properties["Color"]),
            self.hierarchy,
            self.catalog,
            direct_python_types(self.classes, self.hierarchy),
        )
        self.assertEqual({"Uid", "Color"}, {property_.name for property_ in typed})
        self.assertEqual(
            "str",
            next(property_ for property_ in typed if property_.name == "Uid").getter,
        )
        self.assertIn(
            "tuple[float, float, float]",
            next(property_ for property_ in typed if property_.name == "Color").setter,
        )

    def test_grouped_property_members_and_nonmatching_header_are_discovered(self):
        properties = discover_cpp_properties(ROOT_DIR, self.hierarchy)
        box = {
            property_.property_name: property_.property_type_id
            for property_ in properties
            if property_.owner_type_id == "Part::Box"
        }
        self.assertEqual(
            {
                "Length": "App::PropertyLength",
                "Width": "App::PropertyLength",
                "Height": "App::PropertyLength",
            },
            box,
        )

    def test_part_and_sketcher_property_contracts_are_discovered(self):
        properties = discover_cpp_properties(ROOT_DIR, self.hierarchy)
        by_name = {
            (property_.owner_type_id, property_.property_name): property_
            for property_ in properties
        }
        typed = typed_cpp_properties(
            (
                by_name[("Sketcher::SketchObject", "Geometry")],
                by_name[("Sketcher::SketchObject", "Constraints")],
            ),
            self.hierarchy,
            self.catalog,
            direct_python_types(self.classes, self.hierarchy),
        )
        self.assertEqual(
            {property_.name: (property_.getter, property_.setter) for property_ in typed},
            {
                "Geometry": ("list[Part.Geometry]", "Part.Geometry | Sequence[Part.Geometry]"),
                "Constraints": (
                    "list[Sketcher.Constraint]",
                    "Sketcher.Constraint | list[Sketcher.Constraint]",
                ),
            },
        )

    def test_scoped_constructor_properties_are_discovered(self):
        properties = discover_cpp_properties(ROOT_DIR, self.hierarchy)
        feature = {
            property_.property_name: property_.property_type_id
            for property_ in properties
            if property_.owner_type_id == "Part::Feature"
        }
        self.assertEqual(
            {
                "Shape": "Part::PropertyPartShape",
                "ShapeMaterial": "Materials::PropertyMaterial",
            },
            feature,
        )

    def test_single_registration_source_ignores_unrelated_property_calls(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_directory = root / "src/Example"
            source_directory.mkdir(parents=True)
            (source_directory / "Thing.h").write_text(
                "namespace Example {\n"
                "class Thing {\n"
                "public:\n"
                "    App::PropertyLength Length;\n"
                "    App::PropertyLength HelperOnly;\n"
                "};\n"
                "}\n",
                encoding="utf-8",
            )
            (source_directory / "Thing.cpp").write_text(
                '#include "Thing.h"\n'
                "\n"
                "Example::Thing::Thing()\n"
                "    : marker {1}\n"
                "{\n"
                '    ADD_PROPERTY(Length, "Example", "length");\n'
                "}\n"
                "\n"
                "void add_helper_property()\n"
                "{\n"
                '    ADD_PROPERTY(HelperOnly, "Example", "helper");\n'
                "}\n",
                encoding="utf-8",
            )
            hierarchy = TypeHierarchy(
                {
                    "Example::Thing": TypeNode(
                        "Example::Thing",
                        "App::DocumentObject",
                        "src/Example/Thing.cpp",
                        3,
                    )
                }
            )

            properties = discover_cpp_properties(root, hierarchy)

        self.assertEqual(["Length"], [property_.property_name for property_ in properties])

    def test_only_documentation_arguments_become_property_documentation(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_directory = root / "src/Example"
            source_directory.mkdir(parents=True)
            (source_directory / "Thing.h").write_text(
                "namespace Example {\n"
                "class Thing {\n"
                "public:\n"
                "    App::PropertyString String;\n"
                "    App::PropertyLength Length;\n"
                "};\n"
                "}\n",
                encoding="utf-8",
            )
            (source_directory / "Thing.cpp").write_text(
                '#include "Thing.h"\n'
                "Example::Thing::Thing()\n"
                "{\n"
                '    ADD_PROPERTY(String, ("4711"));\n'
                '    ADD_PROPERTY_TYPE(Length, (1), "Example", Prop_None, "The length");\n'
                "}\n",
                encoding="utf-8",
            )
            hierarchy = TypeHierarchy(
                {
                    "Example::Thing": TypeNode(
                        "Example::Thing",
                        "App::DocumentObject",
                        "src/Example/Thing.cpp",
                        2,
                    )
                }
            )

            properties = discover_cpp_properties(root, hierarchy)

        self.assertEqual(
            {property_.property_name: property_.documentation for property_ in properties},
            {"String": None, "Length": "The length"},
        )

    def test_inherited_property_members_are_resolved(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_directory = root / "src/Example"
            source_directory.mkdir(parents=True)
            (source_directory / "Base.h").write_text(
                "namespace Example {\n"
                "class Base {\n"
                "public:\n"
                "    App::PropertyLength Length;\n"
                "};\n"
                "}\n",
                encoding="utf-8",
            )
            (source_directory / "Base.cpp").write_text(
                '#include "Base.h"\n',
                encoding="utf-8",
            )
            (source_directory / "Child.h").write_text(
                '#include "Base.h"\n'
                "namespace Example {\n"
                "class Child: public Base {\n"
                "public:\n"
                "    Child();\n"
                "};\n"
                "}\n",
                encoding="utf-8",
            )
            (source_directory / "Child.cpp").write_text(
                '#include "Child.h"\n'
                "Example::Child::Child()\n"
                "{\n"
                '    ADD_PROPERTY(Length, "Example", "length");\n'
                "}\n",
                encoding="utf-8",
            )
            hierarchy = TypeHierarchy(
                {
                    "Example::Base": TypeNode(
                        "Example::Base",
                        "App::DocumentObject",
                        "src/Example/Base.cpp",
                        1,
                    ),
                    "Example::Child": TypeNode(
                        "Example::Child",
                        "Example::Base",
                        "src/Example/Child.cpp",
                        2,
                    ),
                }
            )

            properties = discover_cpp_properties(root, hierarchy)

        self.assertEqual(
            [
                (property_.owner_type_id, property_.property_name, property_.property_type_id)
                for property_ in properties
            ],
            [("Example::Child", "Length", "App::PropertyLength")],
        )

    def test_unresolved_cpp_properties_are_reported(self):
        diagnostics = []
        property_ = CppProperty(
            "Example::Thing",
            "Value",
            "App::PropertyNotCataloged",
            "src/Example/Thing.cpp",
            12,
        )

        typed = typed_cpp_properties(
            (property_,),
            self.hierarchy,
            self.catalog,
            {"Example::Thing": PublicPythonType("Example", "Thing")},
            diagnostics,
        )

        self.assertEqual((), typed)
        self.assertEqual(1, len(diagnostics))
        self.assertEqual("unresolved_contract", diagnostics[0].kind)
        self.assertIn("PropertyNotCataloged", diagnostics[0].format())

    def test_qualified_property_members_allow_whitespace_around_scope_operator(self):
        properties = discover_cpp_properties(ROOT_DIR, self.hierarchy)
        sketch = {
            property_.property_name: property_.property_type_id
            for property_ in properties
            if property_.owner_type_id == "Sketcher::SketchObject"
        }
        self.assertEqual("Part::PropertyGeometryList", sketch["Geometry"])
        self.assertEqual("App::PropertyLinkSubList", sketch["ExternalGeometry"])

    def test_cpp_properties_are_added_to_public_stubs_only(self):
        source = """\
from __future__ import annotations

class GeoFeature:
    ...
"""
        properties = discover_cpp_properties(ROOT_DIR, self.hierarchy)
        typed = typed_cpp_properties(
            tuple(
                property_
                for property_ in properties
                if property_.owner_type_id == "App::GeoFeature"
            ),
            self.hierarchy,
            self.catalog,
            direct_python_types(self.classes, self.hierarchy),
        )

        module = module_from_source(
            ROOT_DIR,
            ROOT_DIR / "src/Test.pyi",
            source,
            "FreeCAD",
            origin=ApiOrigin.MODULE_STUB,
            include_module_doc=False,
        )
        model, support = add_cpp_properties_to_model(PythonApiModel((module,)), typed)
        rendered = render_module(
            model.modules[0],
            support=StubSupport(module_fragments=support),
        )
        tree = ast.parse(rendered)
        geo_feature = next(node for node in tree.body if isinstance(node, ast.ClassDef))
        placement = next(
            node
            for node in geo_feature.body
            if isinstance(node, ast.FunctionDef) and node.name == "Placement"
        )
        self.assertEqual(ast.unparse(placement.returns), "Base.Placement")
        self.assertIn("from . import Base as Base", rendered)

        with self.assertRaisesRegex(ValueError, "already declares C\\+\\+ properties"):
            collision = module_from_source(
                ROOT_DIR,
                ROOT_DIR / "src/Test.pyi",
                "class GeoFeature:\n    Placement: object\n",
                "FreeCAD",
                origin=ApiOrigin.MODULE_STUB,
                include_module_doc=False,
            )
            add_cpp_properties_to_model(PythonApiModel((collision,)), (typed[0],))

        bool_source = """\
from __future__ import annotations

class DocumentObject:
    ...
"""
        bool_property = next(
            property_
            for property_ in typed_cpp_properties(
                tuple(
                    property_
                    for property_ in properties
                    if property_.owner_type_id == "App::DocumentObject"
                ),
                self.hierarchy,
                self.catalog,
                direct_python_types(self.classes, self.hierarchy),
            )
            if property_.name == "Visibility"
        )
        bool_module = module_from_source(
            ROOT_DIR,
            ROOT_DIR / "src/Test.pyi",
            bool_source,
            "FreeCAD",
            origin=ApiOrigin.MODULE_STUB,
            include_module_doc=False,
        )
        _, bool_support = add_cpp_properties_to_model(
            PythonApiModel((bool_module,)), (bool_property,)
        )
        self.assertFalse(any("Base as Base" in source for _, source in bool_support))

    def test_any_binding_placeholder_is_replaced_by_cpp_property_contract(self):
        property_ = TypedCppProperty(
            PublicPythonType("Path", "FeatureArea"),
            "WorkPlane",
            "Part.Shape",
            "Part.Shape",
            "src/Mod/CAM/App/FeatureArea.cpp",
            49,
        )
        source = """from typing import Any

class FeatureArea:
    WorkPlane: Any
    "The workplane."
"""
        module = module_from_source(
            ROOT_DIR,
            ROOT_DIR / "src/Test.pyi",
            source,
            "Path",
            origin=ApiOrigin.BINDING_SPEC,
            include_module_doc=False,
        )
        model, support = add_cpp_properties_to_model(PythonApiModel((module,)), (property_,))
        rendered = render_module(model.modules[0], support=StubSupport(module_fragments=support))

        tree = ast.parse(rendered)
        feature_area = next(node for node in tree.body if isinstance(node, ast.ClassDef))
        work_plane = next(
            node
            for node in feature_area.body
            if isinstance(node, ast.FunctionDef) and node.name == "WorkPlane"
        )
        self.assertEqual(ast.unparse(work_plane.returns), "Part.Shape")
        self.assertIn("The workplane.", rendered)


if __name__ == "__main__":
    unittest.main()
