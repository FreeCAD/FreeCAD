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

import FreeCAD
import PathScripts.PathGeom as PathGeom
import PathScripts.PathThreadMilling as PathThreadMilling
import math

from PathTests.PathTestUtils import PathTestBase


def radii(internal, major, minor, toolDia, toolCrest):
    """test radii function for simple testing"""
    return (minor, major)


class TestPathThreadMilling(PathTestBase):
    """Test thread milling basics."""

    def assertRadii(self, have, want):
        self.assertRoughly(have[0], want[0])
        self.assertRoughly(have[1], want[1])

    def assertList(self, have, want):
        self.assertEqual(len(have), len(want))
        for i in range(len(have)):
            self.assertRoughly(have[i], want[i])

    def test00(self):
        """Verify internal radii."""
        self.assertRadii(PathThreadMilling.threadRadii(True, 20, 18, 2, 0), (8, 9.2))
        self.assertRadii(PathThreadMilling.threadRadii(True, 20, 19, 2, 0), (8.5, 9.1))

    def test01(self):
        """Verify internal radii with tool crest."""
        self.assertRadii(
            PathThreadMilling.threadRadii(True, 20, 18, 2, 0.1), (8, 9.113397)
        )

    def test10(self):
        """Verify internal thread passes."""
        self.assertList(
            PathThreadMilling.threadPasses(1, radii, True, 10, 9, 0, 0), [10]
        )
        self.assertList(
            PathThreadMilling.threadPasses(2, radii, True, 10, 9, 0, 0), [9.707107, 10]
        )
        self.assertList(
            PathThreadMilling.threadPasses(5, radii, True, 10, 9, 0, 0),
            [9.447214, 9.632456, 9.774597, 9.894427, 10],
        )

    def test20(self):
        """Verify external radii."""
        self.assertRadii(PathThreadMilling.threadRadii(False, 20, 18, 2, 0), (11, 9.6))
        self.assertRadii(PathThreadMilling.threadRadii(False, 20, 19, 2, 0), (11, 10.3))

    def test21(self):
        """Verify external radii with tool crest."""
        self.assertRadii(
            PathThreadMilling.threadRadii(False, 20, 18, 2, 0.1), (11, 9.513397)
        )

    def test30(self):
        """Verify external thread passes."""
        self.assertList(
            PathThreadMilling.threadPasses(1, radii, False, 10, 9, 0, 0), [9]
        )
        self.assertList(
            PathThreadMilling.threadPasses(2, radii, False, 10, 9, 0, 0), [9.292893, 9]
        )
        self.assertList(
            PathThreadMilling.threadPasses(5, radii, False, 10, 9, 0, 0),
            [9.552786, 9.367544, 9.225403, 9.105573, 9],
        )
