# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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
import Part
import Path
import PathScripts.PathChamfer as PathChamfer
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathTests.PathTestUtils as PathTestUtils

from FreeCAD import Vector

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
#PathLog.trackModule(PathLog.thisModule())

def getWire(obj):
    return obj.Tool.Tip.Profile[0].Shape.Wires[0]

def getPositiveShape(obj):
    return obj.Tool.Shape

def getNegativeShape(obj):
    return obj.Shape

class TestPathChamfer(PathTestUtils.PathTestBase):

    def setUp(self):
        self.doc = FreeCAD.open(FreeCAD.getHomePath() + 'Mod/Path/PathTests/test_chamfer.fcstd')
        self.circle = self.doc.getObjectsByLabel('circle-cut')[0]
        self.square = self.doc.getObjectsByLabel('square-cut')[0]
        self.triangle = self.doc.getObjectsByLabel('triangle-cut')[0]
        self.shape = self.doc.getObjectsByLabel('shape-cut')[0]

    def tearDown(self):
        FreeCAD.closeDocument("test_chamfer")

    def test01(self):
        '''Check offsetting a cylinder.'''
        obj = self.circle

        wire = PathChamfer.offsetWire(getWire(obj), getPositiveShape(obj), 3, True)
        self.assertEqual(1, len(wire.Edges))
        edge = wire.Edges[0]
        self.assertCoincide(Vector(), edge.Curve.Center)
        self.assertCoincide(Vector(0, 0, -1), edge.Curve.Axis)
        self.assertRoughly(33, edge.Curve.Radius)

        # the other way around everything's the same except the axis is negative
        wire = PathChamfer.offsetWire(getWire(obj), getPositiveShape(obj), 3, False)
        self.assertEqual(1, len(wire.Edges))
        edge = wire.Edges[0]
        self.assertCoincide(Vector(), edge.Curve.Center)
        self.assertCoincide(Vector(0, 0, +1), edge.Curve.Axis)
        self.assertRoughly(33, edge.Curve.Radius)


    def test02(self):
        '''Check offsetting a box.'''
        obj = self.square

        wire = PathChamfer.offsetWire(getWire(obj), getPositiveShape(obj), 3, True)
        self.assertEqual(8, len(wire.Edges))
        self.assertEqual(4, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(4, len([e for e in wire.Edges if Part.Circle == type(e.Curve)]))
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                if PathGeom.isRoughly(e.Vertexes[0].Point.x, e.Vertexes[1].Point.x):
                    self.assertEqual(40, e.Length)
                if PathGeom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y):
                    self.assertEqual(60, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(3, e.Curve.Radius)
                # As it turns out the arcs are oriented the wrong way
                self.assertCoincide(Vector(0, 0, +1), e.Curve.Axis)


        wire = PathChamfer.offsetWire(getWire(obj), getPositiveShape(obj), 3, False)
        self.assertEqual(8, len(wire.Edges))
        self.assertEqual(4, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(4, len([e for e in wire.Edges if Part.Circle == type(e.Curve)]))
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                if PathGeom.isRoughly(e.Vertexes[0].Point.x, e.Vertexes[1].Point.x):
                    self.assertEqual(40, e.Length)
                if PathGeom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y):
                    self.assertEqual(60, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(3, e.Curve.Radius)
                # As it turns out the arcs are oriented the wrong way
                self.assertCoincide(Vector(0, 0, -1), e.Curve.Axis)

