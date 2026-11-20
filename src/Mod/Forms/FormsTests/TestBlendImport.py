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

import importlib
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import FreeCAD as App
import importBlend

from Forms.blend_import import (
    BlendImportError,
    _create_feature,
    _origin_symmetry,
    _validate_cage,
    import_file,
)
from Forms.form import create_form
from Forms.topology import box_control_cage


def _box_data(name="BlenderBox"):
    vertices, faces = box_control_cage(10.0, 20.0, 30.0)
    return {
        "name": name,
        "vertices": vertices,
        "faces": faces,
        "edge_sharpness": [[0, 1, 2.5]],
        "vertex_sharpness": [0.0] * len(vertices),
    }


class BlendImportTest(unittest.TestCase):
    def tearDown(self):
        for document in list(App.listDocuments().values()):
            if document.Name.startswith("FormsTestBlend"):
                App.closeDocument(document.Name)

    def test_imported_payload_creates_an_editable_form(self):
        document = App.newDocument("FormsTestBlendFeature")
        obj = _create_feature(document, "example.blend", "4.0.1", _box_data())

        self.assertEqual(obj.FormType, "Forms::Form")
        self.assertEqual(obj.CageMode, "Editable")
        self.assertEqual(obj.SourceObject, "BlenderBox")
        self.assertEqual(obj.BlenderVersion, "4.0.1")
        self.assertEqual(len(obj.ControlPoints), 8)
        self.assertEqual(len(obj.ControlFaces), 6)
        self.assertEqual(obj.EdgeSharpness, ["0 1 2.5"])
        self.assertIn("SourceFile", obj.PropertiesList)
        self.assertIn("SourceObject", obj.PropertiesList)
        self.assertIn("BlenderVersion", obj.PropertiesList)
        self.assertFalse(obj.Shape.isNull())
        self.assertEqual(obj.Shape.ShapeType, "Solid")

    def test_open_creates_document_with_file_stem_as_label(self):
        with patch("importBlend.import_file", return_value=([], [])):
            document = importBlend.open("Head phone.blend")
        try:
            self.assertIsNotNone(document)
            self.assertEqual(document.Label, "Head phone")
        finally:
            if document is not None:
                App.closeDocument(document.Name)

    def test_malformed_blender_output_is_reported_without_leaking_a_document(self):
        before = set(App.listDocuments())
        for payload in (
            [1, 2, 3],
            {"format": "AstoCAD Forms control cage", "version": 1, "objects": 5},
        ):
            with self.subTest(payload=payload):
                with patch("Forms.blend_import._run_blender", return_value=payload):
                    with patch("Forms.blend_import.Path.is_file", return_value=True):
                        with self.assertRaises(BlendImportError):
                            import_file("malformed.blend", None)
                        with patch("importBlend._show_error"):
                            self.assertIsNone(importBlend.open("malformed.blend"))
                self.assertEqual(set(App.listDocuments()), before)

    def test_open_closes_its_document_on_unexpected_errors(self):
        before = set(App.listDocuments())
        with patch("importBlend.import_file", side_effect=RuntimeError("unexpected")):
            with self.assertRaisesRegex(RuntimeError, "unexpected"):
                importBlend.open("broken.blend")
        self.assertEqual(set(App.listDocuments()), before)

    def test_blender_source_file_is_not_parsed_as_an_option(self):
        import os
        import subprocess

        from Forms.blender_bridge import run_blender_script

        with patch("Forms.blender_bridge.subprocess.run") as run:
            run.return_value = subprocess.CompletedProcess([], 0, "", "")
            run_blender_script(
                "script.py", ("output.json",), source_file="-scene.blend", executable="blender"
            )
        command = run.call_args.args[0]
        source = command[command.index("--disable-autoexec") + 1]
        self.assertEqual(source, os.path.abspath("-scene.blend"))
        self.assertFalse(source.startswith("-"))
        self.assertNotIn("shell", run.call_args.kwargs)

    def test_missing_configured_blender_is_reported_instead_of_replaced(self):
        import os
        import tempfile

        from Forms.blend_import import _run_blender
        from Forms.blender_bridge import BlenderBridgeError, find_blender_executable

        parameters = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Forms")
        previous = parameters.GetString("BlenderExecutable", "")
        missing = os.path.join(tempfile.gettempdir(), "astocad-no-such-dir", "blender.exe")
        parameters.SetString("BlenderExecutable", missing)
        try:
            # Another Blender is available, but the user chose a specific one.
            with patch("Forms.blender_bridge.shutil.which", return_value=sys.executable):
                with self.assertRaisesRegex(BlenderBridgeError, "'BlenderExecutable'"):
                    find_blender_executable()
                with patch("Forms.blender_bridge.subprocess.run") as run:
                    with self.assertRaisesRegex(BlendImportError, "blender.exe"):
                        _run_blender("scene.blend")
                run.assert_not_called()
        finally:
            if previous:
                parameters.SetString("BlenderExecutable", previous)
            else:
                parameters.RemString("BlenderExecutable")

    def test_legacy_imported_type_is_migrated_on_restore(self):
        document = App.newDocument("FormsTestBlendLegacy")
        obj = create_form(document)
        obj.FormType = "Forms::Imported"

        obj.Proxy.onDocumentRestored(obj)

        self.assertEqual(obj.FormType, "Forms::Form")

    def test_generic_form_has_no_blender_specific_properties(self):
        document = App.newDocument("FormsTestBlendGeneric")
        obj = create_form(document)

        self.assertNotIn("SourceFile", obj.PropertiesList)
        self.assertNotIn("SourceObject", obj.PropertiesList)
        self.assertNotIn("BlenderVersion", obj.PropertiesList)

    def test_nan_creases_are_rejected_instead_of_read_as_smooth(self):
        document = App.newDocument("FormsTestBlendNaNCrease")
        data = _box_data()
        data["vertex_sharpness"][0] = float("nan")
        with self.assertRaisesRegex(BlendImportError, "non-finite vertex creases"):
            _create_feature(document, "example.blend", "4.0.1", data)
        data = _box_data()
        data["edge_sharpness"] = [[0, 1, float("nan")]]
        with self.assertRaisesRegex(BlendImportError, "non-finite edge creases"):
            _create_feature(document, "example.blend", "4.0.1", data)
        self.assertEqual(document.Objects, [])

        # The Blender-side reader rejects the object rather than reading 0.
        with patch.dict(sys.modules, {"bpy": SimpleNamespace()}):
            extract = importlib.import_module("Forms.blender_extract")
        with self.assertRaisesRegex(ValueError, "non-finite crease"):
            extract._sharpness(float("nan"))

    def test_validate_cage_rejects_triangles(self):
        data = _box_data()
        data["faces"][0] = data["faces"][0][:3]
        with self.assertRaisesRegex(BlendImportError, "all-quad"):
            _validate_cage(data)

    def test_origin_symmetry_checks_points_and_faces(self):
        data = _box_data()
        self.assertTrue(_origin_symmetry(data["vertices"], data["faces"], 0))
        vertices = list(data["vertices"])
        vertices[0] = (vertices[0][0] + 1.0, vertices[0][1], vertices[0][2])
        self.assertFalse(_origin_symmetry(vertices, data["faces"], 0))

    def test_origin_symmetry_scales_to_a_dense_cage(self):
        size = 80
        vertices = [
            (column - (size - 1) / 2.0, row - (size - 1) / 2.0, 0.0)
            for row in range(size)
            for column in range(size)
        ]
        faces = [
            (
                row * size + column,
                row * size + column + 1,
                (row + 1) * size + column + 1,
                (row + 1) * size + column,
            )
            for row in range(size - 1)
            for column in range(size - 1)
        ]

        self.assertTrue(_origin_symmetry(vertices, faces, 0))

    def test_file_without_compatible_cage_reports_rejections(self):
        document = App.newDocument("FormsTestBlendEmpty")
        payload = {
            "format": "AstoCAD Forms control cage",
            "version": 1,
            "blender_version": "4.0.1",
            "objects": [],
            "rejected": ["Triangle: the control cage must contain only quad faces"],
        }
        with patch("Forms.blend_import._run_blender", return_value=payload):
            with patch("Forms.blend_import.Path.is_file", return_value=True):
                with self.assertRaisesRegex(BlendImportError, "Triangle"):
                    import_file("empty.blend", document)


def suite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(BlendImportTest)
