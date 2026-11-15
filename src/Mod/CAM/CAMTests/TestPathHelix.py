# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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

import pathlib

import Draft
import Part
import FreeCAD
import Path
import Path.Base.SetupSheetOpPrototype as PathSetupSheetOpPrototype
import Path.Main.Job as PathJob
import Path.Op.Helix as PathHelix
import CAMTests.PathTestUtils as PathTestUtils

FIXTURE_PATH = pathlib.Path(__file__).parent / "Fixtures"

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
# Path.Log.trackModule(Path.Log.thisModule())


class TestPathHelix(PathTestUtils.PathTestBase):
    RotateBy = 45

    def setUp(self):
        self.clone = None
        self.doc = FreeCAD.open(FreeCAD.getHomePath() + "Mod/CAM/CAMTests/test_holes00.fcstd")
        self.job = PathJob.Create("Job", [self.doc.Body])

        # the smallest hole in the fixture is 1mm in diameter, so our tool must be smaller.
        self.job.Tools.Group[0].Tool.Diameter = 0.9

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def test00(self):
        """Verify Helix does not throw an exception."""

        op = PathHelix.Create("Helix")
        op.Proxy.findAllHoles(op)
        op.Proxy.execute(op)

    def testCreateWithPrototype(self):
        """Verify a Helix can be created on a SetupSheet's prototype instead of a real document object"""

        ptt = PathSetupSheetOpPrototype.OpPrototype("Helix")
        op = PathHelix.Create("OpPrototype.Helix", ptt)

    def test01(self):
        """Verify Helix generates proper holes from model"""

        op = PathHelix.Create("Helix")
        op.Proxy.findAllHoles(op)
        proxy = op.Proxy
        for base in op.Base:
            model = base[0]
            for sub in base[1]:
                pos = proxy.holePosition(model, sub)
                self.assertRoughly(round(pos.Length / 10, 0), proxy.holeDiameter(model, sub))

    def test02(self):
        """Verify Helix generates proper holes for rotated model"""

        op = PathHelix.Create("Helix")
        op.Proxy.findAllHoles(op)
        proxy = op.Proxy
        model = self.job.Model.Group[0]

        for deg in range(self.RotateBy, 360, self.RotateBy):
            model.Placement.Rotation = FreeCAD.Rotation(deg, 0, 0)
            for base in op.Base:
                model = base[0]
                for sub in base[1]:
                    pos = proxy.holePosition(model, sub)
                    # Path.Log.track(deg, pos, pos.Length)
                    self.assertRoughly(round(pos.Length / 10, 0), proxy.holeDiameter(model, sub))

    def test03(self):
        """Verify Helix generates proper holes for rotated base model"""

        for deg in range(self.RotateBy, 360, self.RotateBy):
            self.tearDown()
            self.doc = FreeCAD.open(FreeCAD.getHomePath() + "Mod/CAM/CAMTests/test_holes00.fcstd")
            self.doc.Body.Placement.Rotation = FreeCAD.Rotation(deg, 0, 0)

            self.job = PathJob.Create("Job", [self.doc.Body])
            self.job.Tools.Group[0].Tool.Diameter = 0.5

            op = PathHelix.Create("Helix")
            op.Proxy.findAllHoles(op)
            proxy = op.Proxy
            model = self.job.Model.Group[0]

            for base in op.Base:
                model = base[0]
                for sub in base[1]:
                    pos = proxy.holePosition(model, sub)
                    # Path.Log.track(deg, pos, pos.Length)
                    self.assertRoughly(round(pos.Length / 10, 0), proxy.holeDiameter(model, sub))

    def test04(self):
        """Verify Helix generates proper holes for rotated clone base model"""
        for deg in range(self.RotateBy, 360, self.RotateBy):
            self.tearDown()
            self.doc = FreeCAD.open(FreeCAD.getHomePath() + "Mod/CAM/CAMTests/test_holes00.fcstd")
            self.clone = Draft.clone(self.doc.Body)
            self.clone.Placement.Rotation = FreeCAD.Rotation(deg, 0, 0)

            self.job = PathJob.Create("Job", [self.clone])
            self.job.Tools.Group[0].Tool.Diameter = 0.5

            op = PathHelix.Create("Helix")
            op.Proxy.findAllHoles(op)
            proxy = op.Proxy
            model = self.job.Model.Group[0]

            for base in op.Base:
                model = base[0]
                for sub in base[1]:
                    pos = proxy.holePosition(model, sub)
                    # Path.Log.track(deg, pos, pos.Length)
                    self.assertRoughly(round(pos.Length / 10, 0), proxy.holeDiameter(model, sub))

    def testPathDirection(self):
        """Verify that the generated paths obeys the given parameters"""
        helix = PathHelix.Create("Helix")
        helix.Proxy.findAllHoles(helix)

        def check(start_side, cut_mode, expected_direction):
            with self.subTest(f"({start_side}, {cut_mode}) => {expected_direction}"):
                helix.StartAt = start_side
                helix.CutMode = cut_mode

                self.assertSuccessfulRecompute(self.doc, helix)

                self.assertEqual(
                    helix.Direction,
                    expected_direction,
                    msg="Direction was not correctly determined",
                )
                self.assertPathDirection(
                    helix.Path,
                    expected_direction,
                    msg="Path with wrong direction generated",
                )

        check("Inside", "Conventional", "CW")
        check("Outside", "Climb", "CW")
        check("Inside", "Climb", "CCW")
        check("Outside", "Conventional", "CCW")

    def testRecomputeHelixFromV021(self):
        """Verify that we can still open and recompute a Helix created with older FreeCAD"""
        self.tearDown()
        self.doc = FreeCAD.openDocument(str(FIXTURE_PATH / "OpHelix_v0-21.FCStd"))
        created_with = f"created with {self.doc.getProgramVersion()}"

        def check(helix, direction, start_side, cut_mode):
            with self.subTest(f"{helix.Name}: ({direction}, {start_side}) => {cut_mode}"):
                # no recompute yet, i.e. check original as precondition
                self.assertPathDirection(
                    helix.Path,
                    direction,
                    msg=f"Path direction does not match fixture for {helix.Name} {created_with}",
                )
                self.assertEqual(
                    helix.Direction,
                    direction,
                    msg=f"Direction does not match fixture for {helix.Name} {created_with}",
                )
                self.assertEqual(
                    helix.StartAt,
                    start_side,
                    msg=f"StartAt does not match fixture for {helix.Name} {created_with}",
                )

                # now see whether we can recompute the object from the old document
                helix.enforceRecompute()
                self.assertSuccessfulRecompute(
                    self.doc, helix, msg=f"Cannot recompute {helix.Name} {created_with}"
                )
                self.assertEqual(
                    helix.Direction,
                    direction,
                    msg=f"Direction changed after recomputing {helix.Name} {created_with}",
                )
                self.assertEqual(
                    helix.StartAt,
                    start_side,
                    msg=f"StartAt changed after recomputing {helix.Name} {created_with}",
                )
                self.assertEqual(
                    helix.CutMode,
                    cut_mode,
                    msg=f"CutMode not correctly derived for {helix.Name} {created_with}",
                )
                self.assertPathDirection(
                    helix.Path,
                    direction,
                    msg=f"Path with wrong direction generated for {helix.Name} {created_with}",
                )

        # object names and expected values defined in the fixture
        check(self.doc.Helix, "CW", "Inside", "Conventional")
        check(self.doc.Helix001, "CW", "Outside", "Climb")
        check(self.doc.Helix002, "CCW", "Inside", "Climb")
        check(self.doc.Helix003, "CCW", "Outside", "Conventional")

    def assertPathDirection(self, path, expected_direction, msg=None):
        """Asserts that the given path goes into the expected direction.

        For the general case we'd need to check the sign of the second derivative,
        but as we know we work on a helix here, we can take a short cut and just
        look at the G2/G3 arc commands.
        """
        has_g2 = any(filter(lambda cmd: cmd.Name == "G2", path.Commands))
        has_g3 = any(filter(lambda cmd: cmd.Name == "G3", path.Commands))
        if has_g2 and not has_g3:
            self.assertEqual("CW", expected_direction, msg)
        elif has_g3 and not has_g2:
            self.assertEqual("CCW", expected_direction, msg)
        else:
            raise NotImplementedError("Cannot determine direction for arbitrary paths")


class TestPathHelixSteppedHole(PathTestUtils.PathTestBase):
    """A counterbore over its hole: both steps are cut, each to its own depth."""

    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathHelixSteppedHole")
        block = Part.makeBox(40, 40, 20).cut(Part.makeCylinder(7, 5, FreeCAD.Vector(20, 20, 15)))
        body = self.doc.addObject("Part::Feature", "Block")
        body.Shape = block.cut(Part.makeCylinder(4, 20, FreeCAD.Vector(20, 20, 0))).removeSplitter()
        self.doc.recompute()
        self.job = PathJob.Create("Job", [body])
        self.job.Tools.Group[0].Tool.Diameter = 6
        self.model = self.job.Model.Group[0]

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def testBothStepsCut(self):
        """Verify the bore and the counterbore over it are both kept and cut to their own depths"""
        faces = list(enumerate(self.model.Shape.Faces, 1))
        bore = next(i for i, f in faces if Path.Geom.isRoughly(f.BoundBox.XLength, 8))
        floor = next(
            i
            for i, f in faces
            if Path.Geom.isRoughly(f.BoundBox.ZMin, 15)
            and Path.Geom.isRoughly(f.BoundBox.ZLength, 0)
        )
        op = PathHelix.Create("Helix")
        op.Base = [(self.model, [f"Face{bore}", f"Face{floor}"])]
        for prop in ("StartDepth", "FinalDepth", "StepDown"):
            op.setExpression(prop, None)
        op.StartDepth = 20
        op.StepDown = 20

        # the Final Depth at the bore's bottom, partway down the bore, or at the counterbore's floor
        for final, bore in ((0, 0), (5, 5), (15, 15)):
            with self.subTest(final=final):
                op.FinalDepth = final
                self.doc.recompute()
                self.assertEqual(op.Disabled, [])
                # the tool radius is 3: the bore's arcs run 1 from the axis, the counterbore's 3-4
                bottoms = {}
                for cmd in (c for c in op.Path.Commands if c.Name in ("G2", "G3")):
                    off = FreeCAD.Vector(cmd.x - 20, cmd.y - 20, 0).Length
                    step = "bore" if off < 2 else "counterbore"
                    bottoms[step] = min(bottoms.get(step, cmd.z), cmd.z)
                self.assertRoughly(bottoms["bore"], bore)
                self.assertRoughly(bottoms["counterbore"], 15)
