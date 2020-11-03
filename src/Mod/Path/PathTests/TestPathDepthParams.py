# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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

import PathScripts.PathUtils as PU
import unittest


class depthTestCases(unittest.TestCase):
    def test00(self):
        '''Stepping down to zero '''
        clearance_height= 15
        safe_height = 12

        start_depth = 10
        step_down = 2
        z_finish_step = 1
        final_depth = 0
        user_depths =  None

        expected =[8,6,4,2,1,0]

        d = PU.depth_params(clearance_height, safe_height, start_depth, step_down, z_finish_step, final_depth, user_depths)
        r = [i for i in d]
        self.assertListEqual (r, expected)

    def test10(self):
        '''Stepping from zero to a negative depth '''

        clearance_height= 10
        safe_height = 5

        start_depth = 0
        step_down = 2
        z_finish_step = 0
        final_depth = -10
        user_depths =  None

        expected =[-2, -4, -6, -8, -10]

        d = PU.depth_params(clearance_height, safe_height, start_depth, step_down, z_finish_step, final_depth, user_depths)
        r = [i for i in d]
        self.assertListEqual (r, expected)

    def test20(self):
        '''Start and end are equal or start lower than finish '''
        clearance_height= 15
        safe_height = 12

        start_depth = 10
        step_down = 2
        z_finish_step = 0
        final_depth = 10
        user_depths =  None

        expected =[10]

        d = PU.depth_params(clearance_height, safe_height, start_depth, step_down, z_finish_step, final_depth, user_depths)
        r = [i for i in d]
        self.assertListEqual (r, expected)

        start_depth = 10
        final_depth = 15

        expected =[]

        d = PU.depth_params(clearance_height, safe_height, start_depth, step_down, z_finish_step, final_depth, user_depths)
        r = [i for i in d]
        self.assertListEqual (r, expected)

    def test30(self):
        '''User Parameters passed in'''
        clearance_height= 10
        safe_height = 5

        start_depth = 0
        step_down = 2
        z_finish_step = 0
        final_depth = -10
        user_depths =  [2, 4, 8, 10, 11, 12]

        expected =[2, 4, 8, 10, 11, 12]

        d = PU.depth_params(clearance_height, safe_height, start_depth, step_down, z_finish_step, final_depth, user_depths)
        r = [i for i in d]
        self.assertListEqual (r, expected)

    def test40(self):
        '''z_finish_step passed in.'''
        clearance_height= 10
        safe_height = 5

        start_depth = 0
        step_down = 2
        z_finish_step = 1
        final_depth = -10
        user_depths =  None

        expected =[-2, -4, -6, -8, -9, -10]

        d = PU.depth_params(clearance_height, safe_height, start_depth, step_down, z_finish_step, final_depth, user_depths)
        r = [i for i in d]
        self.assertListEqual (r, expected)


    def test50(self):
        '''stepping down with equalstep=True'''
        clearance_height= 10
        safe_height = 5

        start_depth = 10
        step_down = 3
        z_finish_step = 0
        final_depth = 0
        user_depths =  None

        expected =[7.5, 5.0, 2.5, 0]

        d = PU.depth_params(clearance_height, safe_height, start_depth, step_down, z_finish_step, final_depth, user_depths, equalstep=True)
        r = [i for i in d]
        self.assertListEqual (r, expected)


    def test60(self):
        '''stepping down with equalstep=True and a finish depth'''
        clearance_height= 10
        safe_height = 5

        start_depth = 10
        step_down = 3
        z_finish_step = 1
        final_depth = 0
        user_depths =  None

        expected =[7.0, 4.0, 1.0, 0]

        d = PU.depth_params(clearance_height, safe_height, start_depth, step_down, z_finish_step, final_depth, user_depths, equalstep=True)
        r = [i for i in d]
        self.assertListEqual (r, expected)

    def test70(self):
        '''stepping down with stepdown greater than total depth'''
        clearance_height= 10
        safe_height = 5

        start_depth = 10
        step_down = 20
        z_finish_step = 1
        final_depth = 0
        user_depths =  None

        expected =[1.0, 0]

        d = PU.depth_params(clearance_height, safe_height, start_depth, step_down, z_finish_step, final_depth, user_depths)
        r = [i for i in d]
        self.assertListEqual (r, expected)


