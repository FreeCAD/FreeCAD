# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 Russell Johnson (russ4262) <russ4262@gmail.com>    *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Part
import Path.Op.Adaptive as PathAdaptive
import Path.Main.Job as PathJob
from Tests.PathTestUtils import PathTestBase

if FreeCAD.GuiUp:
    import Path.Main.Gui.Job as PathJobGui
    import Path.Op.Gui.Adaptive as PathAdaptiveGui


class TestPathAdaptive(PathTestBase):
    """Unit tests for the Adaptive operation."""

    @classmethod
    def setUpClass(cls):
        """setUpClass()...
        This method is called upon instantiation of this test class.  Add code and objects here
        that are needed for the duration of the test() methods in this class.  In other words,
        set up the 'global' test environment here; use the `setUp()` method to set up a 'local'
        test environment.
        This method does not have access to the class `self` reference, but it
        is able to call static methods within this same class.
        """
        cls.needsInit = True


    @classmethod
    def initClass(cls):
        # Open existing FreeCAD document with test geometry
        cls.needsInit = False
        cls.doc = FreeCAD.open(
            FreeCAD.getHomePath() + "Mod/CAM/Tests/test_adaptive.fcstd"
        )

        # Create Job object, adding geometry objects from file opened above
        cls.job = PathJob.Create("Job", [cls.doc.Fusion], None)
        cls.job.GeometryTolerance.Value = 0.001
        if FreeCAD.GuiUp:
            cls.job.ViewObject.Proxy = PathJobGui.ViewProvider(cls.job.ViewObject)

        # Instantiate an Adaptive operation for querying available properties
        cls.prototype = PathAdaptive.Create("Adaptive")
        cls.prototype.Base = [(cls.doc.Fusion, ["Face3"])]
        cls.prototype.Label = "Prototype"
        _addViewProvider(cls.prototype)

        cls.doc.recompute()

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...
        This method is called prior to destruction of this test class.  Add code and objects here
        that cleanup the test environment after the test() methods in this class have been executed.
        This method does not have access to the class `self` reference.  This method
        is able to call static methods within this same class.
        """
        # FreeCAD.Console.PrintMessage("TestPathAdaptive.tearDownClass()\n")

        # Close geometry document without saving
        if not cls.needsInit:
            FreeCAD.closeDocument(cls.doc.Name)

    # Setup and tear down methods called before and after each unit test
    def setUp(self):
        """setUp()...
        This method is called prior to each `test()` method.  Add code and objects here
        that are needed for multiple `test()` methods.
        """
        if self.needsInit:
            self.initClass()

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        pass

    # Unit tests
    def test00(self):
        """test00() Empty test."""
        return

    def test01(self):
        """test01() Verify path generated on Face3."""

        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [(self.doc.Fusion, ["Face3"])]  # (base, subs_list)
        adaptive.Label = "test01+"
        adaptive.Comment = "test01() Verify path generated on Face3."

        # Set additional operation properties
        # setDepthsAndHeights(adaptive)
        adaptive.FinishingProfile = False
        adaptive.HelixAngle = 75.0
        adaptive.HelixDiameterLimit.Value = 1.0
        adaptive.LiftDistance.Value = 1.0
        adaptive.StepOver = 75
        adaptive.UseOutline = False
        adaptive.setExpression("StepDown", None)
        adaptive.StepDown.Value = (
            20.0  # Have to set expression to None before numerical value assignment
        )

        _addViewProvider(adaptive)
        self.doc.recompute()

        # moves = getGcodeMoves(adaptive.Path.Commands, includeRapids=False)
        # operationMoves = ";  ".join(moves)
        # FreeCAD.Console.PrintMessage("test00_moves: " + operationMoves + "\n")

        # self.assertTrue(expected_moves_test01 == operationMoves,
        #                "expected_moves_test01: {}\noperationMoves: {}".format(expected_moves_test01, operationMoves))
        self.assertTrue(
            len(adaptive.Path.Commands) > 100, "Command count not greater than 100."
        )

    def test02(self):
        """test02() Verify path generated on adjacent, combined Face3 and Face10.  The Z heights are different."""

        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [(self.doc.Fusion, ["Face3", "Face10"])]  # (base, subs_list)
        adaptive.Label = "test02+"
        adaptive.Comment = "test02() Verify path generated on adjacent, combined Face3 and Face10.  The Z heights are different."

        # Set additional operation properties
        # setDepthsAndHeights(adaptive)
        adaptive.FinishingProfile = False
        adaptive.HelixAngle = 75.0
        adaptive.HelixDiameterLimit.Value = 1.0
        adaptive.LiftDistance.Value = 1.0
        adaptive.StepOver = 75
        adaptive.UseOutline = False
        adaptive.setExpression("StepDown", None)
        adaptive.StepDown.Value = (
            20.0  # Have to set expression to None before numerical value assignment
        )

        _addViewProvider(adaptive)
        self.doc.recompute()

        self.assertTrue(
            len(adaptive.Path.Commands) > 100, "Command count not greater than 100."
        )

    def test03(self):
        """test03() Verify path generated on adjacent, combined Face3 and Face10.  The Z heights are different."""

        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [(self.doc.Fusion, ["Face3", "Face10"])]  # (base, subs_list)
        adaptive.Label = "test03+"
        adaptive.Comment = "test03() Verify path generated on adjacent, combined Face3 and Face10.  The Z heights are different."

        # Set additional operation properties
        # setDepthsAndHeights(adaptive)
        adaptive.FinishingProfile = False
        adaptive.HelixAngle = 75.0
        adaptive.HelixDiameterLimit.Value = 1.0
        adaptive.LiftDistance.Value = 1.0
        adaptive.StepOver = 75
        adaptive.UseOutline = True
        adaptive.setExpression("StepDown", None)
        adaptive.StepDown.Value = (
            20.0  # Have to set expression to None before numerical value assignment
        )

        _addViewProvider(adaptive)
        self.doc.recompute()

        self.assertTrue(
            len(adaptive.Path.Commands) > 100, "Command count not greater than 100."
        )

    def test04(self):
        """test04() Verify path generated non-closed edges with differing Z-heights that are closed with Z=1 projection: "Edge9", "Edge2", "Edge8", "Edge15", "Edge30", "Edge31", "Edge29", "Edge19"."""

        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [
            (
                self.doc.Fusion,
                [
                    "Edge9",
                    "Edge2",
                    "Edge8",
                    "Edge15",
                    "Edge30",
                    "Edge31",
                    "Edge29",
                    "Edge19",
                ],
            )
        ]  # (base, subs_list)
        adaptive.Label = "test04+"
        adaptive.Comment = 'test04() Verify path generated non-closed edges with differing Z-heights that are closed with Z=1 projection: "Edge9", "Edge2", "Edge8", "Edge15", "Edge30", "Edge31", "Edge29", "Edge19".'

        # Set additional operation properties
        # setDepthsAndHeights(adaptive)
        adaptive.FinishingProfile = False
        adaptive.HelixAngle = 75.0
        adaptive.HelixDiameterLimit.Value = 1.0
        adaptive.LiftDistance.Value = 1.0
        adaptive.StepOver = 75
        adaptive.UseOutline = False
        adaptive.setExpression("StepDown", None)
        adaptive.StepDown.Value = (
            20.0  # Have to set expression to None before numerical value assignment
        )

        _addViewProvider(adaptive)
        self.doc.recompute()

        self.assertTrue(
            len(adaptive.Path.Commands) > 100, "Command count not greater than 100."
        )

    def test05(self):
        """test05() Verify path generated closed wire with differing Z-heights: "Edge13", "Edge7", "Edge9", "Edge2", "Edge8", "Edge15", "Edge30", "Edge31", "Edge29", "Edge19"."""

        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [
            (
                self.doc.Fusion,
                [
                    "Edge13",
                    "Edge7",
                    "Edge9",
                    "Edge2",
                    "Edge8",
                    "Edge15",
                    "Edge30",
                    "Edge31",
                    "Edge29",
                    "Edge19",
                ],
            )
        ]  # (base, subs_list)
        adaptive.Label = "test05+"
        adaptive.Comment = 'test05() Verify path generated closed wire with differing Z-heights: "Edge13", "Edge7", "Edge9", "Edge2", "Edge8", "Edge15", "Edge30", "Edge31", "Edge29", "Edge19".'

        # Set additional operation properties
        # setDepthsAndHeights(adaptive)
        adaptive.FinishingProfile = False
        adaptive.HelixAngle = 75.0
        adaptive.HelixDiameterLimit.Value = 1.0
        adaptive.LiftDistance.Value = 1.0
        adaptive.StepOver = 75
        adaptive.UseOutline = False
        adaptive.setExpression("StepDown", None)
        adaptive.StepDown.Value = (
            20.0  # Have to set expression to None before numerical value assignment
        )

        _addViewProvider(adaptive)
        self.doc.recompute()

        self.assertTrue(
            len(adaptive.Path.Commands) > 100, "Command count not greater than 100."
        )

    def test06(self):
        """test06() Verify path generated with outer and inner edge loops at same Z height: "Edge15", "Edge30", "Edge31", "Edge29", "Edge19", "Edge18", "Edge35", "Edge32", "Edge34", "Edge33"."""

        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [
            (
                self.doc.Fusion,
                [
                    "Edge15",
                    "Edge30",
                    "Edge31",
                    "Edge29",
                    "Edge19",
                    "Edge18",
                    "Edge35",
                    "Edge32",
                    "Edge34",
                    "Edge33",
                ],
            )
        ]  # (base, subs_list)
        adaptive.Label = "test06+"
        adaptive.Comment = 'test06() Verify path generated with outer and inner edge loops at same Z height: "Edge15", "Edge30", "Edge31", "Edge29", "Edge19", "Edge18", "Edge35", "Edge32", "Edge34", "Edge33".'

        # Set additional operation properties
        # setDepthsAndHeights(adaptive)
        adaptive.FinishingProfile = False
        adaptive.HelixAngle = 75.0
        adaptive.HelixDiameterLimit.Value = 1.0
        adaptive.LiftDistance.Value = 1.0
        adaptive.StepOver = 75
        adaptive.UseOutline = False
        adaptive.setExpression("StepDown", None)
        adaptive.StepDown.Value = (
            20.0  # Have to set expression to None before numerical value assignment
        )

        _addViewProvider(adaptive)
        self.doc.recompute()

        # Check command count
        self.assertTrue(
            len(adaptive.Path.Commands) > 100, "Command count not greater than 100."
        )

        # Check if any paths originate inside inner hole of donut.  They should not.
        isInBox = False
        edges = [
            self.doc.Fusion.Shape.getElement(e)
            for e in ["Edge35", "Edge32", "Edge33", "Edge34"]
        ]
        square = Part.Wire(edges)
        sqrBB = square.BoundBox
        minPoint = FreeCAD.Vector(sqrBB.XMin, sqrBB.YMin, 0.0)
        maxPoint = FreeCAD.Vector(sqrBB.XMax, sqrBB.YMax, 0.0)
        for c in adaptive.Path.Commands:
            if pathOriginatesInBox(c, minPoint, maxPoint):
                isInBox = True
                break
        self.assertFalse(isInBox, "Paths originating within the inner hole.")

    def test07(self):
        """test07() Verify path generated on donut-shaped Face10."""

        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [(self.doc.Fusion, ["Face10"])]  # (base, subs_list)
        adaptive.Label = "test07+"
        adaptive.Comment = "test07() Verify path generated on donut-shaped Face10."

        # Set additional operation properties
        # setDepthsAndHeights(adaptive)
        adaptive.FinishingProfile = False
        adaptive.HelixAngle = 75.0
        adaptive.HelixDiameterLimit.Value = 1.0
        adaptive.LiftDistance.Value = 1.0
        adaptive.StepOver = 75
        adaptive.UseOutline = False
        adaptive.setExpression("StepDown", None)
        adaptive.StepDown.Value = (
            20.0  # Have to set expression to None before numerical value assignment
        )

        _addViewProvider(adaptive)
        self.doc.recompute()

        self.assertTrue(
            len(adaptive.Path.Commands) > 100, "Command count not greater than 100."
        )

        # Check if any paths originate inside inner hole of donut.  They should not.
        isInBox = False
        edges = [
            self.doc.Fusion.Shape.getElement(e)
            for e in ["Edge35", "Edge32", "Edge33", "Edge34"]
        ]
        square = Part.Wire(edges)
        sqrBB = square.BoundBox
        minPoint = FreeCAD.Vector(sqrBB.XMin, sqrBB.YMin, 0.0)
        maxPoint = FreeCAD.Vector(sqrBB.XMax, sqrBB.YMax, 0.0)
        for c in adaptive.Path.Commands:
            if pathOriginatesInBox(c, minPoint, maxPoint):
                isInBox = True
                break
        self.assertFalse(isInBox, "Paths originating within the inner hole.")

        # Set Adaptive op to only use the outline of the face.
        adaptive.UseOutline = True
        self.doc.recompute()

        # Check if any paths originate inside inner hole of donut.  They should not.
        isInBox = False
        edges = [
            self.doc.Fusion.Shape.getElement(e)
            for e in ["Edge35", "Edge32", "Edge33", "Edge34"]
        ]
        square = Part.Wire(edges)
        sqrBB = square.BoundBox
        minPoint = FreeCAD.Vector(sqrBB.XMin, sqrBB.YMin, 0.0)
        maxPoint = FreeCAD.Vector(sqrBB.XMax, sqrBB.YMax, 0.0)
        for c in adaptive.Path.Commands:
            if pathOriginatesInBox(c, minPoint, maxPoint):
                isInBox = True
                break
        self.assertTrue(isInBox, "No paths originating within the inner hole.")


# Eclass


def setDepthsAndHeights(op, strDep=20.0, finDep=0.0):
    """setDepthsAndHeights(op, strDep=20.0, finDep=0.0)... Sets default depths and heights for `op` passed to it"""

    # Set start and final depth in order to eliminate effects of stock (and its default values)
    op.setExpression("StartDepth", None)
    op.StartDepth.Value = strDep
    op.setExpression("FinalDepth", None)
    op.FinalDepth.Value = finDep

    # Set step down so as to only produce one layer path
    op.setExpression("StepDown", None)
    op.StepDown.Value = 20.0

    # Set Heights
    # default values used


def getGcodeMoves(cmdList, includeRapids=True, includeLines=True, includeArcs=True):
    """getGcodeMoves(cmdList, includeRapids=True, includeLines=True, includeArcs=True)...
    Accepts command dict and returns point string coordinate.
    """
    gcode_list = list()
    last = FreeCAD.Vector(0.0, 0.0, 0.0)
    for c in cmdList:
        p = c.Parameters
        name = c.Name
        if includeRapids and name in ["G0", "G00"]:
            gcode = name
            x = last.x
            y = last.y
            z = last.z
            if p.get("X"):
                x = round(p["X"], 2)
                gcode += " X" + str(x)
            if p.get("Y"):
                y = round(p["Y"], 2)
                gcode += " Y" + str(y)
            if p.get("Z"):
                z = round(p["Z"], 2)
                gcode += " Z" + str(z)
            last.x = x
            last.y = y
            last.z = z
            gcode_list.append(gcode)
        elif includeLines and name in ["G1", "G01"]:
            gcode = name
            x = last.x
            y = last.y
            z = last.z
            if p.get("X"):
                x = round(p["X"], 2)
                gcode += " X" + str(x)
            if p.get("Y"):
                y = round(p["Y"], 2)
                gcode += " Y" + str(y)
            if p.get("Z"):
                z = round(p["Z"], 2)
                gcode += " Z" + str(z)
            last.x = x
            last.y = y
            last.z = z
            gcode_list.append(gcode)
        elif includeArcs and name in ["G2", "G3", "G02", "G03"]:
            gcode = name
            x = last.x
            y = last.y
            z = last.z
            i = 0.0
            j = 0.0
            k = 0.0
            if p.get("I"):
                i = round(p["I"], 2)
            gcode += " I" + str(i)
            if p.get("J"):
                j = round(p["J"], 2)
            gcode += " J" + str(j)
            if p.get("K"):
                k = round(p["K"], 2)
            gcode += " K" + str(k)

            if p.get("X"):
                x = round(p["X"], 2)
            gcode += " X" + str(x)
            if p.get("Y"):
                y = round(p["Y"], 2)
            gcode += " Y" + str(y)
            if p.get("Z"):
                z = round(p["Z"], 2)
            gcode += " Z" + str(z)

            gcode_list.append(gcode)
            last.x = x
            last.y = y
            last.z = z
    return gcode_list


def pathOriginatesInBox(cmd, minPoint, maxPoint):
    p = cmd.Parameters
    name = cmd.Name
    if name in ["G0", "G00", "G1", "G01"]:
        if p.get("X") and p.get("Y"):
            x = p.get("X")
            y = p.get("Y")
            if x > minPoint.x and y > minPoint.y and x < maxPoint.x and y < maxPoint.y:
                return True
    return False


def _addViewProvider(adaptiveOp):
    if FreeCAD.GuiUp:
        PathOpGui = PathAdaptiveGui.PathOpGui
        cmdRes = PathAdaptiveGui.Command.res
        adaptiveOp.ViewObject.Proxy = PathOpGui.ViewProvider(
            adaptiveOp.ViewObject, cmdRes
        )


# Example string literal of expected path moves from an operation
# Expected moves for unit test01
expected_moves_test01 = "G1 X32.5 Y32.5 Z5.0;  \
G1 X17.5 Y32.5 Z5.0;  \
G1 X17.5 Y30.0 Z5.0;  \
G1 X32.5 Y30.0 Z5.0;  \
G1 X32.5 Y27.5 Z5.0;  \
G1 X17.5 Y27.5 Z5.0;  \
G1 X17.5 Y25.0 Z5.0;  \
G1 X32.5 Y25.0 Z5.0;  \
G1 X32.5 Y22.5 Z5.0;  \
G1 X17.5 Y22.5 Z5.0;  \
G1 X17.5 Y20.0 Z5.0;  \
G1 X32.5 Y20.0 Z5.0;  \
G1 X32.5 Y17.5 Z5.0;  \
G1 X17.5 Y17.5 Z5.0"
