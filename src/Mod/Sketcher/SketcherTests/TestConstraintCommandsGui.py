# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

import FreeCAD as App
import Part
import Sketcher
from PySide import QtCore
from SketcherTests.GuiTestCase import FreeCADGui as Gui, SketcherGuiTestCase


class TestConstraintCommandsGui(SketcherGuiTestCase):
    def setUp(self):
        super().setUp()
        Gui.activateWorkbench("SketcherWorkbench")
        self.params = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Sketcher")
        self.saved_params = {
            key: self.params.GetBool(key, True)
            for key in ("ContinuousConstraintMode", "ShowDialogOnDistanceConstraint")
        }
        self.params.SetBool("ContinuousConstraintMode", True)
        self.params.SetBool("ShowDialogOnDistanceConstraint", False)
        self.doc = App.newDocument("ConstraintCommands")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.sketch.addGeometry(
            Part.LineSegment(App.Vector(50, 45, 0), App.Vector(10, 15, 0)), False
        )
        self.doc.recompute()
        Gui.activeDocument().setEdit(self.sketch.Name)
        self.flush_gui(50)
        self.view = Gui.activeDocument().activeView()
        self.view.viewTop()
        self.flush_gui(50)
        self.view.fitAll()
        self.flush_gui(150)
        self.viewport = self.view.graphicsView().viewport()

    def tearDown(self):
        try:
            super().tearDown()
        finally:
            for key, value in getattr(self, "saved_params", {}).items():
                self.params.SetBool(key, value)

    def select(self, *names):
        Gui.Selection.clearSelection()
        for name in names:
            Gui.Selection.addSelection(self.sketch, name)

    def assert_distance(self, axis, value, driving=True):
        self.assertEqual(self.sketch.ConstraintCount, 1)
        constraint = self.sketch.Constraints[0]
        self.assertEqual(constraint.Type, "Distance" + axis)
        self.assertAlmostEqual(constraint.Value, value, places=6)
        self.assertEqual(self.sketch.getDriving(0), driving)
        self.assertEqual(self.sketch.solve(), 0)
        self.assertEqual(Gui.Selection.getSelectionEx(), [])
        self.doc.undo()
        self.assertEqual(self.sketch.ConstraintCount, 0)
        self.doc.redo()
        self.assertEqual(self.sketch.ConstraintCount, 1)
        self.assertEqual(self.sketch.getDriving(0), driving)
        self.doc.undo()

    def click_world(self, point):
        pos = self.viewport_to_qpoint(self.view, self.viewport, self.view.getPointOnScreen(point))
        self.move(self.viewport, pos)
        self.click(self.viewport, pos)

    def test_coordinate_distances_from_selection(self):
        for axis, value in (("X", 40), ("Y", 30)):
            for selection in (("Edge1",), ("Vertex1", "Vertex2")):
                with self.subTest(axis=axis, selection=selection):
                    self.select(*selection)
                    Gui.runCommand("Sketcher_ConstrainDistance" + axis)
                    self.assert_distance(axis, value)

    def test_coordinate_distances_from_continuous_picking(self):
        for axis, value in (("X", 40), ("Y", 30)):
            for points in (
                (App.Vector(30, 30, 0),),
                (App.Vector(50, 45, 0), App.Vector(10, 15, 0)),
            ):
                with self.subTest(axis=axis, points=points):
                    Gui.Selection.clearSelection()
                    Gui.runCommand("Sketcher_ConstrainDistance" + axis)
                    for point in points:
                        self.click_world(point)
                    self.assert_distance(axis, value)

    def test_reference_coordinate_distances(self):
        Gui.runCommand("Sketcher_ToggleDrivingConstraint")
        try:
            for axis, value in (("X", 40), ("Y", 30)):
                with self.subTest(axis=axis):
                    self.select("Edge1")
                    Gui.runCommand("Sketcher_ConstrainDistance" + axis)
                    self.assert_distance(axis, value, driving=False)
        finally:
            Gui.runCommand("Sketcher_ToggleDrivingConstraint")

    def test_fixed_geometry_produces_reference_datum(self):
        self.sketch.addConstraint(Sketcher.Constraint("Block", 0))
        self.select("Edge1")
        Gui.runCommand("Sketcher_ConstrainDistanceX")
        self.assertEqual(self.sketch.ConstraintCount, 2)
        self.assertFalse(self.sketch.getDriving(1))
        self.assertEqual(self.sketch.solve(), 0)

    def test_single_vertex_coordinates(self):
        for axis, value in (("X", 50), ("Y", 45)):
            with self.subTest(axis=axis):
                self.select("Vertex1")
                Gui.runCommand("Sketcher_ConstrainDistance" + axis)
                self.assert_distance(axis, value)

    def test_fixed_vertex_coordinates_are_reference(self):
        self.sketch.addConstraint(Sketcher.Constraint("Block", 0))
        for axis, value in (("X", 50), ("Y", 45)):
            with self.subTest(axis=axis):
                self.select("Vertex1")
                Gui.runCommand("Sketcher_ConstrainDistance" + axis)
                self.assertEqual(self.sketch.ConstraintCount, 2)
                self.assertEqual(self.sketch.Constraints[1].Type, "Distance" + axis)
                self.assertAlmostEqual(self.sketch.Constraints[1].Value, value)
                self.assertFalse(self.sketch.getDriving(1))
                self.assertEqual(self.sketch.solve(), 0)
                self.doc.undo()
                self.assertEqual(self.sketch.ConstraintCount, 1)

    def test_coordinate_distances_to_axes(self):
        for axis, name, value in (("X", "V_Axis", 50), ("Y", "H_Axis", 45)):
            for selection in ((name, "Vertex1"), ("Vertex1", name)):
                with self.subTest(axis=axis, selection=selection):
                    self.select(*selection)
                    Gui.runCommand("Sketcher_ConstrainDistance" + axis)
                    self.assert_distance(axis, value)

    def test_external_vertex_coordinates(self):
        source = self.doc.addObject("Part::Feature", "ExternalLine")
        source.Shape = Part.makeLine(App.Vector(-20, -25, 0), App.Vector(-10, -15, 0))
        self.doc.recompute()
        self.sketch.addExternal(source.Name, "Edge1")
        self.doc.recompute()
        self.assertEqual(self.sketch.getGeoVertexIndex(2)[0], -3)
        for axis, value in (("X", -20), ("Y", -25)):
            with self.subTest(axis=axis):
                self.select("Vertex3")
                Gui.runCommand("Sketcher_ConstrainDistance" + axis)
                self.assert_distance(axis, value, driving=False)
                # One external point must not force a movable point's distance to be reference.
                for selection in (("Vertex3", "Vertex1"), ("Vertex1", "Vertex3")):
                    self.select(*selection)
                    Gui.runCommand("Sketcher_ConstrainDistance" + axis)
                    self.assert_distance(axis, 70, driving=True)

    def add_external_source(self):
        source = self.doc.addObject("Part::Feature", "ExternalLine")
        source.Shape = Part.makeLine(App.Vector(-20, -25, 0), App.Vector(-10, -15, 0))
        self.doc.recompute()
        self.doc.clearUndos()
        return source

    def pick_external(self, source, name):
        Gui.Selection.addSelection(self.doc.Name, source.Name, name)
        self.flush_gui()

    def external_count(self):
        return sum(len(names) for _, names in self.sketch.ExternalGeometry)

    def test_cancelling_removes_picked_external_geometry(self):
        source = self.add_external_source()
        cancels = {
            "escape": lambda: self.key_click(self.viewport, QtCore.Qt.Key_Escape),
            "right click": lambda: self.right_click(self.viewport, self.viewport.rect().center()),
            "blank click": lambda: self.click_world(App.Vector(40, -20, 0)),
        }
        for name in ("DistanceX", "Coincident", "Horizontal", "Symmetric"):
            for cancel, action in cancels.items():
                with self.subTest(command=name, cancel=cancel):
                    Gui.Selection.clearSelection()
                    Gui.runCommand("Sketcher_Constrain" + name)
                    self.pick_external(source, "Vertex1")
                    self.assertEqual(self.external_count(), 1)
                    action()
                    self.assertEqual(self.external_count(), 0)
                    self.assertEqual(self.doc.UndoCount, 0)

    def test_cancelling_removes_refused_external_geometry(self):
        notifications = App.ParamGet("User parameter:BaseApp/Preferences/NotificationArea")
        saved = notifications.GetBool("NonIntrusiveNotificationsEnabled", True)
        notifications.SetBool("NonIntrusiveNotificationsEnabled", True)
        try:
            source = self.add_external_source()
            Gui.Selection.clearSelection()
            Gui.runCommand("Sketcher_ConstrainCoincident")
            self.pick_external(source, "Vertex1")
            self.pick_external(source, "Vertex2")
            self.assertEqual(self.external_count(), 2)
            self.assertEqual(self.sketch.ConstraintCount, 0)
            self.key_click(self.viewport, QtCore.Qt.Key_Escape)
            self.assertEqual(self.external_count(), 0)
            self.assertEqual(self.doc.UndoCount, 0)
        finally:
            notifications.SetBool("NonIntrusiveNotificationsEnabled", saved)

    def test_constraint_keeps_picked_external_geometry(self):
        source = self.add_external_source()
        Gui.Selection.clearSelection()
        Gui.runCommand("Sketcher_ConstrainDistanceX")
        self.pick_external(source, "Vertex1")
        self.click_world(App.Vector(50, 45, 0))
        self.assertEqual(self.sketch.ConstraintCount, 1)
        self.assertEqual(self.external_count(), 1)
        self.pick_external(source, "Vertex2")
        self.assertEqual(self.external_count(), 2)
        self.key_click(self.viewport, QtCore.Qt.Key_Escape)
        self.assertEqual(self.sketch.ConstraintCount, 1)
        self.assertEqual(self.external_count(), 1)
        self.assertEqual(self.doc.UndoCount, 2)
        self.doc.undo()
        self.assertEqual(self.sketch.ConstraintCount, 0)
        self.assertEqual(self.external_count(), 1)
        self.doc.undo()
        self.assertEqual(self.external_count(), 0)

    def test_switching_command_drops_picked_external_geometry(self):
        source = self.add_external_source()
        for first in ("DistanceX", "Coincident"):
            for second in ("Horizontal", "DistanceY", "Coincident", "Distance", "Parallel"):
                with self.subTest(first=first, second=second):
                    Gui.Selection.clearSelection()
                    Gui.runCommand("Sketcher_Constrain" + first)
                    self.pick_external(source, "Vertex1")
                    self.assertEqual(self.external_count(), 1)
                    Gui.runCommand("Sketcher_Constrain" + second)
                    self.flush_gui()
                    self.assertEqual(self.external_count(), 0)
                    self.assertEqual(self.sketch.ConstraintCount, 0)
                    self.key_click(self.viewport, QtCore.Qt.Key_Escape)

    def test_radial_dimensions(self):
        circle = Part.Circle(App.Vector(0, 0, 0), App.Vector(0, 0, 1), 10)
        self.sketch.addGeometry(circle, False)
        self.sketch.addGeometry(Part.ArcOfCircle(circle, 0.2, 2.0), False)
        self.doc.recompute()
        for command in ("Radius", "Diameter", "Radiam"):
            for edge, kind, value in (
                (
                    "Edge2",
                    "Radius" if command == "Radius" else "Diameter",
                    10 if command == "Radius" else 20,
                ),
                (
                    "Edge3",
                    "Diameter" if command == "Diameter" else "Radius",
                    20 if command == "Diameter" else 10,
                ),
            ):
                for reference in (False, True):
                    with self.subTest(command=command, edge=edge, reference=reference):
                        if reference:
                            Gui.runCommand("Sketcher_ToggleDrivingConstraint")
                        try:
                            self.select(edge)
                            Gui.runCommand("Sketcher_Constrain" + command)
                            self.assertEqual(self.sketch.ConstraintCount, 1)
                            self.assertEqual(self.sketch.Constraints[0].Type, kind)
                            self.assertAlmostEqual(self.sketch.Constraints[0].Value, value)
                            self.assertEqual(self.sketch.getDriving(0), not reference)
                            self.assertEqual(self.sketch.solve(), 0)
                            self.doc.undo()
                            self.assertEqual(self.sketch.ConstraintCount, 0)
                            self.doc.redo()
                            self.assertEqual(self.sketch.Constraints[0].Type, kind)
                            self.doc.undo()
                        finally:
                            if reference:
                                Gui.runCommand("Sketcher_ToggleDrivingConstraint")

    def test_radial_dimensions_from_continuous_picking(self):
        self.sketch.addGeometry(Part.Circle(App.Vector(0, 0, 0), App.Vector(0, 0, 1), 10), False)
        self.doc.recompute()
        self.view.fitAll()
        self.flush_gui(150)
        for command in ("Radius", "Diameter", "Radiam"):
            with self.subTest(command=command):
                Gui.Selection.clearSelection()
                Gui.runCommand("Sketcher_Constrain" + command)
                # Pick away from the sketch axes so the circle is unambiguous.
                self.click_world(App.Vector(-8, 6, 0))
                self.assertEqual(self.sketch.ConstraintCount, 1)
                expected = "Radius" if command == "Radius" else "Diameter"
                self.assertEqual(self.sketch.Constraints[0].Type, expected)
                self.assertAlmostEqual(
                    self.sketch.Constraints[0].Value, 10 if command == "Radius" else 20
                )
                self.assertTrue(self.sketch.getDriving(0))
                self.assertEqual(self.sketch.solve(), 0)
                self.doc.undo()
                self.assertEqual(self.sketch.ConstraintCount, 0)

    def test_multiple_radial_dimensions(self):
        circle = Part.Circle(App.Vector(0, 0, 0), App.Vector(0, 0, 1), 10)
        self.sketch.addGeometry(circle, False)
        self.sketch.addGeometry(Part.ArcOfCircle(circle, 0.2, 2.0), False)
        self.doc.recompute()
        for command in ("Radius", "Diameter", "Radiam"):
            for edges in (("Edge2", "Edge3"), ("Edge3", "Edge2")):
                for reference in (False, True):
                    with self.subTest(command=command, edges=edges, reference=reference):
                        if reference:
                            Gui.runCommand("Sketcher_ToggleDrivingConstraint")
                        try:
                            self.select(*edges)
                            Gui.runCommand("Sketcher_Constrain" + command)
                            expected = [
                                (
                                    "Diameter"
                                    if command == "Diameter"
                                    or (command == "Radiam" and edge == "Edge2")
                                    else "Radius"
                                )
                                for edge in edges
                            ]
                            if not reference:
                                expected = ["Equal", expected[0]]
                            self.assertEqual([c.Type for c in self.sketch.Constraints], expected)
                            self.assertEqual(self.sketch.solve(), 0)
                            for index in (range(2) if reference else (1,)):
                                self.assertEqual(self.sketch.getDriving(index), not reference)
                            self.doc.undo()
                            self.assertEqual(self.sketch.ConstraintCount, 0)
                        finally:
                            if reference:
                                Gui.runCommand("Sketcher_ToggleDrivingConstraint")

    def test_fixed_and_external_radial_dimensions(self):
        circle = Part.Circle(App.Vector(0, 0, 0), App.Vector(0, 0, 1), 10)
        self.sketch.addGeometry(circle, False)
        self.sketch.addConstraint(Sketcher.Constraint("Block", 1))
        source = self.doc.addObject("Part::Feature", "ExternalCircle")
        source.Shape = circle.toShape()
        self.doc.recompute()
        self.sketch.addExternal(source.Name, "Edge1")
        self.doc.recompute()
        for command in ("Radius", "Diameter", "Radiam"):
            for edge in ("Edge2", "ExternalEdge1"):
                with self.subTest(command=command, edge=edge):
                    self.select(edge)
                    Gui.runCommand("Sketcher_Constrain" + command)
                    self.assertEqual(self.sketch.ConstraintCount, 2)
                    expected = "Radius" if command == "Radius" else "Diameter"
                    self.assertEqual(self.sketch.Constraints[1].Type, expected)
                    self.assertFalse(self.sketch.getDriving(1))
                    self.assertEqual(self.sketch.solve(), 0)
                    self.doc.undo()
                    self.assertEqual(self.sketch.ConstraintCount, 1)

    def test_radial_dimensions_on_bspline_weights(self):
        spline = Part.BSplineCurve()
        spline.interpolate([App.Vector(0, 0, 0), App.Vector(10, 20, 0), App.Vector(30, 0, 0)])
        self.sketch.addGeometry(spline, False)
        self.sketch.exposeInternalGeometry(1)
        self.doc.recompute()
        pole = next(
            index for index, geo in enumerate(self.sketch.Geometry) if isinstance(geo, Part.Circle)
        )
        # Exposing a non-rational spline fixes its first weight automatically.
        weight = next(
            index
            for index, constraint in enumerate(self.sketch.Constraints)
            if constraint.Type == "Weight" and constraint.First == pole
        )
        self.sketch.delConstraint(weight)
        count = self.sketch.ConstraintCount
        for command in ("Radius", "Radiam"):
            with self.subTest(command=command):
                self.select("Edge" + str(pole + 1))
                Gui.runCommand("Sketcher_Constrain" + command)
                self.assertEqual(self.sketch.ConstraintCount, count + 1)
                self.assertEqual(self.sketch.Constraints[-1].Type, "Weight")
                self.assertTrue(self.sketch.getDriving(count))
                self.assertEqual(self.sketch.solve(), 0)
                self.doc.undo()
                self.assertEqual(self.sketch.ConstraintCount, count)

    def test_disabled_continuous_mode_releases_previous_handler(self):
        notifications = App.ParamGet("User parameter:BaseApp/Preferences/NotificationArea")
        saved = notifications.GetBool("NonIntrusiveNotificationsEnabled", True)
        notifications.SetBool("NonIntrusiveNotificationsEnabled", True)
        try:
            for name in ("DistanceX", "Perpendicular", "Tangent", "Equal", "Symmetric"):
                with self.subTest(command=name):
                    self.params.SetBool("ContinuousConstraintMode", True)
                    Gui.Selection.clearSelection()
                    Gui.runCommand("Sketcher_ConstrainDistanceX")
                    self.params.SetBool("ContinuousConstraintMode", False)
                    Gui.runCommand("Sketcher_Constrain" + name)
                    self.flush_gui()
                    self.assertEqual(self.sketch.ConstraintCount, 0)
                    self.assertEqual(self.viewport.cursor().shape(), QtCore.Qt.ArrowCursor)
        finally:
            notifications.SetBool("NonIntrusiveNotificationsEnabled", saved)

    def test_switching_continuous_commands_preserves_sketch(self):
        for name in (
            "Horizontal",
            "Vertical",
            "HorVer",
            "Lock",
            "Block",
            "Coincident",
            "PointOnObject",
            "Distance",
            "DistanceX",
            "DistanceY",
            "Parallel",
            "Perpendicular",
            "Tangent",
            "Radius",
            "Diameter",
            "Radiam",
            "Angle",
            "Equal",
            "Symmetric",
        ):
            with self.subTest(command=name):
                Gui.Selection.clearSelection()
                Gui.runCommand("Sketcher_Constrain" + name)
                self.flush_gui()
                self.assertEqual(self.sketch.ConstraintCount, 0)
                self.assertIsNotNone(Gui.activeDocument().getInEdit())
                self.assertEqual(self.viewport.cursor().shape(), QtCore.Qt.BitmapCursor)
