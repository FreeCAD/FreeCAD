# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD
import Part
import Sketcher
import SketcherGui
from PySide import QtCore, QtGui
from SketcherTests.GuiTestCase import FreeCADGui, SketcherGuiTestCase


class TestSketchDragPreselectionGui(SketcherGuiTestCase):
    def setUp(self):
        super().setUp()

        FreeCADGui.activateWorkbench("SketcherWorkbench")
        self.doc = FreeCAD.newDocument("TestSketchDragPreselectionGui")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.target_edge = self.build_repro_sketch(self.sketch)
        self.doc.recompute()

        FreeCADGui.ActiveDocument.setEdit(self.sketch.Name)
        self.pump(150)

        self.view = FreeCADGui.ActiveDocument.ActiveView
        self.viewport = self.view.graphicsView().viewport()
        self.view.viewFront()
        self.view.fitAll()
        self.viewport.setFocus(QtCore.Qt.OtherFocusReason)
        self.pump(150)

    @staticmethod
    def build_repro_sketch(sketch):
        # Reduced geometry from issue #30606: a shallow top step where the middle edge
        # is dragged in an attached, rotated sketch.
        segments = [
            ((-40.0, -30.0), (40.0, -30.0)),
            ((40.0, -30.0), (40.0, 0.0)),
            ((-40.0, 0.0), (-40.0, -30.0)),
            ((-40.0, 0.0), (-20.0, 0.0)),
            ((-20.0, 0.0), (-20.0, 0.5)),
            ((-20.0, 0.5), (20.0, 0.5)),
            ((20.0, 0.5), (20.0, 0.0)),
            ((20.0, 0.0), (40.0, 0.0)),
        ]

        for start, end in segments:
            sketch.addGeometry(
                Part.LineSegment(
                    FreeCAD.Vector(*start, 0.0),
                    FreeCAD.Vector(*end, 0.0),
                ),
                False,
            )

        sketch.addConstraint(
            [
                Sketcher.Constraint("Coincident", 0, 2, 1, 1),
                Sketcher.Constraint("Coincident", 2, 2, 0, 1),
                Sketcher.Constraint("Vertical", 1),
                Sketcher.Constraint("Vertical", 2),
                Sketcher.Constraint("Coincident", 3, 1, 2, 1),
                Sketcher.Constraint("Horizontal", 3),
                Sketcher.Constraint("Coincident", 3, 2, 4, 1),
                Sketcher.Constraint("Vertical", 4),
                Sketcher.Constraint("Coincident", 4, 2, 5, 1),
                Sketcher.Constraint("Horizontal", 5),
                Sketcher.Constraint("Coincident", 5, 2, 6, 1),
                Sketcher.Constraint("Vertical", 6),
                Sketcher.Constraint("Coincident", 6, 2, 7, 1),
                Sketcher.Constraint("Coincident", 7, 2, 1, 2),
                Sketcher.Constraint("Horizontal", 7),
                Sketcher.Constraint("Equal", 4, 6),
            ]
        )
        sketch.Placement = FreeCAD.Placement(
            FreeCAD.Vector(0.0, 0.0, 0.0),
            FreeCAD.Rotation(FreeCAD.Vector(1.0, 0.0, 0.0), 90.0),
        )
        return 5

    def test_dragged_edge_does_not_snap_back_on_release(self):
        original_midpoint = self.edge_midpoint()
        start_point = self.to_qpoint(self.viewport_point(original_midpoint))
        self.assertTrue(
            self.viewport.rect().contains(start_point),
            f"Expected {start_point} to fall inside the sketch viewport {self.viewport.rect()}",
        )

        self.move(self.viewport, start_point)
        self.assert_preselected_edge(original_midpoint)

        original_screen_point = start_point
        release_point = QtCore.QPoint(start_point.x(), max(10, start_point.y() - 80))

        self.drag_edge(start_point, release_point)

        moved_screen_point = self.to_qpoint(self.viewport_point(self.edge_midpoint()))
        self.assertLess(
            abs(moved_screen_point.y() - release_point.y()),
            abs(original_screen_point.y() - release_point.y()) * 0.75,
            (
                "Expected the committed edge position to stay near the release point. "
                f"original={original_screen_point}, moved={moved_screen_point}, "
                f"release={release_point}"
            ),
        )

    def drag_edge(self, start_point, release_point):
        drag_midpoint = QtCore.QPoint(
            start_point.x(),
            int(round((start_point.y() + release_point.y()) / 2)),
        )

        self.send_drag_mouse(QtCore.QEvent.MouseButtonPress, start_point, QtCore.Qt.LeftButton)
        self.pump(20)
        self.send_drag_mouse(QtCore.QEvent.MouseMove, drag_midpoint, QtCore.Qt.NoButton)
        self.pump(20)
        self.send_drag_mouse(QtCore.QEvent.MouseMove, release_point, QtCore.Qt.NoButton)
        self.pump(20)
        self.send_drag_mouse(
            QtCore.QEvent.MouseButtonRelease,
            release_point,
            QtCore.Qt.LeftButton,
            QtCore.Qt.NoButton,
        )
        self.pump(20)
        self.pump(200)

    def send_drag_mouse(self, event_type, point, button, buttons=QtCore.Qt.LeftButton):
        QtGui.QCursor.setPos(self.viewport.mapToGlobal(point))
        self.send_mouse(self.viewport, event_type, point, button, buttons)

    def edge_midpoint(self):
        edge = self.sketch.Geometry[self.target_edge]
        return (edge.StartPoint + edge.EndPoint) * 0.5

    def viewport_point(self, sketch_point):
        return self.view.getPointOnViewport(self.sketch.Placement.multVec(sketch_point))

    def assert_preselected_edge(self, sketch_point):
        viewport_point = tuple(map(int, self.viewport_point(sketch_point)))
        info = SketcherGui.getActiveSketchPreselection(viewport_point)
        expected_name = f"Edge{self.target_edge + 1}"
        subelement_names = [] if not info else info.get("SubElementNames") or []
        self.assertIn(
            expected_name,
            subelement_names,
            f"Expected {expected_name} preselection, got {info}",
        )

    def device_pixel_ratio(self):
        if hasattr(self.viewport, "devicePixelRatioF"):
            return self.viewport.devicePixelRatioF()
        return float(self.viewport.devicePixelRatio())

    def to_qpoint(self, point):
        _, height = self.view.getSize()
        scale = self.device_pixel_ratio()
        x = int(round(point[0] / scale))
        y = int(round((height - point[1] - 1) / scale))
        return QtCore.QPoint(x, y)
