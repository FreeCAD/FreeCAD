# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Werner Mayer <wmayer[at]users.sourceforge.net>
# SPDX-FileNotice: Part of the xwzCAD project.

import FreeCAD as App
import Part
import math

import unittest


class EdgeSortTests(unittest.TestCase):

    def makeRoundedRectangle(self):
        radius = 20
        xmin = -130
        xmax = 130
        ymin = 100
        ymax = 900
        zmin = 1000

        line1 = Part.makeLine(
            App.Vector(xmin, ymin + radius, zmin), App.Vector(xmin, ymax - radius, zmin)
        )
        line2 = Part.makeLine(
            App.Vector(xmin + radius, ymax, zmin), App.Vector(xmax - radius, ymax, zmin)
        )
        line3 = Part.makeLine(
            App.Vector(xmax, ymax - radius, zmin), App.Vector(xmax, ymin + radius, zmin)
        )
        line4 = Part.makeLine(
            App.Vector(xmax - radius, ymin, zmin), App.Vector(xmin + radius, ymin, zmin)
        )

        circle1 = Part.Circle()
        circle1.Radius = radius
        circle1.Location = App.Vector(xmin + radius, ymax - radius, zmin)
        arc1 = circle1.toShape(math.pi / 2, math.pi)

        circle3 = Part.Circle()
        circle3.Radius = radius
        circle3.Location = App.Vector(xmax - radius, ymin + radius, zmin)
        arc3 = circle3.toShape(1.5 * math.pi, 2 * math.pi)

        circle2 = Part.Circle()
        circle2.Radius = radius
        circle2.Location = App.Vector(xmax - radius, ymax - radius, zmin)
        arc2 = circle2.toShape(0, math.pi / 2)

        circle4 = Part.Circle()
        circle4.Radius = radius
        circle4.Location = App.Vector(xmin + radius, ymin + radius, zmin)
        arc4 = circle4.toShape(math.pi, 1.5 * math.pi)

        poly = Part.Wire([line1, arc1, line2, arc2, line3, arc3, line4, arc4])

        cyl = Part.Cylinder()
        cyl.Radius = radius
        shape = cyl.toShape(0, math.pi, 0, 1200)

        plm = shape.Placement
        plm.Rotation.Axis = App.Vector(1, 0, 0)
        plm.Rotation.Angle = math.pi / 2
        plm.Base = App.Vector(0, 1080, -164)
        shape.Placement = plm

        proj = shape.project([poly])
        wire = Part.Wire(proj.Edges)
        return wire

    def isVertical(self, edge):
        vec = edge.Vertex1.Point - edge.Vertex2.Point
        return abs(vec.dot(App.Vector(0, 0, 1))) > 0.95

    def testEdgeSort(self):
        wire = self.makeRoundedRectangle()
        ps = wire.discretize(Deflection=0.01)
        line_edges = [Part.makeLine(ps[i], ps[i + 1]) for i in range(len(ps) - 1)]
        wire_discritized = Part.Wire(line_edges)
        wall = wire_discritized.extrude(App.Vector(0, 0, 10))
        wall_offset = wall.makeOffsetShape(2.5, tolerance=0.01, join=2)
        edges_offset = [e for e in wall_offset.Edges if not self.isVertical(e)]
        edges_sorted = Part.sortEdges(edges_offset)
        wire_offset_1 = Part.Wire(edges_sorted[0])
        wire_offset_2 = Part.Wire(edges_sorted[1])

        self.assertEqual(len(edges_offset), len(edges_sorted[0]) + len(edges_sorted[1]))
        self.assertEqual(len(wire_offset_1.Edges), len(edges_sorted[0]))
        self.assertEqual(len(wire_offset_2.Edges), len(edges_sorted[1]))
