# -*- coding: utf-8 -*-
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
from PathTests.PathTestUtils import PathTestBase
from PathTests.TestPathAdaptive import getGcodeMoves

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
        cls.doc = FreeCAD.open(
            FreeCAD.getHomePath() + "Mod/Path/PathTests/test_profile.fcstd"
        )

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
        profile.Comment = "test01() Verify path generated on Face18, outside, with tool compensation."

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
        #FreeCAD.Console.PrintMessage("test01_moves: " + operationMoves + "\n")


        expected_moves = \
          "G1 X16.47 Y16.47 Z10.0;  G3 I-2.48 J-2.48 K0.0 X13.93 Y17.5 Z10.0;  " \
          "G1 X-13.93 Y17.5 Z10.0;  G3 I-0.06 J-3.51 K0.0 X-17.5 Y13.93 Z10.0;  " \
          "G1 X-17.5 Y-13.93 Z10.0;  G3 I3.51 J-0.06 K0.0 X-13.93 Y-17.5 Z10.0;  " \
          "G1 X13.93 Y-17.5 Z10.0;  G3 I0.06 J3.51 K0.0 X17.5 Y-13.93 Z10.0;  " \
          "G1 X17.5 Y13.93 Z10.0;  G3 I-3.51 J0.06 K0.0 X16.47 Y16.47 Z10.0;  " \
          "G1 X23.55 Y23.54 Z10.0;  G2 I-9.55 J-9.54 K0.0 X27.5 Y14.1 Z10.0;  " \
          "G1 X27.5 Y-14.0 Z10.0;  G2 I-13.5 J0.0 K0.0 X14.1 Y-27.5 Z10.0;  " \
          "G1 X-14.0 Y-27.5 Z10.0;  G2 I0.0 J13.5 K0.0 X-27.5 Y-14.1 Z10.0;  " \
          "G1 X-27.5 Y14.0 Z10.0;  G2 I13.5 J-0.0 K0.0 X-14.1 Y27.5 Z10.0;  " \
          "G1 X14.0 Y27.5 Z10.0;  G2 I-0.0 J-13.5 K0.0 X23.55 Y23.54 Z10.0"
        self.assertTrue(expected_moves == operationMoves,
                       "expected_moves: {}\noperationMoves: {}".format(expected_moves, operationMoves))

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
        #FreeCAD.Console.PrintMessage("test02_moves: " + operationMoves + "\n")

        expected_moves = "G1 X18.24 Y18.24 Z10.0;  G3 I-4.24 J-4.24 K0.0 X14.0 Y20.0 Z10.0;  " \
          "G1 X-14.0 Y20.0 Z10.0;  G3 I0.0 J-6.0 K0.0 X-20.0 Y14.0 Z10.0;  " \
          "G1 X-20.0 Y-14.0 Z10.0;  G3 I6.0 J0.0 K0.0 X-14.0 Y-20.0 Z10.0;  " \
          "G1 X14.0 Y-20.0 Z10.0;  G3 I-0.0 J6.0 K0.0 X20.0 Y-14.0 Z10.0;  " \
          "G1 X20.0 Y14.0 Z10.0;  G3 I-6.0 J-0.0 K0.0 X18.24 Y18.24 Z10.0;  " \
          "G1 X21.78 Y21.78 Z10.0;  G2 I-7.78 J-7.78 K0.0 X25.0 Y14.0 Z10.0;  " \
          "G1 X25.0 Y-14.0 Z10.0;  G2 I-11.0 J0.0 K0.0 X14.0 Y-25.0 Z10.0;  " \
          "G1 X-14.0 Y-25.0 Z10.0;  G2 I0.0 J11.0 K0.0 X-25.0 Y-14.0 Z10.0;  " \
          "G1 X-25.0 Y14.0 Z10.0;  G2 I11.0 J-0.0 K0.0 X-14.0 Y25.0 Z10.0;  " \
          "G1 X14.0 Y25.0 Z10.0;  G2 I-0.0 J-11.0 K0.0 X21.78 Y21.78 Z10.0"

        self.assertTrue(expected_moves == operationMoves,
                       "expected_moves: {}\noperationMoves: {}".format(expected_moves, operationMoves))
        
    def test03(self):
        """test03() Verify path generated on Face18, outside, 
        with compensation and extra offset -radius."""

        # Instantiate a Profile operation and set Base Geometry
        profile = PathProfile.Create("Profile3")
        profile.Base = [(self.doc.Body, ["Face18"])]  # (base, subs_list)
        profile.Label = "test03+"
        profile.Comment = "test03() Verify path generated on Face4, " \
          "with compensation and extra offset -radius"

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
        #FreeCAD.Console.PrintMessage("test03_moves: " + operationMoves + "\n")

        expected_moves = "G1 X18.24 Y18.24 Z10.0;  G3 I-4.24 J-4.24 K0.0 X14.0 Y20.0 Z10.0;  " \
          "G1 X-14.0 Y20.0 Z10.0;  G3 I0.0 J-6.0 K0.0 X-20.0 Y14.0 Z10.0;  " \
          "G1 X-20.0 Y-14.0 Z10.0;  G3 I6.0 J0.0 K0.0 X-14.0 Y-20.0 Z10.0;  " \
          "G1 X14.0 Y-20.0 Z10.0;  G3 I-0.0 J6.0 K0.0 X20.0 Y-14.0 Z10.0;  " \
          "G1 X20.0 Y14.0 Z10.0;  G3 I-6.0 J-0.0 K0.0 X18.24 Y18.24 Z10.0;  " \
          "G1 X21.78 Y21.78 Z10.0;  G2 I-7.78 J-7.78 K0.0 X25.0 Y14.0 Z10.0;  " \
          "G1 X25.0 Y-14.0 Z10.0;  G2 I-11.0 J0.0 K0.0 X14.0 Y-25.0 Z10.0;  " \
          "G1 X-14.0 Y-25.0 Z10.0;  G2 I0.0 J11.0 K0.0 X-25.0 Y-14.0 Z10.0;  " \
          "G1 X-25.0 Y14.0 Z10.0;  G2 I11.0 J-0.0 K0.0 X-14.0 Y25.0 Z10.0;  " \
          "G1 X14.0 Y25.0 Z10.0;  G2 I-0.0 J-11.0 K0.0 X21.78 Y21.78 Z10.0"

        self.assertTrue(expected_moves == operationMoves,
                       "expected_moves: {}\noperationMoves: {}".format(expected_moves, operationMoves))


def _addViewProvider(profileOp):
    if FreeCAD.GuiUp:
        PathOpGui = PathProfileGui.PathOpGui
        cmdRes = PathProfileGui.Command.res
        profileOp.ViewObject.Proxy = PathOpGui.ViewProvider(
            profileOp.ViewObject, cmdRes
        )

