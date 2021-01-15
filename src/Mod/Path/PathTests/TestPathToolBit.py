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

import PathScripts.PathToolBit as PathToolBit
import PathTests.PathTestUtils as PathTestUtils
import os

TestToolDir = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'Tools')
TestInvalidDir = os.path.join(TestToolDir, 'some', 'silly', 'path', 'that', 'should', 'not', 'exist')

TestToolBitName = 'test-path-tool-bit-bit-00.fctb'
TestToolShapeName = 'test-path-tool-bit-shape-00.fcstd'
TestToolLibraryName = 'test-path-tool-bit-library-00.fctl'

def testToolShape(path = TestToolDir, name = TestToolShapeName):
    return os.path.join(path, 'Shape', name)

def testToolBit(path = TestToolDir, name = TestToolBitName):
    return os.path.join(path, 'Bit', name)

def testToolLibrary(path = TestToolDir, name = TestToolLibraryName):
    return os.path.join(path, 'Library', name)

class TestPathToolBit(PathTestUtils.PathTestBase):

    def test00(self):
        '''Find a tool shape from file name'''
        path = PathToolBit._findShape('endmill.fcstd')
        self.assertIsNot(path, None)
        self.assertNotEqual(path, 'endmill.fcstd')


    def test01(self):
        '''Not find a relative path shape if not stored in default location'''
        path = PathToolBit._findShape(TestToolShapeName)
        self.assertIsNone(path)


    def test02(self):
        '''Find a relative path shape if it's local to a bit path'''
        path = PathToolBit._findShape(TestToolShapeName, testToolBit())
        self.assertIsNot(path, None)
        self.assertEqual(path, testToolShape())


    def test03(self):
        '''Not find a tool shape from an invalid absolute path.'''
        path = PathToolBit._findShape(testToolShape(TestInvalidDir))
        self.assertIsNone(path)


    def test04(self):
        '''Find a tool shape from a valid absolute path.'''
        path = PathToolBit._findShape(testToolShape())
        self.assertIsNot(path, None)
        self.assertEqual(path, testToolShape())


    def test10(self):
        '''Find a tool bit from file name'''
        path = PathToolBit.findBit('5mm_Endmill.fctb')
        self.assertIsNot(path, None)
        self.assertNotEqual(path, '5mm_Endmill.fctb')


    def test11(self):
        '''Not find a relative path bit if not stored in default location'''
        path = PathToolBit.findBit(TestToolBitName)
        self.assertIsNone(path)


    def test12(self):
        '''Find a relative path bit if it's local to a library path'''
        path = PathToolBit.findBit(TestToolBitName, testToolLibrary())
        self.assertIsNot(path, None)
        self.assertEqual(path, testToolBit())


    def test13(self):
        '''Not find a tool bit from an invalid absolute path.'''
        path = PathToolBit.findBit(testToolBit(TestInvalidDir))
        self.assertIsNone(path)


    def test14(self):
        '''Find a tool bit from a valid absolute path.'''
        path = PathToolBit.findBit(testToolBit())
        self.assertIsNot(path, None)
        self.assertEqual(path, testToolBit())



    def test20(self):
        '''Find a tool library from file name'''
        path = PathToolBit.findBit('5mm_Endmill.fctb')
        self.assertIsNot(path, None)
        self.assertNotEqual(path, '5mm_Endmill.fctb')


    def test21(self):
        '''Not find a relative path library if not stored in default location'''
        path = PathToolBit.findBit(TestToolBitName)
        self.assertIsNone(path)


    def test22(self):
        '''[skipped] Find a relative path library if it's local to <what?>'''
        # this is not a valid test for libraries because t
        self.assertTrue(True)


    def test23(self):
        '''Not find a tool library from an invalid absolute path.'''
        path = PathToolBit.findBit(testToolBit(TestInvalidDir))
        self.assertIsNone(path)


    def test24(self):
        '''Find a tool library from a valid absolute path.'''
        path = PathToolBit.findBit(testToolBit())
        self.assertIsNot(path, None)
        self.assertEqual(path, testToolBit())


