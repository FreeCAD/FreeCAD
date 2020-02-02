"""Run this file to create a standard test document for Draft objects.

Use as input to the freecad executable.
    freecad draft_test_objects.py

Or load it as a module and use the defined function.
    import drafttests.draft_test_objects as dt
    dt.create_test_file()
"""
# ***************************************************************************
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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
import os
import datetime
import math
import FreeCAD as App
from FreeCAD import Vector
import Draft
from draftutils.messages import _msg, _wrn
import draftobjects.arc_3points

if App.GuiUp:
    import DraftFillet
    import FreeCADGui as Gui


def _set_text(obj):
    """Set properties of text object."""
    if App.GuiUp:
        obj.ViewObject.FontSize = 75


def create_frame():
    """Draw a frame with information on the version of the software.

    It includes the date created, the version, the release type,
    and the branch.
    """
    version = App.Version()
    now = datetime.datetime.now().strftime("%Y/%m/%dT%H:%M:%S")

    record = Draft.makeText(["Draft test file",
                             "Created: {}".format(now),
                             "\n",
                             "Version: " + ".".join(version[0:3]),
                             "Release: " + " ".join(version[3:5]),
                             "Branch: " + " ".join(version[5:])],
                            Vector(0, -1000, 0))

    if App.GuiUp:
        record.ViewObject.FontSize = 400

    frame = Draft.makeRectangle(21000, 12000)
    frame.Placement.Base = Vector(-1000, -3500)


def create_test_file(file_name="draft_test_objects",
                     font_file="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                     file_path="",
                     save=False):
    """Create a complete test file of Draft objects.

    It draws a frame with information on the software used to create
    the test document, and fills it with every object that can be created.

    Parameters
    ----------
    file_name: str, optional
        It defaults to `'draft_test_objects'`.
        It is the name of document that is created.

    font_file: str, optional
        It defaults to `"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"`.
        It is the full path of a font in the system to be used
        with `Draft_ShapeString`.
        If the font is not found, this object is not created.

    file_path: str, optional
        It defaults to the empty string `''`, in which case,
        it will use the value returned by `App.getUserAppDataDir()`,
        for example, `'/home/user/.FreeCAD/'`.

        The `file_name` will be appended to `file_path`
        to determine the actual path to save. The extension `.FCStd`
        will be added automatically.

    save: bool, optional
        It defaults to `False`. If it is `True` the new document
        will be saved to disk after creating all objects.
    """
    doc = App.newDocument(file_name)
    _msg(16 * "-")
    _msg("Filename: {}".format(file_name))
    _msg("If the units tests fail, this script may fail as well")

    create_frame()

    # Line, wire, and fillet
    _msg(16 * "-")
    _msg("Line")
    Draft.makeLine(Vector(0, 0, 0), Vector(500, 500, 0))
    t_xpos = -50
    t_ypos = -200
    _t = Draft.makeText(["Line"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    _msg(16 * "-")
    _msg("Wire")
    Draft.makeWire([Vector(500, 0, 0),
                    Vector(1000, 500, 0),
                    Vector(1000, 1000, 0)])
    t_xpos += 500
    _t = Draft.makeText(["Wire"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    _msg(16 * "-")
    _msg("Fillet")
    line_h_1 = Draft.makeLine(Vector(1500, 0, 0), Vector(1500, 500, 0))
    line_h_2 = Draft.makeLine(Vector(1500, 500, 0), Vector(2000, 500, 0))
    if App.GuiUp:
        line_h_1.ViewObject.DrawStyle = "Dotted"
        line_h_2.ViewObject.DrawStyle = "Dotted"
    App.ActiveDocument.recompute()

    try:
        DraftFillet.makeFillet([line_h_1, line_h_2], 400)
    except Exception:
        _wrn("Fillet could not be created")
        _wrn("Possible cause: at this moment it may need the interface")
        rect = Draft.makeRectangle(500, 100)
        rect.Placement.Base = Vector(14000, 500)

    t_xpos += 900
    _t = Draft.makeText(["Fillet"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Circle, arc, arc by 3 points
    _msg(16 * "-")
    _msg("Circle")
    circle = Draft.makeCircle(350)
    circle.Placement.Base = Vector(2500, 500, 0)
    t_xpos += 1050
    _t = Draft.makeText(["Circle"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    _msg(16 * "-")
    _msg("Circular arc")
    arc = Draft.makeCircle(350, startangle=0, endangle=100)
    arc.Placement.Base = Vector(3200, 500, 0)
    t_xpos += 800
    _t = Draft.makeText(["Circular arc"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    _msg(16 * "-")
    _msg("Circular arc 3 points")
    draftobjects.arc_3points.make_arc_3points([Vector(4600, 0, 0),
                                               Vector(4600, 800, 0),
                                               Vector(4000, 1000, 0)])
    t_xpos += 600
    _t = Draft.makeText(["Circular arc 3 points"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Ellipse, polygon, rectangle
    _msg(16 * "-")
    _msg("Ellipse")
    ellipse = Draft.makeEllipse(500, 300)
    ellipse.Placement.Base = Vector(5500, 250, 0)
    t_xpos += 1600
    _t = Draft.makeText(["Ellipse"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    _msg(16 * "-")
    _msg("Polygon")
    polygon = Draft.makePolygon(5, 250)
    polygon.Placement.Base = Vector(6500, 500, 0)
    t_xpos += 950
    _t = Draft.makeText(["Polygon"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    _msg(16 * "-")
    _msg("Rectangle")
    rectangle = Draft.makeRectangle(500, 1000, 0)
    rectangle.Placement.Base = Vector(7000, 0, 0)
    t_xpos += 650
    _t = Draft.makeText(["Rectangle"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Text
    _msg(16 * "-")
    _msg("Text")
    text = Draft.makeText(["Testing"], Vector(7700, 500, 0))
    if App.GuiUp:
        text.ViewObject.FontSize = 100
    t_xpos += 700
    _t = Draft.makeText(["Text"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Linear dimension
    _msg(16 * "-")
    _msg("Linear dimension")
    dimension = Draft.makeDimension(Vector(8500, 500, 0),
                                    Vector(8500, 1000, 0),
                                    Vector(9000, 750, 0))
    if App.GuiUp:
        dimension.ViewObject.ArrowSize = 15
        dimension.ViewObject.ExtLines = 1000
        dimension.ViewObject.ExtOvershoot = 100
        dimension.ViewObject.FontSize = 100
        dimension.ViewObject.ShowUnit = False
    t_xpos += 680
    _t = Draft.makeText(["Dimension"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Radius and diameter dimension
    _msg(16 * "-")
    _msg("Radius and diameter dimension")
    arc_h = Draft.makeCircle(500, startangle=0, endangle=90)
    arc_h.Placement.Base = Vector(9500, 0, 0)
    App.ActiveDocument.recompute()

    dimension_r = Draft.makeDimension(arc_h, 0, "radius",
                                      Vector(9750, 200, 0))
    if App.GuiUp:
        dimension_r.ViewObject.ArrowSize = 15
        dimension_r.ViewObject.FontSize = 100
        dimension_r.ViewObject.ShowUnit = False

    arc_h2 = Draft.makeCircle(450, startangle=-120, endangle=80)
    arc_h2.Placement.Base = Vector(10000, 1000, 0)
    App.ActiveDocument.recompute()

    dimension_d = Draft.makeDimension(arc_h2, 0, "diameter",
                                      Vector(10750, 900, 0))
    if App.GuiUp:
        dimension_d.ViewObject.ArrowSize = 15
        dimension_d.ViewObject.FontSize = 100
        dimension_d.ViewObject.ShowUnit = False
    t_xpos += 950
    _t = Draft.makeText(["Radius dimension",
                         "Diameter dimension"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Angular dimension
    _msg(16 * "-")
    _msg("Angular dimension")
    Draft.makeLine(Vector(10500, 300, 0), Vector(11500, 1000, 0))
    Draft.makeLine(Vector(10500, 300, 0), Vector(11500, 0, 0))
    angle1 = math.radians(40)
    angle2 = math.radians(-20)
    dimension_a = Draft.makeAngularDimension(Vector(10500, 300, 0),
                                             [angle1, angle2],
                                             Vector(11500, 300, 0))
    if App.GuiUp:
        dimension_a.ViewObject.ArrowSize = 15
        dimension_a.ViewObject.FontSize = 100
    t_xpos += 1700
    _t = Draft.makeText(["Angle dimension"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # BSpline
    _msg(16 * "-")
    _msg("BSpline")
    Draft.makeBSpline([Vector(12500, 0, 0),
                       Vector(12500, 500, 0),
                       Vector(13000, 500, 0),
                       Vector(13000, 1000, 0)])
    t_xpos += 1500
    _t = Draft.makeText(["BSpline"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Point
    _msg(16 * "-")
    _msg("Point")
    point = Draft.makePoint(13500, 500, 0)
    if App.GuiUp:
        point.ViewObject.PointSize = 10
    t_xpos += 900
    _t = Draft.makeText(["Point"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Shapestring
    _msg(16 * "-")
    _msg("Shapestring")
    try:
        shape_string = Draft.makeShapeString("Testing",
                                             font_file,
                                             100)
        shape_string.Placement.Base = Vector(14000, 500)
    except Exception:
        _wrn("Shapestring could not be created")
        _wrn("Possible cause: the font file may not exist")
        _wrn(font_file)
        rect = Draft.makeRectangle(500, 100)
        rect.Placement.Base = Vector(14000, 500)
    t_xpos += 600
    _t = Draft.makeText(["Shapestring"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Facebinder
    _msg(16 * "-")
    _msg("Facebinder")
    box = App.ActiveDocument.addObject("Part::Box", "Cube")
    box.Length = 200
    box.Width = 500
    box.Height = 100
    box.Placement.Base = Vector(15000, 0, 0)
    if App.GuiUp:
        box.ViewObject.Visibility = False

    facebinder = Draft.makeFacebinder([(box, ("Face1", "Face3", "Face6"))])
    facebinder.Extrusion = 10
    t_xpos += 780
    _t = Draft.makeText(["Facebinder"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Cubic bezier curve, n-degree bezier curve
    _msg(16 * "-")
    _msg("Cubic bezier")
    Draft.makeBezCurve([Vector(15500, 0, 0),
                        Vector(15578, 485, 0),
                        Vector(15879, 154, 0),
                        Vector(15975, 400, 0),
                        Vector(16070, 668, 0),
                        Vector(16423, 925, 0),
                        Vector(16500, 500, 0)], degree=3)
    t_xpos += 680
    _t = Draft.makeText(["Cubic bezier"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    _msg(16 * "-")
    _msg("N-degree bezier")
    Draft.makeBezCurve([Vector(16500, 0, 0),
                        Vector(17000, 500, 0),
                        Vector(17500, 500, 0),
                        Vector(17500, 1000, 0),
                        Vector(17000, 1000, 0),
                        Vector(17063, 1256, 0),
                        Vector(17732, 1227, 0),
                        Vector(17790, 720, 0),
                        Vector(17702, 242, 0)])
    t_xpos += 1200
    _t = Draft.makeText(["n-Bezier"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Label
    _msg(16 * "-")
    _msg("Label")
    place = App.Placement(Vector(18500, 500, 0), App.Rotation())
    label = Draft.makeLabel(targetpoint=Vector(18000, 0, 0),
                            distance=-250,
                            placement=place)
    label.Text = "Testing"
    if App.GuiUp:
        label.ViewObject.ArrowSize = 15
        label.ViewObject.TextSize = 100
    App.ActiveDocument.recompute()
    t_xpos += 1200
    _t = Draft.makeText(["Label"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Orthogonal array and orthogonal link array
    _msg(16 * "-")
    _msg("Orthogonal array")
    rect_h = Draft.makeRectangle(500, 500)
    rect_h.Placement.Base = Vector(1500, 2500, 0)
    if App.GuiUp:
        rect_h.ViewObject.Visibility = False

    Draft.makeArray(rect_h,
                    Vector(600, 0, 0),
                    Vector(0, 600, 0),
                    Vector(0, 0, 0),
                    3, 2, 1)
    t_xpos = 1700
    t_ypos = 2200
    _t = Draft.makeText(["Array"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    rect_h_2 = Draft.makeRectangle(500, 100)
    rect_h_2.Placement.Base = Vector(1500, 5000, 0)
    if App.GuiUp:
        rect_h_2.ViewObject.Visibility = False

    _msg(16 * "-")
    _msg("Orthogonal link array")
    Draft.makeArray(rect_h_2,
                    Vector(800, 0, 0),
                    Vector(0, 500, 0),
                    Vector(0, 0, 0),
                    2, 4, 1,
                    use_link=True)
    t_ypos += 2600
    _t = Draft.makeText(["Link array"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Polar array and polar link array
    _msg(16 * "-")
    _msg("Polar array")
    wire_h = Draft.makeWire([Vector(5500, 3000, 0),
                             Vector(6000, 3500, 0),
                             Vector(6000, 3200, 0),
                             Vector(5800, 3200, 0)])
    if App.GuiUp:
        wire_h.ViewObject.Visibility = False

    Draft.makeArray(wire_h,
                    Vector(5000, 3000, 0),
                    200,
                    8)
    t_xpos = 4600
    t_ypos = 2200
    _t = Draft.makeText(["Polar array"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    _msg(16 * "-")
    _msg("Polar link array")
    wire_h_2 = Draft.makeWire([Vector(5500, 6000, 0),
                               Vector(6000, 6000, 0),
                               Vector(5800, 5700, 0),
                               Vector(5800, 5750, 0)])
    if App.GuiUp:
        wire_h_2.ViewObject.Visibility = False

    Draft.makeArray(wire_h_2,
                    Vector(5000, 6000, 0),
                    200,
                    8,
                    use_link=True)
    t_ypos += 3200
    _t = Draft.makeText(["Polar link array"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Circular array and circular link array
    _msg(16 * "-")
    _msg("Circular array")
    poly_h = Draft.makePolygon(5, 200)
    poly_h.Placement.Base = Vector(8000, 3000, 0)
    if App.GuiUp:
        poly_h.ViewObject.Visibility = False

    Draft.makeArray(poly_h,
                    500, 600,
                    Vector(0, 0, 1),
                    Vector(0, 0, 0),
                    3,
                    1)
    t_xpos = 7700
    t_ypos = 1700
    _t = Draft.makeText(["Circular array"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    _msg(16 * "-")
    _msg("Circular link array")
    poly_h_2 = Draft.makePolygon(6, 150)
    poly_h_2.Placement.Base = Vector(8000, 6250, 0)
    if App.GuiUp:
        poly_h_2.ViewObject.Visibility = False

    Draft.makeArray(poly_h_2,
                    550, 450,
                    Vector(0, 0, 1),
                    Vector(0, 0, 0),
                    3,
                    1,
                    use_link=True)
    t_ypos += 3100
    _t = Draft.makeText(["Circular link array"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Path array and path link array
    _msg(16 * "-")
    _msg("Path array")
    poly_h = Draft.makePolygon(3, 250)
    poly_h.Placement.Base = Vector(10500, 3000, 0)
    if App.GuiUp:
        poly_h.ViewObject.Visibility = False

    bspline_path = Draft.makeBSpline([Vector(10500, 2500, 0),
                                      Vector(11000, 3000, 0),
                                      Vector(11500, 3200, 0),
                                      Vector(12000, 4000, 0)])

    Draft.makePathArray(poly_h, bspline_path, 5)
    t_xpos = 10400
    t_ypos = 2200
    _t = Draft.makeText(["Path array"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    _msg(16 * "-")
    _msg("Path link array")
    poly_h_2 = Draft.makePolygon(4, 200)
    poly_h_2.Placement.Base = Vector(10500, 5000, 0)
    if App.GuiUp:
        poly_h_2.ViewObject.Visibility = False

    bspline_path_2 = Draft.makeBSpline([Vector(10500, 4500, 0),
                                        Vector(11000, 6800, 0),
                                        Vector(11500, 6000, 0),
                                        Vector(12000, 5200, 0)])

    Draft.makePathArray(poly_h_2, bspline_path_2, 6,
                        use_link=True)
    t_ypos += 2000
    _t = Draft.makeText(["Path link array"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Point array
    _msg(16 * "-")
    _msg("Point array")
    poly_h = Draft.makePolygon(3, 250)

    point_1 = Draft.makePoint(13000, 3000, 0)
    point_2 = Draft.makePoint(13000, 3500, 0)
    point_3 = Draft.makePoint(14000, 2500, 0)
    point_4 = Draft.makePoint(14000, 3000, 0)

    add_list, delete_list = Draft.upgrade([point_1, point_2,
                                           point_3, point_4])
    compound = add_list[0]
    if App.GuiUp:
        compound.ViewObject.PointSize = 5

    Draft.makePointArray(poly_h, compound)
    t_xpos = 13000
    t_ypos = 2200
    _t = Draft.makeText(["Point array"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    # Clone and mirror
    _msg(16 * "-")
    _msg("Clone")
    wire_h = Draft.makeWire([Vector(15000, 2500, 0),
                             Vector(15200, 3000, 0),
                             Vector(15500, 2500, 0),
                             Vector(15200, 2300, 0)])

    Draft.clone(wire_h, Vector(0, 1000, 0))
    t_xpos = 15000
    t_ypos = 2100
    _t = Draft.makeText(["Clone"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    _msg(16 * "-")
    _msg("Mirror")
    wire_h = Draft.makeWire([Vector(17000, 2500, 0),
                             Vector(16500, 4000, 0),
                             Vector(16000, 2700, 0),
                             Vector(16500, 2500, 0),
                             Vector(16700, 2700, 0)])

    Draft.mirror(wire_h,
                 Vector(17100, 2000, 0),
                 Vector(17100, 4000, 0))
    t_xpos = 17000
    t_ypos = 2200
    _t = Draft.makeText(["Mirror"], Vector(t_xpos, t_ypos, 0))
    _set_text(_t)

    App.ActiveDocument.recompute()

    if App.GuiUp:
        Gui.runCommand("Std_ViewFitAll")

    # Export
    if not file_path:
        file_path = App.getUserAppDataDir()
    out_name = os.path.join(file_path, file_name + ".FCStd")
    doc.FileName = out_name
    if save:
        doc.save()
        _msg(16 * "-")
        _msg("Saved: {}".format(out_name))

    return doc


if __name__ == "__main__":
    create_test_file()
