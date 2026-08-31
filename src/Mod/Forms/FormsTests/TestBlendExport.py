# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import FreeCAD as App
import Part

from Forms.blend_export import BlendExportError, _run_blender, build_payload, export_file
from Forms.box import create_box


class BlendExportTest(unittest.TestCase):
    def tearDown(self):
        for document in list(App.listDocuments().values()):
            if document.Name.startswith("FormsTestBlendExport"):
                App.closeDocument(document.Name)

    def test_blend_export_type_is_registered(self):
        self.assertIn("importBlend", App.getExportType("blend"))

    def test_form_exports_editable_world_space_subdivision_cage(self):
        document = App.newDocument("FormsTestBlendExportForm")
        obj = create_box(document)
        obj.Placement.Base = App.Vector(7.0, 11.0, 13.0)
        obj.CageMode = "Editable"
        obj.EdgeSharpness = ["0 1 2.5"]
        document.recompute()

        payload = build_payload([obj])
        exported = payload["objects"][0]

        self.assertEqual(exported["kind"], "SUBDIVISION")
        self.assertEqual(len(exported["vertices"]), len(obj.ControlPoints))
        self.assertTrue(all(len(face) == 4 for face in exported["faces"]))
        expected = obj.getGlobalPlacement().multVec(obj.ControlPoints[0])
        self.assertAlmostEqual(exported["vertices"][0][0], expected.x)
        self.assertAlmostEqual(exported["vertices"][0][1], expected.y)
        self.assertAlmostEqual(exported["vertices"][0][2], expected.z)
        self.assertEqual(exported["edge_sharpness"], [[0, 1, 2.5]])

    def test_general_shape_exports_tessellated_mesh(self):
        document = App.newDocument("FormsTestBlendExportShape")
        obj = document.addObject("Part::Feature", "Solid")
        obj.Shape = Part.makeBox(10.0, 20.0, 30.0)
        document.recompute()

        exported = build_payload([obj], deflection=0.5)["objects"][0]

        self.assertEqual(exported["kind"], "MESH")
        self.assertTrue(exported["vertices"])
        self.assertTrue(all(len(face) == 3 for face in exported["faces"]))

    def test_selected_body_exports_its_final_result(self):
        document = App.newDocument("FormsTestBlendExportBody")
        body = document.addObject("PartDesign::Body", "Body")
        feature = body.newObject("PartDesign::Feature", "Result")
        feature.Shape = Part.makeBox(10.0, 10.0, 10.0)
        document.recompute()

        exported = build_payload([body], deflection=0.5)["objects"][0]

        self.assertEqual(exported["kind"], "MESH")
        self.assertTrue(exported["faces"])

    def test_advanced_local_form_topology_falls_back_to_shape(self):
        document = App.newDocument("FormsTestBlendExportLocal")
        obj = create_box(document)
        obj.CageMode = "Editable"
        obj.DissolvedEdges = ["0 1"]
        document.recompute()

        exported = build_payload([obj], deflection=0.5)["objects"][0]

        self.assertEqual(exported["kind"], "MESH")

    def test_empty_selection_is_rejected(self):
        with self.assertRaisesRegex(BlendExportError, "No objects were selected"):
            build_payload([])

    def test_failed_staging_copy_preserves_existing_destination(self):
        payload = {"format": "AstoCAD Blender export", "version": 1, "objects": [{}]}
        with tempfile.TemporaryDirectory() as temporary:
            destination = Path(temporary) / "existing.blend"
            destination.write_bytes(b"previous blend contents")

            def write_blender_output(_script, arguments, **_kwargs):
                Path(arguments[1]).write_bytes(b"new blend contents")

            with patch(
                "Forms.blend_export.run_blender_script", side_effect=write_blender_output
            ):
                with patch(
                    "Forms.blend_export.shutil.copyfile",
                    side_effect=OSError("simulated disk failure"),
                ):
                    with self.assertRaisesRegex(BlendExportError, "could not be saved"):
                        _run_blender(payload, destination, executable="blender")

            self.assertEqual(destination.read_bytes(), b"previous blend contents")
            self.assertEqual(list(Path(temporary).glob(".*.tmp")), [])

    def test_export_file_runs_blender_with_built_payload(self):
        document = App.newDocument("FormsTestBlendExportRun")
        obj = create_box(document)
        document.recompute()

        with patch("Forms.blend_export._run_blender") as run_blender:
            rejected = export_file([obj], "example.blend")

        self.assertEqual(rejected, [])
        payload, filename, executable = run_blender.call_args.args
        self.assertEqual(payload["objects"][0]["kind"], "SUBDIVISION")
        self.assertEqual(filename, "example.blend")
        self.assertIsNone(executable)


def suite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(BlendExportTest)
