# SPDX-License-Identifier: LGPL-2.1-or-later

"""Regression tests for legacy IFC export through the runtime ifc_backend."""

import ast
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import FreeCAD
import Part

from importers import exportIFC
from importers import exportIFCHelper
from nativeifc import ifc_backend


class TestIfcLegacyExport(unittest.TestCase):
    document_name = "IfcLegacyBackendExport"

    def setUp(self):
        ifc_backend.invalidate()
        status = ifc_backend.get_status(capability=ifc_backend.LEGACY_EXPORT)
        if not status.available:
            self.skipTest(f"IfcOpenShell export is unavailable: {status.error}")
        if self.document_name in FreeCAD.listDocuments():
            FreeCAD.closeDocument(self.document_name)
        self.document = FreeCAD.newDocument(self.document_name)
        self.obj = self.document.addObject("Part::Feature", "LegacyBox")
        self.obj.Label = "Legacy Box"
        self.obj.Shape = Part.makeBox(1000, 200, 300)
        self.document.recompute()

    def tearDown(self):
        if self.document_name in FreeCAD.listDocuments():
            FreeCAD.closeDocument(self.document_name)
        ifc_backend.invalidate()

    def test_legacy_export_writes_ifc2x3_and_ifc4(self):
        with tempfile.TemporaryDirectory() as directory:
            for schema in ("IFC2X3", "IFC4"):
                with self.subTest(schema=schema):
                    preferences = exportIFC.getPreferences()
                    preferences["SCHEMA"] = schema
                    output = Path(directory) / f"legacy-{schema}.ifc"

                    exportIFC.export([self.obj], str(output), preferences=preferences)

                    self.assertTrue(output.exists())
                    model = ifc_backend.get_backend(capability=ifc_backend.READ).open(output)
                    self.assertEqual(model.schema, schema)
                    self.assertTrue(model.by_type("IfcProduct"))

    def test_legacy_export_without_nativeifc_apis(self):
        import_module = ifc_backend.importlib.import_module

        def load(name, *args, **kwargs):
            if name == "ifcopenshell.api" or name.startswith("ifcopenshell.api."):
                raise ModuleNotFoundError(name)
            return import_module(name, *args, **kwargs)

        ifc_backend.invalidate()
        with patch.object(ifc_backend.importlib, "import_module", side_effect=load):
            self.assertFalse(ifc_backend.get_status(capability=ifc_backend.EXPORT).available)
            self.test_legacy_export_writes_ifc2x3_and_ifc4()

    def test_legacy_exporters_have_no_direct_ifcopenshell_imports(self):
        violations = []
        for module in (exportIFC, exportIFCHelper):
            path = Path(module.__file__).resolve()
            tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
            for node in ast.walk(tree):
                if isinstance(node, ast.Import):
                    names = [alias.name for alias in node.names]
                elif isinstance(node, ast.ImportFrom) and node.module:
                    names = [node.module]
                else:
                    continue
                if any(
                    name == "ifcopenshell" or name.startswith("ifcopenshell.") for name in names
                ):
                    violations.append(f"{path.name}:{node.lineno}")
        self.assertEqual(violations, [])


if __name__ == "__main__":
    unittest.main()
