# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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
import Path
import PathScripts.PathDressupDogbone as PathDressupDogbone
import PathScripts.PathJob as PathJob
import PathScripts.PathProfileFaces as PathProfileFaces

from PathTests.PathTestUtils import PathTestBase


class TestProfile:
    def __init__(self, side, direction, path):
        self.Side = side
        self.Direction = direction
        self.Path = Path.Path(path)
        self.ToolController = None  # default tool 5mm
        self.Name = "Profile"


class TestFeature:
    def __init__(self):
        self.Path = Path.Path()

    def addProperty(self, typ, nam, category, tip):
        setattr(self, nam, None)

    def setEditorMode(self, prop, mode):
        pass


class TestDressupDogbone(PathTestBase):
    """Unit tests for the Dogbone dressup."""

    def formatBone(self, bone):
        return "%d: (%.2f, %.2f)" % (bone[0], bone[1][0], bone[1][1])

    def test00(self):
        """Verify bones are inserted for simple moves."""
        base = TestProfile(
            "Inside",
            "CW",
            """
        G0 X10 Y10 Z10
        G1 Z0
        G1 Y100
        G1 X12
        G1 Y10
        G1 X10
        G1 Z10
        """,
        )
        obj = TestFeature()
        db = PathDressupDogbone.ObjectDressup(obj, base)
        db.setup(obj, True)
        db.execute(obj, False)
        self.assertEqual(len(db.bones), 4)
        self.assertEqual("1: (10.00, 100.00)", self.formatBone(db.bones[0]))
        self.assertEqual("2: (12.00, 100.00)", self.formatBone(db.bones[1]))
        self.assertEqual("3: (12.00, 10.00)", self.formatBone(db.bones[2]))
        self.assertEqual("4: (10.00, 10.00)", self.formatBone(db.bones[3]))

    def test01(self):
        """Verify bones are inserted if hole ends with rapid move out."""
        base = TestProfile(
            "Inside",
            "CW",
            """
        G0 X10 Y10 Z10
        G1 Z0
        G1 Y100
        G1 X12
        G1 Y10
        G1 X10
        G0 Z10
        """,
        )
        obj = TestFeature()
        db = PathDressupDogbone.ObjectDressup(obj, base)
        db.setup(obj, True)
        db.execute(obj, False)
        self.assertEqual(len(db.bones), 4)
        self.assertEqual("1: (10.00, 100.00)", self.formatBone(db.bones[0]))
        self.assertEqual("2: (12.00, 100.00)", self.formatBone(db.bones[1]))
        self.assertEqual("3: (12.00, 10.00)", self.formatBone(db.bones[2]))
        self.assertEqual("4: (10.00, 10.00)", self.formatBone(db.bones[3]))

    def test02(self):
        """Verify bones are correctly generated for a Profile."""
        doc = FreeCAD.newDocument("TestDressupDogbone")

        # This is a real world test to make sure none of the tool chain broke
        box0 = doc.addObject("Part::Box", "Box")
        box0.Width = 100
        box0.Length = 100
        box0.Height = 10
        box1 = doc.addObject("Part::Box", "Box")
        box1.Width = 50
        box1.Length = 50
        box1.Height = 20
        box1.Placement = FreeCAD.Placement(
            FreeCAD.Vector(25, 25, -5), FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0)
        )
        doc.recompute()
        cut = doc.addObject("Part::Cut", "Cut")
        cut.Base = box0
        cut.Tool = box1
        doc.recompute()

        for i in range(11):
            face = "Face%d" % (i + 1)
            f = cut.Shape.getElement(face)
            if f.Surface.Axis == FreeCAD.Vector(0, 0, 1) and f.Orientation == "Forward":
                break

        PathJob.Create("Job", [cut], None)

        profile = PathProfileFaces.Create("Profile Faces")
        profile.Base = (cut, face)
        profile.StepDown = 5
        # set start and final depth in order to eliminate effects of stock (and its default values)
        profile.setExpression("StartDepth", None)
        profile.StartDepth = 10
        profile.setExpression("FinalDepth", None)
        profile.FinalDepth = 0

        profile.processHoles = True
        profile.processPerimeter = True
        doc.recompute()

        dogbone = PathDressupDogbone.Create(profile)
        doc.recompute()

        dog = dogbone.Proxy
        locs = sorted(
            [bone[1] for bone in dog.bones], key=lambda xy: xy[0] * 1000 + xy[1]
        )

        def formatBoneLoc(pt):
            return "(%.2f, %.2f)" % (pt[0], pt[1])

        # Make sure we get 8 bones, 2 in each corner (different heights)
        # with start point changes it passes back over the same spot multiple times, so just make sure they are in the right locations
        # self.assertEqual(len(locs), 8)
        self.assertEqual("(27.50, 27.50)", formatBoneLoc(locs[0]))
        self.assertEqual("(27.50, 27.50)", formatBoneLoc(locs[1]))
        self.assertEqual("(27.50, 72.50)", formatBoneLoc(locs[2]))
        self.assertEqual("(27.50, 72.50)", formatBoneLoc(locs[3]))
        self.assertEqual("(72.50, 27.50)", formatBoneLoc(locs[4]))
        self.assertEqual("(72.50, 27.50)", formatBoneLoc(locs[5]))
        self.assertEqual("(72.50, 72.50)", formatBoneLoc(locs[6]))
        self.assertEqual("(72.50, 72.50)", formatBoneLoc(locs[7]))

        FreeCAD.closeDocument("TestDressupDogbone")

    def test03(self):
        """Verify no bone is inserted for straight move interrupted by plunge."""
        base = TestProfile(
            "Inside",
            "CW",
            """
        G0 X10 Y10 Z10
        G1 Z0
        G1 X0   ( start)
        G1 Y0
        G1 X15
        G1 Y10
        G1 X10  ( straight line move to start)
        G0 Z10
        """,
        )
        obj = TestFeature()
        db = PathDressupDogbone.ObjectDressup(obj, base)
        db.setup(obj, True)
        db.execute(obj, False)
        self.assertEqual(len(db.bones), 0)

    def test04(self):
        """Verify can handle comments between moves"""
        base = TestProfile(
            "Inside",
            "CW",
            """
        G0 X10 Y10 Z10
        G1 Z0
        G1 X20
        G1 Y0
        G1 X10
        G1 Y10
        G1 Z10
        """,
        )
        obj = TestFeature()
        db = PathDressupDogbone.ObjectDressup(obj, base)
        db.setup(obj, True)
        db.execute(obj, False)
        self.assertEqual(len(db.bones), 4)
        self.assertEqual("1: (20.00, 10.00)", self.formatBone(db.bones[0]))
        self.assertEqual("2: (20.00, 0.00)", self.formatBone(db.bones[1]))
        self.assertEqual("3: (10.00, 0.00)", self.formatBone(db.bones[2]))
        self.assertEqual("4: (10.00, 10.00)", self.formatBone(db.bones[3]))

        base = TestProfile(
            "Inside",
            "CW",
            """
        G0 X10 Y10 Z10
        G1 Z0
        G1 X20
        G1 Y0
        G1 X10
        (some comment or other should not change the output)
        G1 Y10
        G1 Z10
        """,
        )
        obj = TestFeature()
        db = PathDressupDogbone.ObjectDressup(obj, base)
        db.setup(obj, True)
        db.execute(obj, False)
        self.assertEqual(len(db.bones), 4)
        self.assertEqual("1: (20.00, 10.00)", self.formatBone(db.bones[0]))
        self.assertEqual("2: (20.00, 0.00)", self.formatBone(db.bones[1]))
        self.assertEqual("3: (10.00, 0.00)", self.formatBone(db.bones[2]))
        self.assertEqual("4: (10.00, 10.00)", self.formatBone(db.bones[3]))

    def test05(self):
        """Verify can handle noops between moves"""
        base = TestProfile(
            "Inside",
            "CW",
            """
        G0 X10 Y10 Z10
        G1 Z0
        G1 X20
        G1 Y0
        G1 X10
        G1 Y10
        G1 Z10
        """,
        )
        obj = TestFeature()
        db = PathDressupDogbone.ObjectDressup(obj, base)
        db.setup(obj, True)
        db.execute(obj, False)
        self.assertEqual(len(db.bones), 4)
        self.assertEqual("1: (20.00, 10.00)", self.formatBone(db.bones[0]))
        self.assertEqual("2: (20.00, 0.00)", self.formatBone(db.bones[1]))
        self.assertEqual("3: (10.00, 0.00)", self.formatBone(db.bones[2]))
        self.assertEqual("4: (10.00, 10.00)", self.formatBone(db.bones[3]))

        base = TestProfile(
            "Inside",
            "CW",
            """
        G0 X10 Y10 Z10
        G1 Z0
        G1 X20
        G1 Y0
        G1 X10
        G1 X10
        G1 Y10
        G1 Z10
        """,
        )
        obj = TestFeature()
        db = PathDressupDogbone.ObjectDressup(obj, base)
        db.setup(obj, True)
        db.execute(obj, False)
        self.assertEqual(len(db.bones), 4)
        self.assertEqual("1: (20.00, 10.00)", self.formatBone(db.bones[0]))
        self.assertEqual("2: (20.00, 0.00)", self.formatBone(db.bones[1]))
        self.assertEqual("3: (10.00, 0.00)", self.formatBone(db.bones[2]))
        self.assertEqual("4: (10.00, 10.00)", self.formatBone(db.bones[3]))
