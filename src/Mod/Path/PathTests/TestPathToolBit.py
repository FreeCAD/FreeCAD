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

import Path.Tool.Bit as PathToolBit
import PathTests.PathTestUtils as PathTestUtils
import glob
import os

TestToolDir = os.path.join(os.path.dirname(os.path.realpath(__file__)), "Tools")
TestInvalidDir = os.path.join(
    TestToolDir, "some", "silly", "path", "that", "should", "not", "exist"
)

TestToolBitName = "test-path-tool-bit-bit-00.fctb"
TestToolShapeName = "test-path-tool-bit-shape-00.fcstd"
TestToolLibraryName = "test-path-tool-bit-library-00.fctl"


def testToolShape(path=TestToolDir, name=TestToolShapeName):
    return os.path.join(path, "Shape", name)


def testToolBit(path=TestToolDir, name=TestToolBitName):
    return os.path.join(path, "Bit", name)


def testToolLibrary(path=TestToolDir, name=TestToolLibraryName):
    return os.path.join(path, "Library", name)


def printTree(path, indent):
    print("{} {}".format(indent, os.path.basename(path)))
    if os.path.isdir(path):
        if os.path.basename(path).startswith("__"):
            print("{}   ...".format(indent))
        else:
            for foo in sorted(glob.glob(os.path.join(path, "*"))):
                printTree(foo, "{}  ".format(indent))


class TestPathToolBit(PathTestUtils.PathTestBase):
    def test(self):
        """Log test setup directory structure"""
        # Enable this test if there are errors showing up in the build system with the
        # paths that work OK locally. It'll print out the directory tree, and if it
        # doesn't look right you know where to look for it
        print()
        print("realpath : {}".format(os.path.realpath(__file__)))
        print("   Tools : {}".format(TestToolDir))
        print("     dir : {}".format(os.path.dirname(os.path.realpath(__file__))))
        printTree(os.path.dirname(os.path.realpath(__file__)), "         :")

    def test00(self):
        """Find a tool shape from file name"""
        path = PathToolBit.findToolShape("endmill.fcstd")
        self.assertIsNot(path, None)
        self.assertNotEqual(path, "endmill.fcstd")

    def test01(self):
        """Not find a relative path shape if not stored in default location"""
        path = PathToolBit.findToolShape(TestToolShapeName)
        self.assertIsNone(path)

    def test02(self):
        """Find a relative path shape if it's local to a bit path"""
        path = PathToolBit.findToolShape(TestToolShapeName, testToolBit())
        self.assertIsNot(path, None)
        self.assertEqual(path, testToolShape())

    def test03(self):
        """Not find a tool shape from an invalid absolute path."""
        path = PathToolBit.findToolShape(testToolShape(TestInvalidDir))
        self.assertIsNone(path)

    def test04(self):
        """Find a tool shape from a valid absolute path."""
        path = PathToolBit.findToolShape(testToolShape())
        self.assertIsNot(path, None)
        self.assertEqual(path, testToolShape())

    def test10(self):
        """Find a tool bit from file name"""
        path = PathToolBit.findToolBit("5mm_Endmill.fctb")
        self.assertIsNot(path, None)
        self.assertNotEqual(path, "5mm_Endmill.fctb")

    def test11(self):
        """Not find a relative path bit if not stored in default location"""
        path = PathToolBit.findToolBit(TestToolBitName)
        self.assertIsNone(path)

    def test12(self):
        """Find a relative path bit if it's local to a library path"""
        path = PathToolBit.findToolBit(TestToolBitName, testToolLibrary())
        self.assertIsNot(path, None)
        self.assertEqual(path, testToolBit())

    def test13(self):
        """Not find a tool bit from an invalid absolute path."""
        path = PathToolBit.findToolBit(testToolBit(TestInvalidDir))
        self.assertIsNone(path)

    def test14(self):
        """Find a tool bit from a valid absolute path."""
        path = PathToolBit.findToolBit(testToolBit())
        self.assertIsNot(path, None)
        self.assertEqual(path, testToolBit())

    def test20(self):
        """Find a tool library from file name"""
        path = PathToolBit.findToolLibrary("Default.fctl")
        self.assertIsNot(path, None)
        self.assertNotEqual(path, "Default.fctl")

    def test21(self):
        """Not find a relative path library if not stored in default location"""
        path = PathToolBit.findToolLibrary(TestToolLibraryName)
        self.assertIsNone(path)

    def test22(self):
        """[skipped] Find a relative path library if it's local to <what?>"""
        # this is not a valid test for libraries because t
        self.assertTrue(True)

    def test23(self):
        """Not find a tool library from an invalid absolute path."""
        path = PathToolBit.findToolLibrary(testToolLibrary(TestInvalidDir))
        self.assertIsNone(path)

    def test24(self):
        """Find a tool library from a valid absolute path."""
        path = PathToolBit.findToolBit(testToolBit())
        self.assertIsNot(path, None)
        self.assertEqual(path, testToolBit())
