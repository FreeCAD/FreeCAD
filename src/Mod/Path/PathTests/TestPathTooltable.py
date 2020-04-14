# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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

import Path

from PathTests.PathTestUtils import PathTestBase

class TestPathTooltable(PathTestBase):

    def test00(self):
        '''Verify templateAttrs'''

        t = Path.Tool(name='t', diameter=1.2)
        u = Path.Tool(name='u', diameter=3.4)
        v = Path.Tool(name='v', diameter=5.6)

        tt = Path.Tooltable()
        tt.setTool(3, t)
        tt.setTool(1, u)
        tt.addTools(v)

        attrs = tt.templateAttrs()
        self.assertEqual(3, len(attrs))
        self.assertTrue(1 in attrs)
        self.assertFalse(2 in attrs)
        self.assertTrue(3 in attrs)
        self.assertTrue(4 in attrs)

        self.assertEqual(attrs[1]['name'], 'u')
        self.assertEqual(attrs[1]['diameter'], 3.4)
        self.assertEqual(attrs[3]['name'], 't')
        self.assertEqual(attrs[3]['diameter'], 1.2)
        self.assertEqual(attrs[4]['name'], 'v')
        self.assertEqual(attrs[4]['diameter'], 5.6)
        return tt

    def test01(self):
        '''Verify setFromTemplate roundtrip.'''
        tt = self.test00()
        uu = Path.Tooltable()
        uu.setFromTemplate(tt.templateAttrs())

        self.assertEqual(tt.Content, uu.Content)


    def test02(self):
        '''Verify template constructor.'''
        tt = self.test00()
        uu = Path.Tooltable(tt.templateAttrs())
        self.assertEqual(tt.Content, uu.Content)

