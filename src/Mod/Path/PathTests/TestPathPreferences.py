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

import PathScripts.PathPreferences as PathPreferences
import PathTests.PathTestUtils as PathTestUtils

class TestPathPreferences(PathTestUtils.PathTestBase):

    def test00(self):
        '''There is at least one search path.'''

        paths = PathPreferences.searchPaths()
        self.assertGreater(len(paths), 0)

    def test01(self):
        '''PathScripts is part of the posts search path.'''
        paths = PathPreferences.searchPathsPost()
        self.assertEqual(len([p for p in paths if p.endswith('/PathScripts/')]), 1)

    def test02(self):
        '''PathScripts/post is part of the posts search path.'''
        paths = PathPreferences.searchPathsPost()
        self.assertEqual(len([p for p in paths if p.endswith('/PathScripts/post/')]), 1)

    def test03(self):
        '''Available post processors include linuxcnc, grbl and opensbp.'''
        posts = PathPreferences.allAvailablePostProcessors()
        self.assertTrue('linuxcnc' in posts)
        self.assertTrue('grbl' in posts)
        self.assertTrue('opensbp' in posts)


    def test10(self):
        '''Default paths for tools are resolved correctly'''

        self.assertTrue(PathPreferences.pathDefaultToolsPath().endswith('/Path/Tools/'))
        self.assertTrue(PathPreferences.pathDefaultToolsPath('Bit').endswith('/Path/Tools/Bit'))
        self.assertTrue(PathPreferences.pathDefaultToolsPath('Library').endswith('/Path/Tools/Library'))
        self.assertTrue(PathPreferences.pathDefaultToolsPath('Template').endswith('/Path/Tools/Template'))
