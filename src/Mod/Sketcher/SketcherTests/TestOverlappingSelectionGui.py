# SPDX-License-Identifier: LGPL-2.1-or-later
# --------------------------------------------------------------------------
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

"""Tests for issue #5648: choosing between overlapping sketch elements.

Preselection resolves everything under the cursor to a single winner, which
makes elements that are fully covered by another one unreachable. The Clarify
Selection command instead offers all of them, which is what
getActiveSketchSelectionCandidates() reports.
"""

import FreeCAD
import Part

from SketcherTests.GuiTestCase import SketcherGuiTestCase, gui_available

if gui_available():
    import FreeCADGui
    import SketcherGui


class TestOverlappingSelectionGui(SketcherGuiTestCase):

    def setUp(self):
        super().setUp()

        self.doc = FreeCAD.newDocument("TestOverlappingSelection")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")

        FreeCADGui.getMainWindow().show()
        self.flush_gui(50)

    def startEditing(self):
        self.doc.recompute()
        FreeCADGui.ActiveDocument.setEdit(self.sketch.Name)
        self.flush_gui(50)

        self.view = FreeCADGui.ActiveDocument.ActiveView
        self.view.viewTop()
        self.flush_gui(50)
        self.view.fitAll()
        self.flush_gui(50)

    def viewportPoint(self, worldPoint):
        return tuple(int(value) for value in self.view.getPointOnViewport(worldPoint))

    def findCandidates(self, worldPoint, predicate, span=6):
        """Scans around a world point and returns the first candidates matching predicate."""
        centre = self.viewportPoint(worldPoint)

        for dy in range(-span, span + 1):
            for dx in range(-span, span + 1):
                position = (centre[0] + dx, centre[1] + dy)
                candidates = SketcherGui.getActiveSketchSelectionCandidates(position)
                if predicate(candidates):
                    return position, candidates

        return None, []

    def testFullyOverlappingEdgesAreBothOffered(self):
        """Two identical lines: preselection can only reach one, clarify reaches both."""
        start = FreeCAD.Vector(-20, 0, 0)
        end = FreeCAD.Vector(20, 0, 0)
        self.sketch.addGeometry(Part.LineSegment(start, end), False)
        self.sketch.addGeometry(Part.LineSegment(start, end), False)

        self.startEditing()

        midPoint = FreeCAD.Vector(0, 0, 0)
        position, candidates = self.findCandidates(
            midPoint, lambda names: "Edge1" in names and "Edge2" in names
        )

        self.assertIsNotNone(
            position,
            f"no viewport position offered both overlapping edges, last seen: {candidates}",
        )

        # the regular preselection still resolves to a single edge
        preselection = SketcherGui.getActiveSketchPreselection(position)
        self.assertIsNotNone(preselection)
        edges = [name for name in preselection["SubElementNames"] if name.startswith("Edge")]
        self.assertEqual(len(edges), 1)

    def testCandidatesContainThePreselectedElement(self):
        """Whatever a plain click would select must be offered by the menu as well."""
        self.sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(-20, 0, 0), FreeCAD.Vector(20, 0, 0)), False
        )
        self.sketch.addGeometry(Part.Circle(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1), 10))

        self.startEditing()

        probePoint = FreeCAD.Vector(10, 0, 0)
        position, candidates = self.findCandidates(probePoint, lambda names: len(names) > 0)
        self.assertIsNotNone(position, "no sketch element found under the cursor")

        preselection = SketcherGui.getActiveSketchPreselection(position)
        self.assertIsNotNone(preselection)
        for name in preselection["SubElementNames"]:
            self.assertIn(name, candidates)

    def testNoCandidatesOnEmptyArea(self):
        self.sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(-20, 0, 0), FreeCAD.Vector(20, 0, 0)), False
        )

        self.startEditing()

        candidates = SketcherGui.getActiveSketchSelectionCandidates(
            self.viewportPoint(FreeCAD.Vector(0, 100, 0))
        )
        self.assertEqual(candidates, [])
