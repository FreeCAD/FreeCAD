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
import PathScripts.PathJob as PathJob
import PathScripts.PathPocketShape as PathPocketShape
import PathScripts.PathGeom as PathGeom
from PathTests.PathTestUtils import PathTestBase
if FreeCAD.GuiUp:
    import PathScripts.PathPocketShapeGui as PathPocketShapeGui
    import PathScripts.PathJobGui as PathJobGui


class TestPathPocketShape(PathTestBase):
    '''Unit tests for the PocketShape operation.'''

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

        # Open existing document with test geometry
        doc = FreeCAD.open(FreeCAD.getHomePath() + 'Mod/Path/PathTests/test_pocketshape.fcstd')

        # Create Job object, adding geometry objects from file opened above
        job = PathJob.Create('Job', [doc.Body002, doc.Box2, doc.Body, doc.Body001], None)
        job.GeometryTolerance.Value = 0.001
        if FreeCAD.GuiUp:
            job.ViewObject.Proxy = PathJobGui.ViewProvider(job.ViewObject)

        # Instantiate an Adaptive operation for quering available properties
        prototype = PathPocketShape.Create("PocketShape")
        prototype.Base = [(doc.Body002, ["Face17"])]
        prototype.Label = "Prototype"
        _addViewProvider(prototype)

        doc.recompute()

    @classmethod
    def tearDownClass(cls):
        '''tearDownClass()...
        This method is called prior to destruction of this test class.  Add code and objects here
        that cleanup the test environment after the test() methods in this class have been executed.
        This method does not have access to the class `self` reference.  This method
        is able to call static methods within this same class.
        '''

        # Comment out to leave test file open and objects and paths intact after all tests finish
        FreeCAD.closeDocument(FreeCAD.ActiveDocument.Name)
        pass

    # Setup and tear down methods called before and after each unit test
    def setUp(self):
        '''setUp()...
        This method is called prior to each `test()` method.  Add code and objects here
        that are needed for multiple `test()` methods.
        '''
        self.doc = FreeCAD.ActiveDocument
        self.con = FreeCAD.Console

    def tearDown(self):
        '''tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        '''
        pass

    # Unit tests
    def test00(self):
        '''test00() Verify default property values.'''

        # Instantiate a PocketShape operation and set Base Geometry
        pocketShape = FreeCAD.ActiveDocument.getObject('PocketShape')

        defaults = {
            'CutMode': 'Climb',
            'ExtraOffset': 0.0,
            'StartAt': 'Center',
            'StepOver': 100,
            'ZigZagAngle': 45.0,
            'OffsetPattern': 'ZigZag',
            'MinTravel': False,
            'KeepToolDown': False,
            'UseOutline': False
            # 'AreaParams': '',  # Changes when operation is executed
            # 'PathParams': '',  # Changes when operation is executed
            # 'ShowDebugShapes': False
        }
        for k, v in defaults.items():
            prop = getattr(pocketShape, k)
            if hasattr(prop, "Value"):
                self.assertEqual(prop.Value, v)
            else:
                self.assertEqual(prop, v)

    def test01(self):
        '''test01() Verify Use Outline feature.'''

        # Instantiate a PocketShape operation and set Base Geometry
        pocketShape = PathPocketShape.Create('PocketShape')
        pocketShape.Base = [(self.doc.Body002, ["Face20"])]  # (base, subs_list)
        pocketShape.Label = "test01+"
        pocketShape.Comment = "test01() Verify Use Outline feature."

        # Set additional operation properties
        pocketShape.UseOutline = True
        pocketShape.OffsetPattern = "Offset"
        pocketShape.StepOver = 50
        pocketShape.setExpression('StepDown', None)
        pocketShape.StepDown.Value = 20.0  # Have to set expression to None before numerical value assignment

        _addViewProvider(pocketShape)
        self.doc.recompute()

        moves = getGcodeMoves(pocketShape.Path.Commands, includeRapids=False)
        operationMoves = ";  ".join(moves)
        # pocketShape.Comment = operationMoves
        # self.con.PrintMessage("test00_moves: " + operationMoves + "\n")

        self.assertTrue(expected_moves_test01 == operationMoves,
                        "expected_moves_test01: {}\noperationMoves: {}".format(expected_moves_test01, operationMoves))

    def test02(self):
        '''test02() PocketShape check basic functionality with Body001:Face18 and OffsetPattern=Offset; StepOver=50'''

        # Instantiate a PocketShape operation and set Base Geometry
        pocketShape = PathPocketShape.Create('PocketShape')
        pocketShape.Base = [(self.doc.Body001, ["Face18"])]  # (base, subs_list)
        pocketShape.Label = "test02+"
        pocketShape.Comment = "PocketShape check basic functionality with Body001:Face18 and OffsetPattern=Offset; StepOver=50"

        # Set additional operation properties
        pocketShape.OffsetPattern = "Offset"  # "ZigZagOffset"
        pocketShape.StepOver = 50
        pocketShape.setExpression('StepDown', None)
        pocketShape.StepDown.Value = 20.0  # Have to set expression to None before numerical value assignment

        _addViewProvider(pocketShape)
        self.doc.recompute()

        moves = getGcodeMoves(pocketShape.Path.Commands, includeRapids=False)
        operationMoves = ";  ".join(moves)
        # pocketShape.Comment = operationMoves
        # self.con.PrintMessage("test00_moves: " + operationMoves + "\n")

        self.assertTrue(expected_moves_test02 == operationMoves,
                        "expected_moves_test02: {} operationMoves: {}".format(expected_moves_test02, operationMoves))

    def test03(self):
        '''test03() PocketShape verify overhang is ignored with Body001:Face16 and OffsetPattern=Offset; StepOver=50'''

        # Instantiate a PocketShape operation and set Base Geometry
        pocketShape = PathPocketShape.Create('PocketShape')
        pocketShape.Base = [(self.doc.Body001, ["Face16"])]  # (base, subs_list)
        pocketShape.Label = "test03+"
        pocketShape.Comment = "PocketShape verify overhang is ignored with Body001:Face16 and OffsetPattern=Offset; StepOver=50"

        # Set additional operation properties
        pocketShape.OffsetPattern = "Offset"
        pocketShape.StepOver = 50
        pocketShape.setExpression('StepDown', None)
        pocketShape.StepDown.Value = 20.0  # Have to set expression to None before numerical value assignment

        _addViewProvider(pocketShape)
        self.doc.recompute()
        
        moves = getGcodeMoves(pocketShape.Path.Commands, includeRapids=False)
        operationMoves = ";  ".join(moves)
        # pocketShape.Comment = operationMoves
        # self.con.PrintMessage("test01_moves: " + operationMoves + "\n")

        self.assertTrue(expected_moves_test03 == operationMoves,
                        "expected_moves_test03: {} operationMoves: {}".format(expected_moves_test03, operationMoves))

    def test04(self):
        '''test04() PocketShape verify Extra Offset with Body001:Face18 and OffsetPattern=Spiral; StepOver=50; ExtraOffset=-2.5'''

        # Instantiate a PocketShape operation and set Base Geometry
        pocketShape = PathPocketShape.Create('PocketShape')
        pocketShape.Base = [(self.doc.Body001, ["Face18"])]  # (base, subs_list)
        pocketShape.Label = "test04+"
        pocketShape.Comment = "PocketShape verify Extra Offset with Body001:Face18 and OffsetPattern=Offset; StepOver=50; ExtraOffset=-2.5"
        # self.doc.recompute()

        # Set additional operation properties
        pocketShape.OffsetPattern = "Spiral"  # "ZigZagOffset"
        pocketShape.StepOver = 50
        pocketShape.ExtraOffset = -2.5
        pocketShape.setExpression('StepDown', None)
        pocketShape.StepDown.Value = 20.0  # Have to set expression to None before numerical value assignment

        _addViewProvider(pocketShape)
        self.doc.recompute()

        moves = getGcodeMoves(pocketShape.Path.Commands, includeRapids=False)
        operationMoves = ";  ".join(moves)
        # pocketShape.Comment = operationMoves
        # self.con.PrintMessage("test02_moves: " + ";  \\\n".join(moves) + "\n")

        self.assertTrue(expected_moves_test04 == operationMoves,
                        "expected_moves_test04: {} operationMoves: {}".format(expected_moves_test04, operationMoves))

    def test05(self):
        '''test05() PocketShape verify complete, exterior extension only on Body002:Face27 circular face.'''

        # Instantiate a PocketShape operation and set Base Geometry
        pocketShape = PathPocketShape.Create('PocketShape')
        pocketShape.Base = [(self.doc.Body002, ["Face27"])]  # (base, subs_list)
        pocketShape.Label = "test05+"
        pocketShape.Comment = "PocketShape verify complete, exterior extension only on Body002:Face27 circular face"
        # self.doc.recompute()

        # Set additional operation properties
        pocketShape.OffsetPattern = "Offset"
        pocketShape.StepOver = 50
        pocketShape.setExpression('StepDown', None)
        pocketShape.StepDown.Value = 20.0  # Have to set expression to None before numerical value assignment
        pocketShape.ExtensionFeature = [(self.doc.Body002, ('Face27:Edge64',))]

        _addViewProvider(pocketShape)
        self.doc.recompute()

        moves = getGcodeMoves(pocketShape.Path.Commands, includeRapids=False)
        operationMoves = ";  ".join(moves)
        # pocketShape.Comment = operationMoves
        # self.con.PrintMessage("test02_moves: " + ";  \\\n".join(moves) + "\n")

        self.assertTrue(expected_moves_test05 == operationMoves,
                        "expected_moves_test05: {} operationMoves: {}".format(expected_moves_test05, operationMoves))
        
        # Check if any paths originate inside inner hole of donut.  They should not.
        isInBox = False
        edgBB = self.doc.Body002.Shape.getElement("Edge65").BoundBox
        minPoint = FreeCAD.Vector(edgBB.XMin, edgBB.YMin, 0.0)
        maxPoint = FreeCAD.Vector(edgBB.XMax, edgBB.YMax, 0.0)
        for c in pocketShape.Path.Commands:
            if pathOriginatesInBox(c, minPoint, maxPoint):
                isInBox = True
                break
        self.assertFalse(isInBox, "Paths originating within the inner hole.")

    def test06(self):
        '''test06() PocketShape verify multi-height face combination with extension.'''

        # Instantiate a PocketShape operation and set Base Geometry
        pocketShape = PathPocketShape.Create('PocketShape')
        pocketShape.Base = [(self.doc.Body002, ["Face17", "Face15"])]  # (base, subs_list)
        pocketShape.Label = "test06+"
        pocketShape.Comment = "PocketShape verify multi-height face combination with extension"
        # self.doc.recompute()

        # Set additional operation properties
        pocketShape.OffsetPattern = "Offset"
        pocketShape.StepOver = 50
        pocketShape.setExpression('StepDown', None)
        pocketShape.StepDown.Value = 20.0  # Have to set expression to None before numerical value assignment
        pocketShape.ExtensionFeature = [(self.doc.Body002, ('Face17:Edge20',))]

        _addViewProvider(pocketShape)
        self.doc.recompute()

        moves = getGcodeMoves(pocketShape.Path.Commands, includeRapids=False)
        operationMoves = ";  ".join(moves)
        # pocketShape.Comment = operationMoves
        # self.con.PrintMessage("test02_moves: " + ";  \\\n".join(moves) + "\n")

        self.assertTrue(expected_moves_test06 == operationMoves,
                        "expected_moves_test06: {} operationMoves: {}".format(expected_moves_test06, operationMoves))
# Eclass


def getGcodeMoves(cmdList, includeRapids=True, includeLines=True, includeArcs=True):
    '''getGcodeMoves(cmdList, includeRapids=True, includeLines=True, includeArcs=True)...
    Accepts command dict and returns point string coordinate.
    '''
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
            if p.get("X"):
                x = round(p["X"], 2)
                gcode += " X" + str(x) 
            if p.get("Y"):
                y = round(p["Y"], 2)
                gcode += " Y" + str(y) 
            if p.get("Z"):
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
            if p.get("X"):
                x = round(p["X"], 2)
                gcode += " X" + str(x) 
            if p.get("Y"):
                y = round(p["Y"], 2)
                gcode += " Y" + str(y) 
            if p.get("Z"):
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
        PathOpGui = PathPocketShapeGui.PathOpGui
        cmdRes = PathPocketShapeGui.Command.res
        op.ViewObject.Proxy = PathOpGui.ViewProvider(op.ViewObject, cmdRes)
        op.ViewObject.Document.setEdit(op.ViewObject, 5)


# Expected moves for unit tests
expected_moves_test01 = "G1 X32.5 Y32.5 Z5.0;  \
G1 X17.5 Y32.5 Z5.0;  \
G1 X17.5 Y17.5 Z5.0;  \
G1 X32.5 Y17.5 Z5.0;  \
G1 X32.5 Y32.5 Z5.0;  \
G1 X30.0 Y30.0 Z5.0;  \
G1 X20.0 Y30.0 Z5.0;  \
G1 X20.0 Y20.0 Z5.0;  \
G1 X30.0 Y20.0 Z5.0;  \
G1 X30.0 Y30.0 Z5.0;  \
G1 X27.5 Y27.5 Z5.0;  \
G1 X22.5 Y27.5 Z5.0;  \
G1 X22.5 Y22.5 Z5.0;  \
G1 X27.5 Y22.5 Z5.0;  \
G1 X27.5 Y27.5 Z5.0;  \
G1 X26.25 Y26.25 Z5.0;  \
G1 X23.75 Y26.25 Z5.0;  \
G1 X23.75 Y23.75 Z5.0;  \
G1 X26.25 Y23.75 Z5.0;  \
G1 X26.25 Y26.25 Z5.0"

expected_moves_test02 = "G1 X22.5 Y-17.5 Z15.0;  \
G1 X17.5 Y-17.5 Z15.0;  \
G1 X17.5 Y-22.5 Z15.0;  \
G1 X22.5 Y-22.5 Z15.0;  \
G1 X22.5 Y-17.5 Z15.0;  \
G1 X21.25 Y-18.75 Z15.0;  \
G1 X18.75 Y-18.75 Z15.0;  \
G1 X18.75 Y-21.25 Z15.0;  \
G1 X21.25 Y-21.25 Z15.0;  \
G1 X21.25 Y-18.75 Z15.0"

expected_moves_test03 = "G1 X32.5 Y-22.5 Z5.0;  \
G1 X22.5 Y-22.5 Z5.0;  \
G1 X22.5 Y-32.5 Z5.0;  \
G1 X22.76 Y-32.5 Z5.0;  \
G2 I12.21 J-2.47 K0.0 X32.5 Y-22.76 Z5.0;  \
G1 X32.5 Y-22.5 Z5.0;  \
G1 X27.1 Y-23.75 Z5.0;  \
G1 X23.75 Y-23.75 Z5.0;  \
G1 X23.75 Y-27.1 Z5.0;  \
G2 I11.2 J-7.85 K0.0 X27.1 Y-23.75 Z5.0"

expected_moves_test04 = "G1 X25.0 Y-15.0 Z15.0;  \
G1 X25.0 Y-22.5 Z15.0;  \
G1 X22.5 Y-22.5 Z15.0;  \
G1 X17.5 Y-22.5 Z15.0;  \
G1 X17.5 Y-17.5 Z15.0;  \
G1 X22.5 Y-17.5 Z15.0;  \
G1 X22.5 Y-22.5 Z15.0;  \
G1 X25.0 Y-22.5 Z15.0;  \
G1 X25.0 Y-25.0 Z15.0;  \
G1 X15.0 Y-25.0 Z15.0;  \
G1 X15.0 Y-15.0 Z15.0;  \
G1 X25.0 Y-15.0 Z15.0"

expected_moves_test05 = "G1 X32.07 Y67.07 Z15.0;  \
G3 I-7.07 J-7.07 K0.0 X23.44 Y69.87 Z15.0;  \
G3 I1.56 J-9.87 K0.0 X26.56 Y50.13 Z15.0;  \
G3 I-1.56 J9.87 K0.0 X32.07 Y67.07 Z15.0;  \
G1 X31.19 Y66.19 Z15.0;  \
G3 I-6.19 J-6.19 K0.0 X23.63 Y68.64 Z15.0;  \
G3 I1.37 J-8.64 K0.0 X26.37 Y51.36 Z15.0;  \
G3 I-1.37 J8.64 K0.0 X31.19 Y66.19 Z15.0;  \
G1 X30.48 Y65.48 Z15.0;  \
G3 I-5.48 J-5.48 K0.0 X25.78 Y67.71 Z15.0;  \
G3 I-0.78 J-7.71 K0.0 X24.22 Y52.29 Z15.0;  \
G3 I0.78 J7.71 K0.0 X30.48 Y65.48 Z15.0;  \
G1 X29.59 Y64.59 Z15.0;  \
G3 I-4.59 J-4.59 K0.0 X25.61 Y66.47 Z15.0;  \
G3 I-0.61 J-6.47 K0.0 X24.39 Y53.53 Z15.0;  \
G3 I0.61 J6.47 K0.0 X29.59 Y64.59 Z15.0"

expected_moves_test06 = "G1 X57.5 Y60.0 Z15.0;  \
G1 X52.5 Y60.0 Z15.0;  \
G1 X52.5 Y12.5 Z15.0;  \
G1 X57.5 Y12.5 Z15.0;  \
G1 X57.5 Y60.0 Z15.0;  \
G1 X56.25 Y58.75 Z15.0;  \
G1 X53.75 Y58.75 Z15.0;  \
G1 X53.75 Y13.75 Z15.0;  \
G1 X56.25 Y13.75 Z15.0;  \
G1 X56.25 Y58.75 Z15.0"