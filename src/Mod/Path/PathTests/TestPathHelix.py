# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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

import FreeCAD
import PathScripts.PathHelix as PathHelix
import PathScripts.PathJob as PathJob
import PathScripts.PathLog as PathLog
import PathTests.PathTestUtils as PathTestUtils

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule(PathLog.thisModule())


class TestPathHelix(PathTestUtils.PathTestBase):

    def setUp(self):
        self.doc = FreeCAD.open(FreeCAD.getHomePath() + 'Mod/Path/PathTests/test_holes00.fcstd')
        self.job = PathJob.Create('Job', [self.doc.Body])

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def test00(self):
        '''Verify Helix does not throw an exception.'''

        op = PathHelix.Create('Helix')
        op.Proxy.findAllHoles(op)  # add all holes to the operation
        op.Proxy.execute(op)
