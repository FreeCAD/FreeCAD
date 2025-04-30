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

import CAMTests.PathTestUtils as PathTestUtils
import FreeCAD
import os
import pathlib
from Path.Tool.toolbit.util import (
    get_toolbit_filepath_from_name,
    get_tool_library_from_name,
)
from Path.Tool.toolbit import ToolBitBullnose

TestToolDir = pathlib.Path(os.path.realpath(__file__)).parent.parent / "Tools"
TestToolBitDir = TestToolDir / "Bit"


class TestPathToolBit(PathTestUtils.PathTestBase):
    def testGetToolBit(self):
        """Find a tool bit from file name"""
        path = get_toolbit_filepath_from_name("5mm_Endmill.fctb", TestToolBitDir)
        self.assertIsNot(path, None)
        self.assertNotEqual(path, "5mm_Endmill.fctb")

    def testGetLibrary(self):
        """Find a tool library from file name"""
        path = get_tool_library_from_name("Default.fctl")
        self.assertIsNot(path, None)
        self.assertNotEqual(path, "Default.fctl")

    def testBullnose(self):
        """Test ToolBitBullnose basic parameters"""
        # Create a new document and a dummy FreeCAD object
        FreeCAD.newDocument("TestBullnoseDoc")
        obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "TestBullnoseObj")
        bullnose_shape = ToolBitBullnose.SHAPE_CLASS.from_file(
            TestToolDir / "Shape" / "bullnose.fcstd"
        )
        bullnose_bit = ToolBitBullnose(obj, bullnose_shape)
        self.assertEqual(bullnose_bit.obj.ShapeName, "Bullnose")
        self.assertEqual(bullnose_bit.obj.Diameter, FreeCAD.Units.Quantity("5.0 mm"))
        self.assertEqual(bullnose_bit.obj.FlatRadius, FreeCAD.Units.Quantity("1.5 mm"))
        # Clean up the dummy object and close the document
        FreeCAD.ActiveDocument.removeObject("TestBullnoseObj")
        FreeCAD.closeDocument("TestBullnoseDoc")
