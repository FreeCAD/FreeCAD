# SPDX-License-Identifier: LGPL-2.1-or-later

"""Integration tests for IFC importer access through the runtime ifc_backend."""

from pathlib import Path
import unittest

import FreeCAD

from importers import importIFC
from importers import importIFCHelper
from nativeifc import ifc_backend

FIXTURES = Path(__file__).parent / "fixtures"


class TestIfcImportBackend(unittest.TestCase):
    document_name = "IfcBackendMulticoreImport"

    def setUp(self):
        ifc_backend.invalidate()
        status = ifc_backend.get_status(capability=ifc_backend.MULTICORE_IMPORT)
        if not status.available:
            self.skipTest(f"IfcOpenShell multicore import is unavailable: {status.error}")
        if self.document_name in FreeCAD.listDocuments():
            FreeCAD.closeDocument(self.document_name)

    def tearDown(self):
        if self.document_name in FreeCAD.listDocuments():
            FreeCAD.closeDocument(self.document_name)
        ifc_backend.invalidate()

    def test_primary_importer_dispatches_to_multicore_backend(self):
        preferences = importIFCHelper.getPreferences()
        preferences["MULTICORE"] = max(2, preferences["MULTICORE"])
        document = FreeCAD.newDocument(self.document_name)

        result = importIFC.insert(
            str(FIXTURES / "roundtrip_ifc4.ifc"),
            document.Name,
            preferences=preferences,
        )

        self.assertIs(result, document)
        shaped_objects = [obj for obj in document.Objects if hasattr(obj, "Shape")]
        self.assertTrue(shaped_objects)
        self.assertTrue(any(not obj.Shape.isNull() for obj in shaped_objects))

    def test_explorer_backend_opens_and_previews_fixture(self):
        status = ifc_backend.get_status(capability=ifc_backend.EXPLORER)
        if not status.available:
            self.skipTest(f"IfcOpenShell Explorer support is unavailable: {status.error}")
        ifcopenshell = ifc_backend.get_backend(capability=ifc_backend.EXPLORER)
        geom = ifc_backend.get_module("ifcopenshell.geom", capability=ifc_backend.EXPLORER)
        model = ifcopenshell.open(FIXTURES / "roundtrip_ifc4.ifc")

        shape = geom.create_shape(ifc_backend.create_mesh_settings(), model.by_type("IfcWall")[0])

        self.assertGreater(len(shape.geometry.verts), 0)
        self.assertGreater(len(shape.geometry.faces), 0)


if __name__ == "__main__":
    unittest.main()
