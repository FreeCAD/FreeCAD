# -*- coding: utf-8 -*-
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
import FreeCAD
import Path
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
        op.Proxy.execute(op)

    def test01(self):
        """Verify Helix generates proper holes from model"""

        op = PathHelix.Create("Helix")
        proxy = op.Proxy
        for base in op.Base:
            model = base[0]
            for sub in base[1]:
                pos = proxy.holePosition(op, model, sub)
                self.assertRoughly(round(pos.Length / 10, 0), proxy.holeDiameter(op, model, sub))

    def test02(self):
        """Verify Helix generates proper holes for rotated model"""

        op = PathHelix.Create("Helix")
        proxy = op.Proxy
        model = self.job.Model.Group[0]

        for deg in range(self.RotateBy, 360, self.RotateBy):
            model.Placement.Rotation = FreeCAD.Rotation(deg, 0, 0)
            for base in op.Base:
                model = base[0]
                for sub in base[1]:
                    pos = proxy.holePosition(op, model, sub)
                    # Path.Log.track(deg, pos, pos.Length)
                    self.assertRoughly(
                        round(pos.Length / 10, 0), proxy.holeDiameter(op, model, sub)
                    )

    def test03(self):
        """Verify Helix generates proper holes for rotated base model"""

        for deg in range(self.RotateBy, 360, self.RotateBy):
            self.tearDown()
            self.doc = FreeCAD.open(FreeCAD.getHomePath() + "Mod/CAM/CAMTests/test_holes00.fcstd")
            self.doc.Body.Placement.Rotation = FreeCAD.Rotation(deg, 0, 0)

            self.job = PathJob.Create("Job", [self.doc.Body])
            self.job.Tools.Group[0].Tool.Diameter = 0.5

            op = PathHelix.Create("Helix")
            proxy = op.Proxy
            model = self.job.Model.Group[0]

            for base in op.Base:
                model = base[0]
                for sub in base[1]:
                    pos = proxy.holePosition(op, model, sub)
                    # Path.Log.track(deg, pos, pos.Length)
                    self.assertRoughly(
                        round(pos.Length / 10, 0), proxy.holeDiameter(op, model, sub)
                    )

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
            proxy = op.Proxy
            model = self.job.Model.Group[0]

            for base in op.Base:
                model = base[0]
                for sub in base[1]:
                    pos = proxy.holePosition(op, model, sub)
                    # Path.Log.track(deg, pos, pos.Length)
                    self.assertRoughly(
                        round(pos.Length / 10, 0), proxy.holeDiameter(op, model, sub)
                    )

    def testRecomputeHelixFromV021(self):
        """Verify that we can still open and recompute a Helix created with older FreeCAD"""
        self.tearDown()
        self.doc = FreeCAD.openDocument(str(FIXTURE_PATH / "OpHelix_v0-21.FCStd"))
        created_with = f"created with {self.doc.getProgramVersion()}"

        def check(helix, direction, start_side, cut_mode):
            with self.subTest(f"{helix.Name}: {direction}, {start_side}, {cut_mode}"):
                # no recompute yet, i.e. check original as precondition
                self.assertEqual(
                    helix.Direction,
                    direction,
                    msg=f"Direction does not match fixture for helix {created_with}",
                )
                self.assertEqual(
                    helix.StartSide,
                    start_side,
                    msg=f"StartSide does not match fixture for helix {created_with}",
                )

                # now see whether we can recompute the object from the old document
                helix.enforceRecompute()
                self.assertSuccessfulRecompute(
                    self.doc, helix, msg=f"Cannot recompute helix {created_with}"
                )
                self.assertEqual(
                    helix.Direction,
                    direction,
                    msg=f"Direction changed after recomputing helix {created_with}",
                )
                self.assertEqual(
                    helix.StartSide,
                    start_side,
                    msg=f"StartSide changed after recomputing helix {created_with}",
                )
                # self.assertEqual(helix.CutMode, cut_mode,
                #    msg=f"CutMode not correctly derived for helix {created_with}")

        # object names and expected values defined in the fixture
        check(self.doc.Helix, "CW", "Inside", "Conventional")
        check(self.doc.Helix001, "CW", "Outside", "Climb")
        check(self.doc.Helix002, "CCW", "Inside", "Climb")
        check(self.doc.Helix003, "CCW", "Outside", "Conventional")
