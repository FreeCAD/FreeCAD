# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

import ast
from functools import lru_cache
from pathlib import Path
import sys
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
