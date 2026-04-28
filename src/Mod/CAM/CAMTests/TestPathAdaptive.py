# SPDX-License-Identifier: LGPL-2.1-or-later

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
import area
import math
import time

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
        cls.test_times = {}  # For tracking per-test execution time

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

        # Print test timing report
        if cls.test_times:
            print("\n" + "=" * 70)
            print("Test Execution Times:")
            print("=" * 70)

            sorted_times = sorted(cls.test_times.items(), key=lambda x: x[1], reverse=True)
            total_time = sum(cls.test_times.values())

            for test_name, elapsed in sorted_times:
                percentage = (elapsed / total_time * 100) if total_time > 0 else 0
                print(f"{elapsed:7.3f}s ({percentage:5.1f}%)  {test_name}")

            print("-" * 70)
            print(f"Total: {total_time:.3f}s")
            print("=" * 70)

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

        # Start timing this test
        self._test_start_time = time.time()

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        # Clean up scene graph visualization
        PathAdaptive.sceneClean()

        # Record elapsed time for this test
        elapsed = time.time() - self._test_start_time
        test_name = self.id().split(".")[-1]  # Get just the test method name
        self.__class__.test_times[test_name] = elapsed

    def checkAdaptiveErrors(self, adaptiveOutput):
        """Check error flags in C++ AdaptiveOutput object."""
        self.assertFalse(
            adaptiveOutput.StartPointNotFound, "Adaptive failed to find entry/start point"
        )
        self.assertFalse(
            adaptiveOutput.LeadPathFailed,
            "Adaptive failed to generate lead path - overtravel without reaching cleared area",
        )
        self.assertFalse(
            adaptiveOutput.UnexpectedRotateIterations,
            "Adaptive encountered unexpected number of rotation iterations",
        )
        self.assertFalse(
            adaptiveOutput.TooManyFailedEngagements,
            "Adaptive exceeded 10000 failed engagement attempts",
        )
        self.assertFalse(
            adaptiveOutput.UnclearedAreaRemains, "Adaptive terminated with uncleared area remaining"
        )
        self.assertFalse(
            adaptiveOutput.FailedToSetUpFinishingPass,
            "Adaptive failed to set up finishing pass - uncut area would make finishing pass too heavy",
        )
        self.assertFalse(
            adaptiveOutput.FinishingLeadInFailed,
            "Adaptive failed to generate lead-in for finishing pass",
        )

    def _createRectangleGeometry(self, stock_width, stock_height, path_width, path_height):
        """Create centered rectangular geometry for stock and path."""
        # Stock boundary at origin
        stock_x0 = 0.0
        stock_y0 = 0.0
        stock_x1 = stock_x0 + stock_width
        stock_y1 = stock_y0 + stock_height
        stockPath2d = [
            [[stock_x0, stock_y0], [stock_x1, stock_y0], [stock_x1, stock_y1], [stock_x0, stock_y1]]
        ]

        # Path centered within stock
        margin_x = (stock_width - path_width) / 2.0
        margin_y = (stock_height - path_height) / 2.0
        path_x0 = stock_x0 + margin_x
        path_y0 = stock_y0 + margin_y
        path_x1 = path_x0 + path_width
        path_y1 = path_y0 + path_height
        path2d = [[[path_x0, path_y0], [path_x1, path_y0], [path_x1, path_y1], [path_x0, path_y1]]]

        dimensions = {
            "stock_width": stock_width,
            "stock_height": stock_height,
            "path_width": path_width,
            "path_height": path_height,
            "stock_x0": stock_x0,
            "stock_y0": stock_y0,
            "stock_x1": stock_x1,
            "stock_y1": stock_y1,
            "path_x0": path_x0,
            "path_y0": path_y0,
            "path_x1": path_x1,
            "path_y1": path_y1,
        }

        return stockPath2d, path2d, dimensions

    def _executeAdaptive(self, opType, stockPath2d, path2d, clearedArea=None, **kwargs):
        """Create, configure, execute Adaptive2d and return total cleared area."""
        if clearedArea is None:
            clearedArea = []

        # Initialize scene graph for visualization if GUI is up
        PathAdaptive.initSceneGraph(z=10)

        # Create and configure Adaptive2d with defaults
        a2d = area.Adaptive2d()
        a2d.stepOverFactor = kwargs.get("stepOverFactor", 0.20)
        a2d.toolDiameter = kwargs.get("toolDiameter", 5.0)
        a2d.tolerance = kwargs.get("tolerance", 0.1)
        a2d.forceInsideOut = kwargs.get("forceInsideOut", False)
        a2d.finishingProfile = kwargs.get("finishingProfile", True)
        a2d.keepToolDownDistRatio = kwargs.get("keepToolDownDistRatio", 3.0)
        a2d.opType = opType

        # Create progress callback for visualization
        def progressFn(tpaths):
            PathAdaptive.renderProgressCallback(tpaths)
            return False  # Don't stop processing

        # Execute
        results = a2d.Execute(stockPath2d, path2d, clearedArea, progressFn)

        # Validate
        self.assertTrue(len(results) > 0, "Adaptive2d should return at least one result")
        for result in results:
            self.checkAdaptiveErrors(result)

        # Return total cleared area and the configured instance
        total_cleared = sum(r.ClearedArea for r in results)
        return total_cleared, a2d

    def _calculateCornerUnclearableArea(self, tool_diameter):
        """Calculate unclearable area in a single corner due to circular tool."""
        tool_radius = tool_diameter / 2.0
        return tool_radius**2 * (1 - math.pi / 4)

    def testClearInside(self):
        """testClearInside() Test C++ Adaptive2d clearing inside a simple rectangle."""
        # Create geometry
        stockPath2d, path2d, dims = self._createRectangleGeometry(50.0, 50.0, 40.0, 40.0)

        # Execute adaptive clearing
        total_cleared, a2d = self._executeAdaptive(
            area.AdaptiveOperationType.ClearingInside, stockPath2d, path2d
        )

        # Verify cleared area
        corner_unclearable_area = self._calculateCornerUnclearableArea(a2d.toolDiameter)
        total_unclearable_area = 4 * corner_unclearable_area
        expected_area = dims["path_width"] * dims["path_height"] - total_unclearable_area
        delta = corner_unclearable_area / 2.0
        self.assertAlmostEqual(
            total_cleared,
            expected_area,
            delta=delta,
            msg=f"Total cleared area {total_cleared} should be within {delta} of {expected_area}",
        )

    def testClearInsideWithRestMachining(self):
        """testClearInsideWithRestMachining() Test C++ Adaptive2d rest machining with pre-cleared area."""
        # Create geometry
        stockPath2d, path2d, dims = self._createRectangleGeometry(50.0, 50.0, 40.0, 40.0)

        # Pre-cleared area: bottom-left and top-right quadrants of stock
        quad_width = dims["stock_width"] / 2.0
        quad_height = dims["stock_height"] / 2.0

        # Bottom-left quadrant
        bl_x0 = dims["stock_x0"]
        bl_y0 = dims["stock_y0"]
        bl_x1 = bl_x0 + quad_width
        bl_y1 = bl_y0 + quad_height

        # Top-right quadrant
        tr_x0 = dims["stock_x0"] + quad_width
        tr_y0 = dims["stock_y0"] + quad_height
        tr_x1 = tr_x0 + quad_width
        tr_y1 = tr_y0 + quad_height

        clearedArea = [
            [
                [bl_x0, bl_y0],
                [bl_x1, bl_y0],
                [bl_x1, bl_y1],
                [bl_x0, bl_y1],
            ],
            [
                [tr_x0, tr_y0],
                [tr_x1, tr_y0],
                [tr_x1, tr_y1],
                [tr_x0, tr_y1],
            ],
        ]

        # Execute adaptive clearing with pre-cleared area
        total_cleared, a2d = self._executeAdaptive(
            area.AdaptiveOperationType.ClearingInside, stockPath2d, path2d, clearedArea
        )

        # Account for unclearable area in corners due to circular tool
        corner_unclearable_area = self._calculateCornerUnclearableArea(a2d.toolDiameter)
        # only 2 corners are in the pre-cleared area
        total_unclearable_area = 2 * corner_unclearable_area

        # Calculate overlap between path and pre-cleared areas (both quadrants)
        # Bottom-left quadrant overlap
        bl_overlap_x0 = max(dims["path_x0"], bl_x0)
        bl_overlap_y0 = max(dims["path_y0"], bl_y0)
        bl_overlap_x1 = min(dims["path_x1"], bl_x1)
        bl_overlap_y1 = min(dims["path_y1"], bl_y1)
        bl_overlap_width = max(0, bl_overlap_x1 - bl_overlap_x0)
        bl_overlap_height = max(0, bl_overlap_y1 - bl_overlap_y0)
        bl_overlap_area = bl_overlap_width * bl_overlap_height

        # Top-right quadrant overlap
        tr_overlap_x0 = max(dims["path_x0"], tr_x0)
        tr_overlap_y0 = max(dims["path_y0"], tr_y0)
        tr_overlap_x1 = min(dims["path_x1"], tr_x1)
        tr_overlap_y1 = min(dims["path_y1"], tr_y1)
        tr_overlap_width = max(0, tr_overlap_x1 - tr_overlap_x0)
        tr_overlap_height = max(0, tr_overlap_y1 - tr_overlap_y0)
        tr_overlap_area = tr_overlap_width * tr_overlap_height

        overlap_area = bl_overlap_area + tr_overlap_area

        # Expected area: path area - corners - pre-cleared overlap
        expected_area = (
            dims["path_width"] * dims["path_height"] - total_unclearable_area - overlap_area
        )
        # Use half the corner area as tolerance
        delta = corner_unclearable_area / 2.0
        self.assertAlmostEqual(
            total_cleared,
            expected_area,
            delta=delta,
            msg=f"Total cleared area {total_cleared} should be within {delta} of {expected_area}",
        )

    def testClearOutside(self):
        """testClearOutside() Test C++ Adaptive2d clearing outside a simple rectangle."""
        # Create geometry
        stockPath2d, path2d, dims = self._createRectangleGeometry(50.0, 50.0, 40.0, 40.0)

        # Execute adaptive clearing outside
        total_cleared, a2d = self._executeAdaptive(
            area.AdaptiveOperationType.ClearingOutside, stockPath2d, path2d
        )

        # Verify cleared area (stock minus path, no corner adjustment needed)
        expected_area = (dims["stock_width"] * dims["stock_height"]) - (
            dims["path_width"] * dims["path_height"]
        )
        corner_unclearable_area = self._calculateCornerUnclearableArea(a2d.toolDiameter)
        delta = corner_unclearable_area / 2.0
        self.assertAlmostEqual(
            total_cleared,
            expected_area,
            delta=delta,
            msg=f"Total cleared area {total_cleared} should be within {delta} of {expected_area}",
        )

    def testProfilingInside(self):
        """testProfilingInside() Test C++ Adaptive2d profiling inside a rectangle."""
        # Create geometry
        stockPath2d, path2d, dims = self._createRectangleGeometry(50.0, 50.0, 40.0, 40.0)

        # Execute adaptive profiling
        total_cleared, a2d = self._executeAdaptive(
            area.AdaptiveOperationType.ProfilingInside, stockPath2d, path2d, stepOverFactor=0.5
        )

        # Verify cleared area is appropriate for profiling between 2-3 tool diameters
        # Outer boundary is the path2d
        outer_area = dims["path_width"] * dims["path_height"]

        # Inner boundary at 2-3 tool diameters offset
        offset_2_diameters = 2 * a2d.toolDiameter
        inner_width_min = dims["path_width"] - 2 * offset_2_diameters
        inner_height_min = dims["path_height"] - 2 * offset_2_diameters
        inner_area_max = (
            inner_width_min * inner_height_min
            if inner_width_min > 0 and inner_height_min > 0
            else 0
        )

        offset_3_diameters = 3 * a2d.toolDiameter
        inner_width_max = dims["path_width"] - 2 * offset_3_diameters
        inner_height_max = dims["path_height"] - 2 * offset_3_diameters
        inner_area_min = (
            inner_width_max * inner_height_max
            if inner_width_max > 0 and inner_height_max > 0
            else 0
        )

        # Expected area range
        min_area = outer_area - inner_area_max  # 2 diameters deep
        max_area = outer_area - inner_area_min  # 3 diameters deep

        self.assertGreaterEqual(
            total_cleared,
            min_area,
            msg=f"Profiling cleared area {total_cleared} should be at least {min_area} (2 tool diameters deep)",
        )
        self.assertLessEqual(
            total_cleared,
            max_area,
            msg=f"Profiling cleared area {total_cleared} should be at most {max_area} (3 tool diameters deep)",
        )

    def testProfilingOutside(self):
        """testProfilingOutside() Test C++ Adaptive2d profiling outside a rectangle."""
        # Create geometry - use small inner path to allow full 2-3 diameter range
        stockPath2d, path2d, dims = self._createRectangleGeometry(50.0, 50.0, 15.0, 15.0)

        # Execute adaptive profiling
        total_cleared, a2d = self._executeAdaptive(
            area.AdaptiveOperationType.ProfilingOutside, stockPath2d, path2d, stepOverFactor=0.5
        )

        # Calculate expected area range for a profile 2-3 tool diameters wide
        # Inner boundary is the path2d
        inner_area = dims["path_width"] * dims["path_height"]

        # Outer boundary at 2 tool diameters offset (clamped to stock)
        offset_2_diameters = 2 * a2d.toolDiameter
        outer_width_min = min(dims["path_width"] + 2 * offset_2_diameters, dims["stock_width"])
        outer_height_min = min(dims["path_height"] + 2 * offset_2_diameters, dims["stock_height"])
        outer_area_min = outer_width_min * outer_height_min

        # Account for rounded outer corners (tool follows circular arc, not sharp corner)
        # Each outer corner: sharp corner = d², rounded = π*d²/4, difference = d²(4-π)/4
        outer_corner_rounding_2d = offset_2_diameters**2 * (1 - math.pi / 4)
        total_outer_rounding_2d = 4 * outer_corner_rounding_2d

        # Outer boundary at 3 tool diameters offset (clamped to stock)
        offset_3_diameters = 3 * a2d.toolDiameter
        outer_width_max = min(dims["path_width"] + 2 * offset_3_diameters, dims["stock_width"])
        outer_height_max = min(dims["path_height"] + 2 * offset_3_diameters, dims["stock_height"])
        outer_area_max = outer_width_max * outer_height_max

        outer_corner_rounding_3d = offset_3_diameters**2 * (1 - math.pi / 4)
        total_outer_rounding_3d = 4 * outer_corner_rounding_3d

        # Account for inner corners that can't be cleared from outside
        tool_radius = a2d.toolDiameter / 2.0
        inner_corner_unclearable = tool_radius**2 * (1 - math.pi / 4)
        total_inner_unclearable = 4 * inner_corner_unclearable

        # Expected area range
        min_area = outer_area_min - inner_area - total_outer_rounding_2d - total_inner_unclearable
        max_area = outer_area_max - inner_area - total_outer_rounding_3d

        self.assertGreaterEqual(
            total_cleared,
            min_area,
            msg=f"Profiling cleared area {total_cleared} should be at least {min_area} (2 tool diameters wide)",
        )
        self.assertLessEqual(
            total_cleared,
            max_area,
            msg=f"Profiling cleared area {total_cleared} should be at most {max_area} (3 tool diameters wide)",
        )

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
        adaptive.LiftDistance.Value = 1.0
        adaptive.StepOver = 75
        adaptive.UseOutline = False
        adaptive.setExpression("StepDown", None)
        adaptive.ModelAwareExperiment = True
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
        adaptive.LiftDistance.Value = 1.0
        adaptive.StepOver = 75
        adaptive.UseOutline = False
        adaptive.setExpression("StepDown", None)
        adaptive.ModelAwareExperiment = True
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
        adaptive.LiftDistance.Value = 1.0
        adaptive.StepOver = 75
        adaptive.UseOutline = False
        adaptive.setExpression("StepDown", None)
        adaptive.ModelAwareExperiment = True
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
        # - Bounding box at Z=10 does not cut the region to the right
        # - Bounding box at Z=5 and Z=0 are outside of stock

        paths = [c for c in adaptive.Path.Commands if c.Name in ["G1", "G01"]]
        toolr = adaptive.OpToolDiameter.Value / 2
        tol = adaptive.Tolerance

        # Make clean up math below- combine tool radius and tolerance into a
        # single field that can be added/subtracted to/from bounding boxes
        # NOTE: ADD tol here, since we're effectively flipping our normal
        # comparison and want tolerance to make our check looser
        moffset = toolr + tol

        # Update: I'm reducing the strictness of this check because adaptive
        #  is allowed to move the tool where there is no stock if it's convenient,
        #  and the original strictness caused problems. Still, it should not wander
        #  exceptionally far from the stock.
        moffset += toolr * 2

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
