# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 sliptonic <shopinthewoods@gmail.com>               *
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
import Path
import PathTests.PathTestUtils as PathTestUtils

vd = None

def initVD():
    global vd
    if vd is None:
        pts = [(0,0), (3.5,0), (3.5,1), (1,1), (1,2), (2.5,2), (2.5,3), (1,3), (1,4), (3.5, 4), (3.5,5), (0,5)]
        ptv = [FreeCAD.Vector(p[0], p[1]) for p in pts]
        ptv.append(ptv[0])

        vd = Path.Voronoi()
        for i in range(len(pts)):
            vd.addSegment(ptv[i], ptv[i+1])

        vd.construct()

        for e in vd.Edges:
            e.Color = 0 if e.isPrimary() else 1;
        vd.colorExterior(2)
        vd.colorColinear(3)
        vd.colorTwins(4)


class TestPathVoronoi(PathTestUtils.PathTestBase):

    def setUp(self):
        initVD()

    def test00(self):
        '''Check vertex comparison'''

        self.assertTrue(    vd.Vertices[0] == vd.Vertices[0])
        self.assertTrue(    vd.Vertices[1] == vd.Vertices[1])
        self.assertTrue(    vd.Vertices[0] != vd.Vertices[1])
        self.assertEqual(   vd.Vertices[0], vd.Vertices[0])
        self.assertEqual(   vd.Vertices[1], vd.Vertices[1])
        self.assertNotEqual(vd.Vertices[0], vd.Vertices[1])
        self.assertNotEqual(vd.Vertices[1], vd.Vertices[0])

    def test10(self):
        '''Check edge comparison'''

        self.assertTrue(    vd.Edges[0] == vd.Edges[0])
        self.assertTrue(    vd.Edges[1] == vd.Edges[1])
        self.assertTrue(    vd.Edges[0] != vd.Edges[1])
        self.assertEqual(   vd.Edges[0], vd.Edges[0])
        self.assertEqual(   vd.Edges[1], vd.Edges[1])
        self.assertNotEqual(vd.Edges[0], vd.Edges[1])
        self.assertNotEqual(vd.Edges[1], vd.Edges[0])


    def test20(self):
        '''Check cell comparison'''

        self.assertTrue(    vd.Cells[0] == vd.Cells[0])
        self.assertTrue(    vd.Cells[1] == vd.Cells[1])
        self.assertTrue(    vd.Cells[0] != vd.Cells[1])
        self.assertEqual(   vd.Cells[0], vd.Cells[0])
        self.assertEqual(   vd.Cells[1], vd.Cells[1])
        self.assertNotEqual(vd.Cells[0], vd.Cells[1])
        self.assertNotEqual(vd.Cells[1], vd.Cells[0])

