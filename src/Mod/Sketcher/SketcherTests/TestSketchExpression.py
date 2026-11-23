# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2021 Werner Mayer
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


import math, os, sys, unittest
import FreeCAD
import Part
import Sketcher

App = FreeCAD


class TestSketchExpression(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("TestSketchExpr")

    # See https://forum.freecad.org/viewtopic.php?f=3&t=64699
    # and https://forum.freecad.org/viewtopic.php?f=10&t=64718
    def testConstraintWithExpression(self):
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")

        geoList = []
        geoList.append(Part.LineSegment(App.Vector(0, 5, 0), App.Vector(5, 5, 0)))
        geoList.append(Part.LineSegment(App.Vector(5, 5, 0), App.Vector(5, 0, 0)))
        geoList.append(Part.LineSegment(App.Vector(5, 0, 0), App.Vector(0, 0, 0)))
        geoList.append(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(0, 5, 0)))
        sketch.addGeometry(geoList, False)

        conList = []
        conList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        conList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        conList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        conList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        conList.append(Sketcher.Constraint("Horizontal", 0))
        conList.append(Sketcher.Constraint("Horizontal", 2))
        conList.append(Sketcher.Constraint("Vertical", 1))
        conList.append(Sketcher.Constraint("Vertical", 3))
        sketch.addConstraint(conList)
        del geoList, conList

        # Check that circular references get detected
        length = sketch.addConstraint(Sketcher.Constraint("Distance", 0, 6.0))
        height = sketch.addConstraint(Sketcher.Constraint("Distance", 3, 5.0))
        sketch.renameConstraint(length, "Length")
        sketch.renameConstraint(height, "Height")

        sketch.setExpression("Constraints[{}]".format(height), ".Constraints.Length")

        with self.assertRaises(RuntimeError) as context:
            sketch.setExpression("Constraints.Length", ".Constraints.Length * 2")
        assert ".Constraints.Length reference creates a cyclic dependency." in str(
            context.exception
        )

        with self.assertRaises(RuntimeError) as context:
            sketch.setExpression("Constraints.Length", ".Constraints.Height * 2")
        assert ".Constraints.Length reference creates a cyclic dependency." in str(
            context.exception
        )

    def tearDown(self):
        # comment out to omit closing document for debugging
        FreeCAD.closeDocument(self.Doc.Name)
