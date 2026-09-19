# SPDX-License-Identifier: LGPL-2.1-or-later

"""Deterministic IfcOpenShell and NativeIFC round-trip tests."""

from pathlib import Path
import tempfile
import unittest

import FreeCAD

from nativeifc import backend

FIXTURES = Path(__file__).parent / "fixtures"


class TestIfcRoundtrip(unittest.TestCase):
    def setUp(self):
        backend.invalidate()
        status = backend.get_status()
        if not status.available:
            self.skipTest(f"IfcOpenShell is unavailable: {status.error}")
        self.ifcopenshell = backend.get_backend()

    def test_schema_geometry_and_units(self):
        import ifcopenshell.geom
        import ifcopenshell.util.unit

        fixtures = (("roundtrip_ifc2x3.ifc", "IFC2X3"), ("roundtrip_ifc4.ifc", "IFC4"))
        for filename, schema in fixtures:
            with self.subTest(schema=schema):
                model = self.ifcopenshell.open(FIXTURES / filename)
                self.assertEqual(model.schema, schema)
                self.assertAlmostEqual(ifcopenshell.util.unit.calculate_unit_scale(model), 0.001)

                settings = ifcopenshell.geom.settings()
                shape = ifcopenshell.geom.create_shape(settings, model.by_type("IfcWall")[0])
                self.assertGreater(len(shape.geometry.verts), 0)
                self.assertGreater(len(shape.geometry.faces), 0)

    def test_ifcopenshell_edit_write_reopen(self):
        model = self.ifcopenshell.open(FIXTURES / "roundtrip_ifc4.ifc")
        wall = model.by_type("IfcWall")[0]
        global_id = wall.GlobalId
        wall.Name = "Edited Wall"

        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "roundtrip.ifc"
            model.write(output)
            reopened = self.ifcopenshell.open(output)

        reopened_wall = reopened.by_guid(global_id)
        self.assertEqual(reopened_wall.Name, "Edited Wall")
        self.assertEqual(len(reopened.by_type("IfcWall")), 1)

    def test_nativeifc_import_edit_save_reopen(self):
        from nativeifc import ifc_import
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcRoundtrip")
        try:
            ifc_import.insert(
                str(FIXTURES / "roundtrip_ifc4.ifc"),
                document.Name,
                strategy=2,
                shapemode=2,
                switchwb=False,
                silent=True,
                singledoc=False,
            )
            projects = [
                obj for obj in document.Objects if hasattr(getattr(obj, "Proxy", None), "ifcfile")
            ]
            self.assertEqual(len(projects), 1)

            project = projects[0]
            wall = project.Proxy.ifcfile.by_type("IfcWall")[0]
            global_id = wall.GlobalId
            ifc_tools.set_attribute(project.Proxy.ifcfile, wall, "Name", "NativeIFC Edited Wall")

            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "nativeifc-roundtrip.ifc"
                ifc_tools.save_ifc(project, str(output))
                reopened = self.ifcopenshell.open(output)

            self.assertEqual(reopened.by_guid(global_id).Name, "NativeIFC Edited Wall")
        finally:
            FreeCAD.closeDocument(document.Name)


if __name__ == "__main__":
    unittest.main()
