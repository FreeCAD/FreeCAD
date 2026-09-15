# SPDX-License-Identifier: LGPL-2.1-or-later

import os

import FreeCAD as App
import Part
import Sketcher
from PySide import QtCore, QtGui
from SketcherTests.GuiTestCase import FreeCADGui as Gui, SketcherGuiTestCase


class TestSketchGroupsGui(SketcherGuiTestCase):
    def testGroupOnlySelectedGeometry(self):
        Gui.activateWorkbench("SketcherWorkbench")
        self.doc = App.newDocument("GroupSelection")
        s = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        for x in (0, 10, 20):
            s.addGeometry(Part.Circle(App.Vector(x, 0, 0), App.Vector(0, 0, 1), 2))
        self.doc.recompute()
        Gui.activeDocument().setEdit(s.Name)
        Gui.Selection.clearSelection()
        for edge in ("Edge1", "Edge2"):
            Gui.Selection.addSelection(s, edge)
        Gui.runCommand("Sketcher_ConstrainGroup")
        self.assertEqual((s.GeometryCount, s.ConstraintCount), (4, 1))
        untouched = s.Geometry[2].Content
        s.addConstraint(Sketcher.Constraint("Distance", 3, 8.0))
        self.assertEqual(s.solve(), 0)
        self.assertAlmostEqual(s.Geometry[0].Radius, 4)
        self.assertAlmostEqual(s.Geometry[1].Radius, 4)
        self.assertEqual(s.Geometry[2].Content, untouched)

    def testGroupTransforms(self):
        Gui.activateWorkbench("SketcherWorkbench")
        for command in ("Rotate", "Translate", "Scale"):
            for pattern in (False, True):
                with self.subTest(command=command, pattern=pattern):
                    self.doc = App.newDocument("GroupTransform")
                    s = self.doc.addObject("Sketcher::SketchObject", "Sketch")
                    s.addGeometry(Part.Circle(App.Vector(12, 14, 0), App.Vector(0, 0, 1), 2))
                    s.addGeometry(Part.Point(App.Vector(16, 18, 0)))
                    for x in (10, 20):
                        s.addGeometry(
                            Part.LineSegment(App.Vector(x, 10, 0), App.Vector(x, 20, 0)), True
                        )
                    s.addConstraint(Sketcher.Constraint("Group", [2, 0, 0, 0, 1, 0]))
                    s.addConstraint(Sketcher.Constraint("Group", [3, 0, 2, 0]))
                    self.doc.recompute()
                    Gui.activeDocument().setEdit(s.Name)
                    view = Gui.activeDocument().activeView()
                    view.setCamera(
                        "#Inventor V2.1 ascii\nOrthographicCamera { position 0 0 100 "
                        "orientation 0 0 1 0 focalDistance 100 height 100 }"
                    )
                    self.flush_gui(150)
                    viewport = view.graphicsView().viewport()
                    # Selecting a leaf must transform its entire outer group, including points.
                    Gui.Selection.clearSelection()
                    Gui.Selection.addSelection(s, "Edge1")
                    Gui.runCommand("Sketcher_" + command)
                    self.flush_gui(150)
                    points = [(0, 0), (20, 0)]
                    if command == "Rotate":
                        points.append((0, 20))
                    elif command == "Scale":
                        points.append((40, 0))
                    for index, (x, y) in enumerate(points):
                        pos = self.viewport_to_qpoint(
                            view, viewport, view.getPointOnScreen(App.Vector(x, y, 0))
                        )
                        self.move(viewport, pos)
                        self.click(viewport, pos)
                        if index == 0 and pattern:
                            self.key_click(viewport, QtCore.Qt.Key_U, "u")
                    self.assertEqual(s.GeometryCount, 8 if pattern else 4)
                    self.assertEqual(
                        sum(c.Type == "Group" for c in s.Constraints), 4 if pattern else 2
                    )
                    self.assertEqual(s.solve(), 0)
                    first = 4 if pattern else 0
                    root, circle, point, child = s.Geometry[first : first + 4]
                    # Derive the transformation from the outer handle: every member must
                    # follow it exactly, independently of mouse pixel rounding or snapping.
                    y_axis = (root.EndPoint - root.StartPoint) / 10
                    x_axis = App.Vector(y_axis.y, -y_axis.x, 0)

                    def transformed_point(x, y):
                        return root.StartPoint + x_axis * (x - 20) + y_axis * (y - 10)

                    for actual, expected in (
                        (circle.Center, transformed_point(12, 14)),
                        (point.toShape().Point, transformed_point(16, 18)),
                        (child.StartPoint, transformed_point(10, 10)),
                        (child.EndPoint, transformed_point(10, 20)),
                    ):
                        self.assertLess((actual - expected).Length, 1e-6)
                    self.assertAlmostEqual(circle.Radius, 2 * y_axis.Length)
                    self.assertGreater((root.StartPoint - App.Vector(20, 10, 0)).Length, 10)
                    transformed = [g.Content for g in s.Geometry]
                    self.doc.undo()
                    self.assertEqual(s.GeometryCount, 4)
                    self.assertEqual(sum(c.Type == "Group" for c in s.Constraints), 2)
                    self.doc.redo()
                    self.assertEqual([g.Content for g in s.Geometry], transformed)
                    self.cleanup_gui_document(self.doc)
                    self.doc = None

    def testNestedTextGroup(self):
        font = os.path.join(App.getResourceDir(), "examples", "osifont-lgpl3fe.ttf")
        if not os.path.isfile(font):
            self.skipTest("Bundled osifont not found")
        Gui.activateWorkbench("SketcherWorkbench")
        self.doc = App.newDocument("NestedTextGroupTest")
        s = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        for x, text in [(0, "A"), (30, "B")]:
            handle = s.addGeometry(
                Part.LineSegment(App.Vector(x, 0, 0), App.Vector(x, 10, 0)), True
            )
            c = s.addConstraint(Sketcher.Constraint("Text", [handle, 0], text, font, True))
            s.setTextAndFont(c, text, font, True, False)
        self.doc.recompute()
        Gui.activeDocument().setEdit(s.Name)

        def select(ids):
            Gui.Selection.clearSelection()
            for geo in ids:
                Gui.Selection.addSelection(self.doc.Name, s.Name, "Edge{}".format(geo + 1))

        before = s.GeometryCount
        select(range(before))
        Gui.runCommand("Sketcher_ConstrainGroup", 0)
        self.assertEqual(s.solve(), 0)
        self.assertEqual((s.GeometryCount, s.ConstraintCount), (before + 1, 3))

        # Ungrouping two text objects must remove only the outer group's handle.
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(s, "Constraint3")
        Gui.runCommand("Std_Delete", 0)
        self.assertEqual((s.GeometryCount, s.ConstraintCount), (before, 2))
        self.assertEqual(sum(c.Type == "Text" for c in s.Constraints), 2)
        self.assertEqual(s.solve(), 0)
        self.doc.undo()
        self.assertEqual((s.GeometryCount, s.ConstraintCount), (before + 1, 3))

        circle = s.addGeometry(Part.Circle(App.Vector(60, 5, 0), App.Vector(0, 0, 1), 2))
        select([0, circle])  # Selecting a text member must select its outer group.
        Gui.runCommand("Sketcher_ConstrainGroup", 0)
        root = s.GeometryCount - 1
        s.addConstraint(Sketcher.Constraint("Distance", root, 20.0))
        self.assertEqual(s.solve(), 0)
        self.assertAlmostEqual(s.Geometry[circle].Radius, 4)

        # Regeneration changes geometry indices and the order of the text constraints.
        s.setTextAndFont(0, "ABC", font, True, False)
        self.assertEqual(s.solve(), 0)
        self.assertEqual(sum(c.Type == "Text" for c in s.Constraints), 2)
        groups = [c for c in s.Constraints if c.Type == "Group"]
        self.assertEqual(len(groups), 2)
        root = groups[-1].First
        counts = (s.GeometryCount, s.ConstraintCount)

        # Pattern copies must retain both levels of groups and editable text constraints.
        for command in ("Rotate", "Translate", "Scale"):
            select([root])
            view = Gui.activeDocument().activeView()
            view.setCamera(
                "#Inventor V2.1 ascii\nOrthographicCamera { position 0 0 100 "
                "orientation 0 0 1 0 focalDistance 100 height 100 }"
            )
            Gui.runCommand("Sketcher_" + command)
            self.flush_gui(150)
            viewport = view.graphicsView().viewport()
            points = [(0, 0), (20, 0)]
            if command == "Rotate":
                points.append((0, 20))
            elif command == "Scale":
                points.append((40, 0))
            for index, (x, y) in enumerate(points):
                pos = self.viewport_to_qpoint(
                    view, viewport, view.getPointOnScreen(App.Vector(x, y, 0))
                )
                self.move(viewport, pos)
                self.click(viewport, pos)
                if index == 0:
                    self.key_click(viewport, QtCore.Qt.Key_U, "u")
            self.assertEqual(s.solve(), 0, command)
            self.assertEqual(s.GeometryCount, 2 * counts[0], command)
            self.assertEqual(sum(c.Type == "Group" for c in s.Constraints), 4, command)
            self.assertEqual(sum(c.Type == "Text" for c in s.Constraints), 4, command)
            self.doc.undo()
            self.assertEqual((s.GeometryCount, s.ConstraintCount), counts)

        clipboard = QtGui.QApplication.clipboard()
        original = clipboard.mimeData()
        saved = {fmt: original.data(fmt) for fmt in original.formats()} if original else {}
        try:
            select([root])
            Gui.runCommand("Sketcher_CopyClipboard", 0)
            Gui.runCommand("Sketcher_Paste", 0)
            self.assertEqual(s.solve(), 0)
            self.assertEqual((s.GeometryCount, s.ConstraintCount), tuple(2 * n for n in counts))
            self.doc.undo()
            self.assertEqual((s.GeometryCount, s.ConstraintCount), counts)
        finally:
            restored = QtCore.QMimeData()
            for fmt, data in saved.items():
                restored.setData(fmt, data)
            clipboard.setMimeData(restored)

        select([root])
        Gui.runCommand("Std_Delete", 0)
        self.assertEqual((s.GeometryCount, s.ConstraintCount), (0, 0))
        self.doc.undo()
        self.assertEqual(s.solve(), 0)
        self.assertEqual((s.GeometryCount, s.ConstraintCount), counts)
