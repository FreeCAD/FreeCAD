# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import ast
from functools import lru_cache
from pathlib import Path
import sys
import tempfile
import unittest

TYPING_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TYPING_DIR))

from stubgen.discovery import collect_type_registrations  # noqa: E402
from stubgen.document_object_types import (  # noqa: E402
    add_document_add_object_overloads,
    direct_python_types,
    document_object_python_types,
    resolve_document_object_python_type,
)
from stubgen.model import PublicPythonType, PythonObjectType  # noqa: E402
from stubgen.parsing import iter_source_files  # noqa: E402
from stubgen.source_inputs import collect_binding_classes  # noqa: E402
from stubgen.type_hierarchy import TypeHierarchy, discover_type_hierarchy  # noqa: E402
from stubgen.cli import run_generate  # noqa: E402

ROOT_DIR = Path(__file__).resolve().parents[4]


@lru_cache(maxsize=1)
def discovered_object_types() -> tuple[TypeHierarchy, dict[str, PublicPythonType]]:
    hierarchy = discover_type_hierarchy(ROOT_DIR)
    source_files = list(iter_source_files(ROOT_DIR, ROOT_DIR / "src"))
    type_registrations = collect_type_registrations(ROOT_DIR, source_files)
    classes = collect_binding_classes(ROOT_DIR, ROOT_DIR / "src", type_registrations)
    return hierarchy, direct_python_types(classes, hierarchy)


class DocumentObjectTypeTests(unittest.TestCase):
    def test_type_hierarchy_parses_registered_macro_arguments(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source_directory = root / "src/Example"
            source_directory.mkdir(parents=True)
            (source_directory / "Types.cpp").write_text(
                "PROPERTY_SOURCE_ABSTRACT_WITH_EXTENSIONS(\n"
                "    Example::Abstract,\n"
                "    App :: DocumentObject\n"
                ")\n"
                "TYPESYSTEM_SOURCE_TEMPLATE_T(\n"
                "    Example::Concrete,\n"
                "    Example::Abstract\n"
                ")\n",
                encoding="utf-8",
            )

            hierarchy = discover_type_hierarchy(root)

        self.assertTrue(hierarchy.nodes["Example::Abstract"].is_abstract)
        self.assertEqual("App::DocumentObject", hierarchy.nodes["Example::Abstract"].parent)
        self.assertFalse(hierarchy.nodes["Example::Concrete"].is_abstract)
        self.assertEqual("Example::Abstract", hierarchy.nodes["Example::Concrete"].parent)

    def test_type_id_hierarchy_includes_python_variants(self):
        hierarchy, _ = discovered_object_types()

        self.assertEqual("Part::Feature", hierarchy.nodes["Part::FeaturePython"].parent)
        self.assertEqual("Part::Part2DObject", hierarchy.nodes["Part::Part2DObjectPython"].parent)

    def test_type_id_resolution_uses_binding_public_names(self):
        hierarchy, direct = discovered_object_types()

        self.assertEqual(
            PublicPythonType("Part", "Feature"),
            direct.get("Part::Feature"),
        )
        self.assertEqual(
            PublicPythonType("Part", "Feature"),
            resolve_document_object_python_type("Part::FeaturePython", hierarchy, direct),
        )
        self.assertEqual(
            PublicPythonType("Sketcher", "SketchObject"),
            resolve_document_object_python_type("Sketcher::SketchObject", hierarchy, direct),
        )

    def test_document_object_registrations_are_normalized_and_groupable(self):
        hierarchy, direct = discovered_object_types()
        registrations = document_object_python_types(hierarchy, direct)

        self.assertIn(
            PythonObjectType("Part::Feature", "Part", "Feature"),
            registrations,
        )
        self.assertIn(
            PythonObjectType("Part::FeaturePython", "Part", "Feature"),
            registrations,
        )
        self.assertIn(
            PythonObjectType("Sketcher::SketchObject", "Sketcher", "SketchObject"),
            registrations,
        )

    def test_abstract_object_types_are_not_constructible_overloads(self):
        hierarchy, direct = discovered_object_types()
        registrations = {
            registration.type_id for registration in document_object_python_types(hierarchy, direct)
        }

        self.assertTrue(hierarchy.nodes["Part::Primitive"].is_abstract)
        self.assertFalse(hierarchy.nodes["Part::Box"].is_abstract)
        self.assertNotIn("Part::Primitive", registrations)
        self.assertIn("Part::Box", registrations)

    def test_generated_freecad_stub_contains_composed_public_api(self):
        from argparse import Namespace

        with tempfile.TemporaryDirectory() as temp_dir:
            out_dir = Path(temp_dir)
            result = run_generate(
                Namespace(
                    root=ROOT_DIR,
                    source_dir=ROOT_DIR / "src",
                    out_dir=out_dir,
                    overlay_dir=None,
                    no_overlays=False,
                )
            )
            self.assertEqual(0, result)

            freecad_stub = out_dir / "stubs" / "FreeCAD" / "__init__.pyi"
            self.assertTrue(freecad_stub.exists())
            source = freecad_stub.read_text(encoding="utf-8")
            tree = ast.parse(source)
            freecad_gui_stub = out_dir / "stubs" / "FreeCADGui" / "__init__.pyi"
            self.assertTrue(freecad_gui_stub.exists())
            freecad_gui_source = freecad_gui_stub.read_text(encoding="utf-8")
            freecad_gui_tree = ast.parse(freecad_gui_source)
            part_stub = out_dir / "stubs" / "Part" / "__init__.pyi"
            self.assertTrue(part_stub.exists())
            part_source = part_stub.read_text(encoding="utf-8")
            part_tree = ast.parse(part_source)
            sketcher_stub = out_dir / "stubs" / "Sketcher" / "__init__.pyi"
            self.assertTrue(sketcher_stub.exists())
            sketcher_source = sketcher_stub.read_text(encoding="utf-8")
            sketcher_tree = ast.parse(sketcher_source)
            mesh_stub = out_dir / "stubs" / "Mesh" / "__init__.pyi"
            self.assertTrue(mesh_stub.exists())
            mesh_source = mesh_stub.read_text(encoding="utf-8")
            mesh_tree = ast.parse(mesh_source)
            cam_stub = out_dir / "stubs" / "CAM" / "__init__.pyi"
            self.assertTrue(cam_stub.exists())
            cam_source = cam_stub.read_text(encoding="utf-8")
            cam_tree = ast.parse(cam_source)
            spreadsheet_stub = out_dir / "stubs" / "Spreadsheet" / "__init__.pyi"
            self.assertTrue(spreadsheet_stub.exists())
            spreadsheet_source = spreadsheet_stub.read_text(encoding="utf-8")
            spreadsheet_tree = ast.parse(spreadsheet_source)
            units_stub = out_dir / "stubs" / "FreeCAD" / "Units.pyi"
            self.assertTrue(units_stub.exists())
            units_source = units_stub.read_text(encoding="utf-8")
            generated_stubs = list((out_dir / "stubs").rglob("*.pyi"))
            self.assertTrue(generated_stubs)
            for generated_stub in generated_stubs:
                generated_source = generated_stub.read_text(encoding="utf-8")
                self.assertIn(
                    "# Generated by src/Tools/typing/stubgen; do not edit.",
                    generated_source.splitlines()[:8],
                    str(generated_stub),
                )

        self.assertTrue(source.startswith("# Generated by src/Tools/typing/stubgen; do not edit."))
        self.assertIn("# Sources: binding metadata and source-adjacent typing contracts.", source)
        self.assertIn("class FCADLogger:", source)
        self.assertIn("def catch(", source)
        self.assertIn("NanoMetre: Quantity", units_source)
        self.assertIn("class ScaleType(IntEnum):", source)
        self.assertIn("_QuantityInput: TypeAlias", source)

        document = next(
            node for node in tree.body if isinstance(node, ast.ClassDef) and node.name == "Document"
        )
        add_objects = [
            node
            for node in document.body
            if isinstance(node, ast.FunctionDef) and node.name == "addObject"
        ]
        self.assertGreaterEqual(len(add_objects), 4)
        self.assertEqual("DocumentObject", ast.unparse(add_objects[-1].returns))
        overload_returns: dict[str, str] = {}
        for node in add_objects[:-1]:
            argument = next(argument for argument in node.args.args if argument.arg == "type")
            literal_slice = argument.annotation.slice
            literals = (
                literal_slice.elts if isinstance(literal_slice, ast.Tuple) else (literal_slice,)
            )
            for literal in literals:
                if isinstance(literal, ast.Constant):
                    overload_returns[literal.value] = ast.unparse(node.returns)
        self.assertEqual("_Part.Feature", overload_returns["Part::Feature"])
        self.assertEqual("_Part.Feature", overload_returns["Part::FeaturePython"])
        self.assertEqual("_Sketcher.SketchObject", overload_returns["Sketcher::SketchObject"])
        self.assertNotIn("Part::Primitive", overload_returns)

        geo_feature = next(
            node
            for node in tree.body
            if isinstance(node, ast.ClassDef) and node.name == "GeoFeature"
        )
        placement_getter = next(
            node
            for node in geo_feature.body
            if isinstance(node, ast.FunctionDef) and node.name == "Placement"
        )
        placement_setter = next(
            node
            for node in geo_feature.body
            if isinstance(node, ast.FunctionDef)
            and node.name == "Placement"
            and any(
                isinstance(decorator, ast.Attribute) and decorator.attr == "setter"
                for decorator in node.decorator_list
            )
        )
        self.assertEqual("Base.Placement", ast.unparse(placement_getter.returns))
        self.assertEqual(
            "Base.Placement | Base.Matrix",
            ast.unparse(placement_setter.args.args[1].annotation),
        )

        document_object = next(
            node
            for node in tree.body
            if isinstance(node, ast.ClassDef) and node.name == "DocumentObject"
        )
        visibility = next(
            node
            for node in document_object.body
            if isinstance(node, ast.FunctionDef) and node.name == "Visibility"
        )
        self.assertEqual("bool", ast.unparse(visibility.returns))

        view_provider = next(
            node
            for node in freecad_gui_tree.body
            if isinstance(node, ast.ClassDef) and node.name == "ViewProviderDocumentObject"
        )
        view_visibility = next(
            node
            for node in view_provider.body
            if isinstance(node, ast.FunctionDef) and node.name == "Visibility"
        )
        self.assertEqual("bool", ast.unparse(view_visibility.returns))

        label = next(
            node
            for node in document_object.body
            if isinstance(node, ast.FunctionDef)
            and node.name == "Label"
            and any(
                isinstance(decorator, ast.Attribute) and decorator.attr == "setter"
                for decorator in node.decorator_list
            )
        )
        self.assertEqual("str", ast.unparse(label.args.args[1].annotation))

        part = next(
            node for node in tree.body if isinstance(node, ast.ClassDef) and node.name == "Part"
        )
        color = next(
            node for node in part.body if isinstance(node, ast.FunctionDef) and node.name == "Color"
        )
        self.assertEqual("tuple[float, float, float, float]", ast.unparse(color.returns))

        feature = next(
            node
            for node in part_tree.body
            if isinstance(node, ast.ClassDef) and node.name == "Feature"
        )
        shape = next(
            node
            for node in feature.body
            if isinstance(node, ast.FunctionDef) and node.name == "Shape"
        )
        shape_material = next(
            node
            for node in feature.body
            if isinstance(node, ast.FunctionDef) and node.name == "ShapeMaterial"
        )
        self.assertEqual("Shape", ast.unparse(shape.returns))
        self.assertEqual("Materials.Material", ast.unparse(shape_material.returns))
        self.assertIn("import Materials as Materials", part_source)

        sketch_object = next(
            node
            for node in sketcher_tree.body
            if isinstance(node, ast.ClassDef) and node.name == "SketchObject"
        )
        geometry = next(
            node
            for node in sketch_object.body
            if isinstance(node, ast.FunctionDef) and node.name == "Geometry"
        )
        constraints = next(
            node
            for node in sketch_object.body
            if isinstance(node, ast.FunctionDef) and node.name == "Constraints"
        )
        self.assertEqual("list[Part.Geometry]", ast.unparse(geometry.returns))
        self.assertEqual("list[Constraint]", ast.unparse(constraints.returns))

        mesh_feature = next(
            node
            for node in mesh_tree.body
            if isinstance(node, ast.ClassDef) and node.name == "Feature"
        )
        mesh_property = next(
            node
            for node in mesh_feature.body
            if isinstance(node, ast.FunctionDef) and node.name == "Mesh"
        )
        self.assertEqual("Mesh", ast.unparse(mesh_property.returns))

        feature_area = next(
            node
            for node in cam_tree.body
            if isinstance(node, ast.ClassDef) and node.name == "FeatureArea"
        )
        work_plane = next(
            node
            for node in feature_area.body
            if isinstance(node, ast.FunctionDef) and node.name == "WorkPlane"
        )
        self.assertEqual("Part.Shape", ast.unparse(work_plane.returns))

        sheet = next(
            node
            for node in spreadsheet_tree.body
            if isinstance(node, ast.ClassDef) and node.name == "Sheet"
        )
        cells = next(
            node
            for node in sheet.body
            if isinstance(node, ast.FunctionDef) and node.name == "cells"
        )
        column_widths = next(
            node
            for node in sheet.body
            if isinstance(node, ast.FunctionDef) and node.name == "columnWidths"
        )
        row_heights = next(
            node
            for node in sheet.body
            if isinstance(node, ast.FunctionDef) and node.name == "rowHeights"
        )
        self.assertEqual("PropertySheet", ast.unparse(cells.returns))
        self.assertEqual("PropertyColumnWidths", ast.unparse(column_widths.returns))
        self.assertEqual("PropertyRowHeights", ast.unparse(row_heights.returns))
        self.assertFalse(
            any(
                isinstance(node, ast.FunctionDef)
                and node.name == "columnWidths"
                and any(
                    isinstance(decorator, ast.Attribute) and decorator.attr == "setter"
                    for decorator in node.decorator_list
                )
                for node in sheet.body
            )
        )

    def test_generated_overloads_keep_generic_fallback_last(self):
        source = """\
from __future__ import annotations

from FreeCAD import DocumentObject

class Document:
    def addObject(self, type: str, name: str = ..., objProxy: object | None = None, viewProxy: object | None = None, attach: bool = False, viewType: str = ...) -> DocumentObject: ...
    def addObject(self, type: str, name: str = ..., objProxy: object | None = None, viewProxy: object | None = None, attach: bool = False, viewType: str = ...) -> DocumentObject:
        '''Add an object to the document.'''
        ...
"""
        registrations = (
            PythonObjectType("Part::Feature", "Part", "Feature"),
            PythonObjectType("Part::FeaturePython", "Part", "Feature"),
            PythonObjectType("Sketcher::SketchObject", "Sketcher", "SketchObject"),
        )

        rendered = add_document_add_object_overloads(source, registrations)
        tree = ast.parse(rendered)
        document = next(node for node in tree.body if isinstance(node, ast.ClassDef))
        add_objects = [node for node in document.body if isinstance(node, ast.FunctionDef)]

        self.assertEqual(3, len(add_objects))
        self.assertEqual("addObject", add_objects[-1].name)
        self.assertEqual("DocumentObject", ast.unparse(add_objects[-1].returns))
        self.assertTrue(
            any(
                isinstance(decorator, ast.Name) and decorator.id == "overload"
                for decorator in add_objects[-1].decorator_list
            )
        )
        self.assertIn("Add an object", rendered)
        self.assertIn("Part::FeaturePython", rendered)
        self.assertIn("import Part as _Part", rendered)
        self.assertIn("import Sketcher as _Sketcher", rendered)

    def test_source_adjacent_document_stub_is_core_only(self):
        source = (ROOT_DIR / "src/App/Document.pyi").read_text(encoding="utf-8")
        tree = ast.parse(source)
        imported_modules = [
            alias.name for node in tree.body if isinstance(node, ast.Import) for alias in node.names
        ]
        imported_modules += [
            node.module for node in tree.body if isinstance(node, ast.ImportFrom) and node.module
        ]
        self.assertTrue(
            all(
                not module.startswith(("Part", "Sketcher", "PartDesign", "Mesh"))
                for module in imported_modules
            )
        )
        self.assertNotRegex(source, r'Literal\["[^\"]+::')


if __name__ == "__main__":
    unittest.main()
