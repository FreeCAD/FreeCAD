# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
# SPDX-FileCopyrightText: 2025 Furgo
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD as App
import Arch
import ArchSectionPlane
import ArchWindow
import Draft
from bimtests import TestArchBaseGui


class TestArchWindowGui(TestArchBaseGui.TestArchBaseGui):

    def setUp(self):
        super().setUp()
        self.rectangle = Draft.make_rectangle(length=1000, height=1000)
        # The rectangle's Shape is only populated on recompute; makeWindow()
        # derives its default WindowParts from baseobj.Shape.Wires at call
        # time, so without this recompute WindowParts stays empty and the
        # window has no geometry at all.
        App.ActiveDocument.recompute()
        self.window = Arch.makeWindow(self.rectangle)
        App.ActiveDocument.recompute()

    def testWindowHasPreviewExtensions(self):
        self.assertTrue(self.window.hasExtension("Part::PreviewExtensionPython"))
        self.assertTrue(
            self.window.ViewObject.hasExtension("PartGui::ViewProviderPreviewExtensionPython")
        )

    def testRecomputePreviewPublishesShape(self):
        self.window.invalidatePreview()
        self.window.updatePreview()

        self.assertFalse(self.window.PreviewShape.isNull())

    def testPreviewFollowsWindowPartsWithoutDocumentRecompute(self):
        self.window.invalidatePreview()
        self.window.updatePreview()
        volumeBefore = self.window.PreviewShape.Volume

        parts = self.window.WindowParts
        # WindowParts is a flat list of 5-tuples; index 3 is the component
        # thickness, extruded along the panel normal in buildShapes() - doubling
        # it changes the solid's volume.
        parts[3] = str(float(parts[3]) * 2.0)
        self.window.WindowParts = parts

        # Deliberately no Document.recompute() here - that is the point: the
        # preview must follow WindowParts on its own.
        self.window.updatePreview()

        self.assertNotAlmostEqual(self.window.PreviewShape.Volume, volumeBefore, places=3)

    def _makeHostedWindow(self):
        """Wall hosting a window, recomputed once so both have settled geometry."""
        points = [App.Vector(0.0, 0.0, 0.0), App.Vector(2000.0, 0.0, 0.0)]
        line = Draft.make_wire(points)
        wall = Arch.makeWall(line, height=2000)
        wpl = App.Placement(App.Vector(500, 0, 1500), App.Vector(1, 0, 0), -90)
        win = Arch.makeWindowPreset(
            "Open 1-pane",
            width=1000.0,
            height=1000.0,
            h1=50.0,
            h2=50.0,
            h3=50.0,
            w1=100.0,
            w2=50.0,
            o1=0.0,
            o2=50.0,
            placement=wpl,
        )
        win.Hosts = [wall]
        App.ActiveDocument.recompute()
        return wall, win

    def testEditingWidthThroughTaskPanelDoesNotTouchHosts(self):
        """A live property edit must not recompute the host wall - that used to happen on
        every spinbox tick and is now deferred to accept()."""
        wall, win = self._makeHostedWindow()
        wallVolumeBefore = wall.Shape.Volume

        taskd = ArchWindow._ArchWindowTaskPanel()
        taskd.obj = win
        taskd.update()
        taskd.widthWidget.setProperty("rawValue", win.Width.Value * 2.0)
        self.pump_gui_events()

        self.assertAlmostEqual(win.Width.Value, 2000.0, places=3)
        self.assertAlmostEqual(wall.Shape.Volume, wallVolumeBefore, places=3)

        taskd.reject()

    def testAcceptingWindowEditRecomputesHosts(self):
        """accept() must still trigger the real recompute that used to happen per keystroke."""
        wall, win = self._makeHostedWindow()
        wallVolumeBefore = wall.Shape.Volume

        taskd = ArchWindow._ArchWindowTaskPanel()
        taskd.obj = win
        taskd.update()
        taskd.widthWidget.setProperty("rawValue", win.Width.Value * 2.0)
        self.pump_gui_events()

        taskd.accept()

        self.assertAlmostEqual(win.Width.Value, 2000.0, places=3)
        self.assertNotAlmostEqual(wall.Shape.Volume, wallVolumeBefore, places=3)

    def testRejectingWindowEditRevertsWidth(self):
        """Cancel must revert the property change.

        Instantiating the task panel directly bypasses Tree.cpp's double-click handler, which
        is what books the edit transaction in real usage, so this test books one explicitly.
        abortTransaction()'s revert does not depend on who opened the transaction.
        """
        _, win = self._makeHostedWindow()
        originalWidth = win.Width.Value

        App.ActiveDocument.openTransaction("Edit Window")

        taskd = ArchWindow._ArchWindowTaskPanel()
        taskd.obj = win
        taskd.update()
        taskd.widthWidget.setProperty("rawValue", originalWidth * 2.0)
        self.pump_gui_events()
        self.assertNotAlmostEqual(win.Width.Value, originalWidth, places=3)

        taskd.reject()

        self.assertAlmostEqual(win.Width.Value, originalWidth, places=3)

    def test_change_window_opening(self):
        """Tests if changes to a window opening touches the window's chain of hosts"""

        wall, win = self._makeHostedWindow()
        level = Arch.makeFloor()
        level.addObject(wall)
        section = Arch.makeSectionPlane(level)
        App.ActiveDocument.recompute()

        # Change opening from 0 to 50 (= 45 degrees):
        svg = ArchSectionPlane.getSVG(section)
        win.Opening = 50
        App.ActiveDocument.recompute()
        svg_new = ArchSectionPlane.getSVG(section)
        self.assertNotEqual(svg, svg_new)

        # Invert opening:
        svg = svg_new
        win.ViewObject.Proxy.invertOpening()
        App.ActiveDocument.recompute()
        svg_new = ArchSectionPlane.getSVG(section)
        self.assertNotEqual(svg, svg_new)

        # Invert hinge:
        svg = svg_new
        win.ViewObject.Proxy.invertHinge()
        App.ActiveDocument.recompute()
        svg_new = ArchSectionPlane.getSVG(section)
        self.assertNotEqual(svg, svg_new)
