# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2023 Robert Schöftner <rs@unfoo.net>                    *
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
import Path.Op.Profile as PathProfile
import Path.Main.Job as PathJob
from CAMTests.PathTestUtils import PathTestBase
from CAMTests.TestPathAdaptive import getGcodeMoves

if FreeCAD.GuiUp:
    import Path.Main.Gui.Job as PathJobGui
    import Path.Op.Gui.Profile as PathProfileGui


class TestPathProfile(PathTestBase):
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
        cls.doc = FreeCAD.open(FreeCAD.getHomePath() + "Mod/CAM/CAMTests/test_profile.fcstd")
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

        # Create Job object, adding geometry objects from file opened above
        cls.job = PathJob.Create("Job", [cls.doc.Body], None)
        cls.job.GeometryTolerance.Value = 0.001
        if FreeCAD.GuiUp:
            cls.job.ViewObject.Proxy = PathJobGui.ViewProvider(cls.job.ViewObject)

        # Instantiate an Profile operation for querying available properties
        cls.prototype = PathProfile.Create("Profile")
        cls.prototype.Base = [(cls.doc.Body, ["Face18"])]
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
        """test01() Verify path generated on Face18, outside, with tool compensation."""

        # Instantiate a Profile operation and set Base Geometry
        profile = PathProfile.Create("Profile1")
        profile.Base = [(self.doc.Body, ["Face18"])]  # (base, subs_list)
        profile.Label = "test01+"
        profile.Comment = (
            "test01() Verify path generated on Face18, outside, with tool compensation."
        )

        # Set additional operation properties
        # setDepthsAndHeights(adaptive)
        profile.processCircles = True
        profile.processHoles = True
        profile.UseComp = True
        profile.Direction = "CW"
        _addViewProvider(profile)
        self.doc.recompute()

        moves = getGcodeMoves(profile.Path.Commands, includeRapids=False)
        operationMoves = ";  ".join(moves)
        # FreeCAD.Console.PrintMessage("test01_moves: " + operationMoves + "\n")

        expected_moves = (
            "G1 X16.47 Y16.47 Z10.0;  G3 I-2.48 J-2.48 K0.0 X13.93 Y17.5 Z10.0;  "
            "G1 X-13.93 Y17.5 Z10.0;  G3 I-0.06 J-3.51 K0.0 X-17.5 Y13.93 Z10.0;  "
            "G1 X-17.5 Y-13.93 Z10.0;  G3 I3.51 J-0.06 K0.0 X-13.93 Y-17.5 Z10.0;  "
            "G1 X13.93 Y-17.5 Z10.0;  G3 I0.06 J3.51 K0.0 X17.5 Y-13.93 Z10.0;  "
            "G1 X17.5 Y13.93 Z10.0;  G3 I-3.51 J0.06 K0.0 X16.47 Y16.47 Z10.0;  "
            "G1 X23.54 Y23.54 Z10.0;  G2 I-9.55 J-9.55 K0.0 X27.5 Y14.0 Z10.0;  "
            "G1 X27.5 Y-14.0 Z10.0;  G2 I-13.5 J0.0 K0.0 X14.0 Y-27.5 Z10.0;  "
            "G1 X-14.0 Y-27.5 Z10.0;  G2 I0.0 J13.5 K0.0 X-27.5 Y-14.0 Z10.0;  "
            "G1 X-27.5 Y14.0 Z10.0;  G2 I13.5 J0.0 K0.0 X-14.0 Y27.5 Z10.0;  "
            "G1 X14.0 Y27.5 Z10.0;  G2 I0.0 J-13.5 K0.0 X23.54 Y23.54 Z10.0"
        )
        self.assertTrue(
            expected_moves == operationMoves,
            "expected_moves: {}\noperationMoves: {}".format(expected_moves, operationMoves),
        )

    def test02(self):
        """test02() Verify path generated on Face18, outside, without compensation."""

        # Instantiate a Profile operation and set Base Geometry
        profile = PathProfile.Create("Profile2")
        profile.Base = [(self.doc.Body, ["Face18"])]  # (base, subs_list)
        profile.Label = "test02+"
        profile.Comment = "test02() Verify path generated on Face18, outside, without compensation."

        # Set additional operation properties
        # setDepthsAndHeights(adaptive)
        profile.processCircles = True
        profile.processHoles = True
        profile.UseComp = False
        profile.Direction = "CW"
        _addViewProvider(profile)
        self.doc.recompute()

        moves = getGcodeMoves(profile.Path.Commands, includeRapids=False)
        operationMoves = ";  ".join(moves)
        # FreeCAD.Console.PrintMessage("test02_moves: " + operationMoves + "\n")

        expected_moves = (
            "G1 X18.24 Y18.24 Z10.0;  G3 I-4.24 J-4.24 K0.0 X14.0 Y20.0 Z10.0;  "
            "G1 X-14.0 Y20.0 Z10.0;  G3 I0.0 J-6.0 K0.0 X-20.0 Y14.0 Z10.0;  "
            "G1 X-20.0 Y-14.0 Z10.0;  G3 I6.0 J0.0 K0.0 X-14.0 Y-20.0 Z10.0;  "
            "G1 X14.0 Y-20.0 Z10.0;  G3 I0.0 J6.0 K0.0 X20.0 Y-14.0 Z10.0;  "
            "G1 X20.0 Y14.0 Z10.0;  G3 I-6.0 J0.0 K0.0 X18.24 Y18.24 Z10.0;  "
            "G1 X21.78 Y21.78 Z10.0;  G2 I-7.78 J-7.78 K0.0 X25.0 Y14.0 Z10.0;  "
            "G1 X25.0 Y-14.0 Z10.0;  G2 I-11.0 J0.0 K0.0 X14.0 Y-25.0 Z10.0;  "
            "G1 X-14.0 Y-25.0 Z10.0;  G2 I0.0 J11.0 K0.0 X-25.0 Y-14.0 Z10.0;  "
            "G1 X-25.0 Y14.0 Z10.0;  G2 I11.0 J0.0 K0.0 X-14.0 Y25.0 Z10.0;  "
            "G1 X14.0 Y25.0 Z10.0;  G2 I0.0 J-11.0 K0.0 X21.78 Y21.78 Z10.0"
        )

        self.assertTrue(
            expected_moves == operationMoves,
            "expected_moves: {}\noperationMoves: {}".format(expected_moves, operationMoves),
        )

    def test03(self):
        """test03() Verify path generated on Face18, outside,
        with compensation and extra offset -radius."""

        # Instantiate a Profile operation and set Base Geometry
        profile = PathProfile.Create("Profile3")
        profile.Base = [(self.doc.Body, ["Face18"])]  # (base, subs_list)
        profile.Label = "test03+"
        profile.Comment = (
            "test03() Verify path generated on Face4, " "with compensation and extra offset -radius"
        )

        # Set additional operation properties
        # setDepthsAndHeights(adaptive)
        profile.processCircles = True
        profile.processHoles = True
        profile.UseComp = True
        profile.Direction = "CW"
        profile.OffsetExtra = -profile.OpToolDiameter / 2.0
        _addViewProvider(profile)
        self.doc.recompute()

        moves = getGcodeMoves(profile.Path.Commands, includeRapids=False)
        operationMoves = ";  ".join(moves)
        # FreeCAD.Console.PrintMessage("test03_moves: " + operationMoves + "\n")

        expected_moves = (
            "G1 X18.24 Y18.24 Z10.0;  G3 I-4.24 J-4.24 K0.0 X14.0 Y20.0 Z10.0;  "
            "G1 X-14.0 Y20.0 Z10.0;  G3 I0.0 J-6.0 K0.0 X-20.0 Y14.0 Z10.0;  "
            "G1 X-20.0 Y-14.0 Z10.0;  G3 I6.0 J0.0 K0.0 X-14.0 Y-20.0 Z10.0;  "
            "G1 X14.0 Y-20.0 Z10.0;  G3 I0.0 J6.0 K0.0 X20.0 Y-14.0 Z10.0;  "
            "G1 X20.0 Y14.0 Z10.0;  G3 I-6.0 J0.0 K0.0 X18.24 Y18.24 Z10.0;  "
            "G1 X21.78 Y21.78 Z10.0;  G2 I-7.78 J-7.78 K0.0 X25.0 Y14.0 Z10.0;  "
            "G1 X25.0 Y-14.0 Z10.0;  G2 I-11.0 J0.0 K0.0 X14.0 Y-25.0 Z10.0;  "
            "G1 X-14.0 Y-25.0 Z10.0;  G2 I0.0 J11.0 K0.0 X-25.0 Y-14.0 Z10.0;  "
            "G1 X-25.0 Y14.0 Z10.0;  G2 I11.0 J0.0 K0.0 X-14.0 Y25.0 Z10.0;  "
            "G1 X14.0 Y25.0 Z10.0;  G2 I0.0 J-11.0 K0.0 X21.78 Y21.78 Z10.0"
        )

        self.assertTrue(
            expected_moves == operationMoves,
            "expected_moves: {}\noperationMoves: {}".format(expected_moves, operationMoves),
        )

    # Spring passes
    def _springPassOp(self, name, base=("Face18",), **properties):
        """_springPassOp(name, base, **properties) ... creates a recomputed Profile.

        Pass base=None to profile the whole model as a contour, which is the only way
        to get more than one depth layer out of the test geometry."""
        profile = PathProfile.Create(name)
        if base:
            profile.Base = [(self.doc.Body, list(base))]
        profile.Label = name
        profile.UseComp = True
        profile.Direction = "CW"
        for prop, value in properties.items():
            setattr(profile, prop, value)
        _addViewProvider(profile)
        self.doc.recompute()
        return profile

    @staticmethod
    def _cutMovesByDepth(profile):
        """_cutMovesByDepth(profile) ... returns {z: number of cutting moves at that z}."""
        byDepth = {}
        for move in getGcodeMoves(profile.Path.Commands, includeRapids=False):
            z = float(move.rsplit("Z", 1)[1])
            byDepth[z] = byDepth.get(z, 0) + 1
        return byDepth

    def testSpringPassPropertyDefaults(self):
        """testSpringPassPropertyDefaults() Verify a Profile makes no spring passes unless asked."""
        profile = self._springPassOp("ProfileSpringDefaults")

        self.assertEqual(profile.SpringPasses, 0)
        self.assertEqual(profile.SpringPassScope, "Final Depth")
        self.assertEqual(
            profile.getEnumerationsOfProperty("SpringPassScope"),
            ["Final Depth", "Every Depth"],
        )

    def testSpringPassRepeatsFinalPass(self):
        """testSpringPassRepeatsFinalPass() Verify a spring pass repeats the final pass exactly,
        without retracting between the pass and its repeat."""
        one = self._springPassOp("ProfileSpringOne", SpringPasses=1)
        two = self._springPassOp("ProfileSpringTwo", SpringPasses=2)

        cutsOne = getGcodeMoves(one.Path.Commands, includeRapids=False)
        cutsTwo = getGcodeMoves(two.Path.Commands, includeRapids=False)

        self.assertTrue(cutsOne, "Profile produced no cutting moves")
        self.assertEqual(len(cutsOne) % 2, 0, "cutting moves: {}".format(cutsOne))

        # One spring pass cuts the profile twice, two spring passes cut it three times.
        loop = cutsOne[: len(cutsOne) // 2]
        self.assertEqual(cutsOne, loop * 2, "cutting moves: {}".format(cutsOne))
        self.assertEqual(cutsTwo, loop * 3, "cutting moves: {}".format(cutsTwo))

        # A spring pass adds cutting moves and nothing else. If the repeat retracted and
        # plunged back in it would leave a dwell mark on the wall it has just finished.
        rapidsOne = getGcodeMoves(one.Path.Commands, includeLines=False, includeArcs=False)
        rapidsTwo = getGcodeMoves(two.Path.Commands, includeLines=False, includeArcs=False)
        self.assertEqual(
            rapidsOne,
            rapidsTwo,
            "Spring passes added rapids.\nSpringPasses=1: {}\nSpringPasses=2: {}".format(
                rapidsOne, rapidsTwo
            ),
        )

    def testSpringPassRepeatsFinishingOffsetOnly(self):
        """testSpringPassRepeatsFinishingOffsetOnly() Verify that on a multi-pass profile the
        spring pass repeats the finishing offset rather than the whole offset ladder."""
        plain = self._springPassOp("ProfileSpringMulti", NumPasses=3, Stepover=1.0)
        sprung = self._springPassOp(
            "ProfileSpringMultiSprung", NumPasses=3, Stepover=1.0, SpringPasses=1
        )

        cuts = getGcodeMoves(plain.Path.Commands, includeRapids=False)
        sprungCuts = getGcodeMoves(sprung.Path.Commands, includeRapids=False)

        self.assertTrue(cuts, "Multi-pass profile produced no cutting moves")
        self.assertEqual(len(cuts) % 3, 0, "cutting moves: {}".format(cuts))

        # The offsets run from the largest to the smallest, so the finishing offset is
        # the last of the three, and it is the only one a spring pass may repeat.
        finishing = cuts[2 * len(cuts) // 3 :]
        self.assertEqual(
            sprungCuts,
            cuts + finishing,
            "expected the finishing offset repeated once.\n"
            "finishing offset: {}\nsprung: {}".format(finishing, sprungCuts),
        )

    def testSpringPassScopeEveryDepth(self):
        """testSpringPassScopeEveryDepth() Verify spring passes land on the final depth alone
        or on every depth layer, according to SpringPassScope."""
        finalOnly = self._springPassOp("ProfileSpringFinalDepth", base=None, SpringPasses=1)
        everyDepth = self._springPassOp(
            "ProfileSpringEveryDepth",
            base=None,
            SpringPasses=1,
            SpringPassScope="Every Depth",
        )
        for profile in (finalOnly, everyDepth):
            profile.setExpression("StepDown", None)
            profile.StepDown.Value = 5.0
        self.doc.recompute()

        finalCounts = self._cutMovesByDepth(finalOnly)
        everyCounts = self._cutMovesByDepth(everyDepth)

        depths = sorted(everyCounts)  # ascending, so depths[0] is the final depth
        self.assertGreater(len(depths), 1, "Test needs more than one depth layer")
        self.assertEqual(depths, sorted(finalCounts))

        # The topmost layer is never repeated under 'Final Depth', so it sizes one loop.
        loop = finalCounts[depths[-1]]
        for z in depths:
            self.assertEqual(
                finalCounts[z],
                2 * loop if z == depths[0] else loop,
                "'Final Depth' scope, cutting moves per depth: {}".format(finalCounts),
            )
            self.assertEqual(
                everyCounts[z],
                2 * loop,
                "'Every Depth' scope, cutting moves per depth: {}".format(everyCounts),
            )


class TestPathOpenProfile(PathTestBase):
    """Unit tests for profiling open edges (edges of geometry)."""

    @classmethod
    def setUpClass(cls):
        """setUpClass()...
        This method is called upon instantiation of this test class.
        Set up the triangle geometry and job for all tests in this class.
        """
        # Create a new document (becomes active document)
        cls.doc = FreeCAD.newDocument("TestPathOpenProfile")
        FreeCAD.setActiveDocument(cls.doc.Name)

        # Create a triangular solid
        triangle_base = 20.0
        triangle_height = 30.0
        extrude_height = 10.0

        # Create triangle vertices (base on X-axis, apex pointing up in Y)
        v1 = FreeCAD.Vector(0, 0, 0)
        v2 = FreeCAD.Vector(triangle_base, 0, 0)
        v3 = FreeCAD.Vector(triangle_base / 2.0, triangle_height, 0)

        # Create triangle wire and extrude to make solid
        triangle_wire = Part.makePolygon([v1, v2, v3, v1])
        triangle_face = Part.Face(triangle_wire)
        triangle_solid = triangle_face.extrude(FreeCAD.Vector(0, 0, extrude_height))

        # Add triangle part to document
        triangle_part = cls.doc.addObject("Part::Feature", "Triangle")
        triangle_part.Shape = triangle_solid

        # Create a job for the triangle
        job = PathJob.Create("TriangleJob", [triangle_part], None)
        if FreeCAD.GuiUp:
            job.ViewObject.Proxy = PathJobGui.ViewProvider(job.ViewObject)

        # Find edges that pass through (0, 0, extrude_height) - the top vertex at origin
        # We want the 2 top edges (not the vertical edge)
        target_point = FreeCAD.Vector(0, 0, extrude_height)
        tolerance = 0.001

        # Loop through all edges and find top edges at the target point
        edges_at_origin = []
        for i, edge in enumerate(triangle_solid.Edges):
            # Check if either vertex of the edge is at the target point
            if (
                edge.Vertexes[0].Point.distanceToPoint(target_point) < tolerance
                or edge.Vertexes[1].Point.distanceToPoint(target_point) < tolerance
            ):
                # Filter out vertical edges - we only want edges on the top face
                # Top face edges have both vertices at Z = extrude_height
                v0_z = edge.Vertexes[0].Point.z
                v1_z = edge.Vertexes[1].Point.z
                if (
                    abs(v0_z - extrude_height) < tolerance
                    and abs(v1_z - extrude_height) < tolerance
                ):
                    # Both vertices at top Z level - this is a top edge
                    edges_at_origin.append(f"Edge{i+1}")

        # Save triangle geometry for test assertions
        cls.job = job
        cls.triangle_part = triangle_part
        cls.triangle_edges = edges_at_origin
        cls.triangle_base = triangle_base
        cls.triangle_height = triangle_height
        cls.extrude_height = extrude_height

        # Create profile operation for the two edges
        cls.profile = PathProfile.Create("TriangleProfile", parentJob=job)
        cls.profile.Base = [(triangle_part, edges_at_origin)]
        cls.profile.Label = "triangle_profile"
        cls.profile.Comment = "Profile two edges of a triangle."

        # Set operation properties
        cls.profile.Direction = "CCW"
        cls.profile.Side = "Outside"
        cls.profile.JoinType = "Round"

        # Set depth properties for open edge profiling
        # Clear expressions first, then set values
        cls.profile.setExpression("FinalDepth", None)
        cls.profile.setExpression("StartDepth", None)
        cls.profile.setExpression("StepDown", None)

        cls.profile.FinalDepth.Value = 0.0
        cls.profile.StartDepth.Value = extrude_height
        cls.profile.StepDown.Value = extrude_height + 1

        _addViewProvider(cls.profile)

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...
        Cleanup after all tests in this class have been executed.
        """
        # Close the document without saving
        FreeCAD.closeDocument(cls.doc.Name)

    def setUp(self):
        """setUp()...
        This method is called prior to each test() method.
        """
        pass

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method.
        """
        pass

    def testOpenProfileSetup(self):
        """Verify Profile Base contains 2 edges of the triangle."""

        # Verify profile.Base has correct structure: [(part, edges_tuple)]
        self.assertEqual(len(self.profile.Base), 1, "Profile.Base should have 1 entry")
        part, edges = self.profile.Base[0]

        # Assert we are profiling 2 edges
        self.assertEqual(len(edges), 2, "Profile Base should contain 2 edges")

    def test02(self):
        """test02() Recompute and verify gcode moves for triangle profile."""
        import math
        import Path.Base.Language as PathLanguage

        # Perform recompute to generate the path
        self.doc.recompute()

        # Process non-rapid move commands and calculate lengths
        move_lengths = []
        last = FreeCAD.Vector(0.0, 0.0, 0.0)

        for cmd in self.profile.Path.Commands:
            instr = PathLanguage.Maneuver.InstructionFromCommand(cmd, last)
            if instr.isMove() and not instr.isRapid() and last.z == instr.positionEnd().z:
                length = instr.pathLength()
                move_lengths.append(length)
            last = instr.positionEnd()

        # Check expected move count: 2 offset triangle legs and an arc between
        self.assertGreater(len(move_lengths), 2, "Should have at least 3 moves")

        # Check lengths of leg moves
        leg_length = math.hypot(self.triangle_base / 2.0, self.triangle_height)
        expected_straight_total = leg_length + self.triangle_base

        first_move = move_lengths[0]
        last_move = move_lengths[-1]
        straight_total = first_move + last_move

        self.assertAlmostEqual(
            straight_total,
            expected_straight_total,
            delta=expected_straight_total * 0.01,
            msg=f"Straight segments ({straight_total:.2f}) should equal triangle legs ({expected_straight_total:.2f})",
        )

        # Check lengths of arc moves
        arc_moves = move_lengths[1:-1]
        arc_total = sum(arc_moves)

        # Calculate expected arc angle and length
        left_leg = FreeCAD.Vector(self.triangle_base / 2.0, self.triangle_height, 0).normalize()
        base = FreeCAD.Vector(self.triangle_base, 0, 0).normalize()
        left_normal = FreeCAD.Vector(-left_leg.y, left_leg.x, 0)
        base_normal = FreeCAD.Vector(base.y, -base.x, 0)
        dot_product = left_normal.dot(base_normal)
        dot_product = max(-1.0, min(1.0, dot_product))
        arc_angle = math.acos(dot_product)

        tool_radius = self.profile.OpToolDiameter.Value / 2.0
        expected_arc_length = tool_radius * arc_angle

        # Assert: arc segment total ≈ expected arc length
        self.assertAlmostEqual(
            arc_total,
            expected_arc_length,
            delta=expected_arc_length * 0.01,
            msg=f"Arc length ({arc_total:.2f}) should equal radius × angle ({expected_arc_length:.2f})",
        )

    def _openProfileOp(self, name, **properties):
        """_openProfileOp(name, **properties) ... creates a recomputed open-edge Profile
        with two depth layers."""
        profile = PathProfile.Create(name, parentJob=self.job)
        profile.Base = [(self.triangle_part, self.triangle_edges)]
        profile.Label = name
        profile.Direction = "CCW"
        profile.Side = "Outside"
        profile.setExpression("FinalDepth", None)
        profile.setExpression("StartDepth", None)
        profile.setExpression("StepDown", None)
        profile.FinalDepth.Value = 0.0
        profile.StartDepth.Value = self.extrude_height
        profile.StepDown.Value = self.extrude_height / 2.0
        for prop, value in properties.items():
            setattr(profile, prop, value)
        _addViewProvider(profile)
        self.doc.recompute()
        return profile

    @staticmethod
    def _cutMovesByDepth(profile):
        """_cutMovesByDepth(profile) ... returns {z: number of cutting moves at that z}."""
        byDepth = {}
        for move in getGcodeMoves(profile.Path.Commands, includeRapids=False):
            z = float(move.rsplit("Z", 1)[1])
            byDepth[z] = byDepth.get(z, 0) + 1
        return byDepth

    def testSpringPassOnOpenEdges(self):
        """testSpringPassOnOpenEdges() Verify spring passes repeat an open-edge profile at the
        final depth alone, or at every depth, according to SpringPassScope."""
        plain = self._openProfileOp("OpenProfileSpringOff")
        finalOnly = self._openProfileOp("OpenProfileSpringFinal", SpringPasses=1)
        everyDepth = self._openProfileOp(
            "OpenProfileSpringEvery", SpringPasses=1, SpringPassScope="Every Depth"
        )

        plainCounts = self._cutMovesByDepth(plain)
        finalCounts = self._cutMovesByDepth(finalOnly)
        everyCounts = self._cutMovesByDepth(everyDepth)

        depths = sorted(plainCounts)  # ascending, so depths[0] is the final depth
        self.assertGreater(len(depths), 1, "Test needs more than one depth layer")

        for z in depths:
            self.assertEqual(
                finalCounts[z],
                2 * plainCounts[z] if z == depths[0] else plainCounts[z],
                "'Final Depth' scope, cutting moves per depth: {}".format(finalCounts),
            )
            self.assertEqual(
                everyCounts[z],
                2 * plainCounts[z],
                "'Every Depth' scope, cutting moves per depth: {}".format(everyCounts),
            )

    def testSpringPassOnOpenEdgesRepeatsFinishingOffsetOnly(self):
        """testSpringPassOnOpenEdgesRepeatsFinishingOffsetOnly() Verify that on a multi-pass
        open-edge profile the spring pass repeats the finishing offset only."""
        plain = self._openProfileOp("OpenProfileMulti", NumPasses=3, Stepover=1.0)
        sprung = self._openProfileOp(
            "OpenProfileMultiSprung", NumPasses=3, Stepover=1.0, SpringPasses=1
        )

        cuts = getGcodeMoves(plain.Path.Commands, includeRapids=False)
        sprungCuts = getGcodeMoves(sprung.Path.Commands, includeRapids=False)

        # Each offset is emitted as its own wire, deepest layer last, so the finishing
        # offset's final-depth moves are the tail of the path and the repeat follows them.
        added = len(sprungCuts) - len(cuts)
        self.assertGreater(added, 0, "Spring pass added no cutting moves")
        self.assertEqual(sprungCuts[: len(cuts)], cuts, "Spring pass altered the preceding passes")
        self.assertEqual(
            sprungCuts[-added:],
            cuts[-added:],
            "expected the finishing offset repeated once.\n"
            "finishing offset: {}\nrepeat: {}".format(cuts[-added:], sprungCuts[-added:]),
        )


def _addViewProvider(profileOp):
    if FreeCAD.GuiUp:
        PathOpGui = PathProfileGui.PathOpGui
        cmdRes = PathProfileGui.Command.res
        profileOp.ViewObject.Proxy = PathOpGui.ViewProvider(profileOp.ViewObject, cmdRes)
