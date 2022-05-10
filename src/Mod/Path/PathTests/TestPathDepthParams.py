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

import PathScripts.PathUtils as PathUtils
import unittest


class depthTestCases(unittest.TestCase):
    def test00(self):
        """Stepping down to zero"""
        args = {
            "clearance_height": 15,
            "safe_height": 12,
            "start_depth": 10,
            "step_down": 2,
            "z_finish_step": 1,
            "final_depth": 0,
            "user_depths": None,
        }

        expected = [8, 6, 4, 2, 1, 0]

        d = PathUtils.depth_params(**args)
        r = [i for i in d]
        self.assertListEqual(r, expected)

    def test001(self):
        """Stepping from zero to a negative depth"""

        args = {
            "clearance_height": 10,
            "safe_height": 5,
            "start_depth": 0,
            "step_down": 2,
            "z_finish_step": 0,
            "final_depth": -10,
            "user_depths": None,
        }

        expected = [-2, -4, -6, -8, -10]

        d = PathUtils.depth_params(**args)
        r = [i for i in d]
        self.assertListEqual(r, expected)

    def test002(self):
        """Start and end are equal or start lower than finish"""

        args = {
            "clearance_height": 15,
            "safe_height": 12,
            "start_depth": 10,
            "step_down": 2,
            "z_finish_step": 0,
            "final_depth": 10,
            "user_depths": None,
        }
        expected = [10]

        d = PathUtils.depth_params(**args)
        r = [i for i in d]
        self.assertListEqual(r, expected)

        args["start_depth"] = 10
        args["final_depth"] = 15

        expected = []

        d = PathUtils.depth_params(**args)
        r = [i for i in d]
        self.assertListEqual(r, expected)

    def test003(self):
        """User Parameters passed in"""
        args = {
            "clearance_height": 10,
            "safe_height": 5,
            "start_depth": 0,
            "step_down": 2,
            "z_finish_step": 0,
            "final_depth": -10,
            "user_depths": [2, 4, 8, 10, 11, 12],
        }

        expected = [2, 4, 8, 10, 11, 12]

        d = PathUtils.depth_params(**args)
        r = [i for i in d]
        self.assertListEqual(r, expected)

    def test004(self):
        """z_finish_step passed in."""
        args = {
            "clearance_height": 10,
            "safe_height": 5,
            "start_depth": 0,
            "step_down": 2,
            "z_finish_step": 1,
            "final_depth": -10,
            "user_depths": None,
        }

        expected = [-2, -4, -6, -8, -9, -10]

        d = PathUtils.depth_params(**args)
        r = [i for i in d]
        self.assertListEqual(r, expected)

    def test005(self):
        """stepping down with equalstep=True"""
        args = {
            "clearance_height": 10,
            "safe_height": 5,
            "start_depth": 10,
            "step_down": 3,
            "z_finish_step": 0,
            "final_depth": 0,
            "user_depths": None,
            "equalstep": True,
        }

        expected = [7.5, 5.0, 2.5, 0]

        d = PathUtils.depth_params(**args)
        r = [i for i in d]
        self.assertListEqual(r, expected)

    def test006(self):
        """stepping down with equalstep=True and a finish depth"""
        args = {
            "clearance_height": 10,
            "safe_height": 5,
            "start_depth": 10,
            "step_down": 3,
            "z_finish_step": 1,
            "final_depth": 0,
            "user_depths": None,
        }

        expected = [7.0, 4.0, 1.0, 0]

        d = PathUtils.depth_params(**args)
        r = [i for i in d]
        self.assertListEqual(r, expected)

    def test007(self):
        """stepping down with stepdown greater than total depth"""
        args = {
            "clearance_height": 10,
            "safe_height": 5,
            "start_depth": 10,
            "step_down": 20,
            "z_finish_step": 1,
            "final_depth": 0,
            "user_depths": None,
        }

        expected = [1.0, 0]

        d = PathUtils.depth_params(**args)
        r = [i for i in d]
        self.assertListEqual(r, expected)

    def test008(self):
        """Test handling of negative step-down, negative finish step, and relative size of step/finish"""

        # negative steps should be converted to positive values
        args = {
            "clearance_height": 3,
            "safe_height": 3,
            "start_depth": 2,
            "step_down": -1,
            "z_finish_step": -1,
            "final_depth": 0,
            "user_depths": None,
        }

        expected = [1.0, 0]

        d = PathUtils.depth_params(**args)
        r = [i for i in d]
        self.assertListEqual(r, expected)

        # a step_down less than the finish step is an error
        args = {
            "clearance_height": 3,
            "safe_height": 3,
            "start_depth": 2,
            "step_down": 0.1,
            "z_finish_step": 1,
            "final_depth": 0,
            "user_depths": None,
        }
        self.assertRaises(ValueError, PathUtils.depth_params, **args)

    def test009(self):
        """stepping down with single stepdown exactly equal to total depth"""
        args = {
            "clearance_height": 20.0,
            "safe_height": 15.0,
            "start_depth": 10.0,
            "step_down": 10.0,
            "z_finish_step": 0.0,
            "final_depth": 0.0,
            "user_depths": None,
        }

        expected = [0]

        d = PathUtils.depth_params(**args)
        r = [i for i in d]
        self.assertListEqual(
            r, expected, "Expected {}, but result of {}".format(expected, r)
        )

    def test010(self):
        """stepping down with single stepdown roughly equal to total depth"""
        args = {
            "clearance_height": 20.0,
            "safe_height": 15.0,
            "start_depth": 10.000000001,
            "step_down": 10.0,
            "z_finish_step": 0.0,
            "final_depth": 0.0,
            "user_depths": None,
        }

        expected = [0]

        d = PathUtils.depth_params(**args)
        r = [i for i in d]
        self.assertListEqual(
            r, expected, "Expected {}, but result of {}".format(expected, r)
        )

        args = {
            "clearance_height": 20.0,
            "safe_height": 15.0,
            "start_depth": 10.0,
            "step_down": 9.9999999,
            "z_finish_step": 0.0,
            "final_depth": 0.0,
            "user_depths": None,
        }

        d = PathUtils.depth_params(**args)
        r = [i for i in d]
        self.assertListEqual(
            r, expected, "Expected {}, but result of {}".format(expected, r)
        )
