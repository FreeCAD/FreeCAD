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

    def test_part_design_form_exports_its_body_result(self):
        from Forms.additive import create_partdesign_form

        document = App.newDocument("FormsTestBlendExportPartDesignForm")
        body = document.addObject("PartDesign::Body", "Body")
        base = body.newObject("PartDesign::AdditiveBox", "Box")
        base.Length = base.Width = base.Height = 40.0
        document.recompute()
        form = create_partdesign_form(body, base, "Box", operation="Subtractive")
        form.FormPlacement = App.Placement(App.Vector(20.0, 20.0, 20.0), App.Rotation())
        document.recompute()

        exported = build_payload([form], deflection=0.5)["objects"][0]

        # The cutting tool's cage is not the object the user sees: export the
        # Body solid with the cut, as for any other Part Design feature.
        self.assertEqual(exported["kind"], "MESH")
        bounds = App.BoundBox()
        for point in exported["vertices"]:
            bounds.add(App.Vector(*point))
        self.assertAlmostEqual(bounds.XLength, 40.0, places=6)
        self.assertAlmostEqual(bounds.ZLength, 40.0, places=6)

    def test_part_design_form_without_base_exports_like_part_design(self):
        from Forms.additive import create_partdesign_form
        from Forms.blend_export import _shape_payload

        document = App.newDocument("FormsTestBlendExportWithoutBase")
        body = document.addObject("PartDesign::Body", "Body")
        form = create_partdesign_form(
            body, None, "Box", placement=App.Placement(App.Vector(100, 0, 0), App.Rotation())
        )
        document.recompute()

        exported = build_payload([form], deflection=0.5)["objects"][0]

        # Like a native additive feature without a base, the Form is the Body
        # solid itself, so it stays an editable subdivision cage.
        self.assertEqual(exported["kind"], "SUBDIVISION")
        self.assertEqual(len(exported["vertices"]), len(form.ControlPoints))
        cage = App.BoundBox()
        for point in exported["vertices"]:
            cage.add(App.Vector(*point))
        mesh = App.BoundBox()
        for point in _shape_payload(form, 0.5)["vertices"]:
            mesh.add(App.Vector(*point))
        self.assertTrue(cage.Center.isEqual(mesh.Center, 1.0e-6))
        self.assertAlmostEqual(cage.Center.x, 100.0, places=6)

        # A subtractive Form without a base has no result to export.
        other = document.addObject("PartDesign::Body", "OtherBody")
        cut = create_partdesign_form(other, None, "Box", operation="Subtractive")
        document.recompute()
        with self.assertRaisesRegex(BlendExportError, "has no shape"):
            build_payload([cut], deflection=0.5)

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

    def test_exported_view_does_not_clip_metre_sized_parts(self):
        import json

        from Forms.blender_bridge import find_blender_executable, run_blender_script

        if not find_blender_executable():
            self.skipTest("Blender integration requires an installed Blender")
        document = App.newDocument("FormsTestBlendExportClipping")
        obj = create_box(document)
        document.recompute()
        probe = (
            "import bpy, json, sys\n"
            "spaces = [space for screen in bpy.data.screens for area in screen.areas\n"
            "          for space in area.spaces if space.type == 'VIEW_3D']\n"
            "json.dump({'scale': bpy.context.scene.unit_settings.scale_length,\n"
            "           'clip': [[space.clip_start, space.clip_end] for space in spaces]},\n"
            "          open(sys.argv[sys.argv.index('--') + 1], 'w'))\n"
        )
        with tempfile.TemporaryDirectory() as temporary:
            blend = Path(temporary) / "clipping.blend"
            script = Path(temporary) / "probe.py"
            output = Path(temporary) / "clipping.json"
            script.write_text(probe, encoding="utf-8")
            self.assertEqual(export_file([obj], str(blend)), [])
            run_blender_script(script, (output,), source_file=blend, operation="probing")
            result = json.loads(output.read_text(encoding="utf-8"))

        self.assertAlmostEqual(result["scale"], 0.001, places=6)
        self.assertTrue(result["clip"])
        for clip_start, clip_end in result["clip"]:
            # 1 BU is 1 mm: parts several metres long stay inside the far plane,
            # and the depth range is no wider than Blender's factory one.
            self.assertGreaterEqual(clip_end, 100000.0)
            self.assertLessEqual(clip_end / clip_start, 1.0e5 * 1.001)

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
