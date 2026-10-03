# SPDX-License-Identifier: LGPL-2.1-or-later
"""Curvature Match: native geometry accuracy and document lifecycle."""

import math
import re
import tempfile
from pathlib import Path
from types import SimpleNamespace
import unittest

import FreeCAD as App
import Part
import Forms
from Forms.cage import ControlCage
from Forms.brep import ConversionError
from Forms.curvature import curvature_tensor, surface_jet, _tolerances
from Forms.matching import match_boundary, preview_match_shape


class CurvatureTest(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("FormsCurvatureTest")

    def tearDown(self):
        if "FormsCurvatureTest" in App.listDocuments():
            App.closeDocument("FormsCurvatureTest")

    def _plane(self):
        n = 6
        points = [(10*x/n, 10*y/n, 0.3 if 0<x<n and 0<y<n else 0)
                  for y in range(n+1) for x in range(n+1)]
        faces = [(y*(n+1)+x, y*(n+1)+x+1, (y+1)*(n+1)+x+1, (y+1)*(n+1)+x)
                 for y in range(n) for x in range(n)]
        obj = Forms.create_box(self.doc)
        obj.CageMode = "Editable"
        ControlCage(points, faces).write(obj)
        support = self.doc.addObject("Part::Feature", "Support")
        support.Shape = Part.makePlane(10, 10)
        self.doc.recompute()
        return obj, support, "Face1", "SelectedFace"

    def _cylinder(self, count=24, sphere=False):
        points = [(10*math.cos(2*math.pi*x/count), 10*math.sin(2*math.pi*x/count), 3.0*y)
                  for y in range(5) for x in range(count)]
        faces = [(y*count+x, y*count+(x+1)%count, (y+1)*count+(x+1)%count, (y+1)*count+x)
                 for y in range(4) for x in range(count)]
        points.append((0, 0, 12))
        faces += [(4*count+x, 4*count+(x+1)%count, 4*count+(x+2)%count, 5*count)
                  for x in range(0, count, 2)]
        obj = Forms.create_box(self.doc)
        obj.CageMode = "Editable"
        ControlCage(points, faces).write(obj)
        support = self.doc.addObject("Part::Feature", "Support")
        support.Shape = (Part.makeSphere(10, App.Vector(), App.Vector(0, 0, 1), -90, 0, 360)
                         if sphere else Part.makeCylinder(10, 10, App.Vector(0, 0, -10)))
        self.doc.recompute()
        return obj, support, "Face2", "AdjacentFaces"

    def _match(self, fixture):
        obj, support, face, mode = fixture
        match_boundary(obj, ControlCage.from_object(obj).boundary_edges,
                       (support, [face]), "Curvature", mode)
        self.doc.recompute()
        self.assertFalse(obj.Shape.isNull(), obj.ConversionStatus)
        self.assertTrue(obj.Shape.isValid())
        self.assertLessEqual(obj.MatchPositionError.Value, obj.MatchPositionTolerance.Value)
        self.assertLessEqual(obj.MatchAngularError.Value, obj.MatchAngularTolerance.Value)
        self.assertLessEqual(obj.MatchCurvatureError, obj.MatchCurvatureTolerance)
        self.assertTrue(obj.MatchStatus.startswith("G2 checked"))

    def test_surface_tensor_matches_analytic_cylinder_and_sphere(self):
        import numpy as np
        for surface, expected in ((Part.makeCylinder(10, 10).Faces[0].Surface, [0, 0, 0.1]),
                                  (Part.makeSphere(10).Faces[0].Surface, [0, 0.1, 0.1])):
            jet = surface_jet(surface, 0.37, 0.26)
            normal, tensor = curvature_tensor(jet)
            np.testing.assert_allclose(sorted(abs(np.linalg.eigvalsh(tensor))), expected, atol=1e-12)
            # Swapping parameter directions reverses the normal and curvature sign,
            # without changing the geometric continuity of the surface.
            flipped = [jet[index] for index in (0, 2, 1, 5, 4, 3)]
            reverse_normal, reverse_tensor = curvature_tensor(flipped)
            np.testing.assert_allclose(reverse_normal, -normal, atol=1e-12)
            np.testing.assert_allclose(reverse_tensor, -tensor, atol=1e-12)

    def test_trimmed_periodic_support_uses_its_long_arc(self):
        from Forms.curvature import _samples
        support = self.doc.addObject("Part::Feature", "Support")
        support.Shape = Part.makeCylinder(10, 10).Faces[0].Surface.toShape(0, 1.5*math.pi, 0, 10)
        cage = ControlCage([(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)], [(0, 1, 2, 3)])
        obj = SimpleNamespace(MatchSupport=(support, ["Face1"]), Placement=App.Placement(),
                              MatchBoundary=[0, 1, 2, 3], MatchParameters=[0, 0, 1, 0, 1, 1, 0, 1],
                              MatchTangentMode="SelectedFace")
        samples, _scale = _samples(obj, cage, 3)
        midpoint = next(sample[3] for sample in samples if sample[1:3] == (0.5, 0.0))
        self.assertAlmostEqual(midpoint[0], 10*math.cos(0.75*math.pi), places=9)
        self.assertAlmostEqual(midpoint[1], 10*math.sin(0.75*math.pi), places=9)

    def test_planar_preview_preserves_document_and_remote_controls(self):
        fixture = self._plane()
        obj, support, face, mode = fixture
        before = (list(obj.ControlPoints), list(obj.ControlFaces), list(obj.MatchBoundary),
                  obj.Shape.exportBrepToString())
        shape = preview_match_shape(obj, ControlCage.from_object(obj).boundary_edges,
                                    (support, [face]), "Curvature", mode)
        self.assertTrue(shape.isValid())
        self.assertEqual((list(obj.ControlPoints), list(obj.ControlFaces), list(obj.MatchBoundary),
                          obj.Shape.exportBrepToString()), before)
        center = App.Vector(obj.ControlPoints[24])
        self._match(fixture)
        self.assertEqual(obj.ControlPoints[24], center)
        points = list(obj.ControlPoints)
        obj.touch()
        self.doc.recompute()
        self.assertEqual(obj.ControlPoints, points)
        self.assertEqual(obj.ConversionLevel, obj.MaxRefinement)

    def test_cylindrical_seam_native_curvatures_between_validation_samples(self):
        fixture = self._cylinder()
        obj = fixture[0]
        self._match(fixture)
        # Independently check OCCT principal curvatures at new sample positions,
        # including the circular support's parameter wraparound.
        checked = set()
        for name, element in obj.Shape.ElementMap.items():
            match = re.match(r"FormsFace(\d+);", str(name))
            if not match or int(match.group(1)) >= 24:
                continue
            surface = obj.Shape.getElement(str(element)).Surface
            checked.add(int(match.group(1)))
            for u in (0.031, 0.273, 0.619, 0.941):
                point = surface.value(u, 0)
                self.assertAlmostEqual(math.hypot(point.x, point.y), 10, delta=0.001)
                curvatures = sorted(abs(surface.curvature(u, 0, kind)) for kind in ("Min", "Max"))
                self.assertAlmostEqual(curvatures[0], 0, delta=0.001)
                self.assertAlmostEqual(curvatures[1], 0.1, delta=0.001)
        self.assertEqual(checked, set(range(24)))
        from Forms.cage import control_surface_points
        mapped = control_surface_points(obj)
        for name, element in obj.Shape.ElementMap.items():
            match = re.match(r"FormsFace(\d+);", str(name))
            if match and int(match.group(1)) < 24:
                index = int(match.group(1))
                self.assertLess(mapped[index].distanceToPoint(
                    obj.Shape.getElement(str(element)).Surface.value(0, 0)), 1e-7)
        from Forms.preview import mesh_preview
        points, faces = mesh_preview(obj)
        self.assertEqual(len(faces), len(obj.ControlFaces) * 16)
        self.assertTrue(all(0 <= v < len(points) for face in faces for v in face))

    def test_spherical_support_matches_both_principal_curvatures(self):
        fixture = self._cylinder(sphere=True)
        self._match(fixture)
        obj = fixture[0]
        checked = set()
        for name, element in obj.Shape.ElementMap.items():
            match = re.match(r"FormsFace(\d+);", str(name))
            if match and int(match.group(1)) < 24:
                checked.add(int(match.group(1)))
                surface = obj.Shape.getElement(str(element)).Surface
                for u in (0.113, 0.417, 0.839):
                    for kind in ("Min", "Max"):
                        self.assertAlmostEqual(abs(surface.curvature(u, 0, kind)), 0.1, delta=0.001)
        self.assertEqual(checked, set(range(24)))

    def test_failed_match_is_atomic_and_tolerances_can_be_adjusted(self):
        fixture = self._cylinder(12)
        obj, support, face, mode = fixture
        before = (list(obj.ControlPoints), list(obj.MatchBoundary), str(obj.MatchContinuity),
                  obj.Shape.exportBrepToString())
        with self.assertRaisesRegex(ConversionError, "G2 tolerance was not reached"):
            match_boundary(obj, ControlCage.from_object(obj).boundary_edges,
                           (support, [face]), "Curvature", mode)
        self.assertEqual((list(obj.ControlPoints), list(obj.MatchBoundary), str(obj.MatchContinuity),
                          obj.Shape.exportBrepToString()), before)
        obj.MatchPositionTolerance = 0.002
        obj.MatchCurvatureTolerance = 0.03
        self._match(fixture)

    def test_support_motion_save_restore_and_undo_redo(self):
        fixture = self._plane()
        obj, support, _face, _mode = fixture
        self.doc.openTransaction("G2 Match")
        self._match(fixture)
        self.doc.commitTransaction()
        self.doc.undo()
        self.assertFalse(obj.MatchBoundary)
        self.doc.redo()
        self.doc.recompute()
        self.assertEqual(str(obj.MatchContinuity), "Curvature")
        support.Placement.Base.z = 0.2
        self.doc.recompute()
        self.assertFalse(obj.Shape.isNull(), obj.ConversionStatus)
        self.assertLess(obj.MatchPositionError.Value, 0.001)
        with tempfile.TemporaryDirectory() as directory:
            filename = str(Path(directory) / "curvature.FCStd")
            self.doc.saveAs(filename)
            name = obj.Name
            App.closeDocument(self.doc.Name)
            restored = App.openDocument(filename)
            try:
                copy = restored.getObject(name)
                copy.touch()
                restored.recompute()
                self.assertEqual(str(copy.MatchContinuity), "Curvature")
                self.assertFalse(copy.Shape.isNull(), copy.ConversionStatus)
                self.assertLess(copy.MatchCurvatureError, copy.MatchCurvatureTolerance)
            finally:
                App.closeDocument(restored.Name)

    def test_legacy_tangent_mode_migration_and_invalid_tolerances(self):
        from Forms.feature import FormFeatureProxy
        obj = Forms.create_box(self.doc)
        obj.MatchContinuity = ["Connected", "Tangent"]
        obj.MatchContinuity = "Tangent"
        FormFeatureProxy._ensure_match_properties(obj)
        self.assertEqual(str(obj.MatchContinuity), "Tangent")
        self.assertIn("Curvature", obj.getEnumerationsOfProperty("MatchContinuity"))
        for value in (0, -1, float("nan"), float("inf")):
            with self.assertRaisesRegex(ConversionError, "finite and positive"):
                _tolerances(SimpleNamespace(MatchCurvatureTolerance=value))

    def test_profile_shortcut_does_not_bypass_a_deleted_match_support(self):
        profile = Part.Face(Part.Wire([Part.makeCircle(10)]))
        obj = Forms.create_face(self.doc, profile=profile)
        support = self.doc.addObject("Part::Feature", "Support")
        support.Shape = profile
        obj.MatchSupport = (support, ["Face1"])
        obj.MatchBoundary = list(ControlCage.from_object(obj).boundary_loops()[0])
        obj.MatchContinuity = "Curvature"
        self.doc.removeObject(support.Name)
        self.doc.recompute()
        self.assertTrue(obj.Shape.isNull())
        self.assertIn("support is no longer valid", obj.ConversionStatus)

    @unittest.skipUnless(App.GuiUp, "Match task panel requires FreeCADGui")
    def test_task_panel_preview_cancel_apply_and_edit_cancel(self):
        import FreeCADGui as Gui
        from PySide import QtWidgets
        from Forms.edit import active_form_session
        obj, support, face, _mode = self._plane()
        initial = list(obj.ControlPoints)
        gui_doc = Gui.getDocument(self.doc.Name)
        gui_doc.setEdit(obj, 0)
        QtWidgets.QApplication.processEvents()
        session = active_form_session(obj)
        try:
            edges = ControlCage.from_object(obj).boundary_edges
            self.assertTrue(session.start_match_tool(obj, edges, (support, [face])))
            session.match_mode.setCurrentIndex(session.match_mode.findData("CurvatureAdjacentFaces"))
            self.assertFalse(session.match_apply_button.isEnabled())
            self.assertIn("support surface", session.match_preview_status.text())
            session.match_mode.setCurrentIndex(session.match_mode.findData("CurvatureSelectedFace"))
            QtWidgets.QApplication.processEvents()
            self.assertFalse(session.match_preview_shape.isNull(), session.match_preview_status.text())
            self.assertTrue(session.match_apply_button.isEnabled())
            self.assertIn("G2 checked", session.match_preview_status.text())
            self.assertEqual(obj.ControlPoints, initial)
            self.assertFalse(obj.MatchBoundary)
            session.stop_match_tool()
            self.assertEqual(obj.ControlPoints, initial)
            self.assertTrue(session.start_match_tool(obj, edges, (support, [face])))
            session.match_mode.setCurrentIndex(session.match_mode.findData("CurvatureSelectedFace"))
            self.assertTrue(session.apply_match_tool())
            self.assertEqual(str(obj.MatchContinuity), "Curvature")
            self.assertFalse(obj.Shape.isNull())
            session.reject()
            self.assertFalse(obj.MatchBoundary)
            self.assertEqual(obj.ControlPoints, initial)
        finally:
            if gui_doc.getInEdit():
                gui_doc.resetEdit()


def suite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(CurvatureTest)
