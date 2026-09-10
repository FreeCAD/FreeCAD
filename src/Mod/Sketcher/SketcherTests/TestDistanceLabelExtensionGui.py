# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD
import Part
import Sketcher
from pivy import coin

from SketcherTests.GuiTestCase import FreeCADGui, SketcherGuiTestCase


class TestDistanceLabelExtensionGui(SketcherGuiTestCase):
    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("TestDistanceLabelExtensionGui")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")

    def extension_points(self):
        self.doc.recompute()
        FreeCADGui.ActiveDocument.setEdit(self.sketch.Name)
        self.flush_gui(100)

        render_manager = FreeCADGui.ActiveDocument.ActiveView.getViewer().getSoRenderManager()
        scene_root = render_manager.getSceneGraph()
        search = coin.SoSearchAction()
        search.setType(coin.SoType.fromName("SoDatumLabel"))
        search.setInterest(coin.SoSearchAction.ALL)
        search.setSearchingAll(True)
        search.apply(scene_root)

        points = []
        paths = search.getPaths()
        for index in range(paths.getLength()):
            field = paths[index].getTail().getField("extensionLines")
            points.extend(tuple(value.getValue()) for value in field.getValues())
        return points

    def add_point_to_line_distance(self, line_start, line_end, point):
        point_geometry = self.sketch.addGeometry(
            Part.LineSegment(point, point + FreeCAD.Vector(1, 0, 0)),
            False,
        )
        line_geometry = self.sketch.addGeometry(Part.LineSegment(line_start, line_end), False)
        constraint = self.sketch.addConstraint(
            Sketcher.Constraint("Distance", point_geometry, 1, line_geometry, 5.0)
        )
        self.sketch.setDriving(constraint, False)

    def assert_extension_points(self, expected):
        actual = self.extension_points()
        self.assertEqual(len(actual), len(expected))
        for actual_point, expected_point in zip(actual, expected):
            for actual_coordinate, expected_coordinate in zip(actual_point, expected_point):
                self.assertAlmostEqual(actual_coordinate, expected_coordinate, places=6)

    def test_projection_inside_segment_has_no_extension(self):
        self.add_point_to_line_distance(
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Vector(0, 10, 0),
            FreeCAD.Vector(-5, 5, 0),
        )
        self.assert_extension_points([])

    def test_projection_at_endpoint_with_roundoff_has_no_extension(self):
        self.add_point_to_line_distance(
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Vector(0, 10, 0),
            FreeCAD.Vector(-5, 10 + 1e-14, 0),
        )
        self.assert_extension_points([])

    def test_projection_before_segment_connects_to_start(self):
        self.add_point_to_line_distance(
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Vector(0, 10, 0),
            FreeCAD.Vector(-5, -5, 0),
        )
        self.assert_extension_points([(0, 0), (0, -5)])

    def test_projection_after_segment_connects_to_end(self):
        self.add_point_to_line_distance(
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Vector(0, 10, 0),
            FreeCAD.Vector(-5, 15, 0),
        )
        self.assert_extension_points([(0, 10), (0, 15)])

    def test_reversed_segment_connects_to_nearest_endpoint(self):
        self.add_point_to_line_distance(
            FreeCAD.Vector(0, 10, 0),
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Vector(-5, 15, 0),
        )
        self.assert_extension_points([(0, 10), (0, 15)])

    def test_circle_to_line_projection_gets_extension(self):
        circle = self.sketch.addGeometry(
            Part.Circle(FreeCAD.Vector(-5, 15, 0), FreeCAD.Vector(0, 0, 1), 1),
            False,
        )
        line = self.sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 10, 0)),
            False,
        )
        constraint = self.sketch.addConstraint(Sketcher.Constraint("Distance", circle, line, 4.0))
        self.sketch.setDriving(constraint, False)

        self.assert_extension_points([(0, 10), (0, 15)])
