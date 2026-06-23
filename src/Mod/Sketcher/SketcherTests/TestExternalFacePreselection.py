# SPDX-License-Identifier: LGPL-2.1-or-later
# --------------------------------------------------------------------------
#                                                                          *
#    Copyright (c) 2026 Chris Jones github.com/ipatch                      *
#                                                                          *
#    This file is part of FreeCAD.                                         *
#                                                                          *
#    FreeCAD is free software: you can redistribute it and/or modify it    *
#    under the terms of the GNU Lesser General Public License as           *
#    published by the Free Software Foundation, either version 2.1 of the  *
#    License, or (at your option) any later version.                       *
#                                                                          *
#    FreeCAD is distributed in the hope that it will be useful, but        *
#    WITHOUT ANY WARRANTY; without even the implied warranty of            *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
#    Lesser General Public License for more details.                       *
#                                                                          *
#    You should have received a copy of the GNU Lesser General Public      *
#    License along with FreeCAD. If not, see                               *
#    <https://www.gnu.org/licenses/>.                                      *
#                                                                          *
# --------------------------------------------------------------------------

"""Regression test for issue #28639: face preselection under External Geometry tool.

Commit b1fdf659d8 added SoDepthBuffer nodes as direct children of
Sketch_EditRoot to render constraint icons on top of geometry. This
leaked depth-buffer state into the highlight render pass and prevented
face highlights on external geometry from rendering.

The fix replaces SoDepthBuffer with SoAnnotation, which renders its
children on top without affecting depth state for other nodes.
"""

import unittest

import FreeCAD
import Part
import Sketcher
from SketcherTests.GuiTestCase import SketcherGuiTestCase

try:
    import FreeCADGui

    GUI_AVAILABLE = FreeCADGui.getMainWindow() is not None
except (ImportError, AttributeError):
    GUI_AVAILABLE = False

if GUI_AVAILABLE:
    from PySide import QtCore, QtGui


class TestExternalFacePreselection(SketcherGuiTestCase):

    def setUp(self):
        super().setUp()

        FreeCADGui.activateWorkbench("PartDesignWorkbench")
        self.doc = FreeCAD.newDocument("TestExtFacePresel")

        body = self.doc.addObject("PartDesign::Body", "Body")

        sketch = body.newObject("Sketcher::SketchObject", "BaseSketch")
        sketch.AttachmentSupport = [(body.Origin.OriginFeatures[3], "")]
        sketch.MapMode = "FlatFace"

        # 40x40 rectangle
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(-20, -20, 0), FreeCAD.Vector(20, -20, 0))
        )
        sketch.addGeometry(Part.LineSegment(FreeCAD.Vector(20, -20, 0), FreeCAD.Vector(20, 20, 0)))
        sketch.addGeometry(Part.LineSegment(FreeCAD.Vector(20, 20, 0), FreeCAD.Vector(-20, 20, 0)))
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(-20, 20, 0), FreeCAD.Vector(-20, -20, 0))
        )
        sketch.addConstraint(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        self.doc.recompute()

        pad = body.newObject("PartDesign::Pad", "Pad")
        pad.Profile = sketch
        pad.Length = 20.0
        self.doc.recompute()

        # Second sketch on top face for external geometry
        self.testSketch = body.newObject("Sketcher::SketchObject", "TestSketch")
        self.testSketch.AttachmentSupport = [(pad, "Face6")]
        self.testSketch.MapMode = "FlatFace"
        self.doc.recompute()

        self.body = body
        self.pad = pad

    def hover_for_preselection(self, viewport, center_pos, span=6, step=2):
        for dy in range(-span, span + 1, step):
            for dx in range(-span, span + 1, step):
                pos = QtCore.QPoint(center_pos.x() + dx, center_pos.y() + dy)
                event = QtGui.QMouseEvent(
                    QtCore.QEvent.MouseMove,
                    pos,
                    viewport.mapToGlobal(pos),
                    QtCore.Qt.NoButton,
                    QtCore.Qt.NoButton,
                    QtCore.Qt.NoModifier,
                )
                QtGui.QApplication.sendEvent(viewport, event)
                self.pump(100)

                presel = FreeCADGui.Selection.getPreselection()
                if presel.ObjectName:
                    return presel

        return FreeCADGui.Selection.getPreselection()

    @unittest.skipIf(not GUI_AVAILABLE, "GUI not available")
    def testNoDepthBufferInEditRoot(self):
        """SoDepthBuffer nodes must not be direct children of
        Sketch_EditRoot. Bare SoDepthBuffer nodes leak depth state
        to sibling nodes during the highlight render pass, which
        breaks face preselection for external geometry (#28639)."""

        from pivy import coin

        FreeCADGui.ActiveDocument.setEdit(self.testSketch.Name)
        self.pump(300)

        view = FreeCADGui.ActiveDocument.ActiveView
        viewer = view.getViewer()
        scene_root = viewer.getSoRenderManager().getSceneGraph()

        # Find all SoDepthBuffer nodes in the scene graph
        sa = coin.SoSearchAction()
        sa.setType(coin.SoDepthBuffer.getClassTypeId())
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.setSearchingAll(True)
        sa.apply(scene_root)
        paths = sa.getPaths()
        num_paths = paths.getLength()

        bad_nodes = []
        for i in range(num_paths):
            p = paths[i]
            parent = p.getNodeFromTail(1)
            parent_name = parent.getName().getString() if parent.getName() else ""
            if parent_name == "Sketch_EditRoot":
                bad_nodes.append(i)

        self.assertEqual(
            len(bad_nodes),
            0,
            f"Found {len(bad_nodes)} SoDepthBuffer node(s) as direct "
            f"children of Sketch_EditRoot. Use SoAnnotation instead "
            f"to render constraint icons on top without leaking depth "
            f"state (#28639).",
        )

    @unittest.skipIf(not GUI_AVAILABLE, "GUI not available")
    def testConstraintGroupInsideAnnotation(self):
        """The ConstraintGroup node must be inside an SoAnnotation so
        constraint icons render on top of geometry without affecting
        depth state for other scene graph nodes (#28639)."""

        from pivy import coin

        FreeCADGui.ActiveDocument.setEdit(self.testSketch.Name)
        self.pump(300)

        view = FreeCADGui.ActiveDocument.ActiveView
        viewer = view.getViewer()
        scene_root = viewer.getSoRenderManager().getSceneGraph()

        # Find the ConstraintGroup node by name
        sa = coin.SoSearchAction()
        sa.setName("ConstraintGroup")
        sa.setSearchingAll(True)
        sa.apply(scene_root)
        path = sa.getPath()

        self.assertIsNotNone(path, "Could not find 'ConstraintGroup' node in scene graph")

        # Walk up the path looking for an SoAnnotation ancestor
        found_annotation = False
        for i in range(path.getLength()):
            node = path.getNode(i)
            if node.isOfType(coin.SoAnnotation.getClassTypeId()):
                found_annotation = True
                break

        self.assertTrue(
            found_annotation,
            "ConstraintGroup is not inside an SoAnnotation. Without "
            "SoAnnotation, constraint icon rendering can interfere "
            "with face highlight rendering for external geometry "
            "preselection (#28639).",
        )

    @unittest.skipIf(not GUI_AVAILABLE, "GUI not available")
    def testFacePickableUnderExternalTool(self):
        """A face on the Pad must be preselectable via mouse hover
        while the External Geometry tool is active (#28639)."""

        FreeCADGui.ActiveDocument.setEdit(self.testSketch.Name)
        self.pump(300)

        # Add geometry + constraints so constraint icons are rendered.
        self.testSketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(-10, -10, 0), FreeCAD.Vector(10, -10, 0))
        )
        self.testSketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(10, -10, 0), FreeCAD.Vector(10, 10, 0))
        )
        self.testSketch.addConstraint(Sketcher.Constraint("DistanceX", 0, 1, 0, 2, 20.0))
        self.testSketch.addConstraint(Sketcher.Constraint("DistanceY", 1, 1, 1, 2, 20.0))
        self.doc.recompute()
        self.pump(300)

        view = FreeCADGui.ActiveDocument.ActiveView
        view.viewFront()
        view.fitAll()
        self.pump(300)

        face_center_3d = FreeCAD.Vector(0, -20, 10)

        def face_center_is_framed():
            width, height = view.getSize()
            if width <= 0 or height <= 0:
                return False
            screen_x, screen_y = view.getPointOnScreen(face_center_3d)
            margin_x = width * 0.1
            margin_y = height * 0.1
            return (
                margin_x < screen_x < width - margin_x and margin_y < screen_y < height - margin_y
            )

        self.assertTrue(
            self.wait_until(face_center_is_framed, timeout_ms=5000, step_ms=100),
            "Camera never framed the Pad face; getPointOnScreen stayed at the "
            "viewport edge, so no valid interior hover point could be computed.",
        )

        # Activate External Geometry tool
        FreeCADGui.runCommand("Sketcher_Projection", 0)
        self.pump(300)

        # Simulate mouse hover over the front face center
        viewport = view.graphicsView().viewport()
        screen_pt = view.getPointOnScreen(face_center_3d)
        hover_pos = self.viewport_to_qpoint(view, viewport, screen_pt)
        presel = self.hover_for_preselection(viewport, hover_pos)
        self.assertNotEqual(
            presel.ObjectName,
            "",
            "No object was preselected — mouse hover did not produce a "
            f"pick near viewport point {hover_pos}. This may indicate "
            "the #28639 regression.",
        )
        sub_names = presel.SubElementNames
        self.assertTrue(
            any("Face" in s for s in sub_names),
            f"Expected a Face preselection but got " f"SubElementNames={sub_names}.",
        )
