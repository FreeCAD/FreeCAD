# SPDX-License-Identifier: LGPL-2.1-or-later

import math

import FreeCAD
import Part
import Sketcher

from SketcherTests.GuiTestCase import FreeCADGui, SketcherGuiTestCase


class TestConvertToNURBSGui(SketcherGuiTestCase):
    def setUp(self):
        super().setUp()
        FreeCADGui.activateWorkbench("SketcherWorkbench")
        self.doc = FreeCAD.newDocument("ConvertToNURBSGuiTest")
        self.doc.UndoMode = 1
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")

    @staticmethod
    def make_spline():
        spline = Part.BSplineCurve()
        spline.interpolate(
            [FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(5, 10, 0), FreeCAD.Vector(15, 0, 0)]
        )
        return spline

    def add_spline_with_internal_geometry(self):
        geo_id = self.sketch.addGeometry(self.make_spline())
        self.sketch.exposeInternalGeometry(geo_id)
        self.assertGreater(self.sketch.GeometryCount, 1)
        return geo_id

    def convert(self, *sub_names):
        self.doc.recompute()
        FreeCADGui.ActiveDocument.setEdit(self.sketch.Name)
        self.flush_gui()
        FreeCADGui.Selection.clearSelection()
        for name in sub_names:
            FreeCADGui.Selection.addSelection(self.doc.Name, self.sketch.Name, name)
        FreeCADGui.runCommand("Sketcher_BSplineConvertToNURBS", 0)
        self.flush_gui()

    def geometry_types(self):
        return [geometry.TypeId for geometry in self.sketch.Geometry]

    def constraint_signature(self):
        return [
            (
                constraint.Type,
                constraint.First,
                constraint.FirstPos,
                constraint.Second,
                constraint.SecondPos,
                constraint.Third,
                constraint.ThirdPos,
                constraint.Value,
            )
            for constraint in self.sketch.Constraints
        ]

    def test_box_selection_preserves_spline_internal_geometry(self):
        self.add_spline_with_internal_geometry()
        types_before = self.geometry_types()
        constraints_before = self.constraint_signature()

        # The subelements gathered by a box selection include the pole circles.
        self.convert(*(f"Edge{index + 1}" for index in range(self.sketch.GeometryCount)))

        self.assertEqual(self.geometry_types(), types_before)
        self.assertEqual(self.constraint_signature(), constraints_before)

    def test_selecting_only_a_pole_circle_preserves_it(self):
        self.add_spline_with_internal_geometry()
        pole_id = next(
            index
            for index, geometry in enumerate(self.sketch.Geometry)
            if geometry.TypeId == "Part::GeomCircle"
        )
        types_before = self.geometry_types()
        constraints_before = self.constraint_signature()

        self.convert(f"Edge{pole_id + 1}")

        self.assertEqual(self.geometry_types(), types_before)
        self.assertEqual(self.constraint_signature(), constraints_before)

    def test_mixed_selection_still_converts_an_ordinary_circle(self):
        self.add_spline_with_internal_geometry()
        types_before = self.geometry_types()
        constraints_before = self.constraint_signature()
        circle_id = self.sketch.addGeometry(
            Part.Circle(FreeCAD.Vector(30, 0, 0), FreeCAD.Vector(0, 0, 1), 3), True
        )

        self.convert(*(f"Edge{index + 1}" for index in range(self.sketch.GeometryCount)))

        self.assertEqual(self.geometry_types()[:circle_id], types_before)
        self.assertEqual(self.constraint_signature()[: len(constraints_before)], constraints_before)
        self.assertEqual(self.sketch.Geometry[circle_id].TypeId, "Part::GeomBSplineCurve")
        self.assertTrue(self.sketch.Geometry[circle_id].isPeriodic())

    def test_external_spline_is_copied_into_the_sketch(self):
        spline = self.make_spline()
        source = self.doc.addObject("Part::Feature", "Source")
        source.Shape = spline.toShape()
        self.doc.recompute()
        self.sketch.addExternal(source.Name, "Edge1")
        self.doc.recompute()
        external_before = self.sketch.ExternalGeometry
        self.assertEqual(len(external_before), 1)

        self.convert("ExternalEdge1")

        self.assertGreater(self.sketch.GeometryCount, 0)
        copied = self.sketch.Geometry[0]
        self.assertEqual(copied.TypeId, "Part::GeomBSplineCurve")
        for parameter in (
            spline.FirstParameter,
            (spline.FirstParameter + spline.LastParameter) / 2,
            spline.LastParameter,
        ):
            self.assertLess((copied.value(parameter) - spline.value(parameter)).Length, 1e-7)
        self.assertEqual(self.sketch.ExternalGeometry, external_before)
        self.assertTrue(any(c.Type == "InternalAlignment" for c in self.sketch.Constraints))

        self.doc.undo()
        self.assertEqual(self.sketch.GeometryCount, 0)
        self.assertEqual(self.sketch.ExternalGeometry, external_before)

    def test_conic_auxiliary_deletion_remaps_other_selected_geometry(self):
        ellipse_id = self.sketch.addGeometry(Part.Ellipse(FreeCAD.Vector(0, 0, 0), 10, 5))
        self.sketch.exposeInternalGeometry(ellipse_id)
        self.sketch.addGeometry(Part.Circle(FreeCAD.Vector(30, 0, 0), FreeCAD.Vector(0, 0, 1), 3))

        self.convert(*(f"Edge{index + 1}" for index in range(self.sketch.GeometryCount)))

        splines = [g for g in self.sketch.Geometry if g.TypeId == "Part::GeomBSplineCurve"]
        self.assertEqual(len(splines), 2)
        self.assertTrue(all(spline.isPeriodic() for spline in splines))
        self.assertFalse(any(g.TypeId == "Part::GeomLineSegment" for g in self.sketch.Geometry))

    def test_trimmed_conic_hands_internal_geometry_to_sibling_arc(self):
        # Two arcs of one ellipse sharing its center, as left by trimming it.
        ellipse = Part.Ellipse(FreeCAD.Vector(0, 0, 0), 10, 5)
        lower = self.sketch.addGeometry(Part.ArcOfEllipse(ellipse, math.pi, 2 * math.pi))
        self.sketch.exposeInternalGeometry(lower)
        upper = self.sketch.addGeometry(Part.ArcOfEllipse(ellipse, 0, math.pi))
        self.sketch.addConstraint(Sketcher.Constraint("Coincident", lower, 3, upper, 3))
        axis = next(
            c.First
            for c in self.sketch.Constraints
            if c.Type == "InternalAlignment"
            and self.sketch.Geometry[c.First].TypeId == "Part::GeomLineSegment"
        )
        self.sketch.addConstraint(Sketcher.Constraint("Distance", axis, 20))
        types_before = self.geometry_types()

        self.convert(f"Edge{lower + 1}")

        types_after = self.geometry_types()
        self.assertEqual(types_after[lower], "Part::GeomBSplineCurve")
        self.assertEqual(types_after[upper], "Part::GeomArcOfEllipse")
        # The axes and foci survive, now owned by the other arc.
        self.assertEqual(
            types_after.count("Part::GeomLineSegment"), types_before.count("Part::GeomLineSegment")
        )
        alignment = [
            c
            for c in self.sketch.Constraints
            if c.Type == "InternalAlignment" and c.Second != lower
        ]
        self.assertEqual(len(alignment), 4)
        self.assertTrue(all(c.Second == upper for c in alignment))
        self.assertTrue(
            any(c.Type == "Distance" and c.First == axis for c in self.sketch.Constraints)
        )
