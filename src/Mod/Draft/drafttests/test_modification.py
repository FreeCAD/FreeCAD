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
"""Unit test for the Draft Workbench, object modification tests."""

import unittest
import FreeCAD as App
import Draft
import drafttests.auxiliary as aux
from FreeCAD import Vector
from draftutils.messages import _msg, _wrn


class DraftModification(unittest.TestCase):
    """Test Draft modification tools."""

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

    def test_move(self):
        """Create a line and move it."""
        operation = "Draft Move"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 2, 0)
        b = Vector(2, 2, 0)
        _msg("  Line")
        _msg("  a={0}, b={1}".format(a, b))
        obj = Draft.makeLine(a, b)

        c = Vector(3, 1, 0)
        _msg("  Translation vector")
        _msg("  c={}".format(c))
        Draft.move(obj, c)
        self.assertTrue(obj.Start == Vector(3, 3, 0),
                        "'{}' failed".format(operation))

    def test_copy(self):
        """Create a line, then copy and move it."""
        operation = "Draft Move with copy"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 3, 0)
        b = Vector(2, 3, 0)
        _msg("  Line")
        _msg("  a={0}, b={1}".format(a, b))
        line = Draft.makeLine(a, b)

        c = Vector(2, 2, 0)
        _msg("  Translation vector (copy)")
        _msg("  c={}".format(c))
        obj = Draft.move(line, c, copy=True)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_rotate(self):
        """Create a line, then rotate it."""
        operation = "Draft Rotate"
        _msg("  Test '{}'".format(operation))
        a = Vector(1, 1, 0)
        b = Vector(3, 1, 0)
        _msg("  Line")
        _msg("  a={0}, b={1}".format(a, b))
        obj = Draft.makeLine(a, b)
        App.ActiveDocument.recompute()

        c = Vector(-1, 1, 0)
        rot = 90
        _msg("  Rotation")
        _msg("  angle={} degrees".format(rot))
        Draft.rotate(obj, rot)
        self.assertTrue(obj.Start.isEqual(c, 1e-12),
                        "'{}' failed".format(operation))

    def test_offset_open(self):
        """Create a wire, then produce an offset copy."""
        operation = "Draft Offset"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 2, 0)
        b = Vector(2, 4, 0)
        c = Vector(5, 2, 0)
        _msg("  Wire")
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={0}".format(c))
        wire = Draft.makeWire([a, b, c])
        App.ActiveDocument.recompute()

        offset = Vector(-1, 1, 0)
        _msg("  Offset")
        _msg("  vector={}".format(offset))
        obj = Draft.offset(wire, offset, copy=True)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_offset_closed(self):
        """Create a rectangle, then produce an offset copy."""
        operation = "Draft Offset"
        _msg("  Test '{}'".format(operation))
        length = 4
        width = 2
        _msg("  Rectangle")
        _msg("  length={0}, width={1}".format(length, width))
        rect = Draft.makeRectangle(length, width)
        App.ActiveDocument.recompute()

        offset = Vector(-1, -1, 0)
        _msg("  Offset")
        _msg("  vector={}".format(offset))
        obj = Draft.offset(rect, offset, copy=True)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_trim(self):
        """Trim a line. NOT IMPLEMENTED."""
        operation = "Draft Trimex trim"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(3, 3, 0)

        _msg("  Line")
        _msg("  a={0}, b={1}".format(a, b))
        line = Draft.makeLine(a, b)

        c = Vector(2, 2, 0)
        d = Vector(4, 2, 0)
        _msg("  Line 2")
        _msg("  c={0}, d={1}".format(c, d))
        line2 = Draft.makeLine(c, d)
        App.ActiveDocument.recompute()

        Draft.trim_objects = aux._fake_function
        obj = Draft.trim_objects(line, line2)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_extend(self):
        """Extend a line. NOT IMPLEMENTED."""
        operation = "Draft Trimex extend"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(1, 1, 0)
        _msg("  Line")
        _msg("  a={0}, b={1}".format(a, b))
        line = Draft.makeLine(a, b)

        c = Vector(2, 2, 0)
        d = Vector(4, 2, 0)
        _msg("  Line 2")
        _msg("  c={0}, d={1}".format(c, d))
        line2 = Draft.makeLine(c, d)
        App.ActiveDocument.recompute()

        Draft.extrude = aux._fake_function
        obj = Draft.extrude(line, line2)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_join(self):
        """Join two lines into a single Draft Wire."""
        operation = "Draft Join"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(2, 2, 0)
        c = Vector(2, 4, 0)
        _msg("  Line 1")
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  Line 2")
        _msg("  b={0}, c={1}".format(b, c))
        line_1 = Draft.makeLine(a, b)
        line_2 = Draft.makeLine(b, c)

        # obj = Draft.joinWires([line_1, line_2])  # Multiple wires
        obj = Draft.joinTwoWires(line_1, line_2)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_split(self):
        """Split a Draft Wire into two Draft Wires."""
        operation = "Draft_Split"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(2, 2, 0)
        c = Vector(2, 4, 0)
        d = Vector(6, 4, 0)
        _msg("  Wire")
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={0}, d={1}".format(c, d))
        wire = Draft.makeWire([a, b, c, d])

        index = 1
        _msg("  Split at")
        _msg("  p={0}, index={1}".format(b, index))
        obj = Draft.split(wire, b, index)
        # TODO: split needs to be modified so that it returns True or False.
        # Then checking for Wire001 is not needed
        if App.ActiveDocument.Wire001:
            obj = True
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_upgrade(self):
        """Upgrade two Draft Lines into a closed Draft Wire."""
        operation = "Draft Upgrade"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(2, 2, 0)
        c = Vector(2, 4, 0)
        _msg("  Line 1")
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  Line 2")
        _msg("  b={0}, c={1}".format(b, c))
        line_1 = Draft.makeLine(a, b)
        line_2 = Draft.makeLine(b, c)
        App.ActiveDocument.recompute()

        obj = Draft.upgrade([line_1, line_2], delete=True)
        App.ActiveDocument.recompute()
        s = obj[0][0]
        _msg("  1: Result '{0}' ({1})".format(s.Shape.ShapeType, s.TypeId))
        self.assertTrue(bool(obj[0]), "'{}' failed".format(operation))

        obj2 = Draft.upgrade(obj[0], delete=True)
        App.ActiveDocument.recompute()
        s2 = obj2[0][0]
        _msg("  2: Result '{0}' ({1})".format(s2.Shape.ShapeType,
                                              s2.TypeId))
        self.assertTrue(bool(obj2[0]), "'{}' failed".format(operation))

        obj3 = Draft.upgrade(obj2[0], delete=True)
        App.ActiveDocument.recompute()
        s3 = obj3[0][0]
        _msg("  3: Result '{0}' ({1})".format(s3.Shape.ShapeType, s3.TypeId))
        self.assertTrue(bool(obj3[0]), "'{}' failed".format(operation))

        obj4 = Draft.upgrade(obj3[0], delete=True)
        App.ActiveDocument.recompute()
        wire = App.ActiveDocument.Wire
        _msg("  4: Result '{0}' ({1})".format(wire.Proxy.Type, wire.TypeId))
        _msg("  The last object cannot be upgraded further")
        self.assertFalse(bool(obj4[0]), "'{}' failed".format(operation))

    def test_downgrade(self):
        """Downgrade a closed Draft Wire into three simple Part Edges."""
        operation = "Draft Downgrade"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(2, 2, 0)
        c = Vector(2, 4, 0)
        _msg("  Closed wire")
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={0}, a={1}".format(c, a))
        wire = Draft.makeWire([a, b, c, a])
        App.ActiveDocument.recompute()

        obj = Draft.downgrade(wire, delete=True)
        App.ActiveDocument.recompute()
        s = obj[0][0]
        _msg("  1: Result '{0}' ({1})".format(s.Shape.ShapeType, s.TypeId))
        self.assertTrue(bool(obj[0]), "'{}' failed".format(operation))

        obj2 = Draft.downgrade(obj[0], delete=True)
        App.ActiveDocument.recompute()
        s2 = obj2[0][0]
        _msg("  2: Result '{0}' ({1})".format(s2.Shape.ShapeType, s2.TypeId))
        self.assertTrue(bool(obj2[0]), "'{}' failed".format(operation))

        obj3 = Draft.downgrade(obj2[0], delete=True)
        App.ActiveDocument.recompute()
        s3 = obj3[0][0]
        _msg("  3: Result 3 x '{0}' ({1})".format(s3.Shape.ShapeType,
                                                  s3.TypeId))
        self.assertTrue(len(obj3[0]) == 3, "'{}' failed".format(operation))

        obj4 = Draft.downgrade(obj3[0], delete=True)
        App.ActiveDocument.recompute()
        s4 = obj4[0]
        _msg("  4: Result '{}'".format(s4))
        _msg("  The last objects cannot be downgraded further")
        self.assertFalse(bool(obj4[0]), "'{}' failed".format(operation))

    def test_wire_to_bspline(self):
        """Convert a polyline to BSpline and back."""
        operation = "Draft WireToBSpline"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(2, 2, 0)
        c = Vector(2, 4, 0)
        _msg("  Wire")
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={}".format(c))
        wire = Draft.makeWire([a, b, c])

        obj = Draft.makeBSpline(wire.Points)
        App.ActiveDocument.recompute()
        _msg("  1: Result '{0}' ({1})".format(obj.Proxy.Type, obj.TypeId))
        self.assertTrue(obj, "'{}' failed".format(operation))

        obj2 = Draft.makeWire(obj.Points)
        _msg("  2: Result '{0}' ({1})".format(obj2.Proxy.Type, obj2.TypeId))
        self.assertTrue(obj2, "'{}' failed".format(operation))

    def test_shape_2d_view(self):
        """Create a prism and then a 2D projection of it."""
        operation = "Draft Shape2DView"
        _msg("  Test '{}'".format(operation))
        prism = App.ActiveDocument.addObject("Part::Prism")
        prism.Polygon = 5
        # Rotate the prism 45 degrees around the Y axis
        prism.Placement.Rotation.Axis = Vector(0, 1, 0)
        prism.Placement.Rotation.Angle = 45 * (3.14159/180)
        _msg("  Prism")
        _msg("  n_sides={}".format(prism.Polygon))
        _msg("  placement={}".format(prism.Placement))

        direction = Vector(0, 0, 1)
        _msg("  Projection 2D view")
        _msg("  direction={}".format(direction))
        obj = Draft.makeShape2DView(prism, direction)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_draft_to_sketch(self):
        """Convert a Draft object to a Sketch and back."""
        operation = "Draft Draft2Sketch"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(2, 2, 0)
        c = Vector(2, 4, 0)
        _msg("  Wire")
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={}".format(c))
        wire = Draft.makeWire([a, b, c])
        App.ActiveDocument.recompute()

        obj = Draft.makeSketch(wire, autoconstraints=True)
        App.ActiveDocument.recompute()
        _msg("  1: Result '{0}' ({1})".format(obj.Shape.ShapeType,
                                              obj.TypeId))
        self.assertTrue(obj, "'{}' failed".format(operation))

        obj2 = Draft.draftify(obj, delete=False)
        App.ActiveDocument.recompute()
        _msg("  2: Result '{0}' ({1})".format(obj2.Proxy.Type,
                                              obj2.TypeId))
        self.assertTrue(obj2, "'{}' failed".format(operation))

    def test_rectangular_array(self):
        """Create a rectangle, and a rectangular array."""
        operation = "Draft Array"
        _msg("  Test '{}'".format(operation))
        length = 4
        width = 2
        _msg("  Rectangle")
        _msg("  length={0}, width={1}".format(length, width))
        rect = Draft.makeRectangle(length, width)
        App.ActiveDocument.recompute()

        dir_x = Vector(5, 0, 0)
        dir_y = Vector(0, 4, 0)
        number_x = 3
        number_y = 4
        _msg("  Array")
        _msg("  direction_x={}".format(dir_x))
        _msg("  direction_y={}".format(dir_y))
        _msg("  number_x={0}, number_y={1}".format(number_x, number_y))
        obj = Draft.makeArray(rect,
                              dir_x, dir_y,
                              number_x, number_y)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_polar_array(self):
        """Create a rectangle, and a polar array."""
        operation = "Draft PolarArray"
        _msg("  Test '{}'".format(operation))
        length = 4
        width = 2
        _msg("  Rectangle")
        _msg("  length={0}, width={1}".format(length, width))
        rect = Draft.makeRectangle(length, width)
        App.ActiveDocument.recompute()

        center = Vector(-4, 0, 0)
        angle = 180
        number = 5
        _msg("  Array")
        _msg("  center={}".format(center))
        _msg("  polar_angle={0}, number={1}".format(angle, number))
        obj = Draft.makeArray(rect,
                              center, angle, number)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_circular_array(self):
        """Create a rectangle, and a circular array."""
        operation = "Draft CircularArray"
        _msg("  Test '{}'".format(operation))
        length = 4
        width = 2
        _msg("  Rectangle")
        _msg("  length={0}, width={1}".format(length, width))
        rect = Draft.makeRectangle(length, width)
        App.ActiveDocument.recompute()

        rad_distance = 10
        tan_distance = 8
        axis = Vector(0, 0, 1)
        center = Vector(0, 0, 0)
        number = 3
        symmetry = 1
        _msg("  Array")
        _msg("  radial_distance={0}, "
             "tangential_distance={1}".format(rad_distance, tan_distance))
        _msg("  axis={}".format(axis))
        _msg("  center={}".format(center))
        _msg("  number={0}, symmetry={1}".format(number, symmetry))
        obj = Draft.makeArray(rect,
                              rad_distance, tan_distance,
                              axis, center,
                              number, symmetry)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_path_array(self):
        """Create a wire, a polygon, and a path array."""
        operation = "Draft PathArray"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(2, 2, 0)
        c = Vector(2, 4, 0)
        d = Vector(8, 4, 0)
        _msg("  Wire")
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={0}, d={1}".format(c, d))
        wire = Draft.makeWire([a, b, c, d])

        n_faces = 3
        radius = 1
        _msg("  Polygon")
        _msg("  n_faces={0}, radius={1}".format(n_faces, radius))
        poly = Draft.makePolygon(n_faces, radius)

        number = 4
        translation = Vector(0, 1, 0)
        align = False
        _msg("  Path Array")
        _msg("  number={}, translation={}".format(number, translation))
        _msg("  align={}".format(align))
        obj = Draft.makePathArray(poly, wire, number, translation, align)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_point_array(self):
        """Create a polygon, various point, and a point array."""
        operation = "Draft PointArray"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(2, 2, 0)
        c = Vector(2, 4, 0)
        d = Vector(8, 4, 0)
        _msg("  Points")
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={0}, d={1}".format(c, d))
        points = [Draft.makePoint(a),
                  Draft.makePoint(b),
                  Draft.makePoint(c),
                  Draft.makePoint(d)]

        _msg("  Upgrade")
        add, delete = Draft.upgrade(points)
        compound = add[0]

        n_faces = 3
        radius = 1
        _msg("  Polygon")
        _msg("  n_faces={0}, radius={1}".format(n_faces, radius))
        poly = Draft.makePolygon(n_faces, radius)

        _msg("  Point Array")
        obj = Draft.makePointArray(poly, compound)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_clone(self):
        """Create a box, then create a clone of it.

        Test for a bug introduced by changes in attachment code.
        """
        operation = "Draft Clone"
        _msg("  Test '{}'".format(operation))
        box = App.ActiveDocument.addObject("Part::Box")
        App.ActiveDocument.recompute()
        _msg("  object: '{0}' ({1})".format(box.Shape.ShapeType, box.TypeId))

        obj = Draft.clone(box)
        _msg("  clone: '{0}' ({1})".format(obj.Proxy.Type, obj.TypeId))
        self.assertTrue(obj, "'{}' failed".format(operation))
        self.assertTrue(obj.hasExtension("Part::AttachExtension"),
                        "'{}' failed".format(operation))

    def test_draft_to_drawing(self):
        """Create a solid, and then a projected view in a Drawing page."""
        operation = "Draft Drawing"
        _msg("  Test '{}'".format(operation))
        _wrn("  The Drawing Workbench is obsolete since 0.17,")
        _wrn("  consider using the TechDraw Workbench instead")
        prism = App.ActiveDocument.addObject("Part::Prism")
        prism.Polygon = 5
        # Rotate the prism 45 degrees around the Y axis
        prism.Placement.Rotation.Axis = Vector(0, 1, 0)
        prism.Placement.Rotation.Angle = 45 * (3.14159/180)
        _msg("  Prism")
        _msg("  n_sides={}".format(prism.Polygon))
        _msg("  placement={}".format(prism.Placement))

        svg_template = 'Mod/Drawing/Templates/A3_Landscape.svg'
        template = Draft.getParam("template",
                                  App.getResourceDir() + svg_template)
        page = App.ActiveDocument.addObject('Drawing::FeaturePage')
        page.Template = template
        _msg("  Drawing view")
        _msg("  page={}".format(page.TypeId))
        _msg("  template={}".format(page.Template))
        obj = Draft.makeDrawingView(prism, page, otherProjection=None)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_mirror(self):
        """Create a rectangle, then a mirrored shape."""
        operation = "Draft Mirror"
        _msg("  Test '{}'".format(operation))
        length = 4
        width = 2
        _msg("  Rectangle")
        _msg("  length={0}, width={1}".format(length, width))
        rect = Draft.makeRectangle(length, width)
        # App.ActiveDocument.recompute()

        p1 = Vector(6, -2, 0)
        p2 = Vector(6, 2, 0)
        _msg("  Mirror axis")
        _msg("  p1={}".format(p1))
        _msg("  p2={}".format(p2))
        obj = Draft.mirror(rect, p1, p2)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_stretch(self):
        """Stretch a line. NOT IMPLEMENTED."""
        operation = "Draft Stretch"
        _msg("  Test '{}'".format(operation))
        _msg("  This test requires an object and a selection")
        a = Vector(0, 0, 0)
        b = Vector(1, 1, 0)
        _msg("  Line")
        _msg("  a={0}, b={1}".format(a, b))
        line = Draft.makeLine(a, b)
        direction = Vector(4, 1, 0)

        Draft.stretch = aux._fake_function
        obj = Draft.stretch(line, direction)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def tearDown(self):
        """Finish the test.

        This is executed after each test, so we close the document.
        """
        App.closeDocument(self.doc_name)
