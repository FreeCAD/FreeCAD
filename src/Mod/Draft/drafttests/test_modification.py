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
"""Unit tests for the Draft Workbench, object modification tests."""
## @package test_modification
# \ingroup drafttests
# \brief Unit tests for the Draft Workbench, object modification tests.

## \addtogroup drafttests
# @{
import unittest

import FreeCAD as App
import Draft
import drafttests.auxiliary as aux
import Part

from FreeCAD import Vector
from draftutils.messages import _msg, _wrn


class DraftModification(unittest.TestCase):
    """Test Draft modification tools."""

    def setUp(self):
        """Set up a new document to hold the tests.

        This is executed before every test, so we create a document
        to hold the objects.
        """
        aux.draw_header()
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
        obj = Draft.make_line(a, b)
        App.ActiveDocument.recompute()

        c = Vector(3, 1, 0)
        _msg("  Translation vector")
        _msg("  c={}".format(c))
        Draft.move(obj, c)
        App.ActiveDocument.recompute()
        self.assertTrue(obj.Start.isEqual(Vector(3, 3, 0), 1e-6),
                        "'{}' failed".format(operation))

    def test_copy(self):
        """Create a line, then copy and move it."""
        operation = "Draft Move with copy"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 3, 0)
        b = Vector(2, 3, 0)
        _msg("  Line")
        _msg("  a={0}, b={1}".format(a, b))
        line = Draft.make_line(a, b)

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
        obj = Draft.make_line(a, b)
        App.ActiveDocument.recompute()

        c = Vector(-1, 1, 0)
        rot = 90
        _msg("  Rotation")
        _msg("  angle={} degrees".format(rot))
        Draft.rotate(obj, rot)
        App.ActiveDocument.recompute()
        self.assertTrue(obj.Start.isEqual(c, 1e-6),
                        "'{}' failed".format(operation))

    def test_offset_open(self):
        """Create an open wire, then produce an offset copy."""
        operation = "Draft Offset"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 2, 0)
        b = Vector(2, 4, 0)
        c = Vector(5, 2, 0)
        _msg("  Open wire")
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={0}".format(c))
        wire = Draft.make_wire([a, b, c])
        App.ActiveDocument.recompute()

        offset = Vector(-1, 1, 0)
        _msg("  Offset")
        _msg("  vector={}".format(offset))
        obj = Draft.offset(wire, offset, copy=True)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_offset_closed_with_reversed_edge(self):
        """Create a closed wire with a reversed edge, then produce an offset copy."""
        # Regression test for:
        # https://github.com/FreeCAD/FreeCAD/pull/5496
        operation = "Draft Offset"
        _msg("  Test '{}'".format(operation))
        _msg("  Closed wire with reversed edge")
        a = Vector(0, 0, 0)
        b = Vector(10, 0, 0)
        c = Vector(10, 4, 0)
        d = Vector(0, 4, 0)
        edges = [Part.makeLine(a, b),
                 Part.makeLine(b, c),
                 Part.makeLine(c, d),
                 Part.makeLine(a, d)]
        wire = Part.Wire(edges)
        obj = App.ActiveDocument.addObject("Part::Feature")
        obj.Shape = wire

        offset = Vector(0, -1, 0)
        new = Draft.offset(obj, offset, copy=True)
        self.assertTrue(len(new.Points) == 4, "'{}' failed".format(operation))

    def test_offset_rectangle_with_face(self):
        """Create a rectangle with a face, then produce an offset copy."""
        # Regression test for:
        # https://github.com/FreeCAD/FreeCAD/pull/7670
        operation = "Draft Offset"
        _msg("  Test '{}'".format(operation))
        length = 10
        width = 4
        _msg("  Rectangle with face")
        _msg("  length={0}, width={1}".format(length, width))
        rect = Draft.make_rectangle(length, width)
        rect.MakeFace = True
        App.ActiveDocument.recompute()

        offset = Vector(0, -1, 0)
        _msg("  Offset")
        _msg("  vector={}".format(offset))
        obj = Draft.offset(rect, offset, copy=True)
        App.ActiveDocument.recompute()
        obj_is_ok = (obj.Shape.CenterOfGravity == Vector(5, 2, 0)
                     and obj.Length == 12
                     and obj.Height == 6)
        self.assertTrue(obj_is_ok, "'{}' failed".format(operation))

    def test_trim(self):
        """Trim a line. NOT IMPLEMENTED."""
        operation = "Draft Trimex trim"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(3, 3, 0)

        _msg("  Line")
        _msg("  a={0}, b={1}".format(a, b))
        line = Draft.make_line(a, b)

        c = Vector(2, 2, 0)
        d = Vector(4, 2, 0)
        _msg("  Line 2")
        _msg("  c={0}, d={1}".format(c, d))
        line2 = Draft.make_line(c, d)
        App.ActiveDocument.recompute()

        Draft.trim_objects = aux.fake_function
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
        line = Draft.make_line(a, b)

        c = Vector(2, 2, 0)
        d = Vector(4, 2, 0)
        _msg("  Line 2")
        _msg("  c={0}, d={1}".format(c, d))
        line2 = Draft.make_line(c, d)
        App.ActiveDocument.recompute()

        Draft.extrude = aux.fake_function
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
        line_1 = Draft.make_line(a, b)
        line_2 = Draft.make_line(b, c)

        # obj = Draft.join_wires([line_1, line_2])  # Multiple wires
        obj = Draft.join_two_wires(line_1, line_2)
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
        wire = Draft.make_wire([a, b, c, d])

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
        """Upgrade two Lines into a closed Wire, then draftify it."""
        operation = "Draft Upgrade"
        _msg("  Test '{}'".format(operation))
        a = Vector(0, 0, 0)
        b = Vector(2, 2, 0)
        c = Vector(2, 4, 0)
        _msg("  Line 1")
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  Line 2")
        _msg("  b={0}, c={1}".format(b, c))
        shape_line_1 = Part.makeLine(a, b)
        shape_line_2 = Part.makeLine(b, c)
        line_1 = App.ActiveDocument.addObject("Part::Feature")
        line_2 = App.ActiveDocument.addObject("Part::Feature")
        line_1.Shape = shape_line_1
        line_2.Shape = shape_line_2
        App.ActiveDocument.recompute()

        obj = Draft.upgrade([line_1, line_2], delete=True)
        App.ActiveDocument.recompute()
        s = obj[0][0]
        _msg("  1: Result '{0}' ({1})".format(s.Shape.ShapeType, s.TypeId))
        self.assertTrue(bool(obj[0]), "'{}' failed".format(operation))

        obj2 = Draft.upgrade(obj[0], delete=True)
        App.ActiveDocument.recompute()
        s2 = obj2[0][0]
        _msg("  2: Result '{0}' ({1})".format(s2.Shape.ShapeType, s2.TypeId))
        self.assertTrue(bool(obj2[0]), "'{}' failed".format(operation))

        obj3 = Draft.upgrade(obj2[0], delete=True)
        App.ActiveDocument.recompute()
        s3 = obj3[0][0]
        _msg("  3: Result '{0}' ({1})".format(s3.Shape.ShapeType, s3.TypeId))
        self.assertTrue(bool(obj3[0]), "'{}' failed".format(operation))

        # when draftify, upgrade dont return a new object
        Draft.upgrade(obj3[0], delete=True)
        App.ActiveDocument.recompute()
        wire = App.ActiveDocument.Wire
        _msg("  4: Result '{0}' ({1})".format(wire.Proxy.Type, wire.TypeId))
        self.assertTrue(bool(wire), "'{}' failed".format(operation))

        obj4 = Draft.upgrade(wire, delete=True)
        App.ActiveDocument.recompute()
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
        wire = Draft.make_wire([a, b, c, a])
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
        wire = Draft.make_wire([a, b, c])

        obj = Draft.make_bspline(wire.Points)
        App.ActiveDocument.recompute()
        _msg("  1: Result '{0}' ({1})".format(obj.Proxy.Type, obj.TypeId))
        self.assertTrue(obj, "'{}' failed".format(operation))

        obj2 = Draft.make_wire(obj.Points)
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
        obj = Draft.make_shape2dview(prism, direction)
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
        wire = Draft.make_wire([a, b, c])
        App.ActiveDocument.recompute()

        obj = Draft.make_sketch(wire, autoconstraints=True)
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
        operation = "Draft OrthoArray"
        _msg("  Test '{}'".format(operation))
        length = 4
        width = 2
        _msg("  Rectangle")
        _msg("  length={0}, width={1}".format(length, width))
        rect = Draft.make_rectangle(length, width)
        App.ActiveDocument.recompute()

        dir_x = Vector(5, 0, 0)
        dir_y = Vector(0, 4, 0)
        dir_z = Vector(0, 0, 6)
        number_x = 3
        number_y = 4
        number_z = 6
        _msg("  Array")
        _msg("  direction_x={}".format(dir_x))
        _msg("  direction_y={}".format(dir_y))
        _msg("  direction_z={}".format(dir_z))
        _msg("  number_x={0}, number_y={1}, number_z={2}".format(number_x,
                                                                 number_y,
                                                                 number_z))
        obj = Draft.make_ortho_array(rect,
                                     dir_x, dir_y, dir_z,
                                     number_x, number_y, number_z)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_polar_array(self):
        """Create a rectangle, and a polar array."""
        operation = "Draft PolarArray"
        _msg("  Test '{}'".format(operation))
        length = 4
        width = 2
        _msg("  Rectangle")
        _msg("  length={0}, width={1}".format(length, width))
        rect = Draft.make_rectangle(length, width)
        App.ActiveDocument.recompute()

        center = Vector(-4, 0, 0)
        angle = 180
        number = 5
        _msg("  Array")
        _msg("  number={0}, polar_angle={1}".format(number, angle))
        _msg("  center={}".format(center))
        obj = Draft.make_polar_array(rect,
                                     number, angle, center)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_circular_array(self):
        """Create a rectangle, and a circular array."""
        operation = "Draft CircularArray"
        _msg("  Test '{}'".format(operation))
        length = 4
        width = 2
        _msg("  Rectangle")
        _msg("  length={0}, width={1}".format(length, width))
        rect = Draft.make_rectangle(length, width)
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
        _msg("  number={0}, symmetry={1}".format(number, symmetry))
        _msg("  axis={}".format(axis))
        _msg("  center={}".format(center))
        obj = Draft.make_circular_array(rect,
                                        rad_distance, tan_distance,
                                        number, symmetry,
                                        axis, center)
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
        wire = Draft.make_wire([a, b, c, d])

        n_faces = 3
        radius = 1
        _msg("  Polygon")
        _msg("  n_faces={0}, radius={1}".format(n_faces, radius))
        poly = Draft.make_polygon(n_faces, radius)

        number = 4
        translation = Vector(0, 1, 0)
        subelements = "Edge1"
        align = False
        _msg("  Path Array")
        _msg("  number={}, translation={}".format(number, translation))
        _msg("  subelements={}, align={}".format(subelements, align))
        obj = Draft.make_path_array(poly, wire, number,
                                    translation, subelements, align)
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
        points = [Draft.make_point(a),
                  Draft.make_point(b),
                  Draft.make_point(c),
                  Draft.make_point(d)]

        _msg("  Upgrade")
        add, delete = Draft.upgrade(points)
        compound = add[0]

        n_faces = 3
        radius = 1
        _msg("  Polygon")
        _msg("  n_faces={0}, radius={1}".format(n_faces, radius))
        poly = Draft.make_polygon(n_faces, radius)

        _msg("  Point Array")
        obj = Draft.make_point_array(poly, compound)
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

        obj = Draft.make_clone(box)
        _msg("  clone: '{0}' ({1})".format(obj.Proxy.Type, obj.TypeId))
        self.assertTrue(obj, "'{}' failed".format(operation))
        self.assertTrue(obj.hasExtension("Part::AttachExtension"),
                        "'{}' failed".format(operation))

    def test_attached_clone_behavior(self):
        """Check if an attached clone behaves correctly.

        Test for https://github.com/FreeCAD/FreeCAD/issues/8771.
        """
        operation = "Check attached Draft Clone behavior"
        _msg("  Test '{}'".format(operation))

        box1 = App.ActiveDocument.addObject("Part::Box")
        box1.Length = 10
        box2 = App.ActiveDocument.addObject("Part::Box")
        App.ActiveDocument.recompute()

        obj = Draft.make_clone(box1)
        obj.MapMode = "ObjectXY"
        obj.Support = [(box2, ("",))]
        App.ActiveDocument.recompute()

        box1.Length = 1
        App.ActiveDocument.recompute()

        self.assertTrue(obj.Shape.BoundBox.XLength == 1, "'{}' failed".format(operation))

    def test_draft_to_techdraw(self):
        """Create a solid, and then a DraftView on a TechDraw page."""
        operation = "TechDraw DraftView (relies on Draft code)"
        _msg("  Test '{}'".format(operation))
        prism = App.ActiveDocument.addObject("Part::Prism")
        prism.Polygon = 5
        # Rotate the prism 45 degrees around the Y axis
        prism.Placement.Rotation.Axis = Vector(0, 1, 0)
        prism.Placement.Rotation.Angle = 45 * (3.14159/180)
        _msg("  Prism")
        _msg("  n_sides={}".format(prism.Polygon))
        _msg("  placement={}".format(prism.Placement))

        page = App.ActiveDocument.addObject("TechDraw::DrawPage")
        _msg("  page={}".format(page.TypeId))
        template = App.ActiveDocument.addObject("TechDraw::DrawSVGTemplate")
        template.Template = App.getResourceDir() \
                            + "Mod/TechDraw/Templates/A3_Landscape_blank.svg"
        page.Template = template
        _msg("  template={}".format(template.TypeId))
        view = App.ActiveDocument.addObject("TechDraw::DrawViewDraft")
        view.Source = prism
        view.Direction = App.Vector(0, 0, 1)
        page.addView(view)
        _msg("  view={}".format(view.TypeId))
        self.assertTrue(view, "'{}' failed".format(operation))
        self.assertTrue(view in page.OutList, "'{}' failed".format(operation))

    def test_mirror(self):
        """Create a rectangle, then a mirrored shape."""
        operation = "Draft Mirror"
        _msg("  Test '{}'".format(operation))
        length = 4
        width = 2
        _msg("  Rectangle")
        _msg("  length={0}, width={1}".format(length, width))
        rect = Draft.make_rectangle(length, width)
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
        line = Draft.make_line(a, b)
        direction = Vector(4, 1, 0)

        Draft.stretch = aux.fake_function
        obj = Draft.stretch(line, direction)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_scale_part_feature_arcs(self):
        """Create and scale a part feature (arcs)."""
        operation = "Draft Scale part feature (arcs)"
        _msg("  Test '{}'".format(operation))

        base = Vector(3.5, 2.5, 0.0)
        cen = Vector(2.0, 1.0, 0.0) # center for scaling
        sca = Vector(2.0, 3.0, 1.0)
        ends = [Vector(0.0, 0.0, 0.0),
                Vector(4.0, 0.0, 0.0),
                Vector(4.0, 3.0, 0.0),
                Vector(0.0, 3.0, 0.0)]
        mids = [Vector( 2.0, -0.5, 0.0),
                Vector( 4.5,  1.5, 0.0),
                Vector( 2.0,  3.5, 0.0),
                Vector(-0.5,  1.5, 0.0)] # arc midpoints

        shp = Part.Shape([Part.Arc(ends[0], mids[0], ends[1]),
                          Part.Arc(ends[1], mids[1], ends[2]),
                          Part.Arc(ends[2], mids[2], ends[3]),
                          Part.Arc(ends[3], mids[3], ends[0])])
        obj = App.ActiveDocument.addObject("Part::Feature")
        obj.Shape = shp
        obj.Placement.Base = base
        App.ActiveDocument.recompute()
        Draft.scale([obj], sca, cen, False)
        App.ActiveDocument.recompute()

        # check endpoints of arcs:
        newEnds = [Vector( 5.0,  5.5, 0.0),
                   Vector(13.0,  5.5, 0.0),
                   Vector(13.0, 14.5, 0.0),
                   Vector( 5.0, 14.5, 0.0)]
        vrts = obj.Shape.Vertexes
        for i in range(4):
            self.assertTrue(vrts[i].Point.isEqual(newEnds[i], 1e-6),
                            "'{}' failed".format(operation))
        # check midpoints of arcs:
        newMids = [Vector( 9.0,  4.0, 0.0),
                   Vector(14.0, 10.0, 0.0),
                   Vector( 9.0, 16.0, 0.0),
                   Vector( 4.0, 10.0, 0.0)]
        for i in range(4):
            edge = obj.Shape.Edges[i]
            par = (edge.LastParameter - edge.FirstParameter) / 2.0
            self.assertTrue(edge.valueAt(par).isEqual(newMids[i], 1e-6),
                            "'{}' failed".format(operation))

    def test_scale_part_feature_lines(self):
        """Create and scale a part feature (lines)."""
        operation = "Draft Scale part feature (lines)"
        _msg("  Test '{}'".format(operation))

        base = Vector(3.5, 2.5, 0.0)
        cen = Vector(2.0, 1.0, 0.0) # center for scaling
        sca = Vector(2.0, 3.0, 1.0)
        pts = [Vector(0.0, 0.0, 0.0),
               Vector(4.0, 0.0, 0.0),
               Vector(4.0, 3.0, 0.0),
               Vector(0.0, 3.0, 0.0)]

        shp = Part.Shape([Part.LineSegment(pts[0], pts[1]),
                          Part.LineSegment(pts[1], pts[2]),
                          Part.LineSegment(pts[2], pts[3]),
                          Part.LineSegment(pts[3], pts[0])])
        obj = App.ActiveDocument.addObject("Part::Feature")
        obj.Shape = shp
        obj.Placement.Base = base
        App.ActiveDocument.recompute()
        Draft.scale([obj], sca, cen, False)
        App.ActiveDocument.recompute()

        newPts = [Vector( 5.0,  5.5, 0.0),
                  Vector(13.0,  5.5, 0.0),
                  Vector(13.0, 14.5, 0.0),
                  Vector( 5.0, 14.5, 0.0)]
        vrts = obj.Shape.Vertexes
        for i in range(4):
            self.assertTrue(vrts[i].Point.isEqual(newPts[i], 1e-6),
                            "'{}' failed".format(operation))

    def test_scale_rectangle(self):
        """Create and scale a rectangle."""
        operation = "Draft Scale rectangle"
        _msg("  Test '{}'".format(operation))

        base = Vector(3.5, 2.5, 0.0)
        cen = Vector(2.0, 1.0, 0.0) # center for scaling
        sca = Vector(2.0, 3.0, 1.0)
        len = 4.0
        hgt = 3.0

        obj = Draft.make_rectangle(len, hgt)
        obj.Placement.Base = base
        App.ActiveDocument.recompute()
        Draft.scale([obj], sca, cen, False)
        App.ActiveDocument.recompute()

        newBase = Vector(5.0, 5.5, 0.0)
        newLen = 8.0
        newHgt = 9.0
        self.assertTrue(obj.Placement.Base.isEqual(newBase, 1e-6),
                        "'{}' failed".format(operation))
        self.assertAlmostEqual(obj.Length,
                               newLen,
                               delta = 1e-6,
                               msg = "'{}' failed".format(operation))
        self.assertAlmostEqual(obj.Height,
                               newHgt,
                               delta = 1e-6,
                               msg = "'{}' failed".format(operation))

    def test_scale_spline(self):
        """Create and scale a spline."""
        operation = "Draft Scale spline"
        _msg("  Test '{}'".format(operation))

        base = Vector(3.5, 2.5, 0.0)
        cen = Vector(2.0, 1.0, 0.0) # center for scaling
        sca = Vector(2.0, 3.0, 1.0)
        pts = [Vector(0.0, 0.0, 0.0),
               Vector(2.0, 3.0, 0.0),
               Vector(4.0, 0.0, 0.0)]

        obj = Draft.make_bspline(pts, False)
        obj.Placement.Base = base
        App.ActiveDocument.recompute()
        Draft.scale([obj], sca, cen, False)
        App.ActiveDocument.recompute()

        newPts = [Vector( 5.0,  5.5, 0.0),
                  Vector( 9.0, 14.5, 0.0),
                  Vector(13.0,  5.5, 0.0)]
        for i in range(3):
            self.assertTrue(obj.Points[i].add(base).isEqual(newPts[i], 1e-6),
                            "'{}' failed".format(operation))

    def test_scale_wire(self):
        """Create and scale a wire."""
        operation = "Draft Scale wire"
        _msg("  Test '{}'".format(operation))

        base = Vector(3.5, 2.5, 0.0)
        cen = Vector(2.0, 1.0, 0.0) # center for scaling
        sca = Vector(2.0, 3.0, 1.0)
        pts = [Vector(0.0, 0.0, 0.0),
               Vector(4.0, 0.0, 0.0),
               Vector(4.0, 3.0, 0.0),
               Vector(0.0, 3.0, 0.0)]

        obj = Draft.make_wire(pts, True)
        obj.Placement.Base = base
        App.ActiveDocument.recompute()
        Draft.scale([obj], sca, cen, False)
        App.ActiveDocument.recompute()

        newPts = [Vector( 5.0,  5.5, 0.0),
                  Vector(13.0,  5.5, 0.0),
                  Vector(13.0, 14.5, 0.0),
                  Vector( 5.0, 14.5, 0.0)]
        vrts = obj.Shape.Vertexes
        for i in range(4):
            self.assertTrue(vrts[i].Point.isEqual(newPts[i], 1e-6),
                            "'{}' failed".format(operation))

    def tearDown(self):
        """Finish the test.

        This is executed after each test, so we close the document.
        """
        App.closeDocument(self.doc_name)

## @}
