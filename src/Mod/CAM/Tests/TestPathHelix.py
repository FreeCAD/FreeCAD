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

import Draft
import FreeCAD
import Path
import Path.Main.Job as PathJob
import Path.Op.Helix as PathHelix
import Tests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
# Path.Log.trackModule(Path.Log.thisModule())


class TestPathHelix(PathTestUtils.PathTestBase):
    RotateBy = 45

    def setUp(self):
        self.clone = None
        self.doc = FreeCAD.open(
            FreeCAD.getHomePath() + "Mod/CAM/Tests/test_holes00.fcstd"
        )
        self.job = PathJob.Create("Job", [self.doc.Body])

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
                self.assertRoughly(
                    round(pos.Length / 10, 0), proxy.holeDiameter(op, model, sub)
                )

    def test02(self):
        """Verify Helix generates proper holes for rotated model"""

        self.job.Tools.Group[0].Tool.Diameter = 0.5

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
            self.doc = FreeCAD.open(
                FreeCAD.getHomePath() + "Mod/CAM/Tests/test_holes00.fcstd"
            )
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
            self.doc = FreeCAD.open(
                FreeCAD.getHomePath() + "Mod/CAM/Tests/test_holes00.fcstd"
            )
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
