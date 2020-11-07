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


class TestPathToolBit(PathTestUtils.PathTestBase):

    def test00(self):
        '''Find a tool shapee from file name'''

        path = PathToolBit.findShape('endmill.fcstd')
        self.assertIsNot(path, None)
        self.assertNotEqual(path, 'endmill.fcstd')

    def test01(self):
        '''Find a tool shapee from an invalid absolute path.'''

        path = PathToolBit.findShape('/this/is/unlikely/a/valid/path/v-bit.fcstd')
        self.assertIsNot(path, None)
        self.assertNotEqual(path, '/this/is/unlikely/a/valid/path/v-bit.fcstd')


    def test10(self):
        '''find the relative path of a tool bit'''
        shape = 'endmill.fcstd'
        path = PathToolBit.findShape(shape)
        self.assertIsNot(path, None)
        self.assertGreater(len(path), len(shape))
        rel = PathToolBit.findRelativePathShape(path)
        self.assertEqual(rel, shape)

    def test11(self):
        '''store full path if relative path isn't found'''
        path = '/this/is/unlikely/a/valid/path/v-bit.fcstd'
        rel = PathToolBit.findRelativePathShape(path)
        self.assertEqual(rel, path)

