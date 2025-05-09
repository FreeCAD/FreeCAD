# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Furgo                                              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import Arch
import Draft
import os
import FreeCAD as App
from bimtests import TestArchBase


class TestArchSectionPlane(TestArchBase.TestArchBase):

    def test_makeSectionPlane(self):
        """Test the makeSectionPlane function."""
        operation = "Testing makeSectionPlane function"
        self.printTestMessage(operation)

        section_plane = Arch.makeSectionPlane(name="TestSectionPlane")
        self.assertIsNotNone(
            section_plane, "makeSectionPlane failed to create a section plane object."
        )
        self.assertEqual(
            section_plane.Label, "TestSectionPlane", "Section plane label is incorrect."
        )

    def testTechDrawViewGeneration(self):
        """Tests the whole TD view generation workflow"""

        # Create a few objects
        points = [App.Vector(0.0, 0.0, 0.0), App.Vector(2000.0, 0.0, 0.0)]
        line = Draft.make_wire(points)
        wall = Arch.makeWall(line, height=2000)
        wpl = App.Placement(App.Vector(500, 0, 1500), App.Vector(1, 0, 0), -90)
        win = Arch.makeWindowPreset(
            "Fixed",
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
        profile = Arch.makeProfile([169, "HEA", "HEA100", "H", 100.0, 96.0, 5.0, 8.0])
        column = Arch.makeStructure(profile, height=2000.0)
        column.Profile = "HEA100"
        column.Placement.Base = App.Vector(500.0, 600.0, 0.0)
        level = Arch.makeFloor()
        level.addObjects([wall, column])
        App.ActiveDocument.recompute()

        # Create a drawing view
        section = Arch.makeSectionPlane(level)
        drawing = Arch.make2DDrawing()
        view = Draft.make_shape2dview(section)
        cut = Draft.make_shape2dview(section)
        cut.InPlace = False
        cut.ProjectionMode = "Cutfaces"
        drawing.addObjects([view, cut])
        App.ActiveDocument.recompute()

        # Create a TD page
        page = App.ActiveDocument.addObject("TechDraw::DrawPage", "Page")
        template = App.ActiveDocument.addObject("TechDraw::DrawSVGTemplate", "Template")
        template.Template = App.getResourceDir() \
                            + "Mod/TechDraw/Templates/Blank/A3/landscape/A3_blank_landscape.svg"
        page.Template = template
        view = App.ActiveDocument.addObject("TechDraw::DrawViewDraft", "DraftView")
        view.Source = drawing
        page.addView(view)
        view.Scale = 1.0
        view.X = "20cm"
        view.Y = "15cm"
        App.ActiveDocument.recompute()
        assert True

    def testShape2DViewGeneration(self):
        """Tests Draft_Shape2DView face with hole creation"""

        # Create a wall based on a clock-wise wire starting at the lower left corner.
        # Such a wire would previously result in an invalid face in the Shape2DView.
        wire = Draft.make_wire(
            [
                App.Vector(0, 0, 0),
                App.Vector(0, 1000, 0),
                App.Vector(2000, 1000, 0),
                App.Vector(2000, 0, 0),
            ],
            closed=True,
        )
        wire.MakeFace = False
        wall = Arch.makeWall(wire, height=3000, width=200)
        App.ActiveDocument.recompute()

        section = Arch.makeSectionPlane(wall)
        shp_view = Draft.make_shape2dview(section)
        shp_view.InPlace = False
        shp_view.ProjectionMode = "Cutfaces"
        App.ActiveDocument.recompute()

        face = shp_view.Shape.Faces[0]
        self.assertTrue(face.isValid())

        area_expected = 1200 * 2200 - 800 * 1800
        self.assertAlmostEqual(face.Area, area_expected)
