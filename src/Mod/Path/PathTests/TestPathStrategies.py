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
from PathTests.PathTestUtils import PathTestBase
import PathScripts.PathJob as PathJob
import PathScripts.operations.PathClearing as Clearing
if FreeCAD.GuiUp:
    import FreeCADGui
    import PathScripts.operations.PathClearingGui as ClearingGui
    import PathScripts.PathJobGui as PathJobGui

StrategyClearing = Clearing.PathStrategyClearing

class TestPathStrategies(PathTestBase):
    '''Unit tests for the Adaptive operation.'''

    @classmethod
    def setUpClass(cls):
        '''setUpClass()...
        This method is called upon instantiation of this test class.  Add code and objects here
        that are needed for the duration of the test() methods in this class.  In other words,
        set up the 'global' test environment here; use the `setUp()` method to set up a 'local'
        test environment. 
        This method does not have access to the class `self` reference, but it
        is able to call static methods within this same class.
        '''

        # Create new document
        doc = FreeCAD.newDocument("TestPathStrategies")

        # Open existing FreeCAD document with test geometry
        # doc = FreeCAD.open(FreeCAD.getHomePath() + 'Mod/Path/PathTests/test_strategies.fcstd')

        # Create test geometry
        circleSmall = doc.addObject("Part::Feature", "CircleSmall")
        circleSmall.Shape = Part.Face(Part.Wire(Part.makeCircle(10.0)))

        circleLarge = doc.addObject("Part::Feature", "CircleLarge")
        circleLarge.Shape = Part.Face(Part.Wire(Part.makeCircle(25.0)))

        squareSmall = doc.addObject("Part::Feature", "SquareSmall")
        squareSmall.Shape = makeRectangleFace(10.0, 10.0)
        squareSmall.Placement.Base = FreeCAD.Vector(-5.0, -5.0, 0.0)

        squareLarge = doc.addObject("Part::Feature", "SquareLarge")
        squareLarge.Shape = makeRectangleFace(25.0, 25.0)
        squareLarge.Placement.Base = FreeCAD.Vector(-12.5, -12.5, 0.0)

        donutRound = doc.addObject("Part::Feature", "DonutRound")
        donutRound.Shape = circleLarge.Shape.cut(circleSmall.Shape)

        donutSquare = doc.addObject("Part::Feature", "DonutSquare")
        donutSquare.Shape = squareLarge.Shape.cut(squareSmall.Shape)

        # Create Job object, adding geometry objects from file opened above
        modelBases = [circleSmall, circleLarge, squareSmall, squareLarge, donutRound, donutSquare]
        job = PathJob.Create('Job', modelBases, None)
        job.GeometryTolerance.Value = 0.001

        if FreeCAD.GuiUp:
            FreeCADGui.SendMsgToActiveView("ViewFit")
            FreeCADGui.ActiveDocument.activeView().viewIsometric()
            job.ViewObject.Proxy = PathJobGui.ViewProvider(job.ViewObject)
            job.ViewObject.Visibility = False  # Hide job object
            for mb in modelBases:  # Hide source model bases
                mb.ViewObject.Visibility = False

        doc.recompute()

    @classmethod
    def tearDownClass(cls):
        '''tearDownClass()...
        This method is called prior to destruction of this test class.  Add code and objects here
        that cleanup the test environment after the test() methods in this class have been executed.
        This method does not have access to the class `self` reference.  This method
        is able to call static methods within this same class.
        '''
        # FreeCAD.Console.PrintMessage("TestPathAdaptive.tearDownClass()\n")

        # Close geometry document without saving
        FreeCAD.closeDocument(FreeCAD.ActiveDocument.Name)
        pass

    # Setup and tear down methods called before and after each unit test
    def setUp(self):
        '''setUp()...
        This method is called prior to each `test()` method.  Add code and objects here
        that are needed for multiple `test()` methods.
        '''
        self.doc = FreeCAD.ActiveDocument
        self.job = FreeCAD.ActiveDocument.Job
        self.con = FreeCAD.Console

    def tearDown(self):
        '''tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        '''
        pass

    # Unit tests
    # Circular pattern
    def test00(self):
        '''test00() Verify Circular Climb cut pattern.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test00 Circular Climb'
        op.Comment = 'Verify Circular Climb cut pattern.'
        op.CutDirection = 'Climb'
        op.CutPattern = 'Circular'
        op.CutPatternReversed = False
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'CircleLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test00_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test00 == operationMoves,
                        "expected moves test00:\n{}\n\noperation moves test00:\n{}\n".format(expected_moves_test00.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test01(self):
        '''test01() Verify Circular Climb cut pattern Reversed.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test01 Circular Climb Reversed'
        op.Comment = 'Verify Circular Climb cut pattern Reversed.'
        op.CutDirection = 'Climb'
        op.CutPattern = 'Circular'
        op.CutPatternReversed = True
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'CircleLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test01_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test01 == operationMoves,
                        "expected moves test01:\n{}\n\noperation moves test01:\n{}\n".format(expected_moves_test01.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test02(self):
        '''test02() Verify Circular Conventional cut pattern.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test02 Circular Conventional'
        op.Comment = 'Verify Circular Conventional cut pattern.'
        op.CutDirection = 'Conventional'
        op.CutPattern = 'Circular'
        op.CutPatternReversed = False
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'CircleLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test02_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test02 == operationMoves,
                        "expected moves test02:\n{}\n\noperation moves test02:\n{}\n".format(expected_moves_test02.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test03(self):
        '''test03() Verify Circular Conventional cut pattern Reversed.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test03 Circular Conventional Reversed'
        op.Comment = 'Verify Circular Conventional cut pattern Reversed.'
        op.CutDirection = 'Conventional'
        op.CutPattern = 'Circular'
        op.CutPatternReversed = True
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'CircleLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test03_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test03 == operationMoves,
                        "expected moves test03:\n{}\n\noperation moves test03:\n{}\n".format(expected_moves_test03.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    # Line pattern
    def test04(self):
        '''test04() Verify Line Climb cut pattern.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test04 Line Climb'
        op.Comment = 'Verify Line Climb cut pattern.'
        op.CutPattern = 'Line'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Climb'
        op.CutPatternReversed = False
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test04_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test04 == operationMoves,
                        "expected moves test04:\n{}\n\noperation moves test04:\n{}\n".format(expected_moves_test04.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test05(self):
        '''test05() Verify Line Climb cut pattern Reversed.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test05 Line Climb Reversed'
        op.Comment = 'Verify Line Climb cut pattern Reversed.'
        op.CutPattern = 'Line'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Climb'
        op.CutPatternReversed = True
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test05_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test05 == operationMoves,
                        "expected moves test05:\n{}\n\noperation moves test05:\n{}\n".format(expected_moves_test05.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test06(self):
        '''test06() Verify Line Conventional cut pattern.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test06 Line Conventional'
        op.Comment = 'Verify Line Conventional cut pattern.'
        op.CutPattern = 'Line'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Conventional'
        op.CutPatternReversed = False
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test06_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test06 == operationMoves,
                        "expected moves test06:\n{}\n\noperation moves test06:\n{}\n".format(expected_moves_test06.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test07(self):
        '''test07() Verify Line Conventional cut pattern Reversed.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test07 Line Conventional Reversed'
        op.Comment = 'Verify Line Conventional cut pattern Reversed.'
        op.CutPattern = 'Line'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Conventional'
        op.CutPatternReversed = True
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test07_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test07 == operationMoves,
                        "expected moves test07:\n{}\n\noperation moves test07:\n{}\n".format(expected_moves_test07.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    # ZigZag pattern
    def test08(self):
        '''test08() Verify ZigZag Climb cut pattern.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test08 ZigZag Climb'
        op.Comment = 'Verify ZigZag Climb cut pattern.'
        op.CutPattern = 'ZigZag'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Climb'
        op.CutPatternReversed = False
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test08_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test08 == operationMoves,
                        "expected moves test08:\n{}\n\noperation moves test08:\n{}\n".format(expected_moves_test08.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test09(self):
        '''test09() Verify ZigZag Climb cut pattern Reversed.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test09 ZigZag Climb Reversed'
        op.Comment = 'Verify ZigZag Climb cut pattern Reversed.'
        op.CutPattern = 'ZigZag'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Climb'
        op.CutPatternReversed = True
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test09_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test09 == operationMoves,
                        "expected moves test09:\n{}\n\noperation moves test09:\n{}\n".format(expected_moves_test09.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test10(self):
        '''test10() Verify ZigZag Conventional cut pattern.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test10 ZigZag Conventional'
        op.Comment = 'Verify ZigZag Conventional cut pattern.'
        op.CutPattern = 'ZigZag'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Conventional'
        op.CutPatternReversed = False
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test10_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test10 == operationMoves,
                        "expected moves test10:\n{}\n\noperation moves test10:\n{}\n".format(expected_moves_test10.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test11(self):
        '''test11() Verify ZigZag Conventional cut pattern Reversed.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test11 ZigZag Conventional Reversed'
        op.Comment = 'Verify ZigZag Conventional cut pattern Reversed.'
        op.CutPattern = 'ZigZag'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Conventional'
        op.CutPatternReversed = True
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test11_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test11 == operationMoves,
                        "expected moves test11:\n{}\n\noperation moves test11:\n{}\n".format(expected_moves_test11.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    # Offset pattern
    def test12(self):
        '''test12() Verify Offset Climb cut pattern.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test12 Offset Climb'
        op.Comment = 'Verify Offset Climb cut pattern.'
        op.CutPattern = 'Offset'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Climb'
        op.CutPatternReversed = False
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test12_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test12 == operationMoves,
                        "expected moves test12:\n{}\n\noperation moves test12:\n{}\n".format(expected_moves_test12.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test13(self):
        '''test13() Verify Offset Climb cut pattern Reversed.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test13 Offset Climb Reversed'
        op.Comment = 'Verify Offset Climb cut pattern Reversed.'
        op.CutPattern = 'Offset'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Climb'
        op.CutPatternReversed = True
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test13_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test13 == operationMoves,
                        "expected moves test13:\n{}\n\noperation moves test13:\n{}\n".format(expected_moves_test13.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test14(self):
        '''test14() Verify Offset Conventional cut pattern.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test14 Offset Conventional'
        op.Comment = 'Verify Offset Conventional cut pattern.'
        op.CutPattern = 'Offset'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Conventional'
        op.CutPatternReversed = False
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test14_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test14 == operationMoves,
                        "expected moves test14:\n{}\n\noperation moves test14:\n{}\n".format(expected_moves_test14.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test15(self):
        '''test15() Verify Offset Conventional cut pattern Reversed.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test15 Offset Conventional Reversed'
        op.Comment = 'Verify Offset Conventional cut pattern Reversed.'
        op.CutPattern = 'Offset'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Conventional'
        op.CutPatternReversed = True
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test15_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test15 == operationMoves,
                        "expected moves test15:\n{}\n\noperation moves test15:\n{}\n".format(expected_moves_test15.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    # Spiral pattern
    def test16(self):
        '''test16() Verify Spiral Climb cut pattern.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test16 Spiral Climb'
        op.Comment = 'Verify Spiral Climb cut pattern.'
        op.CutPattern = 'Spiral'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Climb'
        op.CutPatternReversed = False
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test16_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test16 == operationMoves,
                        "expected moves test16:\n{}\n\noperation moves test16:\n{}\n".format(expected_moves_test16.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test17(self):
        '''test17() Verify Spiral Climb cut pattern Reversed.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test17 Spiral Climb Reversed'
        op.Comment = 'Verify Spiral Climb cut pattern Reversed.'
        op.CutPattern = 'Spiral'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Climb'
        op.CutPatternReversed = True
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test17_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test17 == operationMoves,
                        "expected moves test17:\n{}\n\noperation moves test17:\n{}\n".format(expected_moves_test17.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test18(self):
        '''test18() Verify Spiral Conventional cut pattern.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test18 Spiral Conventional'
        op.Comment = 'Verify Spiral Conventional cut pattern.'
        op.CutPattern = 'Spiral'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Conventional'
        op.CutPatternReversed = False
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test18_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test18 == operationMoves,
                        "expected moves test18:\n{}\n\noperation moves test18:\n{}\n".format(expected_moves_test18.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    def test19(self):
        '''test19() Verify Spiral Conventional cut pattern Reversed.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test19 Spiral Conventional Reversed'
        op.Comment = 'Verify Spiral Conventional cut pattern Reversed.'
        op.CutPattern = 'Spiral'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Conventional'
        op.CutPatternReversed = True
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'SquareLarge'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test19_moves: " + operationMoves + "\n")
        self.assertTrue(expected_moves_test19 == operationMoves,
                        "expected moves test19:\n{}\n\noperation moves test19:\n{}\n".format(expected_moves_test19.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))

    # LineOffset pattern
    def test20(self):
        '''test20() Verify LineOffset Climb cut pattern.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test20 LineOffset Climb'
        op.Comment = 'Verify LineOffset Climb cut pattern.'
        op.CutPattern = 'LineOffset'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Climb'
        op.CutPatternReversed = False
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'DonutSquare'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test20_moves: " + operationMoves + "\n")
        # self.assertTrue(expected_moves_test20 == operationMoves,
        #                "expected moves test20:\n{}\n\noperation moves test20:\n{}\n".format(expected_moves_test20.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))
        # self.con.PrintMessage("test20_moves: " + "\n")
        # for m in moves:
        #    self.con.PrintMessage("  {};\\".format(m))
        self.assertTrue(expected_moves_test20 == operationMoves, "test20: Operation gcode does not match expected gcode.")

    def test21(self):
        '''test21() Verify LineOffset Climb cut pattern Reversed.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test21 LineOffset Climb Reversed'
        op.Comment = 'Verify LineOffset Climb cut pattern Reversed.'
        op.CutPattern = 'LineOffset'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Climb'
        op.CutPatternReversed = True
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'DonutSquare'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test21_moves: " + operationMoves + "\n")
        # self.assertTrue(expected_moves_test21 == operationMoves,
        #                "expected moves test21:\n{}\n\noperation moves test21:\n{}\n".format(expected_moves_test21.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))
        # self.con.PrintMessage("test21_moves: " + "\n")
        # for m in moves:
        #    self.con.PrintMessage("  {};\\".format(m))
        self.assertTrue(expected_moves_test21 == operationMoves, "test21: Operation gcode does not match expected gcode.")

    def test22(self):
        '''test22() Verify LineOffset Conventional cut pattern.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test22 LineOffset Conventional'
        op.Comment = 'Verify LineOffset Conventional cut pattern.'
        op.CutPattern = 'LineOffset'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Conventional'
        op.CutPatternReversed = False
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'DonutSquare'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test22_moves: " + operationMoves + "\n")
        # self.assertTrue(expected_moves_test22 == operationMoves,
        #                "expected moves test22:\n{}\n\noperation moves test22:\n{}\n".format(expected_moves_test22.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))
        # self.con.PrintMessage("test22_moves: " + "\n")
        # for m in moves:
        #    self.con.PrintMessage("  {};\\".format(m))
        self.assertTrue(expected_moves_test22 == operationMoves, "test22: Operation gcode does not match expected gcode.")

    def test23(self):
        '''test23() Verify LineOffset Conventional cut pattern Reversed.'''
        op = Clearing.Create('Clearing')
        op.Label = 'test23 LineOffset Conventional Reversed'
        op.Comment = 'Verify LineOffset Conventional cut pattern Reversed.'
        op.CutPattern = 'LineOffset'
        op.CutPatternAngle = 0.0
        op.CutDirection = 'Conventional'
        op.CutPatternReversed = True
        op.StepOver = 100.0
        op.Base = [(findModelForBase(self.job, 'DonutSquare'), ['Face1'])]

        self.doc.recompute()

        _addViewProvider(op)
        self.doc.recompute()

        moves = getGcodeMoves(op.Path.Commands)
        operationMoves = ";  ".join(moves)
        # self.con.PrintMessage("test23_moves: " + operationMoves + "\n")
        # self.assertTrue(expected_moves_test23 == operationMoves,
        #                "expected moves test23:\n{}\n\noperation moves test23:\n{}\n".format(expected_moves_test23.replace(";  ", ";\\\n  "), operationMoves.replace(";  ", ";\\\n  ")))
        # self.con.PrintMessage("test23_moves: " + "\n")
        # for m in moves:
        #    self.con.PrintMessage("  {};\\".format(m))
        self.assertTrue(expected_moves_test23 == operationMoves, "test23: Operation gcode does not match expected gcode.")
# Eclass


def setDepthsAndHeights(op, strDep=20.0, finDep=0.0):
    '''setDepthsAndHeights(op, strDep=20.0, finDep=0.0)... Sets default depths and heights for `op` passed to it'''

    # Set start and final depth in order to eliminate effects of stock (and its default values)
    op.setExpression('StartDepth', None)
    op.StartDepth.Value = strDep
    op.setExpression('FinalDepth', None)
    op.FinalDepth.Value = finDep

    # Set step down so as to only produce one layer path
    op.setExpression('StepDown', None)
    op.StepDown.Value = 20.0

    # Set Heights
    # default values used


def getGcodeMoves(cmdList, includeRapids=True, includeLines=True, includeArcs=True):
    '''getGcodeMoves(cmdList, includeRapids=True, includeLines=True, includeArcs=True)...
    Accepts command dict and returns point string coordinate.
    '''
    gcode_list = list()
    last = FreeCAD.Vector(0.0, 0.0, 0.0)

    def processMove(p, last):
        gcode = ""
        xx = None
        yy = None
        zz = None
        x = last.x
        y = last.y
        z = last.z

        if "X" in p.keys():
            v = round(p["X"], 2)
            if v != last.x:
                xx = v
        if "Y" in p.keys():
            v = round(p["Y"], 2)
            if v != last.y:
                yy = v
        if "Z" in p.keys():
            v = round(p["Z"], 2)
            if v != last.z:
                zz = v

        if xx is not None:
            gcode += " X" + str(xx)
            last.x = xx
        if yy is not None:
            gcode += " Y" + str(yy)
            last.y = yy
        if zz is not None:
            gcode += " Z" + str(zz)
            last.z = zz

        return gcode

    for c in cmdList:
        p = c.Parameters
        name = c.Name
        if includeRapids and name in ["G0", "G00"]:
            move = processMove(p, last)
            if move:
                gcode_list.append(name + move)
        elif includeLines and name in ["G1", "G01"]:
            move = processMove(p, last)
            if move:
                gcode_list.append(name + move)
        elif includeArcs and name in ["G2", "G3", "G02", "G03"]:
            gcode = name
            x = last.x
            y = last.y
            z = last.z
            if p.get("Z"):
                z = round(p["Z"], 2)
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


def _addViewProvider(op):
    if FreeCAD.GuiUp:
        PathOpGui = ClearingGui.PathOpGui
        cmdRes = ClearingGui.Command.res
        op.ViewObject.Proxy = PathOpGui.ViewProvider(op.ViewObject, cmdRes)
        op.ViewObject.Proxy.deleteOnReject = False
        op.ViewObject.Visibility = False  # Hide operation


def findModelForBase(job, base):
    if isinstance(base, str):
        baseName = base
    else:
        baseName = base.Name
    models = job.Model.Group
    for m in models:
        if m.Objects[0].Name == baseName:
            return m
    return None


def makeRectangleFace(length, width):
    '''makeBoundBoxFace(bBox, offset=0.0, zHeight=0.0)...
    Function to create boundbox face, with possible extra offset and custom Z-height.'''
    p1 = FreeCAD.Vector(0.0, 0.0, 0.0)
    p2 = FreeCAD.Vector(length, 0.0, 0.0)
    p3 = FreeCAD.Vector(length, width, 0.0)
    p4 = FreeCAD.Vector(0.0, width, 0.0)

    L1 = Part.makeLine(p1, p2)
    L2 = Part.makeLine(p2, p3)
    L3 = Part.makeLine(p3, p4)
    L4 = Part.makeLine(p4, p1)

    return Part.Face(Part.Wire([L1, L2, L3, L4]))


# Expected path commands for various tests
expected_moves_test00 = "G0 Z6.0;\
  G0 X20.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G2 I-20.0 J0.0 K0.0 X-20.0 Y-0.0 Z0.0;\
  G2 I20.0 J0.0 K0.0 X20.0 Y0.0 Z0.0;\
  G0 Z6.0;\
  G0 X15.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G2 I-15.0 J0.0 K0.0 X-15.0 Y-0.0 Z0.0;\
  G2 I15.0 J0.0 K0.0 X15.0 Y0.0 Z0.0;\
  G0 Z6.0;\
  G0 X10.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G2 I-10.0 J0.0 K0.0 X-10.0 Y-0.0 Z0.0;\
  G2 I10.0 J0.0 K0.0 X10.0 Y0.0 Z0.0;\
  G0 Z6.0;\
  G0 X5.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G2 I-5.0 J0.0 K0.0 X-5.0 Y-0.0 Z0.0;\
  G2 I5.0 J0.0 K0.0 X5.0 Y0.0 Z0.0;\
  G0 Z6.0"

expected_moves_test01 = "G0 Z6.0;\
  G0 X5.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G3 I-5.0 J0.0 K0.0 X-5.0 Y0.0 Z0.0;\
  G3 I5.0 J-0.0 K0.0 X5.0 Y-0.0 Z0.0;\
  G0 Z6.0;\
  G0 X10.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G3 I-10.0 J0.0 K0.0 X-10.0 Y0.0 Z0.0;\
  G3 I10.0 J-0.0 K0.0 X10.0 Y-0.0 Z0.0;\
  G0 Z6.0;\
  G0 X15.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G3 I-15.0 J0.0 K0.0 X-15.0 Y0.0 Z0.0;\
  G3 I15.0 J-0.0 K0.0 X15.0 Y-0.0 Z0.0;\
  G0 Z6.0;\
  G0 X20.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G3 I-20.0 J0.0 K0.0 X-20.0 Y0.0 Z0.0;\
  G3 I20.0 J-0.0 K0.0 X20.0 Y-0.0 Z0.0;\
  G0 Z6.0"

expected_moves_test02 = "G0 Z6.0;\
  G0 X20.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G3 I-20.0 J0.0 K0.0 X-20.0 Y0.0 Z0.0;\
  G3 I20.0 J-0.0 K0.0 X20.0 Y-0.0 Z0.0;\
  G0 Z6.0;\
  G0 X15.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G3 I-15.0 J0.0 K0.0 X-15.0 Y0.0 Z0.0;\
  G3 I15.0 J-0.0 K0.0 X15.0 Y-0.0 Z0.0;\
  G0 Z6.0;\
  G0 X10.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G3 I-10.0 J0.0 K0.0 X-10.0 Y0.0 Z0.0;\
  G3 I10.0 J-0.0 K0.0 X10.0 Y-0.0 Z0.0;\
  G0 Z6.0;\
  G0 X5.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G3 I-5.0 J0.0 K0.0 X-5.0 Y0.0 Z0.0;\
  G3 I5.0 J-0.0 K0.0 X5.0 Y-0.0 Z0.0;\
  G0 Z6.0"

expected_moves_test03 = "G0 Z6.0;\
  G0 X5.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G2 I-5.0 J0.0 K0.0 X-5.0 Y-0.0 Z0.0;\
  G2 I5.0 J0.0 K0.0 X5.0 Y0.0 Z0.0;\
  G0 Z6.0;\
  G0 X10.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G2 I-10.0 J0.0 K0.0 X-10.0 Y-0.0 Z0.0;\
  G2 I10.0 J0.0 K0.0 X10.0 Y0.0 Z0.0;\
  G0 Z6.0;\
  G0 X15.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G2 I-15.0 J0.0 K0.0 X-15.0 Y-0.0 Z0.0;\
  G2 I15.0 J0.0 K0.0 X15.0 Y0.0 Z0.0;\
  G0 Z6.0;\
  G0 X20.0;\
  G0 Z4.0;\
  G1 Z0.0;\
  G2 I-20.0 J0.0 K0.0 X-20.0 Y-0.0 Z0.0;\
  G2 I20.0 J0.0 K0.0 X20.0 Y0.0 Z0.0;\
  G0 Z6.0"

expected_moves_test04 = "G0 Z6.0;\
  G0 X10.0 Y-10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y-5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y0.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test05 = "G0 Z6.0;\
  G0 X-10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y0.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y-5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y-10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test06 = "G0 Z6.0;\
  G0 X-10.0 Y-10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y-5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y0.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test07 = "G0 Z6.0;\
  G0 X10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y0.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y-5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y-10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test08 = "G0 Z6.0;\
  G0 X-10.0 Y-10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 Y-5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 Y0.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test09 = "G0 Z6.0;\
  G0 X10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 Y0.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 Y-5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 Y-10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test10 = "G0 Z6.0;\
  G0 X10.0 Y-10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 Y-5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 Y0.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test11 = "G0 Z6.0;\
  G0 X-10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 Y0.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 Y-5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 Y-10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test12 = "G0 Z6.0;\
  G0 X10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 Y-10.0;\
  G1 X-10.0;\
  G1 Y10.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X5.0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 Y-5.0;\
  G1 X-5.0;\
  G1 Y5.0;\
  G1 X5.0;\
  G0 Z4.0;\
  G0 X0.0 Y0.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test13 = "G0 Z6.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G0 Z4.0;\
  G0 X5.0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-5.0;\
  G1 Y-5.0;\
  G1 X5.0;\
  G1 Y5.0;\
  G0 Z4.0;\
  G0 X10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G1 Y-10.0;\
  G1 X10.0;\
  G1 Y10.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test14 = "G0 Z6.0;\
  G0 X10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G1 Y-10.0;\
  G1 X10.0;\
  G1 Y10.0;\
  G0 Z4.0;\
  G0 X5.0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-5.0;\
  G1 Y-5.0;\
  G1 X5.0;\
  G1 Y5.0;\
  G0 Z4.0;\
  G0 X0.0 Y0.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test15 = "G0 Z6.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G0 Z4.0;\
  G0 X5.0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 Y-5.0;\
  G1 X-5.0;\
  G1 Y5.0;\
  G1 X5.0;\
  G0 Z4.0;\
  G0 X10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 Y-10.0;\
  G1 X-10.0;\
  G1 Y10.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test16 = "G0 Z6.0;\
  G0 X-8.57 Y-10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-9.92 Y-8.5;\
  G1 X-10.0 Y-8.37;\
  G0 Z4.0;\
  G0 Y6.64;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-9.74 Y7.02;\
  G1 X-8.34 Y8.44;\
  G1 X-6.75 Y9.6;\
  G1 X-5.95 Y10.0;\
  G0 Z4.0;\
  G0 X4.35;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X5.69 Y9.19;\
  G1 X7.04 Y8.02;\
  G1 X8.17 Y6.66;\
  G1 X9.05 Y5.15;\
  G1 X9.65 Y3.53;\
  G1 X9.98 Y1.85;\
  G1 X9.93 Y-0.66;\
  G1 X9.26 Y-3.03;\
  G1 X8.06 Y-5.12;\
  G1 X6.4 Y-6.81;\
  G1 X4.42 Y-8.01;\
  G1 X2.25 Y-8.66;\
  G1 X0.04 Y-8.75;\
  G1 X-2.08 Y-8.3;\
  G1 X-3.97 Y-7.35;\
  G1 X-5.53 Y-5.99;\
  G1 X-6.68 Y-4.33;\
  G1 X-7.35 Y-2.48;\
  G1 X-7.54 Y-0.57;\
  G1 X-7.25 Y1.28;\
  G1 X-6.53 Y2.95;\
  G1 X-5.44 Y4.35;\
  G1 X-4.07 Y5.4;\
  G1 X-2.53 Y6.06;\
  G1 X-0.93 Y6.3;\
  G1 X0.64 Y6.13;\
  G1 X2.07 Y5.6;\
  G1 X3.28 Y4.75;\
  G1 X4.2 Y3.66;\
  G1 X4.8 Y2.42;\
  G1 X5.05 Y1.11;\
  G1 X4.58 Y-1.33;\
  G1 X3.1 Y-3.09;\
  G1 X1.13 Y-3.82;\
  G1 X-0.75 Y-3.5;\
  G1 X-2.08 Y-2.41;\
  G1 X-2.61 Y-0.98;\
  G1 X-2.36 Y0.34;\
  G1 X-1.59 Y1.19;\
  G1 X-0.66 Y1.45;\
  G1 X0.08 Y1.19;\
  G1 X0.43 Y0.67;\
  G1 X0.35 Y0.19;\
  G1 X0.0 Y0.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test17 = "G0 Z6.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X0.35 Y0.19;\
  G1 X0.43 Y0.67;\
  G1 X0.08 Y1.19;\
  G1 X-0.66 Y1.45;\
  G1 X-1.59 Y1.19;\
  G1 X-2.36 Y0.34;\
  G1 X-2.61 Y-0.98;\
  G1 X-2.08 Y-2.41;\
  G1 X-0.75 Y-3.5;\
  G1 X1.13 Y-3.82;\
  G1 X3.1 Y-3.09;\
  G1 X4.58 Y-1.33;\
  G1 X5.05 Y1.11;\
  G1 X4.8 Y2.42;\
  G1 X4.2 Y3.66;\
  G1 X3.28 Y4.75;\
  G1 X2.07 Y5.6;\
  G1 X0.64 Y6.13;\
  G1 X-0.93 Y6.3;\
  G1 X-2.53 Y6.06;\
  G1 X-4.07 Y5.4;\
  G1 X-5.44 Y4.35;\
  G1 X-6.53 Y2.95;\
  G1 X-7.25 Y1.28;\
  G1 X-7.54 Y-0.57;\
  G1 X-7.35 Y-2.48;\
  G1 X-6.68 Y-4.33;\
  G1 X-5.53 Y-5.99;\
  G1 X-3.97 Y-7.35;\
  G1 X-2.08 Y-8.3;\
  G1 X0.04 Y-8.75;\
  G1 X2.25 Y-8.66;\
  G1 X4.42 Y-8.01;\
  G1 X6.4 Y-6.81;\
  G1 X8.06 Y-5.12;\
  G1 X9.26 Y-3.03;\
  G1 X9.93 Y-0.66;\
  G1 X9.98 Y1.85;\
  G1 X9.65 Y3.53;\
  G1 X9.05 Y5.15;\
  G1 X8.17 Y6.66;\
  G1 X7.04 Y8.02;\
  G1 X5.69 Y9.19;\
  G1 X4.35 Y10.0;\
  G0 Z4.0;\
  G0 X-5.95;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-6.75 Y9.6;\
  G1 X-8.34 Y8.44;\
  G1 X-9.74 Y7.02;\
  G1 X-10.0 Y6.64;\
  G0 Z4.0;\
  G0 Y-8.37;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-9.92 Y-8.5;\
  G1 X-8.57 Y-10.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test18 = "G0 Z6.0;\
  G0 X8.57 Y-10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X9.92 Y-8.5;\
  G1 X10.0 Y-8.37;\
  G0 Z4.0;\
  G0 Y6.64;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X9.74 Y7.02;\
  G1 X8.34 Y8.44;\
  G1 X6.75 Y9.6;\
  G1 X5.95 Y10.0;\
  G0 Z4.0;\
  G0 X-4.35;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-5.69 Y9.19;\
  G1 X-7.04 Y8.02;\
  G1 X-8.17 Y6.66;\
  G1 X-9.05 Y5.15;\
  G1 X-9.65 Y3.53;\
  G1 X-9.98 Y1.85;\
  G1 X-9.93 Y-0.66;\
  G1 X-9.26 Y-3.03;\
  G1 X-8.06 Y-5.12;\
  G1 X-6.4 Y-6.81;\
  G1 X-4.42 Y-8.01;\
  G1 X-2.25 Y-8.66;\
  G1 X-0.04 Y-8.75;\
  G1 X2.08 Y-8.3;\
  G1 X3.97 Y-7.35;\
  G1 X5.53 Y-5.99;\
  G1 X6.68 Y-4.33;\
  G1 X7.35 Y-2.48;\
  G1 X7.54 Y-0.57;\
  G1 X7.25 Y1.28;\
  G1 X6.53 Y2.95;\
  G1 X5.44 Y4.35;\
  G1 X4.07 Y5.4;\
  G1 X2.53 Y6.06;\
  G1 X0.93 Y6.3;\
  G1 X-0.64 Y6.13;\
  G1 X-2.07 Y5.6;\
  G1 X-3.28 Y4.75;\
  G1 X-4.2 Y3.66;\
  G1 X-4.8 Y2.42;\
  G1 X-5.05 Y1.11;\
  G1 X-4.58 Y-1.33;\
  G1 X-3.1 Y-3.09;\
  G1 X-1.13 Y-3.82;\
  G1 X0.75 Y-3.5;\
  G1 X2.08 Y-2.41;\
  G1 X2.61 Y-0.98;\
  G1 X2.36 Y0.34;\
  G1 X1.59 Y1.19;\
  G1 X0.66 Y1.45;\
  G1 X-0.08 Y1.19;\
  G1 X-0.43 Y0.67;\
  G1 X-0.35 Y0.19;\
  G1 X0.0 Y0.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test19 = "G0 Z6.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-0.35 Y0.19;\
  G1 X-0.43 Y0.67;\
  G1 X-0.08 Y1.19;\
  G1 X0.66 Y1.45;\
  G1 X1.59 Y1.19;\
  G1 X2.36 Y0.34;\
  G1 X2.61 Y-0.98;\
  G1 X2.08 Y-2.41;\
  G1 X0.75 Y-3.5;\
  G1 X-1.13 Y-3.82;\
  G1 X-3.1 Y-3.09;\
  G1 X-4.58 Y-1.33;\
  G1 X-5.05 Y1.11;\
  G1 X-4.8 Y2.42;\
  G1 X-4.2 Y3.66;\
  G1 X-3.28 Y4.75;\
  G1 X-2.07 Y5.6;\
  G1 X-0.64 Y6.13;\
  G1 X0.93 Y6.3;\
  G1 X2.53 Y6.06;\
  G1 X4.07 Y5.4;\
  G1 X5.44 Y4.35;\
  G1 X6.53 Y2.95;\
  G1 X7.25 Y1.28;\
  G1 X7.54 Y-0.57;\
  G1 X7.35 Y-2.48;\
  G1 X6.68 Y-4.33;\
  G1 X5.53 Y-5.99;\
  G1 X3.97 Y-7.35;\
  G1 X2.08 Y-8.3;\
  G1 X-0.04 Y-8.75;\
  G1 X-2.25 Y-8.66;\
  G1 X-4.42 Y-8.01;\
  G1 X-6.4 Y-6.81;\
  G1 X-8.06 Y-5.12;\
  G1 X-9.26 Y-3.03;\
  G1 X-9.93 Y-0.66;\
  G1 X-9.98 Y1.85;\
  G1 X-9.65 Y3.53;\
  G1 X-9.05 Y5.15;\
  G1 X-8.17 Y6.66;\
  G1 X-7.04 Y8.02;\
  G1 X-5.69 Y9.19;\
  G1 X-4.35 Y10.0;\
  G0 Z4.0;\
  G0 X5.95;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X6.75 Y9.6;\
  G1 X8.34 Y8.44;\
  G1 X9.74 Y7.02;\
  G1 X10.0 Y6.64;\
  G0 Z4.0;\
  G0 Y-8.37;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X9.92 Y-8.5;\
  G1 X8.57 Y-10.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test20 = "G0 Z6.0;\
  G0 X10.0 Y-10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y-5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X7.5;\
  G0 Z4.0;\
  G0 X-7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y0.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X7.5;\
  G0 Z4.0;\
  G0 X-7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X7.5;\
  G0 Z4.0;\
  G0 X-7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 Y-10.0;\
  G1 X10.0;\
  G1 Y10.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X-5.04 Y-7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-5.09;\
  G1 X-5.13;\
  G1 X-5.18 Y-7.49;\
  G1 X-5.22;\
  G1 X-5.27;\
  G1 X-5.31 Y-7.48;\
  G1 X-5.36 Y-7.47;\
  G1 X-5.4;\
  G1 X-5.44 Y-7.46;\
  G1 X-5.49 Y-7.45;\
  G1 X-5.53 Y-7.44;\
  G1 X-5.57 Y-7.43;\
  G1 X-5.62 Y-7.42;\
  G1 X-5.66 Y-7.41;\
  G1 X-5.7 Y-7.4;\
  G1 X-5.75 Y-7.39;\
  G1 X-5.79 Y-7.37;\
  G1 X-5.83 Y-7.36;\
  G1 X-5.87 Y-7.34;\
  G1 X-5.92 Y-7.33;\
  G1 X-5.96 Y-7.31;\
  G1 X-6.0 Y-7.29;\
  G1 X-6.04 Y-7.27;\
  G1 X-6.08 Y-7.26;\
  G1 X-6.12 Y-7.24;\
  G1 X-6.16 Y-7.22;\
  G1 X-6.2 Y-7.19;\
  G1 X-6.24 Y-7.17;\
  G1 X-6.28 Y-7.15;\
  G1 X-6.31 Y-7.13;\
  G1 X-6.35 Y-7.1;\
  G1 X-6.39 Y-7.08;\
  G1 X-6.43 Y-7.05;\
  G1 X-6.46 Y-7.03;\
  G1 X-6.5 Y-7.0;\
  G1 X-6.53 Y-6.97;\
  G1 X-6.57 Y-6.95;\
  G1 X-6.6 Y-6.92;\
  G1 X-6.64 Y-6.89;\
  G1 X-6.67 Y-6.86;\
  G1 X-6.7 Y-6.83;\
  G1 X-6.74 Y-6.8;\
  G1 X-6.77 Y-6.77;\
  G1 X-6.8 Y-6.74;\
  G1 X-6.83 Y-6.7;\
  G1 X-6.86 Y-6.67;\
  G1 X-6.89 Y-6.64;\
  G1 X-6.92 Y-6.6;\
  G1 X-6.95 Y-6.57;\
  G1 X-6.97 Y-6.53;\
  G1 X-7.0 Y-6.5;\
  G1 X-7.03 Y-6.46;\
  G1 X-7.05 Y-6.43;\
  G1 X-7.08 Y-6.39;\
  G1 X-7.1 Y-6.35;\
  G1 X-7.13 Y-6.31;\
  G1 X-7.15 Y-6.28;\
  G1 X-7.17 Y-6.24;\
  G1 X-7.19 Y-6.2;\
  G1 X-7.22 Y-6.16;\
  G1 X-7.24 Y-6.12;\
  G1 X-7.26 Y-6.08;\
  G1 X-7.27 Y-6.04;\
  G1 X-7.29 Y-6.0;\
  G1 X-7.31 Y-5.96;\
  G1 X-7.33 Y-5.92;\
  G1 X-7.34 Y-5.87;\
  G1 X-7.36 Y-5.83;\
  G1 X-7.37 Y-5.79;\
  G1 X-7.39 Y-5.75;\
  G1 X-7.4 Y-5.7;\
  G1 X-7.41 Y-5.66;\
  G1 X-7.42 Y-5.62;\
  G1 X-7.43 Y-5.57;\
  G1 X-7.44 Y-5.53;\
  G1 X-7.45 Y-5.49;\
  G1 X-7.46 Y-5.44;\
  G1 X-7.47 Y-5.4;\
  G1 Y-5.36;\
  G1 X-7.48 Y-5.31;\
  G1 X-7.49 Y-5.27;\
  G1 Y-5.22;\
  G1 Y-5.18;\
  G1 X-7.5 Y-5.13;\
  G1 Y-5.09;\
  G1 Y-5.04;\
  G1 Y-5.0;\
  G1 Y5.0;\
  G1 Y5.04;\
  G1 Y5.09;\
  G1 Y5.13;\
  G1 X-7.49 Y5.18;\
  G1 Y5.22;\
  G1 Y5.27;\
  G1 X-7.48 Y5.31;\
  G1 X-7.47 Y5.36;\
  G1 Y5.4;\
  G1 X-7.46 Y5.44;\
  G1 X-7.45 Y5.49;\
  G1 X-7.44 Y5.53;\
  G1 X-7.43 Y5.57;\
  G1 X-7.42 Y5.62;\
  G1 X-7.41 Y5.66;\
  G1 X-7.4 Y5.7;\
  G1 X-7.39 Y5.75;\
  G1 X-7.37 Y5.79;\
  G1 X-7.36 Y5.83;\
  G1 X-7.34 Y5.87;\
  G1 X-7.33 Y5.92;\
  G1 X-7.31 Y5.96;\
  G1 X-7.29 Y6.0;\
  G1 X-7.27 Y6.04;\
  G1 X-7.26 Y6.08;\
  G1 X-7.24 Y6.12;\
  G1 X-7.22 Y6.16;\
  G1 X-7.19 Y6.2;\
  G1 X-7.17 Y6.24;\
  G1 X-7.15 Y6.28;\
  G1 X-7.13 Y6.31;\
  G1 X-7.1 Y6.35;\
  G1 X-7.08 Y6.39;\
  G1 X-7.05 Y6.43;\
  G1 X-7.03 Y6.46;\
  G1 X-7.0 Y6.5;\
  G1 X-6.97 Y6.53;\
  G1 X-6.95 Y6.57;\
  G1 X-6.92 Y6.6;\
  G1 X-6.89 Y6.64;\
  G1 X-6.86 Y6.67;\
  G1 X-6.83 Y6.7;\
  G1 X-6.8 Y6.74;\
  G1 X-6.77 Y6.77;\
  G1 X-6.74 Y6.8;\
  G1 X-6.7 Y6.83;\
  G1 X-6.67 Y6.86;\
  G1 X-6.64 Y6.89;\
  G1 X-6.6 Y6.92;\
  G1 X-6.57 Y6.95;\
  G1 X-6.53 Y6.97;\
  G1 X-6.5 Y7.0;\
  G1 X-6.46 Y7.03;\
  G1 X-6.43 Y7.05;\
  G1 X-6.39 Y7.08;\
  G1 X-6.35 Y7.1;\
  G1 X-6.31 Y7.13;\
  G1 X-6.28 Y7.15;\
  G1 X-6.24 Y7.17;\
  G1 X-6.2 Y7.19;\
  G1 X-6.16 Y7.22;\
  G1 X-6.12 Y7.24;\
  G1 X-6.08 Y7.26;\
  G1 X-6.04 Y7.27;\
  G1 X-6.0 Y7.29;\
  G1 X-5.96 Y7.31;\
  G1 X-5.92 Y7.33;\
  G1 X-5.87 Y7.34;\
  G1 X-5.83 Y7.36;\
  G1 X-5.79 Y7.37;\
  G1 X-5.75 Y7.39;\
  G1 X-5.7 Y7.4;\
  G1 X-5.66 Y7.41;\
  G1 X-5.62 Y7.42;\
  G1 X-5.57 Y7.43;\
  G1 X-5.53 Y7.44;\
  G1 X-5.49 Y7.45;\
  G1 X-5.44 Y7.46;\
  G1 X-5.4 Y7.47;\
  G1 X-5.36;\
  G1 X-5.31 Y7.48;\
  G1 X-5.27 Y7.49;\
  G1 X-5.22;\
  G1 X-5.18;\
  G1 X-5.13 Y7.5;\
  G1 X-5.09;\
  G1 X-5.04;\
  G1 X-5.0;\
  G1 X5.0;\
  G1 X5.04;\
  G1 X5.09;\
  G1 X5.13;\
  G1 X5.18 Y7.49;\
  G1 X5.22;\
  G1 X5.27;\
  G1 X5.31 Y7.48;\
  G1 X5.36 Y7.47;\
  G1 X5.4;\
  G1 X5.44 Y7.46;\
  G1 X5.49 Y7.45;\
  G1 X5.53 Y7.44;\
  G1 X5.57 Y7.43;\
  G1 X5.62 Y7.42;\
  G1 X5.66 Y7.41;\
  G1 X5.7 Y7.4;\
  G1 X5.75 Y7.39;\
  G1 X5.79 Y7.37;\
  G1 X5.83 Y7.36;\
  G1 X5.87 Y7.34;\
  G1 X5.92 Y7.33;\
  G1 X5.96 Y7.31;\
  G1 X6.0 Y7.29;\
  G1 X6.04 Y7.27;\
  G1 X6.08 Y7.26;\
  G1 X6.12 Y7.24;\
  G1 X6.16 Y7.22;\
  G1 X6.2 Y7.19;\
  G1 X6.24 Y7.17;\
  G1 X6.28 Y7.15;\
  G1 X6.31 Y7.13;\
  G1 X6.35 Y7.1;\
  G1 X6.39 Y7.08;\
  G1 X6.43 Y7.05;\
  G1 X6.46 Y7.03;\
  G1 X6.5 Y7.0;\
  G1 X6.53 Y6.97;\
  G1 X6.57 Y6.95;\
  G1 X6.6 Y6.92;\
  G1 X6.64 Y6.89;\
  G1 X6.67 Y6.86;\
  G1 X6.7 Y6.83;\
  G1 X6.74 Y6.8;\
  G1 X6.77 Y6.77;\
  G1 X6.8 Y6.74;\
  G1 X6.83 Y6.7;\
  G1 X6.86 Y6.67;\
  G1 X6.89 Y6.64;\
  G1 X6.92 Y6.6;\
  G1 X6.95 Y6.57;\
  G1 X6.97 Y6.53;\
  G1 X7.0 Y6.5;\
  G1 X7.03 Y6.46;\
  G1 X7.05 Y6.43;\
  G1 X7.08 Y6.39;\
  G1 X7.1 Y6.35;\
  G1 X7.13 Y6.31;\
  G1 X7.15 Y6.28;\
  G1 X7.17 Y6.24;\
  G1 X7.19 Y6.2;\
  G1 X7.22 Y6.16;\
  G1 X7.24 Y6.12;\
  G1 X7.26 Y6.08;\
  G1 X7.27 Y6.04;\
  G1 X7.29 Y6.0;\
  G1 X7.31 Y5.96;\
  G1 X7.33 Y5.92;\
  G1 X7.34 Y5.87;\
  G1 X7.36 Y5.83;\
  G1 X7.37 Y5.79;\
  G1 X7.39 Y5.75;\
  G1 X7.4 Y5.7;\
  G1 X7.41 Y5.66;\
  G1 X7.42 Y5.62;\
  G1 X7.43 Y5.57;\
  G1 X7.44 Y5.53;\
  G1 X7.45 Y5.49;\
  G1 X7.46 Y5.44;\
  G1 X7.47 Y5.4;\
  G1 Y5.36;\
  G1 X7.48 Y5.31;\
  G1 X7.49 Y5.27;\
  G1 Y5.22;\
  G1 Y5.18;\
  G1 X7.5 Y5.13;\
  G1 Y5.09;\
  G1 Y5.04;\
  G1 Y5.0;\
  G1 Y-5.0;\
  G1 Y-5.04;\
  G1 Y-5.09;\
  G1 Y-5.13;\
  G1 X7.49 Y-5.18;\
  G1 Y-5.22;\
  G1 Y-5.27;\
  G1 X7.48 Y-5.31;\
  G1 X7.47 Y-5.36;\
  G1 Y-5.4;\
  G1 X7.46 Y-5.44;\
  G1 X7.45 Y-5.49;\
  G1 X7.44 Y-5.53;\
  G1 X7.43 Y-5.57;\
  G1 X7.42 Y-5.62;\
  G1 X7.41 Y-5.66;\
  G1 X7.4 Y-5.7;\
  G1 X7.39 Y-5.75;\
  G1 X7.37 Y-5.79;\
  G1 X7.36 Y-5.83;\
  G1 X7.34 Y-5.87;\
  G1 X7.33 Y-5.92;\
  G1 X7.31 Y-5.96;\
  G1 X7.29 Y-6.0;\
  G1 X7.27 Y-6.04;\
  G1 X7.26 Y-6.08;\
  G1 X7.24 Y-6.12;\
  G1 X7.22 Y-6.16;\
  G1 X7.19 Y-6.2;\
  G1 X7.17 Y-6.24;\
  G1 X7.15 Y-6.28;\
  G1 X7.13 Y-6.31;\
  G1 X7.1 Y-6.35;\
  G1 X7.08 Y-6.39;\
  G1 X7.05 Y-6.43;\
  G1 X7.03 Y-6.46;\
  G1 X7.0 Y-6.5;\
  G1 X6.97 Y-6.53;\
  G1 X6.95 Y-6.57;\
  G1 X6.92 Y-6.6;\
  G1 X6.89 Y-6.64;\
  G1 X6.86 Y-6.67;\
  G1 X6.83 Y-6.7;\
  G1 X6.8 Y-6.74;\
  G1 X6.77 Y-6.77;\
  G1 X6.74 Y-6.8;\
  G1 X6.7 Y-6.83;\
  G1 X6.67 Y-6.86;\
  G1 X6.64 Y-6.89;\
  G1 X6.6 Y-6.92;\
  G1 X6.57 Y-6.95;\
  G1 X6.53 Y-6.97;\
  G1 X6.5 Y-7.0;\
  G1 X6.46 Y-7.03;\
  G1 X6.43 Y-7.05;\
  G1 X6.39 Y-7.08;\
  G1 X6.35 Y-7.1;\
  G1 X6.31 Y-7.13;\
  G1 X6.28 Y-7.15;\
  G1 X6.24 Y-7.17;\
  G1 X6.2 Y-7.19;\
  G1 X6.16 Y-7.22;\
  G1 X6.12 Y-7.24;\
  G1 X6.08 Y-7.26;\
  G1 X6.04 Y-7.27;\
  G1 X6.0 Y-7.29;\
  G1 X5.96 Y-7.31;\
  G1 X5.92 Y-7.33;\
  G1 X5.87 Y-7.34;\
  G1 X5.83 Y-7.36;\
  G1 X5.79 Y-7.37;\
  G1 X5.75 Y-7.39;\
  G1 X5.7 Y-7.4;\
  G1 X5.66 Y-7.41;\
  G1 X5.62 Y-7.42;\
  G1 X5.57 Y-7.43;\
  G1 X5.53 Y-7.44;\
  G1 X5.49 Y-7.45;\
  G1 X5.44 Y-7.46;\
  G1 X5.4 Y-7.47;\
  G1 X5.36;\
  G1 X5.31 Y-7.48;\
  G1 X5.27 Y-7.49;\
  G1 X5.22;\
  G1 X5.18;\
  G1 X5.13 Y-7.5;\
  G1 X5.09;\
  G1 X5.04;\
  G1 X5.0;\
  G1 X-5.0;\
  G1 X-5.04;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test21 = "G0 Z6.0;\
  G0 X-10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-7.5;\
  G0 Z4.0;\
  G0 X7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y0.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-7.5;\
  G0 Z4.0;\
  G0 X7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y-5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-7.5;\
  G0 Z4.0;\
  G0 X7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y-10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 Y-10.0;\
  G1 X10.0;\
  G1 Y10.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X-5.04 Y-7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-5.09;\
  G1 X-5.13;\
  G1 X-5.18 Y-7.49;\
  G1 X-5.22;\
  G1 X-5.27;\
  G1 X-5.31 Y-7.48;\
  G1 X-5.36 Y-7.47;\
  G1 X-5.4;\
  G1 X-5.44 Y-7.46;\
  G1 X-5.49 Y-7.45;\
  G1 X-5.53 Y-7.44;\
  G1 X-5.57 Y-7.43;\
  G1 X-5.62 Y-7.42;\
  G1 X-5.66 Y-7.41;\
  G1 X-5.7 Y-7.4;\
  G1 X-5.75 Y-7.39;\
  G1 X-5.79 Y-7.37;\
  G1 X-5.83 Y-7.36;\
  G1 X-5.87 Y-7.34;\
  G1 X-5.92 Y-7.33;\
  G1 X-5.96 Y-7.31;\
  G1 X-6.0 Y-7.29;\
  G1 X-6.04 Y-7.27;\
  G1 X-6.08 Y-7.26;\
  G1 X-6.12 Y-7.24;\
  G1 X-6.16 Y-7.22;\
  G1 X-6.2 Y-7.19;\
  G1 X-6.24 Y-7.17;\
  G1 X-6.28 Y-7.15;\
  G1 X-6.31 Y-7.13;\
  G1 X-6.35 Y-7.1;\
  G1 X-6.39 Y-7.08;\
  G1 X-6.43 Y-7.05;\
  G1 X-6.46 Y-7.03;\
  G1 X-6.5 Y-7.0;\
  G1 X-6.53 Y-6.97;\
  G1 X-6.57 Y-6.95;\
  G1 X-6.6 Y-6.92;\
  G1 X-6.64 Y-6.89;\
  G1 X-6.67 Y-6.86;\
  G1 X-6.7 Y-6.83;\
  G1 X-6.74 Y-6.8;\
  G1 X-6.77 Y-6.77;\
  G1 X-6.8 Y-6.74;\
  G1 X-6.83 Y-6.7;\
  G1 X-6.86 Y-6.67;\
  G1 X-6.89 Y-6.64;\
  G1 X-6.92 Y-6.6;\
  G1 X-6.95 Y-6.57;\
  G1 X-6.97 Y-6.53;\
  G1 X-7.0 Y-6.5;\
  G1 X-7.03 Y-6.46;\
  G1 X-7.05 Y-6.43;\
  G1 X-7.08 Y-6.39;\
  G1 X-7.1 Y-6.35;\
  G1 X-7.13 Y-6.31;\
  G1 X-7.15 Y-6.28;\
  G1 X-7.17 Y-6.24;\
  G1 X-7.19 Y-6.2;\
  G1 X-7.22 Y-6.16;\
  G1 X-7.24 Y-6.12;\
  G1 X-7.26 Y-6.08;\
  G1 X-7.27 Y-6.04;\
  G1 X-7.29 Y-6.0;\
  G1 X-7.31 Y-5.96;\
  G1 X-7.33 Y-5.92;\
  G1 X-7.34 Y-5.87;\
  G1 X-7.36 Y-5.83;\
  G1 X-7.37 Y-5.79;\
  G1 X-7.39 Y-5.75;\
  G1 X-7.4 Y-5.7;\
  G1 X-7.41 Y-5.66;\
  G1 X-7.42 Y-5.62;\
  G1 X-7.43 Y-5.57;\
  G1 X-7.44 Y-5.53;\
  G1 X-7.45 Y-5.49;\
  G1 X-7.46 Y-5.44;\
  G1 X-7.47 Y-5.4;\
  G1 Y-5.36;\
  G1 X-7.48 Y-5.31;\
  G1 X-7.49 Y-5.27;\
  G1 Y-5.22;\
  G1 Y-5.18;\
  G1 X-7.5 Y-5.13;\
  G1 Y-5.09;\
  G1 Y-5.04;\
  G1 Y-5.0;\
  G1 Y5.0;\
  G1 Y5.04;\
  G1 Y5.09;\
  G1 Y5.13;\
  G1 X-7.49 Y5.18;\
  G1 Y5.22;\
  G1 Y5.27;\
  G1 X-7.48 Y5.31;\
  G1 X-7.47 Y5.36;\
  G1 Y5.4;\
  G1 X-7.46 Y5.44;\
  G1 X-7.45 Y5.49;\
  G1 X-7.44 Y5.53;\
  G1 X-7.43 Y5.57;\
  G1 X-7.42 Y5.62;\
  G1 X-7.41 Y5.66;\
  G1 X-7.4 Y5.7;\
  G1 X-7.39 Y5.75;\
  G1 X-7.37 Y5.79;\
  G1 X-7.36 Y5.83;\
  G1 X-7.34 Y5.87;\
  G1 X-7.33 Y5.92;\
  G1 X-7.31 Y5.96;\
  G1 X-7.29 Y6.0;\
  G1 X-7.27 Y6.04;\
  G1 X-7.26 Y6.08;\
  G1 X-7.24 Y6.12;\
  G1 X-7.22 Y6.16;\
  G1 X-7.19 Y6.2;\
  G1 X-7.17 Y6.24;\
  G1 X-7.15 Y6.28;\
  G1 X-7.13 Y6.31;\
  G1 X-7.1 Y6.35;\
  G1 X-7.08 Y6.39;\
  G1 X-7.05 Y6.43;\
  G1 X-7.03 Y6.46;\
  G1 X-7.0 Y6.5;\
  G1 X-6.97 Y6.53;\
  G1 X-6.95 Y6.57;\
  G1 X-6.92 Y6.6;\
  G1 X-6.89 Y6.64;\
  G1 X-6.86 Y6.67;\
  G1 X-6.83 Y6.7;\
  G1 X-6.8 Y6.74;\
  G1 X-6.77 Y6.77;\
  G1 X-6.74 Y6.8;\
  G1 X-6.7 Y6.83;\
  G1 X-6.67 Y6.86;\
  G1 X-6.64 Y6.89;\
  G1 X-6.6 Y6.92;\
  G1 X-6.57 Y6.95;\
  G1 X-6.53 Y6.97;\
  G1 X-6.5 Y7.0;\
  G1 X-6.46 Y7.03;\
  G1 X-6.43 Y7.05;\
  G1 X-6.39 Y7.08;\
  G1 X-6.35 Y7.1;\
  G1 X-6.31 Y7.13;\
  G1 X-6.28 Y7.15;\
  G1 X-6.24 Y7.17;\
  G1 X-6.2 Y7.19;\
  G1 X-6.16 Y7.22;\
  G1 X-6.12 Y7.24;\
  G1 X-6.08 Y7.26;\
  G1 X-6.04 Y7.27;\
  G1 X-6.0 Y7.29;\
  G1 X-5.96 Y7.31;\
  G1 X-5.92 Y7.33;\
  G1 X-5.87 Y7.34;\
  G1 X-5.83 Y7.36;\
  G1 X-5.79 Y7.37;\
  G1 X-5.75 Y7.39;\
  G1 X-5.7 Y7.4;\
  G1 X-5.66 Y7.41;\
  G1 X-5.62 Y7.42;\
  G1 X-5.57 Y7.43;\
  G1 X-5.53 Y7.44;\
  G1 X-5.49 Y7.45;\
  G1 X-5.44 Y7.46;\
  G1 X-5.4 Y7.47;\
  G1 X-5.36;\
  G1 X-5.31 Y7.48;\
  G1 X-5.27 Y7.49;\
  G1 X-5.22;\
  G1 X-5.18;\
  G1 X-5.13 Y7.5;\
  G1 X-5.09;\
  G1 X-5.04;\
  G1 X-5.0;\
  G1 X5.0;\
  G1 X5.04;\
  G1 X5.09;\
  G1 X5.13;\
  G1 X5.18 Y7.49;\
  G1 X5.22;\
  G1 X5.27;\
  G1 X5.31 Y7.48;\
  G1 X5.36 Y7.47;\
  G1 X5.4;\
  G1 X5.44 Y7.46;\
  G1 X5.49 Y7.45;\
  G1 X5.53 Y7.44;\
  G1 X5.57 Y7.43;\
  G1 X5.62 Y7.42;\
  G1 X5.66 Y7.41;\
  G1 X5.7 Y7.4;\
  G1 X5.75 Y7.39;\
  G1 X5.79 Y7.37;\
  G1 X5.83 Y7.36;\
  G1 X5.87 Y7.34;\
  G1 X5.92 Y7.33;\
  G1 X5.96 Y7.31;\
  G1 X6.0 Y7.29;\
  G1 X6.04 Y7.27;\
  G1 X6.08 Y7.26;\
  G1 X6.12 Y7.24;\
  G1 X6.16 Y7.22;\
  G1 X6.2 Y7.19;\
  G1 X6.24 Y7.17;\
  G1 X6.28 Y7.15;\
  G1 X6.31 Y7.13;\
  G1 X6.35 Y7.1;\
  G1 X6.39 Y7.08;\
  G1 X6.43 Y7.05;\
  G1 X6.46 Y7.03;\
  G1 X6.5 Y7.0;\
  G1 X6.53 Y6.97;\
  G1 X6.57 Y6.95;\
  G1 X6.6 Y6.92;\
  G1 X6.64 Y6.89;\
  G1 X6.67 Y6.86;\
  G1 X6.7 Y6.83;\
  G1 X6.74 Y6.8;\
  G1 X6.77 Y6.77;\
  G1 X6.8 Y6.74;\
  G1 X6.83 Y6.7;\
  G1 X6.86 Y6.67;\
  G1 X6.89 Y6.64;\
  G1 X6.92 Y6.6;\
  G1 X6.95 Y6.57;\
  G1 X6.97 Y6.53;\
  G1 X7.0 Y6.5;\
  G1 X7.03 Y6.46;\
  G1 X7.05 Y6.43;\
  G1 X7.08 Y6.39;\
  G1 X7.1 Y6.35;\
  G1 X7.13 Y6.31;\
  G1 X7.15 Y6.28;\
  G1 X7.17 Y6.24;\
  G1 X7.19 Y6.2;\
  G1 X7.22 Y6.16;\
  G1 X7.24 Y6.12;\
  G1 X7.26 Y6.08;\
  G1 X7.27 Y6.04;\
  G1 X7.29 Y6.0;\
  G1 X7.31 Y5.96;\
  G1 X7.33 Y5.92;\
  G1 X7.34 Y5.87;\
  G1 X7.36 Y5.83;\
  G1 X7.37 Y5.79;\
  G1 X7.39 Y5.75;\
  G1 X7.4 Y5.7;\
  G1 X7.41 Y5.66;\
  G1 X7.42 Y5.62;\
  G1 X7.43 Y5.57;\
  G1 X7.44 Y5.53;\
  G1 X7.45 Y5.49;\
  G1 X7.46 Y5.44;\
  G1 X7.47 Y5.4;\
  G1 Y5.36;\
  G1 X7.48 Y5.31;\
  G1 X7.49 Y5.27;\
  G1 Y5.22;\
  G1 Y5.18;\
  G1 X7.5 Y5.13;\
  G1 Y5.09;\
  G1 Y5.04;\
  G1 Y5.0;\
  G1 Y-5.0;\
  G1 Y-5.04;\
  G1 Y-5.09;\
  G1 Y-5.13;\
  G1 X7.49 Y-5.18;\
  G1 Y-5.22;\
  G1 Y-5.27;\
  G1 X7.48 Y-5.31;\
  G1 X7.47 Y-5.36;\
  G1 Y-5.4;\
  G1 X7.46 Y-5.44;\
  G1 X7.45 Y-5.49;\
  G1 X7.44 Y-5.53;\
  G1 X7.43 Y-5.57;\
  G1 X7.42 Y-5.62;\
  G1 X7.41 Y-5.66;\
  G1 X7.4 Y-5.7;\
  G1 X7.39 Y-5.75;\
  G1 X7.37 Y-5.79;\
  G1 X7.36 Y-5.83;\
  G1 X7.34 Y-5.87;\
  G1 X7.33 Y-5.92;\
  G1 X7.31 Y-5.96;\
  G1 X7.29 Y-6.0;\
  G1 X7.27 Y-6.04;\
  G1 X7.26 Y-6.08;\
  G1 X7.24 Y-6.12;\
  G1 X7.22 Y-6.16;\
  G1 X7.19 Y-6.2;\
  G1 X7.17 Y-6.24;\
  G1 X7.15 Y-6.28;\
  G1 X7.13 Y-6.31;\
  G1 X7.1 Y-6.35;\
  G1 X7.08 Y-6.39;\
  G1 X7.05 Y-6.43;\
  G1 X7.03 Y-6.46;\
  G1 X7.0 Y-6.5;\
  G1 X6.97 Y-6.53;\
  G1 X6.95 Y-6.57;\
  G1 X6.92 Y-6.6;\
  G1 X6.89 Y-6.64;\
  G1 X6.86 Y-6.67;\
  G1 X6.83 Y-6.7;\
  G1 X6.8 Y-6.74;\
  G1 X6.77 Y-6.77;\
  G1 X6.74 Y-6.8;\
  G1 X6.7 Y-6.83;\
  G1 X6.67 Y-6.86;\
  G1 X6.64 Y-6.89;\
  G1 X6.6 Y-6.92;\
  G1 X6.57 Y-6.95;\
  G1 X6.53 Y-6.97;\
  G1 X6.5 Y-7.0;\
  G1 X6.46 Y-7.03;\
  G1 X6.43 Y-7.05;\
  G1 X6.39 Y-7.08;\
  G1 X6.35 Y-7.1;\
  G1 X6.31 Y-7.13;\
  G1 X6.28 Y-7.15;\
  G1 X6.24 Y-7.17;\
  G1 X6.2 Y-7.19;\
  G1 X6.16 Y-7.22;\
  G1 X6.12 Y-7.24;\
  G1 X6.08 Y-7.26;\
  G1 X6.04 Y-7.27;\
  G1 X6.0 Y-7.29;\
  G1 X5.96 Y-7.31;\
  G1 X5.92 Y-7.33;\
  G1 X5.87 Y-7.34;\
  G1 X5.83 Y-7.36;\
  G1 X5.79 Y-7.37;\
  G1 X5.75 Y-7.39;\
  G1 X5.7 Y-7.4;\
  G1 X5.66 Y-7.41;\
  G1 X5.62 Y-7.42;\
  G1 X5.57 Y-7.43;\
  G1 X5.53 Y-7.44;\
  G1 X5.49 Y-7.45;\
  G1 X5.44 Y-7.46;\
  G1 X5.4 Y-7.47;\
  G1 X5.36;\
  G1 X5.31 Y-7.48;\
  G1 X5.27 Y-7.49;\
  G1 X5.22;\
  G1 X5.18;\
  G1 X5.13 Y-7.5;\
  G1 X5.09;\
  G1 X5.04;\
  G1 X5.0;\
  G1 X-5.0;\
  G1 X-5.04;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test22 = "G0 Z6.0;\
  G0 X-10.0 Y-10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y-5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-7.5;\
  G0 Z4.0;\
  G0 X7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y0.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-7.5;\
  G0 Z4.0;\
  G0 X7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-7.5;\
  G0 Z4.0;\
  G0 X7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 Y-10.0;\
  G1 X-10.0;\
  G1 Y10.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-5.0 Y-7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X5.0;\
  G1 X5.04;\
  G1 X5.09;\
  G1 X5.13;\
  G1 X5.18 Y-7.49;\
  G1 X5.22;\
  G1 X5.27;\
  G1 X5.31 Y-7.48;\
  G1 X5.36 Y-7.47;\
  G1 X5.4;\
  G1 X5.44 Y-7.46;\
  G1 X5.49 Y-7.45;\
  G1 X5.53 Y-7.44;\
  G1 X5.57 Y-7.43;\
  G1 X5.62 Y-7.42;\
  G1 X5.66 Y-7.41;\
  G1 X5.7 Y-7.4;\
  G1 X5.75 Y-7.39;\
  G1 X5.79 Y-7.37;\
  G1 X5.83 Y-7.36;\
  G1 X5.87 Y-7.34;\
  G1 X5.92 Y-7.33;\
  G1 X5.96 Y-7.31;\
  G1 X6.0 Y-7.29;\
  G1 X6.04 Y-7.27;\
  G1 X6.08 Y-7.26;\
  G1 X6.12 Y-7.24;\
  G1 X6.16 Y-7.22;\
  G1 X6.2 Y-7.19;\
  G1 X6.24 Y-7.17;\
  G1 X6.28 Y-7.15;\
  G1 X6.31 Y-7.13;\
  G1 X6.35 Y-7.1;\
  G1 X6.39 Y-7.08;\
  G1 X6.43 Y-7.05;\
  G1 X6.46 Y-7.03;\
  G1 X6.5 Y-7.0;\
  G1 X6.53 Y-6.97;\
  G1 X6.57 Y-6.95;\
  G1 X6.6 Y-6.92;\
  G1 X6.64 Y-6.89;\
  G1 X6.67 Y-6.86;\
  G1 X6.7 Y-6.83;\
  G1 X6.74 Y-6.8;\
  G1 X6.77 Y-6.77;\
  G1 X6.8 Y-6.74;\
  G1 X6.83 Y-6.7;\
  G1 X6.86 Y-6.67;\
  G1 X6.89 Y-6.64;\
  G1 X6.92 Y-6.6;\
  G1 X6.95 Y-6.57;\
  G1 X6.97 Y-6.53;\
  G1 X7.0 Y-6.5;\
  G1 X7.03 Y-6.46;\
  G1 X7.05 Y-6.43;\
  G1 X7.08 Y-6.39;\
  G1 X7.1 Y-6.35;\
  G1 X7.13 Y-6.31;\
  G1 X7.15 Y-6.28;\
  G1 X7.17 Y-6.24;\
  G1 X7.19 Y-6.2;\
  G1 X7.22 Y-6.16;\
  G1 X7.24 Y-6.12;\
  G1 X7.26 Y-6.08;\
  G1 X7.27 Y-6.04;\
  G1 X7.29 Y-6.0;\
  G1 X7.31 Y-5.96;\
  G1 X7.33 Y-5.92;\
  G1 X7.34 Y-5.87;\
  G1 X7.36 Y-5.83;\
  G1 X7.37 Y-5.79;\
  G1 X7.39 Y-5.75;\
  G1 X7.4 Y-5.7;\
  G1 X7.41 Y-5.66;\
  G1 X7.42 Y-5.62;\
  G1 X7.43 Y-5.57;\
  G1 X7.44 Y-5.53;\
  G1 X7.45 Y-5.49;\
  G1 X7.46 Y-5.44;\
  G1 X7.47 Y-5.4;\
  G1 Y-5.36;\
  G1 X7.48 Y-5.31;\
  G1 X7.49 Y-5.27;\
  G1 Y-5.22;\
  G1 Y-5.18;\
  G1 X7.5 Y-5.13;\
  G1 Y-5.09;\
  G1 Y-5.04;\
  G1 Y-5.0;\
  G1 Y5.0;\
  G1 Y5.04;\
  G1 Y5.09;\
  G1 Y5.13;\
  G1 X7.49 Y5.18;\
  G1 Y5.22;\
  G1 Y5.27;\
  G1 X7.48 Y5.31;\
  G1 X7.47 Y5.36;\
  G1 Y5.4;\
  G1 X7.46 Y5.44;\
  G1 X7.45 Y5.49;\
  G1 X7.44 Y5.53;\
  G1 X7.43 Y5.57;\
  G1 X7.42 Y5.62;\
  G1 X7.41 Y5.66;\
  G1 X7.4 Y5.7;\
  G1 X7.39 Y5.75;\
  G1 X7.37 Y5.79;\
  G1 X7.36 Y5.83;\
  G1 X7.34 Y5.87;\
  G1 X7.33 Y5.92;\
  G1 X7.31 Y5.96;\
  G1 X7.29 Y6.0;\
  G1 X7.27 Y6.04;\
  G1 X7.26 Y6.08;\
  G1 X7.24 Y6.12;\
  G1 X7.22 Y6.16;\
  G1 X7.19 Y6.2;\
  G1 X7.17 Y6.24;\
  G1 X7.15 Y6.28;\
  G1 X7.13 Y6.31;\
  G1 X7.1 Y6.35;\
  G1 X7.08 Y6.39;\
  G1 X7.05 Y6.43;\
  G1 X7.03 Y6.46;\
  G1 X7.0 Y6.5;\
  G1 X6.97 Y6.53;\
  G1 X6.95 Y6.57;\
  G1 X6.92 Y6.6;\
  G1 X6.89 Y6.64;\
  G1 X6.86 Y6.67;\
  G1 X6.83 Y6.7;\
  G1 X6.8 Y6.74;\
  G1 X6.77 Y6.77;\
  G1 X6.74 Y6.8;\
  G1 X6.7 Y6.83;\
  G1 X6.67 Y6.86;\
  G1 X6.64 Y6.89;\
  G1 X6.6 Y6.92;\
  G1 X6.57 Y6.95;\
  G1 X6.53 Y6.97;\
  G1 X6.5 Y7.0;\
  G1 X6.46 Y7.03;\
  G1 X6.43 Y7.05;\
  G1 X6.39 Y7.08;\
  G1 X6.35 Y7.1;\
  G1 X6.31 Y7.13;\
  G1 X6.28 Y7.15;\
  G1 X6.24 Y7.17;\
  G1 X6.2 Y7.19;\
  G1 X6.16 Y7.22;\
  G1 X6.12 Y7.24;\
  G1 X6.08 Y7.26;\
  G1 X6.04 Y7.27;\
  G1 X6.0 Y7.29;\
  G1 X5.96 Y7.31;\
  G1 X5.92 Y7.33;\
  G1 X5.87 Y7.34;\
  G1 X5.83 Y7.36;\
  G1 X5.79 Y7.37;\
  G1 X5.75 Y7.39;\
  G1 X5.7 Y7.4;\
  G1 X5.66 Y7.41;\
  G1 X5.62 Y7.42;\
  G1 X5.57 Y7.43;\
  G1 X5.53 Y7.44;\
  G1 X5.49 Y7.45;\
  G1 X5.44 Y7.46;\
  G1 X5.4 Y7.47;\
  G1 X5.36;\
  G1 X5.31 Y7.48;\
  G1 X5.27 Y7.49;\
  G1 X5.22;\
  G1 X5.18;\
  G1 X5.13 Y7.5;\
  G1 X5.09;\
  G1 X5.04;\
  G1 X5.0;\
  G1 X-5.0;\
  G1 X-5.04;\
  G1 X-5.09;\
  G1 X-5.13;\
  G1 X-5.18 Y7.49;\
  G1 X-5.22;\
  G1 X-5.27;\
  G1 X-5.31 Y7.48;\
  G1 X-5.36 Y7.47;\
  G1 X-5.4;\
  G1 X-5.44 Y7.46;\
  G1 X-5.49 Y7.45;\
  G1 X-5.53 Y7.44;\
  G1 X-5.57 Y7.43;\
  G1 X-5.62 Y7.42;\
  G1 X-5.66 Y7.41;\
  G1 X-5.7 Y7.4;\
  G1 X-5.75 Y7.39;\
  G1 X-5.79 Y7.37;\
  G1 X-5.83 Y7.36;\
  G1 X-5.87 Y7.34;\
  G1 X-5.92 Y7.33;\
  G1 X-5.96 Y7.31;\
  G1 X-6.0 Y7.29;\
  G1 X-6.04 Y7.27;\
  G1 X-6.08 Y7.26;\
  G1 X-6.12 Y7.24;\
  G1 X-6.16 Y7.22;\
  G1 X-6.2 Y7.19;\
  G1 X-6.24 Y7.17;\
  G1 X-6.28 Y7.15;\
  G1 X-6.31 Y7.13;\
  G1 X-6.35 Y7.1;\
  G1 X-6.39 Y7.08;\
  G1 X-6.43 Y7.05;\
  G1 X-6.46 Y7.03;\
  G1 X-6.5 Y7.0;\
  G1 X-6.53 Y6.97;\
  G1 X-6.57 Y6.95;\
  G1 X-6.6 Y6.92;\
  G1 X-6.64 Y6.89;\
  G1 X-6.67 Y6.86;\
  G1 X-6.7 Y6.83;\
  G1 X-6.74 Y6.8;\
  G1 X-6.77 Y6.77;\
  G1 X-6.8 Y6.74;\
  G1 X-6.83 Y6.7;\
  G1 X-6.86 Y6.67;\
  G1 X-6.89 Y6.64;\
  G1 X-6.92 Y6.6;\
  G1 X-6.95 Y6.57;\
  G1 X-6.97 Y6.53;\
  G1 X-7.0 Y6.5;\
  G1 X-7.03 Y6.46;\
  G1 X-7.05 Y6.43;\
  G1 X-7.08 Y6.39;\
  G1 X-7.1 Y6.35;\
  G1 X-7.13 Y6.31;\
  G1 X-7.15 Y6.28;\
  G1 X-7.17 Y6.24;\
  G1 X-7.19 Y6.2;\
  G1 X-7.22 Y6.16;\
  G1 X-7.24 Y6.12;\
  G1 X-7.26 Y6.08;\
  G1 X-7.27 Y6.04;\
  G1 X-7.29 Y6.0;\
  G1 X-7.31 Y5.96;\
  G1 X-7.33 Y5.92;\
  G1 X-7.34 Y5.87;\
  G1 X-7.36 Y5.83;\
  G1 X-7.37 Y5.79;\
  G1 X-7.39 Y5.75;\
  G1 X-7.4 Y5.7;\
  G1 X-7.41 Y5.66;\
  G1 X-7.42 Y5.62;\
  G1 X-7.43 Y5.57;\
  G1 X-7.44 Y5.53;\
  G1 X-7.45 Y5.49;\
  G1 X-7.46 Y5.44;\
  G1 X-7.47 Y5.4;\
  G1 Y5.36;\
  G1 X-7.48 Y5.31;\
  G1 X-7.49 Y5.27;\
  G1 Y5.22;\
  G1 Y5.18;\
  G1 X-7.5 Y5.13;\
  G1 Y5.09;\
  G1 Y5.04;\
  G1 Y5.0;\
  G1 Y-5.0;\
  G1 Y-5.04;\
  G1 Y-5.09;\
  G1 Y-5.13;\
  G1 X-7.49 Y-5.18;\
  G1 Y-5.22;\
  G1 Y-5.27;\
  G1 X-7.48 Y-5.31;\
  G1 X-7.47 Y-5.36;\
  G1 Y-5.4;\
  G1 X-7.46 Y-5.44;\
  G1 X-7.45 Y-5.49;\
  G1 X-7.44 Y-5.53;\
  G1 X-7.43 Y-5.57;\
  G1 X-7.42 Y-5.62;\
  G1 X-7.41 Y-5.66;\
  G1 X-7.4 Y-5.7;\
  G1 X-7.39 Y-5.75;\
  G1 X-7.37 Y-5.79;\
  G1 X-7.36 Y-5.83;\
  G1 X-7.34 Y-5.87;\
  G1 X-7.33 Y-5.92;\
  G1 X-7.31 Y-5.96;\
  G1 X-7.29 Y-6.0;\
  G1 X-7.27 Y-6.04;\
  G1 X-7.26 Y-6.08;\
  G1 X-7.24 Y-6.12;\
  G1 X-7.22 Y-6.16;\
  G1 X-7.19 Y-6.2;\
  G1 X-7.17 Y-6.24;\
  G1 X-7.15 Y-6.28;\
  G1 X-7.13 Y-6.31;\
  G1 X-7.1 Y-6.35;\
  G1 X-7.08 Y-6.39;\
  G1 X-7.05 Y-6.43;\
  G1 X-7.03 Y-6.46;\
  G1 X-7.0 Y-6.5;\
  G1 X-6.97 Y-6.53;\
  G1 X-6.95 Y-6.57;\
  G1 X-6.92 Y-6.6;\
  G1 X-6.89 Y-6.64;\
  G1 X-6.86 Y-6.67;\
  G1 X-6.83 Y-6.7;\
  G1 X-6.8 Y-6.74;\
  G1 X-6.77 Y-6.77;\
  G1 X-6.74 Y-6.8;\
  G1 X-6.7 Y-6.83;\
  G1 X-6.67 Y-6.86;\
  G1 X-6.64 Y-6.89;\
  G1 X-6.6 Y-6.92;\
  G1 X-6.57 Y-6.95;\
  G1 X-6.53 Y-6.97;\
  G1 X-6.5 Y-7.0;\
  G1 X-6.46 Y-7.03;\
  G1 X-6.43 Y-7.05;\
  G1 X-6.39 Y-7.08;\
  G1 X-6.35 Y-7.1;\
  G1 X-6.31 Y-7.13;\
  G1 X-6.28 Y-7.15;\
  G1 X-6.24 Y-7.17;\
  G1 X-6.2 Y-7.19;\
  G1 X-6.16 Y-7.22;\
  G1 X-6.12 Y-7.24;\
  G1 X-6.08 Y-7.26;\
  G1 X-6.04 Y-7.27;\
  G1 X-6.0 Y-7.29;\
  G1 X-5.96 Y-7.31;\
  G1 X-5.92 Y-7.33;\
  G1 X-5.87 Y-7.34;\
  G1 X-5.83 Y-7.36;\
  G1 X-5.79 Y-7.37;\
  G1 X-5.75 Y-7.39;\
  G1 X-5.7 Y-7.4;\
  G1 X-5.66 Y-7.41;\
  G1 X-5.62 Y-7.42;\
  G1 X-5.57 Y-7.43;\
  G1 X-5.53 Y-7.44;\
  G1 X-5.49 Y-7.45;\
  G1 X-5.44 Y-7.46;\
  G1 X-5.4 Y-7.47;\
  G1 X-5.36;\
  G1 X-5.31 Y-7.48;\
  G1 X-5.27 Y-7.49;\
  G1 X-5.22;\
  G1 X-5.18;\
  G1 X-5.13 Y-7.5;\
  G1 X-5.09;\
  G1 X-5.04;\
  G1 X-5.0;\
  G0 Z4.0;\
  G0 Z6.0"

expected_moves_test23 = "G0 Z6.0;\
  G0 X10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X7.5;\
  G0 Z4.0;\
  G0 X-7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y0.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X7.5;\
  G0 Z4.0;\
  G0 X-7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y-5.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X7.5;\
  G0 Z4.0;\
  G0 X-7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y-10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X-10.0;\
  G0 Z4.0;\
  G0 X10.0 Y10.0;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 Y-10.0;\
  G1 X-10.0;\
  G1 Y10.0;\
  G1 X10.0;\
  G0 Z4.0;\
  G0 X-5.0 Y-7.5;\
  G0 Z4.1;\
  G1 Z0.0;\
  G1 X5.0;\
  G1 X5.04;\
  G1 X5.09;\
  G1 X5.13;\
  G1 X5.18 Y-7.49;\
  G1 X5.22;\
  G1 X5.27;\
  G1 X5.31 Y-7.48;\
  G1 X5.36 Y-7.47;\
  G1 X5.4;\
  G1 X5.44 Y-7.46;\
  G1 X5.49 Y-7.45;\
  G1 X5.53 Y-7.44;\
  G1 X5.57 Y-7.43;\
  G1 X5.62 Y-7.42;\
  G1 X5.66 Y-7.41;\
  G1 X5.7 Y-7.4;\
  G1 X5.75 Y-7.39;\
  G1 X5.79 Y-7.37;\
  G1 X5.83 Y-7.36;\
  G1 X5.87 Y-7.34;\
  G1 X5.92 Y-7.33;\
  G1 X5.96 Y-7.31;\
  G1 X6.0 Y-7.29;\
  G1 X6.04 Y-7.27;\
  G1 X6.08 Y-7.26;\
  G1 X6.12 Y-7.24;\
  G1 X6.16 Y-7.22;\
  G1 X6.2 Y-7.19;\
  G1 X6.24 Y-7.17;\
  G1 X6.28 Y-7.15;\
  G1 X6.31 Y-7.13;\
  G1 X6.35 Y-7.1;\
  G1 X6.39 Y-7.08;\
  G1 X6.43 Y-7.05;\
  G1 X6.46 Y-7.03;\
  G1 X6.5 Y-7.0;\
  G1 X6.53 Y-6.97;\
  G1 X6.57 Y-6.95;\
  G1 X6.6 Y-6.92;\
  G1 X6.64 Y-6.89;\
  G1 X6.67 Y-6.86;\
  G1 X6.7 Y-6.83;\
  G1 X6.74 Y-6.8;\
  G1 X6.77 Y-6.77;\
  G1 X6.8 Y-6.74;\
  G1 X6.83 Y-6.7;\
  G1 X6.86 Y-6.67;\
  G1 X6.89 Y-6.64;\
  G1 X6.92 Y-6.6;\
  G1 X6.95 Y-6.57;\
  G1 X6.97 Y-6.53;\
  G1 X7.0 Y-6.5;\
  G1 X7.03 Y-6.46;\
  G1 X7.05 Y-6.43;\
  G1 X7.08 Y-6.39;\
  G1 X7.1 Y-6.35;\
  G1 X7.13 Y-6.31;\
  G1 X7.15 Y-6.28;\
  G1 X7.17 Y-6.24;\
  G1 X7.19 Y-6.2;\
  G1 X7.22 Y-6.16;\
  G1 X7.24 Y-6.12;\
  G1 X7.26 Y-6.08;\
  G1 X7.27 Y-6.04;\
  G1 X7.29 Y-6.0;\
  G1 X7.31 Y-5.96;\
  G1 X7.33 Y-5.92;\
  G1 X7.34 Y-5.87;\
  G1 X7.36 Y-5.83;\
  G1 X7.37 Y-5.79;\
  G1 X7.39 Y-5.75;\
  G1 X7.4 Y-5.7;\
  G1 X7.41 Y-5.66;\
  G1 X7.42 Y-5.62;\
  G1 X7.43 Y-5.57;\
  G1 X7.44 Y-5.53;\
  G1 X7.45 Y-5.49;\
  G1 X7.46 Y-5.44;\
  G1 X7.47 Y-5.4;\
  G1 Y-5.36;\
  G1 X7.48 Y-5.31;\
  G1 X7.49 Y-5.27;\
  G1 Y-5.22;\
  G1 Y-5.18;\
  G1 X7.5 Y-5.13;\
  G1 Y-5.09;\
  G1 Y-5.04;\
  G1 Y-5.0;\
  G1 Y5.0;\
  G1 Y5.04;\
  G1 Y5.09;\
  G1 Y5.13;\
  G1 X7.49 Y5.18;\
  G1 Y5.22;\
  G1 Y5.27;\
  G1 X7.48 Y5.31;\
  G1 X7.47 Y5.36;\
  G1 Y5.4;\
  G1 X7.46 Y5.44;\
  G1 X7.45 Y5.49;\
  G1 X7.44 Y5.53;\
  G1 X7.43 Y5.57;\
  G1 X7.42 Y5.62;\
  G1 X7.41 Y5.66;\
  G1 X7.4 Y5.7;\
  G1 X7.39 Y5.75;\
  G1 X7.37 Y5.79;\
  G1 X7.36 Y5.83;\
  G1 X7.34 Y5.87;\
  G1 X7.33 Y5.92;\
  G1 X7.31 Y5.96;\
  G1 X7.29 Y6.0;\
  G1 X7.27 Y6.04;\
  G1 X7.26 Y6.08;\
  G1 X7.24 Y6.12;\
  G1 X7.22 Y6.16;\
  G1 X7.19 Y6.2;\
  G1 X7.17 Y6.24;\
  G1 X7.15 Y6.28;\
  G1 X7.13 Y6.31;\
  G1 X7.1 Y6.35;\
  G1 X7.08 Y6.39;\
  G1 X7.05 Y6.43;\
  G1 X7.03 Y6.46;\
  G1 X7.0 Y6.5;\
  G1 X6.97 Y6.53;\
  G1 X6.95 Y6.57;\
  G1 X6.92 Y6.6;\
  G1 X6.89 Y6.64;\
  G1 X6.86 Y6.67;\
  G1 X6.83 Y6.7;\
  G1 X6.8 Y6.74;\
  G1 X6.77 Y6.77;\
  G1 X6.74 Y6.8;\
  G1 X6.7 Y6.83;\
  G1 X6.67 Y6.86;\
  G1 X6.64 Y6.89;\
  G1 X6.6 Y6.92;\
  G1 X6.57 Y6.95;\
  G1 X6.53 Y6.97;\
  G1 X6.5 Y7.0;\
  G1 X6.46 Y7.03;\
  G1 X6.43 Y7.05;\
  G1 X6.39 Y7.08;\
  G1 X6.35 Y7.1;\
  G1 X6.31 Y7.13;\
  G1 X6.28 Y7.15;\
  G1 X6.24 Y7.17;\
  G1 X6.2 Y7.19;\
  G1 X6.16 Y7.22;\
  G1 X6.12 Y7.24;\
  G1 X6.08 Y7.26;\
  G1 X6.04 Y7.27;\
  G1 X6.0 Y7.29;\
  G1 X5.96 Y7.31;\
  G1 X5.92 Y7.33;\
  G1 X5.87 Y7.34;\
  G1 X5.83 Y7.36;\
  G1 X5.79 Y7.37;\
  G1 X5.75 Y7.39;\
  G1 X5.7 Y7.4;\
  G1 X5.66 Y7.41;\
  G1 X5.62 Y7.42;\
  G1 X5.57 Y7.43;\
  G1 X5.53 Y7.44;\
  G1 X5.49 Y7.45;\
  G1 X5.44 Y7.46;\
  G1 X5.4 Y7.47;\
  G1 X5.36;\
  G1 X5.31 Y7.48;\
  G1 X5.27 Y7.49;\
  G1 X5.22;\
  G1 X5.18;\
  G1 X5.13 Y7.5;\
  G1 X5.09;\
  G1 X5.04;\
  G1 X5.0;\
  G1 X-5.0;\
  G1 X-5.04;\
  G1 X-5.09;\
  G1 X-5.13;\
  G1 X-5.18 Y7.49;\
  G1 X-5.22;\
  G1 X-5.27;\
  G1 X-5.31 Y7.48;\
  G1 X-5.36 Y7.47;\
  G1 X-5.4;\
  G1 X-5.44 Y7.46;\
  G1 X-5.49 Y7.45;\
  G1 X-5.53 Y7.44;\
  G1 X-5.57 Y7.43;\
  G1 X-5.62 Y7.42;\
  G1 X-5.66 Y7.41;\
  G1 X-5.7 Y7.4;\
  G1 X-5.75 Y7.39;\
  G1 X-5.79 Y7.37;\
  G1 X-5.83 Y7.36;\
  G1 X-5.87 Y7.34;\
  G1 X-5.92 Y7.33;\
  G1 X-5.96 Y7.31;\
  G1 X-6.0 Y7.29;\
  G1 X-6.04 Y7.27;\
  G1 X-6.08 Y7.26;\
  G1 X-6.12 Y7.24;\
  G1 X-6.16 Y7.22;\
  G1 X-6.2 Y7.19;\
  G1 X-6.24 Y7.17;\
  G1 X-6.28 Y7.15;\
  G1 X-6.31 Y7.13;\
  G1 X-6.35 Y7.1;\
  G1 X-6.39 Y7.08;\
  G1 X-6.43 Y7.05;\
  G1 X-6.46 Y7.03;\
  G1 X-6.5 Y7.0;\
  G1 X-6.53 Y6.97;\
  G1 X-6.57 Y6.95;\
  G1 X-6.6 Y6.92;\
  G1 X-6.64 Y6.89;\
  G1 X-6.67 Y6.86;\
  G1 X-6.7 Y6.83;\
  G1 X-6.74 Y6.8;\
  G1 X-6.77 Y6.77;\
  G1 X-6.8 Y6.74;\
  G1 X-6.83 Y6.7;\
  G1 X-6.86 Y6.67;\
  G1 X-6.89 Y6.64;\
  G1 X-6.92 Y6.6;\
  G1 X-6.95 Y6.57;\
  G1 X-6.97 Y6.53;\
  G1 X-7.0 Y6.5;\
  G1 X-7.03 Y6.46;\
  G1 X-7.05 Y6.43;\
  G1 X-7.08 Y6.39;\
  G1 X-7.1 Y6.35;\
  G1 X-7.13 Y6.31;\
  G1 X-7.15 Y6.28;\
  G1 X-7.17 Y6.24;\
  G1 X-7.19 Y6.2;\
  G1 X-7.22 Y6.16;\
  G1 X-7.24 Y6.12;\
  G1 X-7.26 Y6.08;\
  G1 X-7.27 Y6.04;\
  G1 X-7.29 Y6.0;\
  G1 X-7.31 Y5.96;\
  G1 X-7.33 Y5.92;\
  G1 X-7.34 Y5.87;\
  G1 X-7.36 Y5.83;\
  G1 X-7.37 Y5.79;\
  G1 X-7.39 Y5.75;\
  G1 X-7.4 Y5.7;\
  G1 X-7.41 Y5.66;\
  G1 X-7.42 Y5.62;\
  G1 X-7.43 Y5.57;\
  G1 X-7.44 Y5.53;\
  G1 X-7.45 Y5.49;\
  G1 X-7.46 Y5.44;\
  G1 X-7.47 Y5.4;\
  G1 Y5.36;\
  G1 X-7.48 Y5.31;\
  G1 X-7.49 Y5.27;\
  G1 Y5.22;\
  G1 Y5.18;\
  G1 X-7.5 Y5.13;\
  G1 Y5.09;\
  G1 Y5.04;\
  G1 Y5.0;\
  G1 Y-5.0;\
  G1 Y-5.04;\
  G1 Y-5.09;\
  G1 Y-5.13;\
  G1 X-7.49 Y-5.18;\
  G1 Y-5.22;\
  G1 Y-5.27;\
  G1 X-7.48 Y-5.31;\
  G1 X-7.47 Y-5.36;\
  G1 Y-5.4;\
  G1 X-7.46 Y-5.44;\
  G1 X-7.45 Y-5.49;\
  G1 X-7.44 Y-5.53;\
  G1 X-7.43 Y-5.57;\
  G1 X-7.42 Y-5.62;\
  G1 X-7.41 Y-5.66;\
  G1 X-7.4 Y-5.7;\
  G1 X-7.39 Y-5.75;\
  G1 X-7.37 Y-5.79;\
  G1 X-7.36 Y-5.83;\
  G1 X-7.34 Y-5.87;\
  G1 X-7.33 Y-5.92;\
  G1 X-7.31 Y-5.96;\
  G1 X-7.29 Y-6.0;\
  G1 X-7.27 Y-6.04;\
  G1 X-7.26 Y-6.08;\
  G1 X-7.24 Y-6.12;\
  G1 X-7.22 Y-6.16;\
  G1 X-7.19 Y-6.2;\
  G1 X-7.17 Y-6.24;\
  G1 X-7.15 Y-6.28;\
  G1 X-7.13 Y-6.31;\
  G1 X-7.1 Y-6.35;\
  G1 X-7.08 Y-6.39;\
  G1 X-7.05 Y-6.43;\
  G1 X-7.03 Y-6.46;\
  G1 X-7.0 Y-6.5;\
  G1 X-6.97 Y-6.53;\
  G1 X-6.95 Y-6.57;\
  G1 X-6.92 Y-6.6;\
  G1 X-6.89 Y-6.64;\
  G1 X-6.86 Y-6.67;\
  G1 X-6.83 Y-6.7;\
  G1 X-6.8 Y-6.74;\
  G1 X-6.77 Y-6.77;\
  G1 X-6.74 Y-6.8;\
  G1 X-6.7 Y-6.83;\
  G1 X-6.67 Y-6.86;\
  G1 X-6.64 Y-6.89;\
  G1 X-6.6 Y-6.92;\
  G1 X-6.57 Y-6.95;\
  G1 X-6.53 Y-6.97;\
  G1 X-6.5 Y-7.0;\
  G1 X-6.46 Y-7.03;\
  G1 X-6.43 Y-7.05;\
  G1 X-6.39 Y-7.08;\
  G1 X-6.35 Y-7.1;\
  G1 X-6.31 Y-7.13;\
  G1 X-6.28 Y-7.15;\
  G1 X-6.24 Y-7.17;\
  G1 X-6.2 Y-7.19;\
  G1 X-6.16 Y-7.22;\
  G1 X-6.12 Y-7.24;\
  G1 X-6.08 Y-7.26;\
  G1 X-6.04 Y-7.27;\
  G1 X-6.0 Y-7.29;\
  G1 X-5.96 Y-7.31;\
  G1 X-5.92 Y-7.33;\
  G1 X-5.87 Y-7.34;\
  G1 X-5.83 Y-7.36;\
  G1 X-5.79 Y-7.37;\
  G1 X-5.75 Y-7.39;\
  G1 X-5.7 Y-7.4;\
  G1 X-5.66 Y-7.41;\
  G1 X-5.62 Y-7.42;\
  G1 X-5.57 Y-7.43;\
  G1 X-5.53 Y-7.44;\
  G1 X-5.49 Y-7.45;\
  G1 X-5.44 Y-7.46;\
  G1 X-5.4 Y-7.47;\
  G1 X-5.36;\
  G1 X-5.31 Y-7.48;\
  G1 X-5.27 Y-7.49;\
  G1 X-5.22;\
  G1 X-5.18;\
  G1 X-5.13 Y-7.5;\
  G1 X-5.09;\
  G1 X-5.04;\
  G1 X-5.0;\
  G0 Z4.0;\
  G0 Z6.0"
