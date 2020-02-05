# ***************************************************************************
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Unit test for the Draft Workbench, object creation tests."""

import unittest
import math
import FreeCAD as App
import Draft
import drafttests.auxiliary as aux
from FreeCAD import Vector
from draftutils.messages import _msg


class DraftCreation(unittest.TestCase):
    """Test Draft creation functions."""

    def setUp(self):
        """Set up a new document to hold the tests.

        This is executed before every test, so we create a document
        to hold the objects.
        """
        aux._draw_header()
        self.doc_name = self.__class__.__name__
        if App.ActiveDocument:
            if App.ActiveDocument.Name != self.doc_name:
                App.newDocument(self.doc_name)
        else:
            App.newDocument(self.doc_name)
        App.setActiveDocument(self.doc_name)
        self.doc = App.ActiveDocument
        _msg("  Temporary document '{}'".format(self.doc_name))

    def test_line(self):
        """Create a line."""
        operation = "Draft Line"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(2, 0, 0)
        _msg("  a={0}, b={1}".format(a, b))
        obj = Draft.makeLine(a, b)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_polyline(self):
        """Create a polyline."""
        operation = "Draft Wire"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(2, 0, 0)
        c = Vector(2, 2, 0)
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={}".format(c))
        obj = Draft.makeWire([a, b, c])
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_fillet(self):
        """Create two lines, and a fillet between them."""
        operation = "Draft Fillet"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(8, 0, 0)
        c = Vector(8, 8, 0)
        _msg("  Lines")
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  b={0}, c={1}".format(b, c))
        L1 = Draft.makeLine(a, b)
        L2 = Draft.makeLine(b, c)
        App.ActiveDocument.recompute()

        if not App.GuiUp:
            aux._no_gui("DraftFillet")
            self.assertTrue(True)
            return

        import DraftFillet
        radius = 4
        _msg("  Fillet")
        _msg("  radius={}".format(radius))
        obj = DraftFillet.makeFillet([L1, L2], radius)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_circle(self):
        """Create a circle."""
        operation = "Draft Circle"
        _msg("  Test '{}'".format(operation))
        radius = 3
        _msg("  radius={}".format(radius))
        obj = Draft.makeCircle(radius)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_arc(self):
        """Create a circular arc."""
        operation = "Draft Arc"
        _msg("  Test '{}'".format(operation))
        radius = 2
        start_angle = 0
        end_angle = 90
        _msg("  radius={}".format(radius))
        _msg("  startangle={0}, endangle={1}".format(start_angle,
                                                     end_angle))
        obj = Draft.makeCircle(radius,
                               startangle=start_angle, endangle=end_angle)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_arc_3points(self):
        """Create a circular arc from three points."""
        operation = "Draft Arc 3Points"
        _msg("  Test '{}'".format(operation))
        a = Vector(5, 0, 0)
        b = Vector(4, 3, 0)
        c = Vector(0, 5, 0)
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={}".format(c))

        import draftobjects.arc_3points as arc3
        obj = arc3.make_arc_3points([a, b, c])
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_ellipse(self):
        """Create an ellipse."""
        operation = "Draft Ellipse"
        _msg("  Test '{}'".format(operation))
        a = 5
        b = 3
        _msg("  major_axis={0}, minor_axis={1}".format(a, b))
        obj = Draft.makeEllipse(a, b)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_polygon(self):
        """Create a regular polygon."""
        operation = "Draft Polygon"
        _msg("  Test '{}'".format(operation))
        n_faces = 6
        radius = 5
        _msg("  n_faces={0}, radius={1}".format(n_faces, radius))
        obj = Draft.makePolygon(n_faces, radius)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_rectangle(self):
        """Create a rectangle."""
        operation = "Draft Rectangle"
        _msg("  Test '{}'".format(operation))
        length = 5
        width = 2
        _msg("  length={0}, width={1}".format(length, width))
        obj = Draft.makeRectangle(length, width)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_text(self):
        """Create a text object."""
        operation = "Draft Text"
        _msg("  Test '{}'".format(operation))
        text = "Testing testing"
        _msg("  text='{}'".format(text))
        obj = Draft.makeText(text)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_dimension_linear(self):
        """Create a linear dimension."""
        operation = "Draft Dimension"
        _msg("  Test '{}'".format(operation))
        _msg("  Occasionally crashes")
        a = Vector(0, 0, 0)
        b = Vector(9, 0, 0)
        c = Vector(4, -1, 0)
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={}".format(c))
        obj = Draft.makeDimension(a, b, c)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_dimension_radial(self):
        """Create a circle and then a radial dimension."""
        operation = "Draft Dimension Radial"
        _msg("  Test '{}'".format(operation))
        radius = 3
        start_angle = 0
        end_angle = 90
        _msg("  radius={}".format(radius))
        _msg("  startangle={0}, endangle={1}".format(start_angle,
                                                     end_angle))
        circ = Draft.makeCircle(radius,
                                startangle=start_angle, endangle=end_angle)
        obj1 = Draft.makeDimension(circ, 0,
                                   p3="radius", p4=Vector(1, 1, 0))
        obj2 = Draft.makeDimension(circ, 0,
                                   p3="diameter", p4=Vector(3, 1, 0))
        self.assertTrue(obj1 and obj2, "'{}' failed".format(operation))

    def test_dimension_angular(self):
        """Create an angular dimension between two lines at given angles."""
        operation = "Draft Dimension Angular"
        _msg("  Test '{}'".format(operation))
        _msg("  Occasionally crashes")
        center = Vector(0, 0, 0)
        angle1 = math.radians(60)
        angle2 = math.radians(10)
        p3 = Vector(3, 1, 0)
        _msg("  center={}".format(center))
        _msg("  angle1={0}, angle2={1}".format(math.degrees(angle1),
                                               math.degrees(angle2)))
        _msg("  point={}".format(p3))
        obj = Draft.makeAngularDimension(center, [angle1, angle2], p3)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_bspline(self):
        """Create a BSpline of three points."""
        operation = "Draft BSpline"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(2, 0, 0)
        c = Vector(2, 2, 0)
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={}".format(c))
        obj = Draft.makeBSpline([a, b, c])
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_point(self):
        """Create a point."""
        operation = "Draft Point"
        _msg("  Test '{}'".format(operation))
        p = Vector(5, 3, 2)
        _msg("  p.x={0}, p.y={1}, p.z={2}".format(p.x, p.y, p.z))
        obj = Draft.makePoint(p.x, p.y, p.z)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_shapestring(self):
        """Create a ShapeString. NOT IMPLEMENTED CURRENTLY."""
        operation = "Draft ShapeString"
        _msg("  Test '{}'".format(operation))
        _msg("  In order to test this, a font file is needed.")
        text = "Testing Shapestring "
        # TODO: find a reliable way to always get a font file here
        font = None
        _msg("  text='{0}', font='{1}'".format(text, font))
        Draft.makeShapeString = aux._fake_function
        obj = Draft.makeShapeString("Text", font)
        # Draft.makeShapeString("Text", FontFile="")
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_facebinder(self):
        """Create a box, and then a facebinder from its faces."""
        operation = "Draft Facebinder"
        _msg("  Test '{}'".format(operation))
        _msg("  In order to test this, a selection is needed")
        _msg("  or an App::PropertyLinkSubList")

        _msg("  Box")
        box = App.ActiveDocument.addObject("Part::Box")
        App.ActiveDocument.recompute()
        # The facebinder function accepts a Gui selection set,
        # or a 'PropertyLinkSubList'

        # Gui selection set only works when the graphical interface is up
        # Gui.Selection.addSelection(box, ('Face1', 'Face6'))
        # selection_set = Gui.Selection.getSelectionEx()
        # elements = selection_set[0].SubElementNames

        # PropertyLinkSubList
        selection_set = [(box, ("Face1", "Face6"))]
        elements = selection_set[0][1]
        _msg("  object='{0}' ({1})".format(box.Shape.ShapeType, box.TypeId))
        _msg("  sub-elements={}".format(elements))
        obj = Draft.makeFacebinder(selection_set)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_cubicbezcurve(self):
        """Create a cubic bezier curve of four points."""
        operation = "Draft CubBezCurve"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(2, 2, 0)
        c = Vector(5, 3, 0)
        d = Vector(9, 0, 0)
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={0}, d={1}".format(c, d))
        obj = Draft.makeBezCurve([a, b, c, d], degree=3)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_bezcurve(self):
        """Create a bezier curve of six points, degree five."""
        operation = "Draft BezCurve"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(2, 2, 0)
        c = Vector(5, 3, 0)
        d = Vector(9, 0, 0)
        e = Vector(12, 5, 0)
        f = Vector(12, 8, 0)
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={0}, d={1}".format(c, d))
        _msg("  e={0}, f={1}".format(e, f))
        obj = Draft.makeBezCurve([a, b, c, d, e, f])
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_label(self):
        """Create a label."""
        operation = "Draft Label"
        _msg("  Test '{}'".format(operation))
        _msg("  Occasionally crashes")
        target_point = Vector(0, 0, 0)
        distance = -25
        placement = App.Placement(Vector(50, 50, 0), App.Rotation())
        _msg("  target_point={0}, "
             "distance={1}".format(target_point, distance))
        _msg("  placement={}".format(placement))
        obj = Draft.makeLabel(targetpoint=target_point,
                              distance=distance,
                              placement=placement)
        App.ActiveDocument.recompute()
        self.assertTrue(obj, "'{}' failed".format(operation))

    def tearDown(self):
        """Finish the test.

        This is executed after each test, so we close the document.
        """
        App.closeDocument(self.doc_name)
