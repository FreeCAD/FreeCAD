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
from CAMTests.PathTestUtils import PathTestBase

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
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        cls.doc = FreeCAD.open(FreeCAD.getHomePath() + "Mod/CAM/CAMTests/test_adaptive.fcstd")
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

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
    def testFaceSingleSimple(self):
        """testFaceSingleSimple() Verify path generated on Face3."""

        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [(self.doc.Fusion, ["Face3"])]  # (base, subs_list)
        adaptive.Label = "testFaceSingleSimple+"
        adaptive.Comment = "testFaceSingleSimple() Verify path generated on Face3."

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
        self.assertTrue(len(adaptive.Path.Commands) > 100, "Command count not greater than 100.")

    def testFacesMergedDifferentZ(self):
        """testFacesMergedDifferentZ() Verify path generated on adjacent, combined
        Face3 and Face10.  The Z heights are different."""

        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [(self.doc.Fusion, ["Face3", "Face10"])]  # (base, subs_list)
        adaptive.Label = "testFacesMergedDifferentZ+"
        adaptive.Comment = "testFacesMergedDifferentZ() Verify path generated on adjacent, combined Face3 and Face10. The Z heights are different. UseOutline = False"

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

        self.assertTrue(len(adaptive.Path.Commands) > 100, "Command count not greater than 100.")

    def testFacesMergedDifferentZUseOutline(self):
        """testFacesMergedDifferentZUseOutline() Verify path generated on adjacent, combined Face3 and Face10.
        The Z heights are different."""

        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [(self.doc.Fusion, ["Face3", "Face10"])]  # (base, subs_list)
        adaptive.Label = "testFacesMergedDifferentZUseOutline+"
        adaptive.Comment = "testFacesMergedDifferentZUseOutline() Verify path generated on adjacent, combined Face3 and Face10.  The Z heights are different. UseOutline = True."

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

        self.assertTrue(len(adaptive.Path.Commands) > 100, "Command count not greater than 100.")

    def testOutlineDifferentZDiscontinuousEdges(self):
        """testOutlineDifferentZDiscontinuous() Verify path generated non-closed edges with differing Z-heights that are closed with Z=1 projection: "Edge9", "Edge2", "Edge8", "Edge15", "Edge30", "Edge31", "Edge29", "Edge19"."""

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
        adaptive.Label = "testOutlineDifferentZDiscontinuous+"
        adaptive.Comment = 'testOutlineDifferentZDiscontinuous() Verify path generated non-closed edges with differing Z-heights that are closed with Z=1 projection: "Edge9", "Edge2", "Edge8", "Edge15", "Edge30", "Edge31", "Edge29", "Edge19".'

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

        self.assertTrue(len(adaptive.Path.Commands) > 100, "Command count not greater than 100.")

    def testOutlineDifferentZContinuousEdges(self):
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
        adaptive.Label = "testOutlineDifferentZContinuous+"
        adaptive.Comment = 'testOutlineDifferentZContinuous() Verify path generated closed wire with differing Z-heights: "Edge13", "Edge7", "Edge9", "Edge2", "Edge8", "Edge15", "Edge30", "Edge31", "Edge29", "Edge19".'

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

        self.assertTrue(len(adaptive.Path.Commands) > 100, "Command count not greater than 100.")

    def testOutlineWithCutout(self):
        """testOutlineWithCutout() Verify path generated with outer and inner edge loops at same Z height: "Edge15", "Edge30", "Edge31", "Edge29", "Edge19", "Edge18", "Edge35", "Edge32", "Edge34", "Edge33"."""

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
        adaptive.Label = "testOutlineWithCutout+"
        adaptive.Comment = 'testOutlineWithCutout() Verify path generated with outer and inner edge loops at same Z height: "Edge15", "Edge30", "Edge31", "Edge29", "Edge19", "Edge18", "Edge35", "Edge32", "Edge34", "Edge33".'

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
        self.assertTrue(len(adaptive.Path.Commands) > 100, "Command count not greater than 100.")

        # Check if any paths originate inside inner hole of donut.  They should not.
        isInBox = False
        edges = [
            self.doc.Fusion.Shape.getElement(e) for e in ["Edge35", "Edge32", "Edge33", "Edge34"]
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

    def testFaceWithCutout(self):
        """testFaceWithCutout() Verify path generated on donut-shaped Face10."""

        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [(self.doc.Fusion, ["Face10"])]  # (base, subs_list)
        adaptive.Label = "testFaceWithCutout+"
        adaptive.Comment = "testFaceWithCutout() Verify path generated on donut-shaped Face10."

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

        self.assertTrue(len(adaptive.Path.Commands) > 100, "Command count not greater than 100.")

        # Check if any paths originate inside inner hole of donut.  They should not.
        isInBox = False
        edges = [
            self.doc.Fusion.Shape.getElement(e) for e in ["Edge35", "Edge32", "Edge33", "Edge34"]
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
            self.doc.Fusion.Shape.getElement(e) for e in ["Edge35", "Edge32", "Edge33", "Edge34"]
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

    def testModelStockAwareness(self):
        """testModelStockAwareness() Tests stock awareness- avoids cutting into the model regardless
        of bounding box selected."""
        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [(self.doc.Fusion, ["Face3", "Face10"])]  # (base, subs_list)
        adaptive.Label = "testModelStockAwareness+"
        adaptive.Comment = "testModelStockAwareness() Verify path generated on adjacent, combined Face3 and Face10.  The Z heights are different. Result should be the combination at Z=10 (faces from (0,0) to (40,25), minus tool radius), and only the lower face at Z=5: (15,0) to (40,25)."

        # Set additional operation properties
        setDepthsAndHeights(adaptive, 15, 0)
        adaptive.FinishingProfile = False
        adaptive.HelixAngle = 75.0
        adaptive.HelixDiameterLimit.Value = 1.0
        adaptive.LiftDistance.Value = 1.0
        adaptive.StepOver = 75
        adaptive.UseOutline = False
        adaptive.setExpression("StepDown", None)
        adaptive.StepDown.Value = (
            5.0  # Have to set expression to None before numerical value assignment
        )
        # Don't use helix entry- ensures helix moves are counted in the path
        # boundary calculation. This should be unnecessary, as the helices are
        # grown out of the cut area, and thus must be inside of it.
        adaptive.UseHelixArcs = False

        _addViewProvider(adaptive)
        self.doc.recompute()

        # Check:
        # - Bounding box at Z=10 stays within Face3 and Face10- so -X for Face3,
        # +X and +/-Y for Face10
        # - bounding box at Z=5 stays within Face10
        # - No toolpaths at Z=0

        paths = [c for c in adaptive.Path.Commands if c.Name in ["G0", "G00", "G1", "G01"]]
        toolr = adaptive.OpToolDiameter.Value / 2
        tol = adaptive.Tolerance

        # Make clean up math below- combine tool radius and tolerance into a
        # single field that can be added/subtracted to/from bounding boxes
        moffset = toolr - tol

        zDict = getPathBoundaries(paths, [10, 5, 0])

        # NOTE: Face3 is at Z=10, Face10 is at Z=5
        bbf3 = self.doc.Fusion.Shape.getElement("Face3").BoundBox
        bbf10 = self.doc.Fusion.Shape.getElement("Face10").BoundBox

        okAt10 = (
            zDict[10] is not None
            and zDict[10]["min"][0] >= bbf3.XMin + moffset
            and zDict[10]["min"][1] >= bbf10.YMin + moffset
            and zDict[10]["max"][0] <= bbf10.XMax - moffset
            and zDict[10]["max"][1] <= bbf10.YMax - moffset
        )

        okAt5 = (
            zDict[5] is not None
            and zDict[5]["min"][0] >= bbf10.XMin + moffset
            and zDict[5]["min"][1] >= bbf10.YMin + moffset
            and zDict[5]["max"][0] < bbf10.XMax - moffset
            and zDict[5]["max"][1] < bbf10.YMax - moffset
        )

        okAt0 = not zDict[0]

        self.assertTrue(okAt10 and okAt5 and okAt0, "Path boundaries outside of expected regions")

    def testZStockToLeave(self):
        """testZStockToLeave() Tests Z stock to leave- with 1mm Z stock to leave, machining
        at the top of the model should not touch the top model face"""
        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [(self.doc.Fusion, ["Face3", "Face10"])]  # (base, subs_list)
        adaptive.Label = "testZStockToLeave+"
        adaptive.Comment = "testZStockToLeave() Verify Z stock is left as requested"

        # Set additional operation properties
        setDepthsAndHeights(adaptive, 15, 10)
        adaptive.FinishingProfile = False
        adaptive.HelixAngle = 75.0
        adaptive.HelixDiameterLimit.Value = 1.0
        adaptive.LiftDistance.Value = 1.0
        adaptive.StepOver = 75
        adaptive.UseOutline = False
        adaptive.setExpression("StepDown", None)
        adaptive.StepDown.Value = (
            5.0  # Have to set expression to None before numerical value assignment
        )
        # Add some Z stock to leave so we avoid Face3 in this stepdown at Z=10
        adaptive.setExpression("ZStockToLeave", None)
        adaptive.ZStockToLeave.Value = 1

        _addViewProvider(adaptive)
        self.doc.recompute()

        # Check:
        # - No feed path at depth Z=10 touchs Face3
        toolr = adaptive.OpToolDiameter.Value / 2
        tol = adaptive.Tolerance

        # Make clean up math below- combine tool radius and tolerance into a
        # single field that can be added/subtracted to/from bounding boxes
        moffset = toolr - tol

        # Offset the face we don't expect to touch, verify no move is within
        # that boundary
        # NOTE: This isn't a perfect test (won't catch moves that start and end
        # outside of our face, but cut through/across it), but combined with
        # other tests should be sufficient.
        noPathTouchesFace3 = True
        foffset = self.doc.Fusion.Shape.getElement("Face3").makeOffset2D(moffset)
        # NOTE: Face3 is at Z=10, and the only feed moves will be at Z=10
        lastpt = FreeCAD.Vector(0, 0, 10)
        for p in [c.Parameters for c in adaptive.Path.Commands if c.Name in ["G1", "G01"]]:
            pt = FreeCAD.Vector(lastpt)
            if "X" in p:
                pt.x = p.get("X")
            if "Y" in p:
                pt.x = p.get("Y")

            if foffset.isInside(pt, 0.001, True):
                noPathTouchesFace3 = False
                break

            lastpt = pt

        self.assertTrue(noPathTouchesFace3, "No feed moves within the top face.")

    def testFullModelAdaptiveRoughing(self):
        """testFullModelAdaptiveRoughing() Tests full roughing- should machine entire model with no inputs"""
        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [(self.doc.Fusion, [])]  # (base, subs_list)
        adaptive.Label = "testFullModelAdaptiveRoughing+"
        adaptive.Comment = (
            "testFullModelAdaptiveRoughing() Verify path generated with no subs roughs entire model"
        )

        # Set additional operation properties
        setDepthsAndHeights(adaptive, 15, 0)
        adaptive.FinishingProfile = False
        adaptive.HelixAngle = 75.0
        adaptive.HelixDiameterLimit.Value = 1.0
        adaptive.LiftDistance.Value = 1.0
        adaptive.StepOver = 75
        adaptive.UseOutline = False
        adaptive.setExpression("StepDown", None)
        adaptive.StepDown.Value = (
            5.0  # Have to set expression to None before numerical value assignment
        )
        # Don't use helix entry- ensures helix moves are counted in the path
        # boundary calculation. This should be unnecessary, as the helices are
        # grown out of the cut area, and thus must be inside of it.
        adaptive.UseHelixArcs = False

        _addViewProvider(adaptive)
        self.doc.recompute()

        # Check:
        # - Bounding box at Z=0 goes outside the model box + tool diameter
        # (has to profile the model)
        # - Bounding box at Z=5 should go past the model in -X, but only up to the
        # stock edges in +X and Y
        # - Bounding box at Z=10 goes to at least stock bounding box edges,
        # minus tool diameter (has to machine the entire top of the stock off)
        # - [Should maybe check] At least one move Z = [10,5] is within the model
        # - [Should maybe check] No moves at Z = 0 are within the model

        paths = [c for c in adaptive.Path.Commands if c.Name in ["G0", "G00", "G1", "G01"]]
        toolr = adaptive.OpToolDiameter.Value / 2
        tol = adaptive.Tolerance

        # Make clean up math below- combine tool radius and tolerance into a
        # single field that can be added/subtracted to/from bounding boxes
        moffset = toolr - tol

        zDict = getPathBoundaries(paths, [10, 5, 0])
        mbb = self.doc.Fusion.Shape.BoundBox
        sbb = adaptive.Document.Stock.Shape.BoundBox

        okAt10 = (
            zDict[10] is not None
            and zDict[10]["min"][0] <= sbb.XMin + moffset
            and zDict[10]["min"][1] <= sbb.YMin + moffset
            and zDict[10]["max"][0] >= sbb.XMax - moffset
            and zDict[10]["max"][1] >= sbb.YMax - moffset
        )

        okAt5 = (
            zDict[5] is not None
            and zDict[5]["min"][0] <= mbb.XMin - moffset
            and zDict[5]["min"][1] <= sbb.YMin + moffset
            and zDict[5]["max"][0] >= sbb.XMax - moffset
            and zDict[5]["max"][1] >= sbb.YMax - moffset
        )

        okAt0 = (
            zDict[0] is not None
            and zDict[0]["min"][0] <= mbb.XMin - moffset
            and zDict[0]["min"][1] <= mbb.YMin - moffset
            and zDict[0]["max"][0] >= mbb.XMax + moffset
            and zDict[0]["max"][1] >= mbb.YMax + moffset
        )

        self.assertTrue(
            okAt10 and okAt5 and okAt0, "Path boundaries don't include expected regions"
        )

    def testStockLimitsAwareness(self):
        """testStockLimitsAwareness() Tests stock handling- should rough full model, but not cut
        air excessively where there's not stock"""
        # Instantiate a Adaptive operation and set Base Geometry
        adaptive = PathAdaptive.Create("Adaptive")
        adaptive.Base = [(self.doc.Fusion, [])]  # (base, subs_list)
        adaptive.Label = "testStockLimitsAwareness+"
        adaptive.Comment = (
            "testStockLimitsAwareness() Verify machining region is limited to the stock"
        )

        # Set additional operation properties
        setDepthsAndHeights(adaptive, 15, 5)
        adaptive.FinishingProfile = False
        adaptive.HelixAngle = 75.0
        adaptive.HelixDiameterLimit.Value = 1.0
        adaptive.LiftDistance.Value = 1.0
        adaptive.StepOver = 75
        adaptive.UseOutline = False
        adaptive.setExpression("StepDown", None)
        adaptive.StepDown.Value = (
            5.0  # Have to set expression to None before numerical value assignment
        )
        # Don't use helix entry- ensures helix moves are counted in the path
        # boundary calculation. This should be unnecessary, as the helices are
        # grown out of the cut area, and thus must be inside of it.
        adaptive.UseHelixArcs = False

        # Create and assign new stock that will create different bounds at
        # different stepdowns
        btall = Part.makeBox(17, 27, 11, FreeCAD.Vector(-1, -1, 0))
        bshort = Part.makeBox(42, 27, 6, FreeCAD.Vector(-1, -1, 0))
        adaptive.Document.Job.Stock.Shape = btall.fuse(bshort)

        _addViewProvider(adaptive)
        # NOTE: Do NOT recompute entire doc, which will undo our stock change!
        adaptive.recompute()

        # Check:
        # - Bounding box at Z=10 stays basically above "btall"
        # - Bounding box at Z=5 and Z=0 are outside of stock

        paths = [c for c in adaptive.Path.Commands if c.Name in ["G1", "G01"]]
        toolr = adaptive.OpToolDiameter.Value / 2
        tol = adaptive.Tolerance

        # Make clean up math below- combine tool radius and tolerance into a
        # single field that can be added/subtracted to/from bounding boxes
        # NOTE: ADD tol here, since we're effectively flipping our normal
        # comparison and want tolerance to make our check looser
        moffset = toolr + tol

        zDict = getPathBoundaries(paths, [10, 5])
        sbb = adaptive.Document.Stock.Shape.BoundBox
        sbb10 = btall.BoundBox

        # These should be no more than a tool radius outside of the "btall"
        # XY section of the stock
        okAt10 = (
            zDict[10] is not None
            and zDict[10]["min"][0] >= sbb10.XMin - moffset
            and zDict[10]["min"][1] >= sbb10.YMin - moffset
            and zDict[10]["max"][0] <= sbb10.XMax + moffset
            and zDict[10]["max"][1] <= sbb10.YMax + moffset
        )

        # These should be no more than a tool radius outside of the overall
        # stock bounding box
        okAt5 = (
            zDict[5] is not None
            and zDict[5]["min"][0] >= sbb.XMin - moffset
            and zDict[5]["min"][1] >= sbb.YMin - moffset
            and zDict[5]["max"][0] <= sbb.XMax + moffset
            and zDict[5]["max"][1] <= sbb.YMax + moffset
        )

        self.assertTrue(okAt10 and okAt5, "Path feeds extend excessively in +X")

    # POSSIBLY MISSING TESTS:
    # - Something for region ordering
    # - Known-edge cases: cones/spheres/cylinders (especially partials on edges
    # of model + strange angles- especially for cylinders)
    # - Multiple models/stock
    # - XY stock to leave


# Eclass


def getPathBoundaries(paths, zLevels):
    """getPathBoundaries(paths, zLevels): Takes the list of paths and list of Z
    depths of interest, and finds the bounding box of the paths at each depth.
    A dictionary of depth: {"min": (x,y), "max": (x,y)} entries is returned.

    NOTE: You'd think that using Path.BoundBox would give us what we want,
    but... no, for whatever reason it appears to always extend to (0,0,0)
    """
    last = FreeCAD.Vector(0.0, 0.0, 0.0)
    # First make sure each element has X, Y, and Z coordinates
    for p in paths:
        params = p.Parameters
        last.x = p.X if "X" in params else last.x
        last.y = p.Y if "Y" in params else last.y
        last.z = p.Z if "Z" in params else last.z

        p.X = last.x
        p.Y = last.y
        p.Z = last.z

    zDict = {}
    for z in zLevels:
        zpaths = [k for k in paths if k.Z == z]
        if not zpaths:
            zDict[z] = None
            continue
        xmin = min([k.X for k in zpaths])
        xmax = max([k.X for k in zpaths])
        ymin = min([k.Y for k in zpaths])
        ymax = max([k.Y for k in zpaths])
        zDict[z] = {"min": (xmin, ymin), "max": (xmax, ymax)}

    return zDict


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

    # NOTE: Can NOT just check "if p.get("X")" or similar- that chokes when X is
    # zero. That becomes especially obvious when Z=0, and moves end up on the
    # wrong depth
    gcode_list = list()
    last = FreeCAD.Vector(0.0, 0.0, 0.0)
    for c in cmdList:
        p = c.Parameters
        name = c.Name
        if (includeRapids and name in ["G0", "G00"]) or (includeLines and name in ["G1", "G01"]):
            gcode = name
            x = last.x
            y = last.y
            z = last.z
            if "X" in p:
                x = round(p["X"], 2)
            gcode += " X" + str(x)
            if "Y" in p:
                y = round(p["Y"], 2)
            gcode += " Y" + str(y)
            if "Z" in p:
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
            if "I" in p:
                i = round(p["I"], 2)
            gcode += " I" + str(i)
            if "J" in p:
                j = round(p["J"], 2)
            gcode += " J" + str(j)
            if "K" in p:
                k = round(p["K"], 2)
            gcode += " K" + str(k)

            if "X" in p:
                x = round(p["X"], 2)
            gcode += " X" + str(x)
            if "Y" in p:
                y = round(p["Y"], 2)
            gcode += " Y" + str(y)
            if "Z" in p:
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
        if "X" in p and "Y" in p:
            x = p.get("X")
            y = p.get("Y")
            if x > minPoint.x and y > minPoint.y and x < maxPoint.x and y < maxPoint.y:
                return True
    return False


def _addViewProvider(adaptiveOp):
    if FreeCAD.GuiUp:
        PathOpGui = PathAdaptiveGui.PathOpGui
        cmdRes = PathAdaptiveGui.Command.res
        adaptiveOp.ViewObject.Proxy = PathOpGui.ViewProvider(adaptiveOp.ViewObject, cmdRes)


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
