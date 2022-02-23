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
        self.assertRadii(PathThreadMilling.radiiInternal(20, 18, 2, 0), (8, 9.2))
        self.assertRadii(PathThreadMilling.radiiInternal(20, 19, 2, 0), (8.5, 9.1))

    def test01(self):
        """Verify internal radii with tool crest."""
        self.assertRadii(PathThreadMilling.threadRadii(True, 20, 18, 2, 0.1), (8, 9.113397))

    def test10(self):
        '''Verify internal thread passes.'''
        self.assertList(PathThreadMilling.threadPasses(1, radii, True, 10, 9, 0, 0), [10])
        self.assertList(PathThreadMilling.threadPasses(2, radii, True, 10, 9, 0, 0), [9.707107, 10])
        self.assertList(PathThreadMilling.threadPasses(5, radii, True, 10, 9, 0, 0), [9.447214, 9.632456, 9.774597, 9.894427, 10])

    def test20(self):
        '''Verify external radii.'''
        self.assertRadii(PathThreadMilling.threadRadii(False, 20, 18, 2, 0), (11,  9.6))
        self.assertRadii(PathThreadMilling.threadRadii(False, 20, 19, 2, 0), (11, 10.3))

    def test21(self):
        '''Verify external radii with tool crest.'''
        self.assertRadii(PathThreadMilling.threadRadii(False, 20, 18, 2, 0.1), (11, 9.513397))

    def test30(self):
        '''Verify external thread passes.'''
        self.assertList(PathThreadMilling.threadPasses(1, radii, False, 10, 9, 0, 0), [9])
        self.assertList(PathThreadMilling.threadPasses(2, radii, False, 10, 9, 0, 0), [9.292893, 9])
        self.assertList(PathThreadMilling.threadPasses(5, radii, False, 10, 9, 0, 0), [9.552786, 9.367544, 9.225403, 9.105573, 9])

    def test40(self):
        '''Verify thread commands for a single thread'''

        center      = FreeCAD.Vector()
        cmd         = 'G2'
        zStart      = 0
        zFinal      = 1
        pitch       = 1
        radius      = 3
        leadInOut   = False
        elevator    = 2

        path, start = PathThreadMilling.threadCommands(center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, None)

        gcode = [
                'G0 X0.000000 Y2.000000',
                'G0 Z0.000000', 
                'G1 Y3.000000', 
                'G2 J-3.000000 Y-3.000000 Z0.500000', 
                'G2 J3.000000 Y3.000000 Z1.000000', 
                'G1 X0.000000 Y2.000000', 
                ]
        self.assertEqual([p.toGCode() for p in path], gcode)
        self.assertCoincide(start, FreeCAD.Vector(0, 2, zFinal))

    def test41(self):
        '''Verify thread commands for a thwo threads'''

        center      = FreeCAD.Vector()
        cmd         = 'G2'
        zStart      = 0
        zFinal      = 2
        pitch       = 1
        radius      = 3
        leadInOut   = False
        elevator    = 2

        path, start = PathThreadMilling.threadCommands(center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, None)

        gcode = [
                'G0 X0.000000 Y2.000000',
                'G0 Z0.000000', 
                'G1 Y3.000000', 
                'G2 J-3.000000 Y-3.000000 Z0.500000', 
                'G2 J3.000000 Y3.000000 Z1.000000', 
                'G2 J-3.000000 Y-3.000000 Z1.500000', 
                'G2 J3.000000 Y3.000000 Z2.000000', 
                'G1 X0.000000 Y2.000000', 
                ]
        self.assertEqual([p.toGCode() for p in path], gcode)
        self.assertCoincide(start, FreeCAD.Vector(0, 2, zFinal))

    def test42(self):
        '''Verify thread commands for a one and a half threads'''

        center      = FreeCAD.Vector()
        cmd         = 'G2'
        zStart      = 0
        zFinal      = 1.5
        pitch       = 1
        radius      = 3
        leadInOut   = False
        elevator    = 2

        path, start = PathThreadMilling.threadCommands(center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, None)

        gcode = [
                'G0 X0.000000 Y2.000000',
                'G0 Z0.000000', 
                'G1 Y3.000000', 
                'G2 J-3.000000 Y-3.000000 Z0.500000', 
                'G2 J3.000000 Y3.000000 Z1.000000', 
                'G2 J-3.000000 Y-3.000000 Z1.500000', 
                'G1 X0.000000 Y-2.000000', 
                ]
        self.assertEqual([p.toGCode() for p in path], gcode)
        self.assertCoincide(start, FreeCAD.Vector(0, -2, zFinal))

    def test43(self):
        '''Verify thread commands for a one and 3 quarter threads'''

        center      = FreeCAD.Vector()
        cmd         = 'G2'
        zStart      = 0
        zFinal      = 1.75
        pitch       = 1
        radius      = 3
        leadInOut   = False
        elevator    = 2

        path, start = PathThreadMilling.threadCommands(center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, None)

        gcode = [
                'G0 X0.000000 Y2.000000',
                'G0 Z0.000000', 
                'G1 Y3.000000', 
                'G2 J-3.000000 Y-3.000000 Z0.500000', 
                'G2 J3.000000 Y3.000000 Z1.000000', 
                'G2 J-3.000000 Y-3.000000 Z1.500000', 
                #'(------- finish-thread -------)',
                'G2 J3.000000 X-3.000000 Y0.000000 Z1.750000', 
                #'(------- finish-thread -------)',
                'G1 X-2.000000 Y0.000000', 
                ]
        self.assertEqual([p.toGCode() for p in path], gcode)
        self.assertCoincide(start, FreeCAD.Vector(-2, 0, zFinal))

    def test44(self):
        '''Verify thread commands for a one and 3 quarter threads - CCW'''

        center      = FreeCAD.Vector()
        cmd         = 'G3'
        zStart      = 0
        zFinal      = 1.75
        pitch       = 1
        radius      = 3
        leadInOut   = False
        elevator    = 2

        path, start = PathThreadMilling.threadCommands(center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, None)

        gcode = [
                'G0 X0.000000 Y2.000000',
                'G0 Z0.000000', 
                'G1 Y3.000000', 
                'G3 J-3.000000 Y-3.000000 Z0.500000', 
                'G3 J3.000000 Y3.000000 Z1.000000', 
                'G3 J-3.000000 Y-3.000000 Z1.500000', 
                #'(------- finish-thread -------)',
                'G3 J3.000000 X3.000000 Y0.000000 Z1.750000', 
                #'(------- finish-thread -------)',
                'G1 X2.000000 Y0.000000', 
                ]
        self.assertEqual([p.toGCode() for p in path], gcode)
        self.assertCoincide(start, FreeCAD.Vector(2, 0, zFinal))

    def test50(self):
        '''Verify lead in/out commands for a single thread'''

        center      = FreeCAD.Vector()
        cmd         = 'G2'
        zStart      = 0
        zFinal      = 1
        pitch       = 1
        radius      = 3
        leadInOut   = True
        elevator    = 2

        path, start = PathThreadMilling.threadCommands(center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, None)

        gcode = [
                'G0 X0.000000 Y2.000000',
                'G0 Z0.000000', 
                #'(------- lead-in -------)',
                'G2 J0.500000 Y3.000000',
                #'(------- lead-in -------)',
                'G2 J-3.000000 Y-3.000000 Z0.500000', 
                'G2 J3.000000 Y3.000000 Z1.000000', 
                #'(------- lead-out -------)',
                'G2 I0.000000 J-0.500000 X0.000000 Y2.000000', 
                #'(------- lead-out -------)',
                ]
        self.assertEqual([p.toGCode() for p in path], gcode)
        self.assertCoincide(start, FreeCAD.Vector(0, 2, zFinal))

    def test51(self):
        '''Verify lead in/out commands for one and a half threads'''

        center      = FreeCAD.Vector()
        cmd         = 'G2'
        zStart      = 0
        zFinal      = 1.5
        pitch       = 1
        radius      = 3
        leadInOut   = True
        elevator    = 2

        path, start = PathThreadMilling.threadCommands(center, cmd, zStart, zFinal, pitch, radius, leadInOut, elevator, None)

        gcode = [
                'G0 X0.000000 Y2.000000',
                'G0 Z0.000000', 
                #'(------- lead-in -------)',
                'G2 J0.500000 Y3.000000',
                #'(------- lead-in -------)',
                'G2 J-3.000000 Y-3.000000 Z0.500000', 
                'G2 J3.000000 Y3.000000 Z1.000000', 
                'G2 J-3.000000 Y-3.000000 Z1.500000', 
                #'(------- lead-out -------)',
                'G2 I0.000000 J0.500000 X0.000000 Y-2.000000', 
                #'(------- lead-out -------)',
                ]
        self.assertEqual([p.toGCode() for p in path], gcode)
        self.assertCoincide(start, FreeCAD.Vector(0, -2, zFinal))

