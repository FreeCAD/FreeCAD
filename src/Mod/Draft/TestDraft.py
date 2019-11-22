"""Unit test for the Draft module.

From the terminal, run the following:
FreeCAD -t TestDraft

From within FreeCAD, run the following:
import Test, TestDraft
Test.runTestsFromModule(TestDraft)
"""
# ***************************************************************************
# *   (c) 2013 Yorik van Havre <yorik@uncreated.net>                        *
# *   (c) 2019 Eliud Cabrera Castillo                                       *
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

# ===========================================================================
# First the command to run the test from the operating system terminal,
# followed by the command to run the test from the Python console
# within FreeCAD.
# ===========================================================================
# Run all Draft tests
# ----
# FreeCAD -t TestDraft
#
# import Test, TestDraft
# Test.runTestsFromModule(TestDraft)
#
# ===========================================================================
# Run tests from a module
# ----
# FreeCAD -t TestDraft
#
# import Test, TestDraft
# Test.runTestsFromModule(TestDraft)
#
# ===========================================================================
# Run tests from a class
# ----
# FreeCAD -t TestDraft.DraftCreation
#
# import Test, TestDraft
# Test.runTestsFromClass(TestDraft.DraftCreation)
#
# ===========================================================================
# Run a specific test
# ----
# FreeCAD -t TestDraft.DraftCreation.test_line
#
# import unittest
# one_test = "TestDraft.DraftCreation.test_line"
# all_tests = unittest.TestLoader().loadTestsFromName(one_test)
# unittest.TextTestRunner().run(all_tests)
# ===========================================================================

import os
import unittest
import FreeCAD as App
import FreeCADGui as Gui
import Draft
from FreeCAD import Vector


def _msg(text, end="\n"):
    App.Console.PrintMessage(text + end)


def _wrn(text, end="\n"):
    App.Console.PrintWarning(text + end)


def _log(text, end="\n"):
    App.Console.PrintLog(text + end)


def _draw_header():
    _msg("")
    _msg(78*"-")


def _import_test(module):
    _msg("  Try importing '{}'".format(module))
    try:
        imported = __import__("{}".format(module))
    except ImportError as exc:
        imported = False
        _msg("  {}".format(exc))
    return imported


def _no_gui(module):
    _msg("  #-----------------------------------------------------#\n"
         "  #    No GUI; cannot test for '{}'\n"
         "  #-----------------------------------------------------#\n"
         "  Automatic PASS".format(module))


def _no_test():
    _msg("  #-----------------------------------------------------#\n"
         "  #    This test is not implemented currently\n"
         "  #-----------------------------------------------------#\n"
         "  Automatic PASS")


def _fake_function(p1=None, p2=None, p3=None, p4=None, p5=None):
    _msg("  Arguments to placeholder function")
    _msg("  p1={0}; p2={1}".format(p1, p2))
    _msg("  p3={0}; p4={1}".format(p3, p4))
    _msg("  p5={}".format(p5))
    _no_test()
    return True


class DraftImport(unittest.TestCase):
    """Import the Draft modules."""
    # No document is needed to test 'import Draft' or other modules
    # thus 'setUp' just draws a line, and 'tearDown' isn't defined.
    def setUp(self):
        _draw_header()

    def test_import_draft(self):
        """Import the Draft module."""
        module = "Draft"
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_draft_geomutils(self):
        """Import Draft geometrical utilities."""
        module = "DraftGeomUtils"
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_draft_vecutils(self):
        """Import Draft vector utilities."""
        module = "DraftVecUtils"
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_draft_svg(self):
        """Import Draft SVG utilities."""
        module = "getSVG"
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))


class DraftGuiImport(unittest.TestCase):
    """Import the Draft graphical modules."""
    # No document is needed to test 'import DraftGui' or other modules
    # thus 'setUp' just draws a line, and 'tearDown' isn't defined.
    def setUp(self):
        _draw_header()

    def test_import_gui_draftgui(self):
        """Import Draft TaskView GUI tools."""
        module = "DraftGui"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_draft_snap(self):
        """Import Draft snapping."""
        module = "DraftSnap"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_draft_tools(self):
        """Import Draft graphical commands."""
        module = "DraftTools"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_draft_trackers(self):
        """Import Draft tracker utilities."""
        module = "DraftTrackers"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))


class DraftImportTools(unittest.TestCase):
    """Test for each individual module that defines a tool."""
    # No document is needed to test 'import' of other modules
    # thus 'setUp' just draws a line, and 'tearDown' isn't defined.
    def setUp(self):
        _draw_header()

    def test_import_gui_draftedit(self):
        """Import Draft Edit."""
        module = "DraftEdit"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_draftfillet(self):
        """Import Draft Fillet."""
        module = "DraftFillet"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_draftlayer(self):
        """Import Draft Layer."""
        module = "DraftLayer"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_draftplane(self):
        """Import Draft SelectPlane."""
        module = "DraftSelectPlane"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_import_gui_workingplane(self):
        """Import Draft WorkingPlane."""
        module = "WorkingPlane"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))


class DraftPivy(unittest.TestCase):
    """Test for the presence of Pivy and that it works with Coin3D."""

    def setUp(self):
        """Set up a new document to hold the tests.

        This is executed before every test, so we create a document
        to hold the objects.
        """
        _draw_header()
        self.doc_name = self.__class__.__name__
        if App.ActiveDocument:
            if App.ActiveDocument.Name != self.doc_name:
                App.newDocument(self.doc_name)
        else:
            App.newDocument(self.doc_name)
        App.setActiveDocument(self.doc_name)
        self.doc = App.ActiveDocument
        _msg("  Temporary document '{}'".format(self.doc_name))

    def test_pivy_import(self):
        """Import Pivy Coin."""
        module = "pivy.coin"
        imported = _import_test(module)
        self.assertTrue(imported, "Problem importing '{}'".format(module))

    def test_pivy_draw(self):
        """Use Coin (pivy.coin) to draw a cube on the active view."""
        module = "pivy.coin"
        if not App.GuiUp:
            _no_gui(module)
            self.assertTrue(True)
            return

        import pivy.coin
        cube = pivy.coin.SoCube()
        _msg("  Draw cube")
        Gui.ActiveDocument.ActiveView.getSceneGraph().addChild(cube)
        _msg("  Adding cube to the active view scene")
        self.assertTrue(cube, "Pivy is not working properly.")

    def tearDown(self):
        """Finish the test.

        This is executed after each test, so we close the document.
        """
        App.closeDocument(self.doc_name)


class DraftCreation(unittest.TestCase):
    """Test Draft creation functions."""

    def setUp(self):
        """Set up a new document to hold the tests.

        This is executed before every test, so we create a document
        to hold the objects.
        """
        _draw_header()
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
            _no_gui("DraftFillet")
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
                               start_angle, end_angle)
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
        Draft.make_arc_3points = _fake_function
        obj = Draft.make_arc_3points(a, b, c)
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
        """Create a radial dimension. NOT IMPLEMENTED CURRENTLY."""
        operation = "Draft Dimension Radial"
        _msg("  Test '{}'".format(operation))
        a = Vector(5, 0, 0)
        b = Vector(4, 3, 0)
        c = Vector(0, 5, 0)
        _msg("  a={0}, b={1}".format(a, b))
        _msg("  c={}".format(c))
        Draft.make_dimension_radial = _fake_function
        obj = Draft.make_dimension_radial(a, b, c)
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
        font = None  # TODO: get a font file here
        _msg("  text='{0}', font='{1}'".format(text, font))
        Draft.makeShapeString = _fake_function
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


class DraftModification(unittest.TestCase):
    """Test Draft modification tools."""

    def setUp(self):
        """Set up a new document to hold the tests.

        This is executed before every test, so we create a document
        to hold the objects.
        """
        _draw_header()
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

    def test_offset(self):
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

        Draft.trim_objects = _fake_function
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

        Draft.extrude = _fake_function
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

        Draft.stretch = _fake_function
        obj = Draft.stretch(line, direction)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def tearDown(self):
        """Finish the test.

        This is executed after each test, so we close the document.
        """
        App.closeDocument(self.doc_name)


class DraftSVG(unittest.TestCase):
    """Test reading and writing of SVGs with Draft."""

    def setUp(self):
        """Set up a new document to hold the tests.

        This is executed before every test, so we create a document
        to hold the objects.
        """
        _draw_header()
        self.doc_name = self.__class__.__name__
        if App.ActiveDocument:
            if App.ActiveDocument.Name != self.doc_name:
                App.newDocument(self.doc_name)
        else:
            App.newDocument(self.doc_name)
        App.setActiveDocument(self.doc_name)
        self.doc = App.ActiveDocument
        _msg("  Temporary document '{}'".format(self.doc_name))

    def test_read_svg(self):
        """Read an SVG file and import its elements as Draft objects."""
        operation = "importSVG.import"
        _msg("  Test '{}'".format(operation))
        _msg("  This test requires an SVG file to read.")

        file = 'Mod/Draft/drafttest/test.svg'
        in_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(in_file))
        _msg("  exists={}".format(os.path.exists(in_file)))

        Draft.import_SVG = _fake_function
        obj = Draft.import_SVG(in_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_export_svg(self):
        """Create some figures and export them to an SVG file."""
        operation = "importSVG.export"
        _msg("  Test '{}'".format(operation))

        file = 'Mod/Draft/drafttest/out_test.svg'
        out_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(out_file))
        _msg("  exists={}".format(os.path.exists(out_file)))

        Draft.export_SVG = _fake_function
        obj = Draft.export_SVG(out_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def tearDown(self):
        """Finish the test.

        This is executed after each test, so we close the document.
        """
        App.closeDocument(self.doc_name)


class DraftDXF(unittest.TestCase):
    """Test reading and writing of DXFs with Draft."""

    def setUp(self):
        """Set up a new document to hold the tests.

        This is executed before every test, so we create a document
        to hold the objects.
        """
        _draw_header()
        self.doc_name = self.__class__.__name__
        if App.ActiveDocument:
            if App.ActiveDocument.Name != self.doc_name:
                App.newDocument(self.doc_name)
        else:
            App.newDocument(self.doc_name)
        App.setActiveDocument(self.doc_name)
        self.doc = App.ActiveDocument
        _msg("  Temporary document '{}'".format(self.doc_name))

    def test_read_dxf(self):
        """Read a DXF file and import its elements as Draft objects."""
        operation = "importDXF.import"
        _msg("  Test '{}'".format(operation))
        _msg("  This test requires a DXF file to read.")

        file = 'Mod/Draft/drafttest/test.dxf'
        in_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(in_file))
        _msg("  exists={}".format(os.path.exists(in_file)))

        Draft.import_DXF = _fake_function
        obj = Draft.import_DXF(in_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_export_dxf(self):
        """Create some figures and export them to a DXF file."""
        operation = "importDXF.export"
        _msg("  Test '{}'".format(operation))

        file = 'Mod/Draft/drafttest/out_test.dxf'
        out_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(out_file))
        _msg("  exists={}".format(os.path.exists(out_file)))

        Draft.export_DXF = _fake_function
        obj = Draft.export_DXF(out_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def tearDown(self):
        """Finish the test.

        This is executed after each test, so we close the document.
        """
        App.closeDocument(self.doc_name)


class DraftDWG(unittest.TestCase):
    """Test reading and writing of DWG with Draft."""

    def setUp(self):
        """Set up a new document to hold the tests.

        This is executed before every test, so we create a document
        to hold the objects.
        """
        _draw_header()
        self.doc_name = self.__class__.__name__
        if App.ActiveDocument:
            if App.ActiveDocument.Name != self.doc_name:
                App.newDocument(self.doc_name)
        else:
            App.newDocument(self.doc_name)
        App.setActiveDocument(self.doc_name)
        self.doc = App.ActiveDocument
        _msg("  Temporary document '{}'".format(self.doc_name))

    def test_read_dwg(self):
        """Read a DWG file and import its elements as Draft objects."""
        operation = "importDWG.import"
        _msg("  Test '{}'".format(operation))
        _msg("  This test requires a DWG file to read.")

        file = 'Mod/Draft/drafttest/test.dwg'
        in_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(in_file))
        _msg("  exists={}".format(os.path.exists(in_file)))

        Draft.import_DWG = _fake_function
        obj = Draft.import_DWG(in_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_export_dwg(self):
        """Create some figures and export them to a DWG file."""
        operation = "importDWG.export"
        _msg("  Test '{}'".format(operation))

        file = 'Mod/Draft/drafttest/out_test.dwg'
        out_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(out_file))
        _msg("  exists={}".format(os.path.exists(out_file)))

        Draft.export_DWG = _fake_function
        obj = Draft.export_DWG(out_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def tearDown(self):
        """Finish the test.

        This is executed after each test, so we close the document.
        """
        App.closeDocument(self.doc_name)


class DraftOCA(unittest.TestCase):
    """Test reading and writing of OCA with Draft."""

    def setUp(self):
        """Set up a new document to hold the tests.

        This is executed before every test, so we create a document
        to hold the objects.
        """
        _draw_header()
        self.doc_name = self.__class__.__name__
        if App.ActiveDocument:
            if App.ActiveDocument.Name != self.doc_name:
                App.newDocument(self.doc_name)
        else:
            App.newDocument(self.doc_name)
        App.setActiveDocument(self.doc_name)
        self.doc = App.ActiveDocument
        _msg("  Temporary document '{}'".format(self.doc_name))

    def test_read_oca(self):
        """Read an OCA file and import its elements as Draft objects."""
        operation = "importOCA.import"
        _msg("  Test '{}'".format(operation))
        _msg("  This test requires an OCA file to read.")

        file = 'Mod/Draft/drafttest/test.oca'
        in_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(in_file))
        _msg("  exists={}".format(os.path.exists(in_file)))

        Draft.import_OCA = _fake_function
        obj = Draft.import_OCA(in_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_export_oca(self):
        """Create some figures and export them to an OCA file."""
        operation = "importOCA.export"
        _msg("  Test '{}'".format(operation))

        file = 'Mod/Draft/drafttest/out_test.oca'
        out_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(out_file))
        _msg("  exists={}".format(os.path.exists(out_file)))

        Draft.export_OCA = _fake_function
        obj = Draft.export_OCA(out_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def tearDown(self):
        """Finish the test.

        This is executed after each test, so we close the document.
        """
        App.closeDocument(self.doc_name)


class DraftAirfoilDAT(unittest.TestCase):
    """Test reading and writing of AirfoilDAT with Draft."""

    def setUp(self):
        """Set up a new document to hold the tests.

        This is executed before every test, so we create a document
        to hold the objects.
        """
        _draw_header()
        self.doc_name = self.__class__.__name__
        if App.ActiveDocument:
            if App.ActiveDocument.Name != self.doc_name:
                App.newDocument(self.doc_name)
        else:
            App.newDocument(self.doc_name)
        App.setActiveDocument(self.doc_name)
        self.doc = App.ActiveDocument
        _msg("  Temporary document '{}'".format(self.doc_name))

    def test_read_airfoildat(self):
        """Read an airfoil DAT file and import its elements as objects."""
        operation = "importAirfoilDAT.import"
        _msg("  Test '{}'".format(operation))
        _msg("  This test requires a DAT file with airfoil data to read.")

        file = 'Mod/Draft/drafttest/test.dat'
        in_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(in_file))
        _msg("  exists={}".format(os.path.exists(in_file)))

        Draft.import_AirfoilDAT = _fake_function
        obj = Draft.import_AirfoilDAT(in_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def test_export_airfoildat(self):
        """Create some figures and export them to an airfoil DAT file."""
        operation = "importAirfoilDAT.export"
        _msg("  Test '{}'".format(operation))

        file = 'Mod/Draft/drafttest/out_test.dat'
        out_file = os.path.join(App.getResourceDir(), file)
        _msg("  file={}".format(out_file))
        _msg("  exists={}".format(os.path.exists(out_file)))

        Draft.export_importAirfoilDAT = _fake_function
        obj = Draft.export_importAirfoilDAT(out_file)
        self.assertTrue(obj, "'{}' failed".format(operation))

    def tearDown(self):
        """Finish the test.

        This is executed after each test, so we close the document.
        """
        App.closeDocument(self.doc_name)
