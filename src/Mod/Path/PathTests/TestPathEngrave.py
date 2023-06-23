# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2023 Russell Johnson (russ4262) <russ4262@gmail.com>    *
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
import Sketcher
import Path.Op.Engrave as Engrave
from PathTests.PathTestUtils import PathTestBase

if FreeCAD.GuiUp:
    import FreeCADGui
    import Path.Main.Gui.Job as JobGui
    import Path.Op.Gui.Engrave as EngraveGui

    Job = JobGui.PathJob
else:
    import Path.Main.Job as Job


class TestPathEngrave(PathTestBase):
    """Unit tests for the Engrave operation."""

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
        # FreeCAD.Console.PrintMessage("TestPathEngrave.setUpClass()\n")

        # Based on some PartDesign tests
        # cls.doc = FreeCAD.newDocument("Test_Engrave")
        cls.doc = FreeCAD.open(
            FreeCAD.getHomePath() + "Mod/Path/PathTests/test_engrave.fcstd"
        )

        if FreeCAD.GuiUp:
            FreeCADGui.activateWorkbench("PathWorkbench")

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...
        This method is called prior to destruction of this test class.  Add code and objects here
        that cleanup the test environment after the test() methods in this class have been executed.
        This method does not have access to the class `self` reference.  This method
        is able to call static methods within this same class.
        """
        # FreeCAD.Console.PrintMessage("TestPathEngrave.tearDownClass()\n")

        # Close geometry document without saving
        FreeCAD.closeDocument(cls.doc.Name)
        pass

    # Setup and tear down methods called before and after each unit test
    def setUp(self):
        """setUp()...
        This method is called prior to each `test()` method.  Add code and objects here
        that are needed for multiple `test()` methods.
        """
        pass

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        pass

    # Unit tests
    def test00(self):
        """test00() Return True if Engrave gcode is correct for three single, unconnected lines."""
        # Test 1 of 2 to confirm fix for Issue #9303

        doc = FreeCAD.ActiveDocument
        # Create 3 Sketch and single Job objects
        sketches2 = createSquareSketches(doc, None)
        job = createJob(sketches2)
        job.Label = "test00_"
        doc.recompute()

        op = createEngraveOp(job)
        for base, sub in [
            (sketches2[0], "Edge4"),
            (sketches2[1], "Edge4"),
            (sketches2[2], "Edge4"),
        ]:
            op.Proxy.addBase(op, base, sub)
        setDepthsAndHeights(
            op,
            strDep=job.Stock.Shape.BoundBox.ZMax,
            finDep=job.Stock.Shape.BoundBox.ZMin + 1.0,
        )
        doc.recompute()
        setView()

        moves = getGcodeMoves(op.Path.Commands, includeRapids=True)
        operationMoves = ";  ".join(moves)
        # FreeCAD.Console.PrintMessage("test00()_moves: " + operationMoves + "\n")

        self.assertTrue(
            expected_moves_test00 == operationMoves,
            "expected_moves_test00: {}\noperationMoves: {}".format(
                expected_moves_test00, operationMoves
            ),
        )

    def test01(self):
        """test01() Return True if Engrave gcode is correct for three concentric squares."""
        # Test 2 of 2 to confirm fix for Issue #9303

        doc = FreeCAD.ActiveDocument
        # Create Body, 3 Sketch, and Job objects
        body = doc.addObject("PartDesign::Body", "Body")
        sketches = createSquareSketches(doc, body)
        job = createJob(sketches)
        job.Label = "test01_"
        doc.recompute()

        op = createEngraveOp(job)  # Use all models available in Job object
        setDepthsAndHeights(
            op,
            strDep=job.Stock.Shape.BoundBox.ZMax,
            finDep=job.Stock.Shape.BoundBox.ZMin + 1.0,
        )
        doc.recompute()
        setView()

        moves = getGcodeMoves(op.Path.Commands, includeRapids=True)
        operationMoves = ";  ".join(moves)
        # FreeCAD.Console.PrintMessage("test01()_moves: " + operationMoves + "\n")

        self.assertTrue(
            expected_moves_test01 == operationMoves,
            "expected_moves_test01: {}\noperationMoves: {}".format(
                expected_moves_test01, operationMoves
            ),
        )

    def test02(self):
        """test02() Return True if Engrave gcode is correct
        for all available models with none selected."""
        # Test 1 of 4 to confirm fix for Issue #9114

        doc = FreeCAD.ActiveDocument
        # Create Body, 3 Sketch, and Job objects
        items = [doc.ShapeString, doc.Line, doc.Rectangle, doc.Circle, doc.Arc]
        job = createJob(items)
        job.Label = "test02_"
        doc.recompute()

        # Use all models available in Job object without actively selecting any
        op = createEngraveOp(job)
        setDepthsAndHeights(
            op,
            strDep=job.Stock.Shape.BoundBox.ZMax,
            finDep=job.Stock.Shape.BoundBox.ZMin + 1.0,
        )
        doc.recompute()
        setView()

        moves = getGcodeMoves(op.Path.Commands, includeRapids=True)
        operationMoves = ";  ".join(moves)
        # FreeCAD.Console.PrintMessage("test02()_moves: " + operationMoves + "\n")

        self.assertTrue(
            expected_moves_test02 == operationMoves,
            "expected_moves_test02: {}\noperationMoves: {}".format(
                expected_moves_test02, operationMoves
            ),
        )

    def test03(self):
        """test03() Return True if Engrave gcode is correct Arc.Edge1."""
        # Test 2 of 4 to confirm fix for Issue #9114

        doc = FreeCAD.ActiveDocument
        # Create Body, 3 Sketch, and Job objects
        items = [doc.ShapeString, doc.Line, doc.Rectangle, doc.Circle, doc.Arc]
        job = createJob(items)
        job.Label = "test03_"
        doc.recompute()

        op = createEngraveOp(job)  # Use all models available in Job object
        for base, sub in [
            (job.Proxy.resourceClone(job, doc.Arc), "Edge1"),
        ]:
            op.Proxy.addBase(op, base, sub)

        setDepthsAndHeights(
            op,
            strDep=job.Stock.Shape.BoundBox.ZMax,
            finDep=job.Stock.Shape.BoundBox.ZMin + 1.0,
        )
        doc.recompute()
        setView()

        moves = getGcodeMoves(op.Path.Commands, includeRapids=True)
        operationMoves = ";  ".join(moves)
        # FreeCAD.Console.PrintMessage("test03()_moves: " + operationMoves + "\n")

        self.assertTrue(
            expected_moves_test03 == operationMoves,
            "expected_moves_test03: {}\noperationMoves: {}".format(
                expected_moves_test03, operationMoves
            ),
        )

    def test04(self):
        """test04() Return True if Engrave gcode is correct for all available models selected."""
        # Test 3 of 4 to confirm fix for Issue #9114

        doc = FreeCAD.ActiveDocument
        # Create Body, 3 Sketch, and Job objects
        items = [doc.ShapeString, doc.Line, doc.Rectangle, doc.Circle, doc.Arc]
        job = createJob(items)
        job.Label = "test04_"
        doc.recompute()

        op = createEngraveOp(job)
        op.BaseShapes = [job.Proxy.resourceClone(job, itm) for itm in items]

        setDepthsAndHeights(
            op,
            strDep=job.Stock.Shape.BoundBox.ZMax,
            finDep=job.Stock.Shape.BoundBox.ZMin + 1.0,
        )
        doc.recompute()
        setView()

        moves = getGcodeMoves(op.Path.Commands, includeRapids=True)
        operationMoves = ";  ".join(moves)
        # FreeCAD.Console.PrintMessage("test04()_moves: " + operationMoves + "\n")

        self.assertTrue(
            expected_moves_test04 == operationMoves,
            "expected_moves_test04: {}\noperationMoves: {}".format(
                expected_moves_test04, operationMoves
            ),
        )

    def test05(self):
        """test05() Return True if Engrave gcode is correct
        for single selected edge and remaining selected models."""
        # Test 4 of 4 to confirm fix for Issue #9114

        doc = FreeCAD.ActiveDocument
        # Create Body, 3 Sketch, and Job objects
        items = [doc.ShapeString, doc.Line, doc.Rectangle, doc.Circle]
        job = createJob(items)
        job.Label = "test05_"
        doc.recompute()

        op = createEngraveOp(job)  # Use all models available in Job object
        for base, sub in [
            (doc.Arc, "Edge1"),
        ]:
            op.Proxy.addBase(op, base, sub)
        op.BaseShapes = [job.Proxy.resourceClone(job, itm) for itm in items]

        setDepthsAndHeights(
            op,
            strDep=job.Stock.Shape.BoundBox.ZMax,
            finDep=job.Stock.Shape.BoundBox.ZMin + 1.0,
        )
        doc.recompute()
        setView()

        moves = getGcodeMoves(op.Path.Commands, includeRapids=True)
        operationMoves = ";  ".join(moves)
        # FreeCAD.Console.PrintMessage("test05()_moves: " + operationMoves + "\n")

        self.assertTrue(
            expected_moves_test05 == operationMoves,
            "expected_moves_test05: {}\noperationMoves: {}".format(
                expected_moves_test05, operationMoves
            ),
        )


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
            if p.get("X") is not None:
                x = round(p["X"], 2)
                gcode += " X" + str(x)
            if p.get("Y") is not None:
                y = round(p["Y"], 2)
                gcode += " Y" + str(y)
            if p.get("Z") is not None:
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
            if p.get("X") is not None:
                x = round(p["X"], 2)
                gcode += " X" + str(x)
            if p.get("Y") is not None:
                y = round(p["Y"], 2)
                gcode += " Y" + str(y)
            if p.get("Z") is not None:
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
            if p.get("I") is not None:
                i = round(p["I"], 2)
            gcode += " I" + str(i)
            if p.get("J") is not None:
                j = round(p["J"], 2)
            gcode += " J" + str(j)
            if p.get("K") is not None:
                k = round(p["K"], 2)
            gcode += " K" + str(k)

            if p.get("X") is not None:
                x = round(p["X"], 2)
            gcode += " X" + str(x)
            if p.get("Y") is not None:
                y = round(p["Y"], 2)
            gcode += " Y" + str(y)
            if p.get("Z") is not None:
                z = round(p["Z"], 2)
            gcode += " Z" + str(z)

            gcode_list.append(gcode)
            last.x = x
            last.y = y
            last.z = z
    return gcode_list


def setView():
    if FreeCAD.GuiUp:
        # FreeCADGui.activeDocument().activeView().viewTop()
        FreeCADGui.activeDocument().activeView().viewIsometric()
        FreeCADGui.SendMsgToActiveView("ViewFit")


#######################################
# Support functions for sketches and job and engrave op
def newSketch(doc, body):
    sketch = doc.addObject("Sketcher::SketchObject", "Sketch")
    if body:
        # sketch.Support = (doc.XY_Plane, [""])
        xy_plane = body.Origin.OriginFeatures[3]
        sketch.Support = (xy_plane, [""])
        sketch.MapMode = "FlatFace"
        body.addObject(sketch)
    else:
        # sketch.Support = (doc.XY_Plane, [""])  # Enabling causes linking out of scope issues.
        sketch.MapMode = "FlatFace"
    # doc.recompute()
    return sketch


def createSquareInSketch(sketch, startIdx, sideLength):
    p1 = FreeCAD.Vector(0, 0, 0)
    p2 = FreeCAD.Vector(sideLength, 0, 0)
    p3 = FreeCAD.Vector(sideLength, sideLength, 0)
    p4 = FreeCAD.Vector(0, sideLength, 0)

    # Create four edges for square
    sketch.addGeometry(Part.LineSegment(p1, p2))
    sketch.addGeometry(Part.LineSegment(p2, p3))
    sketch.addGeometry(Part.LineSegment(p3, p4))
    sketch.addGeometry(Part.LineSegment(p4, p1))

    # Constrain first vertex to origin
    # sketch.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))

    # Constrain all sides of square to horizontal and vertical, respectively
    sketch.addConstraint(Sketcher.Constraint("Horizontal", startIdx + 0))
    sketch.addConstraint(Sketcher.Constraint("Vertical", startIdx + 1))
    sketch.addConstraint(Sketcher.Constraint("Horizontal", startIdx + 2))
    sketch.addConstraint(Sketcher.Constraint("Vertical", startIdx + 3))

    # Constrain coincidental vertexes of edges
    sketch.addConstraint(
        Sketcher.Constraint("Coincident", startIdx + 0, 2, startIdx + 1, 1)
    )
    sketch.addConstraint(
        Sketcher.Constraint("Coincident", startIdx + 1, 2, startIdx + 2, 1)
    )
    sketch.addConstraint(
        Sketcher.Constraint("Coincident", startIdx + 2, 2, startIdx + 3, 1)
    )
    # Constrain last vertex on last edge, to first vertex on first edge
    sketch.addConstraint(
        Sketcher.Constraint("Coincident", startIdx + 3, 2, startIdx + 0, 1)
    )

    # Constrain horizontal distance of first edge to side length
    sketch.addConstraint(
        Sketcher.Constraint("DistanceX", startIdx + 0, 1, startIdx + 0, 2, sideLength)
    )
    # Constrain vertical distance of second edge to side length
    sketch.addConstraint(
        Sketcher.Constraint("DistanceY", startIdx + 1, 1, startIdx + 1, 2, sideLength)
    )


def createSquareSketches(doc, body=None):
    idxStart = 0

    sketch = newSketch(doc, body)
    createSquareInSketch(sketch, idxStart, 10.0)
    # Constrain horizontal and vertical distances of first vertex of first edge, from origin
    x = 10.0
    y = 10.0
    sketch.addConstraint(Sketcher.Constraint("DistanceX", idxStart + 0, 1, -1, 1, -x))
    sketch.addConstraint(Sketcher.Constraint("DistanceY", idxStart + 0, 1, -1, 1, -y))

    sketch2 = newSketch(doc, body)
    createSquareInSketch(sketch2, idxStart, 8.0)
    # Constrain horizontal and vertical distances of first vertex of first edge, from origin
    x = 11.0
    y = 11.0
    sketch2.addConstraint(Sketcher.Constraint("DistanceX", idxStart + 0, 1, -1, 1, -x))
    sketch2.addConstraint(Sketcher.Constraint("DistanceY", idxStart + 0, 1, -1, 1, -y))

    sketch3 = newSketch(doc, body)
    createSquareInSketch(sketch3, idxStart, 6.0)
    # Constrain horizontal and vertical distances of first vertex of first edge, from origin
    x = 12.0
    y = 12.0
    sketch3.addConstraint(Sketcher.Constraint("DistanceX", idxStart + 0, 1, -1, 1, -x))
    sketch3.addConstraint(Sketcher.Constraint("DistanceY", idxStart + 0, 1, -1, 1, -y))

    return [sketch, sketch2, sketch3]


def createJob(models):
    if FreeCAD.GuiUp:
        return JobGui.Create(models, openTaskPanel=False)

    return Job.Create("Job", models)


def createEngraveOp(job=None):
    op = Engrave.Create("Engrave", parentJob=job)
    if FreeCAD.GuiUp:
        # Add view provider
        res = EngraveGui.Command.res
        op.ViewObject.Proxy = EngraveGui.PathOpGui.ViewProvider(op.ViewObject, res)
        op.ViewObject.Proxy.deleteOnReject = False
        op.ViewObject.Visibility = True
    return op


#######################################
# Expected gcode for associated test cases
expected_moves_test00 = "G0 Z6.0;  \
G0 X10.0 Y20.0;  \
G0 Z4.0;  \
G1 X10.0 Y20.0 Z0.0;  \
G1 X10.0 Y10.0 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X11.0 Y19.0;  \
G0 Z4.0;  \
G1 X11.0 Y19.0 Z0.0;  \
G1 X11.0 Y11.0 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X12.0 Y18.0;  \
G0 Z4.0;  \
G1 X12.0 Y18.0 Z0.0;  \
G1 X12.0 Y12.0 Z0.0;  \
G0 Z6.0"

expected_moves_test01 = "G0 Z6.0;  \
G0 X10.0 Y10.0;  \
G0 Z4.0;  \
G1 X10.0 Y10.0 Z0.0;  \
G1 X20.0 Y10.0 Z0.0;  \
G1 X20.0 Y20.0 Z0.0;  \
G1 X10.0 Y20.0 Z0.0;  \
G1 X10.0 Y10.0 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X11.0 Y11.0;  \
G0 Z4.0;  \
G1 X11.0 Y11.0 Z0.0;  \
G1 X19.0 Y11.0 Z0.0;  \
G1 X19.0 Y19.0 Z0.0;  \
G1 X11.0 Y19.0 Z0.0;  \
G1 X11.0 Y11.0 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X12.0 Y12.0;  \
G0 Z4.0;  \
G1 X12.0 Y12.0 Z0.0;  \
G1 X18.0 Y12.0 Z0.0;  \
G1 X18.0 Y18.0 Z0.0;  \
G1 X12.0 Y18.0 Z0.0;  \
G1 X12.0 Y12.0 Z0.0;  \
G0 Z6.0"

expected_moves_test02 = "G0 Z6.0;  \
G0 X-28.78 Y-19.45;  \
G0 Z4.0;  \
G1 X-28.78 Y-19.45 Z0.0;  \
G1 X-27.76 Y-19.45 Z0.0;  \
G1 X-27.56 Y-19.48 Z0.0;  \
G1 X-27.49 Y-19.52 Z0.0;  \
G1 X-27.42 Y-19.64 Z0.0;  \
G1 X-27.41 Y-19.72 Z0.0;  \
G1 X-27.44 Y-19.86 Z0.0;  \
G1 X-27.49 Y-19.92 Z0.0;  \
G1 X-27.61 Y-19.98 Z0.0;  \
G1 X-27.76 Y-20.0 Z0.0;  \
G1 X-29.72 Y-20.0 Z0.0;  \
G1 X-29.93 Y-19.97 Z0.0;  \
G1 X-30.0 Y-19.92 Z0.0;  \
G1 X-30.07 Y-19.8 Z0.0;  \
G1 X-30.08 Y-19.72 Z0.0;  \
G1 X-30.05 Y-19.58 Z0.0;  \
G1 X-30.0 Y-19.52 Z0.0;  \
G1 X-29.88 Y-19.46 Z0.0;  \
G1 X-29.72 Y-19.45 Z0.0;  \
G1 X-29.37 Y-19.45 Z0.0;  \
G1 X-26.97 Y-12.99 Z0.0;  \
G1 X-28.56 Y-12.99 Z0.0;  \
G1 X-28.77 Y-12.96 Z0.0;  \
G1 X-28.84 Y-12.92 Z0.0;  \
G1 X-28.91 Y-12.8 Z0.0;  \
G1 X-28.92 Y-12.72 Z0.0;  \
G1 X-28.89 Y-12.57 Z0.0;  \
G1 X-28.84 Y-12.52 Z0.0;  \
G1 X-28.72 Y-12.46 Z0.0;  \
G1 X-28.56 Y-12.44 Z0.0;  \
G1 X-25.78 Y-12.44 Z0.0;  \
G1 X-23.16 Y-19.45 Z0.0;  \
G1 X-22.83 Y-19.45 Z0.0;  \
G1 X-22.62 Y-19.48 Z0.0;  \
G1 X-22.55 Y-19.52 Z0.0;  \
G1 X-22.48 Y-19.64 Z0.0;  \
G1 X-22.47 Y-19.72 Z0.0;  \
G1 X-22.5 Y-19.86 Z0.0;  \
G1 X-22.55 Y-19.92 Z0.0;  \
G1 X-22.67 Y-19.98 Z0.0;  \
G1 X-22.83 Y-20.0 Z0.0;  \
G1 X-24.76 Y-20.0 Z0.0;  \
G1 X-24.96 Y-19.97 Z0.0;  \
G1 X-25.03 Y-19.92 Z0.0;  \
G1 X-25.11 Y-19.8 Z0.0;  \
G1 X-25.12 Y-19.72 Z0.0;  \
G1 X-25.08 Y-19.58 Z0.0;  \
G1 X-25.03 Y-19.52 Z0.0;  \
G1 X-24.92 Y-19.47 Z0.0;  \
G1 X-24.76 Y-19.45 Z0.0;  \
G1 X-23.75 Y-19.45 Z0.0;  \
G1 X-24.48 Y-17.49 Z0.0;  \
G1 X-28.06 Y-17.49 Z0.0;  \
G1 X-28.78 Y-19.45 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X-26.39 Y-12.99;  \
G0 Z4.0;  \
G1 X-26.39 Y-12.99 Z0.0;  \
G1 X-27.85 Y-16.94 Z0.0;  \
G1 X-24.69 Y-16.94 Z0.0;  \
G1 X-26.16 Y-12.99 Z0.0;  \
G1 X-26.39 Y-12.99 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X-20.0 Y-20.0;  \
G0 Z4.0;  \
G1 X-20.0 Y-20.0 Z0.0;  \
G1 X-11.53 Y-6.28 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X-8.0 Y-20.0;  \
G0 Z4.0;  \
G1 X-8.0 Y-20.0 Z0.0;  \
G1 X4.0 Y-20.0 Z0.0;  \
G1 X4.0 Y-7.0 Z0.0;  \
G1 X-8.0 Y-7.0 Z0.0;  \
G1 X-8.0 Y-20.0 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X22.0 Y-13.0;  \
G0 Z4.0;  \
G1 X22.0 Y-13.0 Z0.0;  \
G3 I-7.0 J0.0 K0.0 X22.0 Y-13.0 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X37.49 Y-8.74;  \
G0 Z4.0;  \
G1 X37.49 Y-8.74 Z0.0;  \
G3 I-2.89 J-5.26 K0.0 X38.03 Y-18.92 Z0.0;  \
G0 Z6.0"

expected_moves_test03 = "G0 Z6.0;  \
G0 X37.49 Y-8.74;  \
G0 Z4.0;  \
G1 X37.49 Y-8.74 Z0.0;  \
G3 I-2.89 J-5.26 K0.0 X38.03 Y-18.92 Z0.0;  \
G0 Z6.0"

expected_moves_test04 = "G0 Z6.0;  \
G0 X-28.78 Y-19.45;  \
G0 Z4.0;  \
G1 X-28.78 Y-19.45 Z0.0;  \
G1 X-27.76 Y-19.45 Z0.0;  \
G1 X-27.56 Y-19.48 Z0.0;  \
G1 X-27.49 Y-19.52 Z0.0;  \
G1 X-27.42 Y-19.64 Z0.0;  \
G1 X-27.41 Y-19.72 Z0.0;  \
G1 X-27.44 Y-19.86 Z0.0;  \
G1 X-27.49 Y-19.92 Z0.0;  \
G1 X-27.61 Y-19.98 Z0.0;  \
G1 X-27.76 Y-20.0 Z0.0;  \
G1 X-29.72 Y-20.0 Z0.0;  \
G1 X-29.93 Y-19.97 Z0.0;  \
G1 X-30.0 Y-19.92 Z0.0;  \
G1 X-30.07 Y-19.8 Z0.0;  \
G1 X-30.08 Y-19.72 Z0.0;  \
G1 X-30.05 Y-19.58 Z0.0;  \
G1 X-30.0 Y-19.52 Z0.0;  \
G1 X-29.88 Y-19.46 Z0.0;  \
G1 X-29.72 Y-19.45 Z0.0;  \
G1 X-29.37 Y-19.45 Z0.0;  \
G1 X-26.97 Y-12.99 Z0.0;  \
G1 X-28.56 Y-12.99 Z0.0;  \
G1 X-28.77 Y-12.96 Z0.0;  \
G1 X-28.84 Y-12.92 Z0.0;  \
G1 X-28.91 Y-12.8 Z0.0;  \
G1 X-28.92 Y-12.72 Z0.0;  \
G1 X-28.89 Y-12.57 Z0.0;  \
G1 X-28.84 Y-12.52 Z0.0;  \
G1 X-28.72 Y-12.46 Z0.0;  \
G1 X-28.56 Y-12.44 Z0.0;  \
G1 X-25.78 Y-12.44 Z0.0;  \
G1 X-23.16 Y-19.45 Z0.0;  \
G1 X-22.83 Y-19.45 Z0.0;  \
G1 X-22.62 Y-19.48 Z0.0;  \
G1 X-22.55 Y-19.52 Z0.0;  \
G1 X-22.48 Y-19.64 Z0.0;  \
G1 X-22.47 Y-19.72 Z0.0;  \
G1 X-22.5 Y-19.86 Z0.0;  \
G1 X-22.55 Y-19.92 Z0.0;  \
G1 X-22.67 Y-19.98 Z0.0;  \
G1 X-22.83 Y-20.0 Z0.0;  \
G1 X-24.76 Y-20.0 Z0.0;  \
G1 X-24.96 Y-19.97 Z0.0;  \
G1 X-25.03 Y-19.92 Z0.0;  \
G1 X-25.11 Y-19.8 Z0.0;  \
G1 X-25.12 Y-19.72 Z0.0;  \
G1 X-25.08 Y-19.58 Z0.0;  \
G1 X-25.03 Y-19.52 Z0.0;  \
G1 X-24.92 Y-19.47 Z0.0;  \
G1 X-24.76 Y-19.45 Z0.0;  \
G1 X-23.75 Y-19.45 Z0.0;  \
G1 X-24.48 Y-17.49 Z0.0;  \
G1 X-28.06 Y-17.49 Z0.0;  \
G1 X-28.78 Y-19.45 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X-26.39 Y-12.99;  \
G0 Z4.0;  \
G1 X-26.39 Y-12.99 Z0.0;  \
G1 X-27.85 Y-16.94 Z0.0;  \
G1 X-24.69 Y-16.94 Z0.0;  \
G1 X-26.16 Y-12.99 Z0.0;  \
G1 X-26.39 Y-12.99 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X-20.0 Y-20.0;  \
G0 Z4.0;  \
G1 X-20.0 Y-20.0 Z0.0;  \
G1 X-11.53 Y-6.28 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X-8.0 Y-20.0;  \
G0 Z4.0;  \
G1 X-8.0 Y-20.0 Z0.0;  \
G1 X4.0 Y-20.0 Z0.0;  \
G1 X4.0 Y-7.0 Z0.0;  \
G1 X-8.0 Y-7.0 Z0.0;  \
G1 X-8.0 Y-20.0 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X22.0 Y-13.0;  \
G0 Z4.0;  \
G1 X22.0 Y-13.0 Z0.0;  \
G3 I-7.0 J0.0 K0.0 X22.0 Y-13.0 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X37.49 Y-8.74;  \
G0 Z4.0;  \
G1 X37.49 Y-8.74 Z0.0;  \
G3 I-2.89 J-5.26 K0.0 X38.03 Y-18.92 Z0.0;  \
G0 Z6.0"

expected_moves_test05 = "G0 Z6.0;  \
G0 X37.49 Y-8.74;  \
G0 Z4.0;  \
G1 X37.49 Y-8.74 Z0.0;  \
G3 I-2.89 J-5.26 K0.0 X38.03 Y-18.92 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X-28.78 Y-19.45;  \
G0 Z4.0;  \
G1 X-28.78 Y-19.45 Z0.0;  \
G1 X-27.76 Y-19.45 Z0.0;  \
G1 X-27.56 Y-19.48 Z0.0;  \
G1 X-27.49 Y-19.52 Z0.0;  \
G1 X-27.42 Y-19.64 Z0.0;  \
G1 X-27.41 Y-19.72 Z0.0;  \
G1 X-27.44 Y-19.86 Z0.0;  \
G1 X-27.49 Y-19.92 Z0.0;  \
G1 X-27.61 Y-19.98 Z0.0;  \
G1 X-27.76 Y-20.0 Z0.0;  \
G1 X-29.72 Y-20.0 Z0.0;  \
G1 X-29.93 Y-19.97 Z0.0;  \
G1 X-30.0 Y-19.92 Z0.0;  \
G1 X-30.07 Y-19.8 Z0.0;  \
G1 X-30.08 Y-19.72 Z0.0;  \
G1 X-30.05 Y-19.58 Z0.0;  \
G1 X-30.0 Y-19.52 Z0.0;  \
G1 X-29.88 Y-19.46 Z0.0;  \
G1 X-29.72 Y-19.45 Z0.0;  \
G1 X-29.37 Y-19.45 Z0.0;  \
G1 X-26.97 Y-12.99 Z0.0;  \
G1 X-28.56 Y-12.99 Z0.0;  \
G1 X-28.77 Y-12.96 Z0.0;  \
G1 X-28.84 Y-12.92 Z0.0;  \
G1 X-28.91 Y-12.8 Z0.0;  \
G1 X-28.92 Y-12.72 Z0.0;  \
G1 X-28.89 Y-12.57 Z0.0;  \
G1 X-28.84 Y-12.52 Z0.0;  \
G1 X-28.72 Y-12.46 Z0.0;  \
G1 X-28.56 Y-12.44 Z0.0;  \
G1 X-25.78 Y-12.44 Z0.0;  \
G1 X-23.16 Y-19.45 Z0.0;  \
G1 X-22.83 Y-19.45 Z0.0;  \
G1 X-22.62 Y-19.48 Z0.0;  \
G1 X-22.55 Y-19.52 Z0.0;  \
G1 X-22.48 Y-19.64 Z0.0;  \
G1 X-22.47 Y-19.72 Z0.0;  \
G1 X-22.5 Y-19.86 Z0.0;  \
G1 X-22.55 Y-19.92 Z0.0;  \
G1 X-22.67 Y-19.98 Z0.0;  \
G1 X-22.83 Y-20.0 Z0.0;  \
G1 X-24.76 Y-20.0 Z0.0;  \
G1 X-24.96 Y-19.97 Z0.0;  \
G1 X-25.03 Y-19.92 Z0.0;  \
G1 X-25.11 Y-19.8 Z0.0;  \
G1 X-25.12 Y-19.72 Z0.0;  \
G1 X-25.08 Y-19.58 Z0.0;  \
G1 X-25.03 Y-19.52 Z0.0;  \
G1 X-24.92 Y-19.47 Z0.0;  \
G1 X-24.76 Y-19.45 Z0.0;  \
G1 X-23.75 Y-19.45 Z0.0;  \
G1 X-24.48 Y-17.49 Z0.0;  \
G1 X-28.06 Y-17.49 Z0.0;  \
G1 X-28.78 Y-19.45 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X-26.39 Y-12.99;  \
G0 Z4.0;  \
G1 X-26.39 Y-12.99 Z0.0;  \
G1 X-27.85 Y-16.94 Z0.0;  \
G1 X-24.69 Y-16.94 Z0.0;  \
G1 X-26.16 Y-12.99 Z0.0;  \
G1 X-26.39 Y-12.99 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X-20.0 Y-20.0;  \
G0 Z4.0;  \
G1 X-20.0 Y-20.0 Z0.0;  \
G1 X-11.53 Y-6.28 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X-8.0 Y-20.0;  \
G0 Z4.0;  \
G1 X-8.0 Y-20.0 Z0.0;  \
G1 X4.0 Y-20.0 Z0.0;  \
G1 X4.0 Y-7.0 Z0.0;  \
G1 X-8.0 Y-7.0 Z0.0;  \
G1 X-8.0 Y-20.0 Z0.0;  \
G0 Z6.0;  \
G0 Z6.0;  \
G0 X22.0 Y-13.0;  \
G0 Z4.0;  \
G1 X22.0 Y-13.0 Z0.0;  \
G3 I-7.0 J0.0 K0.0 X22.0 Y-13.0 Z0.0;  \
G0 Z6.0"
