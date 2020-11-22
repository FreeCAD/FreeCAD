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
"""Run this file to create a standard test document for Draft objects.

Use it as input to the program executable.

::

    freecad draft_test_objects.py

Or load it as a module and use the defined function.

>>> import drafttests.draft_test_objects as dt
>>> dt.create_test_file()
"""
## @package draft_test_objects
# \ingroup drafttests
# \brief Run this file to create a standard test document for Draft objects.

## \addtogroup drafttests
# @{
import datetime
import os

import FreeCAD as App
import Part
import Draft

from draftutils.messages import _msg, _wrn
from FreeCAD import Vector

if App.GuiUp:
    import FreeCADGui as Gui


def _set_text(text_list, position):
    """Set a text annotation with properties."""
    obj = App.ActiveDocument.addObject("App::Annotation", "Annotation")
    obj.LabelText = text_list
    obj.Position = position

    if App.GuiUp:
        obj.ViewObject.DisplayMode = "World"
        obj.ViewObject.FontSize = 75
        obj.ViewObject.TextColor = (0.0, 0.0, 0.0)


def _create_frame(doc=None):
    """Draw a frame with information on the version of the software.

    It includes the date created, the version, the release type,
    and the branch.

    Parameters
    ----------
    doc: App::Document, optional
        It defaults to `None`, which then defaults to the current
        active document, or creates a new document.
    """
    if not doc:
        doc = App.activeDocument()
    if not doc:
        doc = App.newDocument()

    version = App.Version()
    now = datetime.datetime.now().strftime("%Y/%m/%dT%H:%M:%S")

    _text = ["Draft test file",
             "Created: {}".format(now),
             "\n",
             "Version: " + ".".join(version[0:3]),
             "Release: " + " ".join(version[3:5]),
             "Branch: " + " ".join(version[5:])]

    record = doc.addObject("App::Annotation", "Description")
    record.LabelText = _text
    record.Position = Vector(0, -1000, 0)

    if App.GuiUp:
        record.ViewObject.DisplayMode = "World"
        record.ViewObject.FontSize = 400
        record.ViewObject.TextColor = (0.0, 0.0, 0.0)

    p1 = Vector(-1000, -3500, 0)
    p2 = Vector(20000, -3500, 0)
    p3 = Vector(20000, 8500, 0)
    p4 = Vector(-1000, 8500, 0)

    poly = Part.makePolygon([p1, p2, p3, p4, p1])
    frame = doc.addObject("Part::Feature", "Frame")
    frame.Shape = poly


def _create_objects(doc=None,
                    font_file="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"):
    """Create the objects of the test file.

    Parameters
    ----------
    doc: App::Document, optional
        It defaults to `None`, which then defaults to the current
        active document, or creates a new document.
    """
    if not doc:
        doc = App.activeDocument()
    if not doc:
        doc = App.newDocument()

    # Line, wire, and fillet
    _msg(16 * "-")
    _msg("Line")
    Draft.make_line(Vector(0, 0, 0), Vector(500, 500, 0))
    t_xpos = -50
    t_ypos = -200
    _set_text(["Line"], Vector(t_xpos, t_ypos, 0))

    _msg(16 * "-")
    _msg("Wire")
    Draft.make_wire([Vector(500, 0, 0),
                     Vector(1000, 500, 0),
                     Vector(1000, 1000, 0)])
    t_xpos += 500
    _set_text(["Wire"], Vector(t_xpos, t_ypos, 0))

    _msg(16 * "-")
    _msg("Fillet")
    line_h_1 = Draft.make_line(Vector(1500, 0, 0), Vector(1500, 500, 0))
    line_h_2 = Draft.make_line(Vector(1500, 500, 0), Vector(2000, 500, 0))
    if App.GuiUp:
        line_h_1.ViewObject.DrawStyle = "Dotted"
        line_h_2.ViewObject.DrawStyle = "Dotted"
    doc.recompute()

    Draft.make_fillet([line_h_1, line_h_2], 400)
    t_xpos += 900
    _set_text(["Fillet"], Vector(t_xpos, t_ypos, 0))

    # Circle, arc, arc by 3 points
    _msg(16 * "-")
    _msg("Circle")
    circle = Draft.make_circle(350)
    circle.Placement.Base = Vector(2500, 500, 0)
    t_xpos += 1050
    _set_text(["Circle"], Vector(t_xpos, t_ypos, 0))

    _msg(16 * "-")
    _msg("Circular arc")
    arc = Draft.make_circle(350, startangle=0, endangle=100)
    arc.Placement.Base = Vector(3200, 500, 0)
    t_xpos += 800
    _set_text(["Circular arc"], Vector(t_xpos, t_ypos, 0))

    _msg(16 * "-")
    _msg("Circular arc 3 points")
    Draft.make_arc_3points([Vector(4600, 0, 0),
                            Vector(4600, 800, 0),
                            Vector(4000, 1000, 0)])
    t_xpos += 600
    _set_text(["Circular arc 3 points"], Vector(t_xpos, t_ypos, 0))

    # Ellipse, polygon, rectangle
    _msg(16 * "-")
    _msg("Ellipse")
    ellipse = Draft.make_ellipse(500, 300)
    ellipse.Placement.Base = Vector(5500, 250, 0)
    t_xpos += 1600
    _set_text(["Ellipse"], Vector(t_xpos, t_ypos, 0))

    _msg(16 * "-")
    _msg("Polygon")
    polygon = Draft.make_polygon(5, 250)
    polygon.Placement.Base = Vector(6500, 500, 0)
    t_xpos += 950
    _set_text(["Polygon"], Vector(t_xpos, t_ypos, 0))

    _msg(16 * "-")
    _msg("Rectangle")
    rectangle = Draft.make_rectangle(500, 1000, 0)
    rectangle.Placement.Base = Vector(7000, 0, 0)
    t_xpos += 650
    _set_text(["Rectangle"], Vector(t_xpos, t_ypos, 0))

    # Text
    _msg(16 * "-")
    _msg("Text")
    text = Draft.make_text(["Testing", "text"], Vector(7700, 500, 0))
    if App.GuiUp:
        text.ViewObject.FontSize = 100
    t_xpos += 700
    _set_text(["Text"], Vector(t_xpos, t_ypos, 0))

    # Linear dimension
    _msg(16 * "-")
    _msg("Linear dimension")
    line = Draft.make_wire([Vector(8700, 200, 0),
                            Vector(8700, 1200, 0)])

    dimension = Draft.make_linear_dimension(Vector(8600, 200, 0),
                                            Vector(8600, 1000, 0),
                                            Vector(8400, 750, 0))
    if App.GuiUp:
        dimension.ViewObject.ArrowSize = 15
        dimension.ViewObject.ExtLines = 1000
        dimension.ViewObject.ExtOvershoot = 100
        dimension.ViewObject.DimOvershoot = 50
        dimension.ViewObject.FontSize = 100
        dimension.ViewObject.ShowUnit = False
    doc.recompute()

    dim_obj = Draft.make_linear_dimension_obj(line, 1, 2,
                                              Vector(9000, 750, 0))
    if App.GuiUp:
        dim_obj.ViewObject.ArrowSize = 15
        dim_obj.ViewObject.ArrowType = "Arrow"
        dim_obj.ViewObject.ExtLines = 100
        dim_obj.ViewObject.ExtOvershoot = 100
        dim_obj.ViewObject.DimOvershoot = 50
        dim_obj.ViewObject.FontSize = 100
        dim_obj.ViewObject.ShowUnit = False

    t_xpos += 680
    _set_text(["Dimension"], Vector(t_xpos, t_ypos, 0))

    # Radius and diameter dimension
    _msg(16 * "-")
    _msg("Radius and diameter dimension")
    arc_h = Draft.make_circle(500, startangle=0, endangle=90)
    arc_h.Placement.Base = Vector(9500, 0, 0)
    doc.recompute()

    dimension_r = Draft.make_radial_dimension_obj(arc_h, 1,
                                                  "radius",
                                                  Vector(9750, 200, 0))
    if App.GuiUp:
        dimension_r.ViewObject.ArrowSize = 15
        dimension_r.ViewObject.FontSize = 100
        dimension_r.ViewObject.ShowUnit = False

    arc_h2 = Draft.make_circle(450, startangle=-120, endangle=80)
    arc_h2.Placement.Base = Vector(10000, 1000, 0)
    doc.recompute()

    dimension_d = Draft.make_radial_dimension_obj(arc_h2, 1,
                                                  "diameter",
                                                  Vector(10750, 900, 0))
    if App.GuiUp:
        dimension_d.ViewObject.ArrowSize = 15
        dimension_d.ViewObject.FontSize = 100
        dimension_d.ViewObject.ShowUnit = False
    t_xpos += 950
    _set_text(["Radius dimension",
               "Diameter dimension"], Vector(t_xpos, t_ypos, 0))

    # Angular dimension
    _msg(16 * "-")
    _msg("Angular dimension")
    Draft.make_line(Vector(10500, 300, 0), Vector(11500, 1000, 0))
    Draft.make_line(Vector(10500, 300, 0), Vector(11500, 0, 0))
    angle1 = -20
    angle2 = 40
    dimension_a = Draft.make_angular_dimension(Vector(10500, 300, 0),
                                               [angle1, angle2],
                                               Vector(11500, 300, 0))
    if App.GuiUp:
        dimension_a.ViewObject.ArrowSize = 15
        dimension_a.ViewObject.FontSize = 100
    t_xpos += 1700
    _set_text(["Angle dimension"], Vector(t_xpos, t_ypos, 0))

    # BSpline
    _msg(16 * "-")
    _msg("BSpline")
    Draft.make_bspline([Vector(12500, 0, 0),
                        Vector(12500, 500, 0),
                        Vector(13000, 500, 0),
                        Vector(13000, 1000, 0)])
    t_xpos += 1500
    _set_text(["BSpline"], Vector(t_xpos, t_ypos, 0))

    # Point
    _msg(16 * "-")
    _msg("Point")
    point = Draft.make_point(13500, 500, 0)
    if App.GuiUp:
        point.ViewObject.PointSize = 10
    t_xpos += 900
    _set_text(["Point"], Vector(t_xpos, t_ypos, 0))

    # Shapestring
    _msg(16 * "-")
    _msg("Shapestring")
    try:
        shape_string = Draft.make_shapestring("Testing",
                                              font_file,
                                              100)
        shape_string.Placement.Base = Vector(14000, 500)
    except Exception:
        _wrn("Shapestring could not be created")
        _wrn("Possible cause: the font file may not exist")
        _wrn(font_file)
        rect = Draft.make_rectangle(500, 100)
        rect.Placement.Base = Vector(14000, 500)
    t_xpos += 600
    _set_text(["Shapestring"], Vector(t_xpos, t_ypos, 0))

    # Facebinder
    _msg(16 * "-")
    _msg("Facebinder")
    box = doc.addObject("Part::Box", "Cube")
    box.Length = 200
    box.Width = 500
    box.Height = 100
    box.Placement.Base = Vector(15000, 0, 0)
    if App.GuiUp:
        box.ViewObject.Visibility = False

    facebinder = Draft.make_facebinder([(box, ("Face1", "Face3", "Face6"))])
    facebinder.Extrusion = 10
    t_xpos += 780
    _set_text(["Facebinder"], Vector(t_xpos, t_ypos, 0))

    # Cubic bezier curve, n-degree bezier curve
    _msg(16 * "-")
    _msg("Cubic bezier")
    Draft.make_bezcurve([Vector(15500, 0, 0),
                         Vector(15578, 485, 0),
                         Vector(15879, 154, 0),
                         Vector(15975, 400, 0),
                         Vector(16070, 668, 0),
                         Vector(16423, 925, 0),
                         Vector(16500, 500, 0)], degree=3)
    t_xpos += 680
    _set_text(["Cubic bezier"], Vector(t_xpos, t_ypos, 0))

    _msg(16 * "-")
    _msg("N-degree bezier")
    Draft.make_bezcurve([Vector(16500, 0, 0),
                         Vector(17000, 500, 0),
                         Vector(17500, 500, 0),
                         Vector(17500, 1000, 0),
                         Vector(17000, 1000, 0),
                         Vector(17063, 1256, 0),
                         Vector(17732, 1227, 0),
                         Vector(17790, 720, 0),
                         Vector(17702, 242, 0)])
    t_xpos += 1200
    _set_text(["n-Bezier"], Vector(t_xpos, t_ypos, 0))

    # Label
    _msg(16 * "-")
    _msg("Label")
    place = App.Placement(Vector(18500, 500, 0), App.Rotation())
    label = Draft.make_label(target_point=Vector(18000, 0, 0),
                             placement=place,
                             custom_text="Example label",
                             distance=-250)
    label.Text = "Testing"
    if App.GuiUp:
        label.ViewObject.ArrowSize = 15
        label.ViewObject.TextSize = 100
    doc.recompute()
    t_xpos += 1200
    _set_text(["Label"], Vector(t_xpos, t_ypos, 0))

    # Orthogonal array and orthogonal link array
    _msg(16 * "-")
    _msg("Orthogonal array")
    rect_h = Draft.make_rectangle(500, 500)
    rect_h.Placement.Base = Vector(1500, 2500, 0)
    if App.GuiUp:
        rect_h.ViewObject.Visibility = False

    Draft.make_ortho_array(rect_h,
                           Vector(600, 0, 0),
                           Vector(0, 600, 0),
                           Vector(0, 0, 0),
                           3, 2, 1,
                           use_link=False)
    t_xpos = 1700
    t_ypos = 2200
    _set_text(["Array"], Vector(t_xpos, t_ypos, 0))

    rect_h_2 = Draft.make_rectangle(500, 100)
    rect_h_2.Placement.Base = Vector(1500, 5000, 0)
    if App.GuiUp:
        rect_h_2.ViewObject.Visibility = False

    _msg(16 * "-")
    _msg("Orthogonal link array")
    Draft.make_ortho_array(rect_h_2,
                           Vector(800, 0, 0),
                           Vector(0, 500, 0),
                           Vector(0, 0, 0),
                           2, 4, 1,
                           use_link=True)
    t_ypos += 2600
    _set_text(["Link array"], Vector(t_xpos, t_ypos, 0))

    # Polar array and polar link array
    _msg(16 * "-")
    _msg("Polar array")
    wire_h = Draft.make_wire([Vector(5500, 3000, 0),
                              Vector(6000, 3500, 0),
                              Vector(6000, 3200, 0),
                              Vector(5800, 3200, 0)])
    if App.GuiUp:
        wire_h.ViewObject.Visibility = False

    Draft.make_polar_array(wire_h,
                           8,
                           200,
                           Vector(5000, 3000, 0),
                           use_link=False)

    t_xpos = 4600
    t_ypos = 2200
    _set_text(["Polar array"], Vector(t_xpos, t_ypos, 0))

    _msg(16 * "-")
    _msg("Polar link array")
    wire_h_2 = Draft.make_wire([Vector(5500, 6000, 0),
                                Vector(6000, 6000, 0),
                                Vector(5800, 5700, 0),
                                Vector(5800, 5750, 0)])
    if App.GuiUp:
        wire_h_2.ViewObject.Visibility = False

    Draft.make_polar_array(wire_h_2,
                           8,
                           200,
                           Vector(5000, 6000, 0),
                           use_link=True)
    t_ypos += 3200
    _set_text(["Polar link array"], Vector(t_xpos, t_ypos, 0))

    # Circular array and circular link array
    _msg(16 * "-")
    _msg("Circular array")
    poly_h = Draft.make_polygon(5, 200)
    poly_h.Placement.Base = Vector(8000, 3000, 0)
    if App.GuiUp:
        poly_h.ViewObject.Visibility = False

    Draft.make_circular_array(poly_h,
                              500, 600,
                              3,
                              1,
                              Vector(0, 0, 1),
                              Vector(0, 0, 0),
                              use_link=False)
    t_xpos = 7700
    t_ypos = 1700
    _set_text(["Circular array"], Vector(t_xpos, t_ypos, 0))

    _msg(16 * "-")
    _msg("Circular link array")
    poly_h_2 = Draft.make_polygon(6, 150)
    poly_h_2.Placement.Base = Vector(8000, 6250, 0)
    if App.GuiUp:
        poly_h_2.ViewObject.Visibility = False

    Draft.make_circular_array(poly_h_2,
                              550, 450,
                              3,
                              1,
                              Vector(0, 0, 1),
                              Vector(0, 0, 0),
                              use_link=True)
    t_ypos += 3100
    _set_text(["Circular link array"], Vector(t_xpos, t_ypos, 0))

    # Path array and path link array
    _msg(16 * "-")
    _msg("Path array")
    poly_h = Draft.make_polygon(3, 250)
    poly_h.Placement.Base = Vector(10000, 3000, 0)
    if App.GuiUp:
        poly_h.ViewObject.Visibility = False

    bspline_path = Draft.make_bspline([Vector(10500, 2500, 0),
                                       Vector(11000, 3000, 0),
                                       Vector(11500, 3200, 0),
                                       Vector(12000, 4000, 0)])

    Draft.make_path_array(poly_h, bspline_path, 5,
                          use_link=False)
    t_xpos = 10400
    t_ypos = 2200
    _set_text(["Path array"], Vector(t_xpos, t_ypos, 0))

    _msg(16 * "-")
    _msg("Path link array")
    poly_h_2 = Draft.make_polygon(4, 200)
    poly_h_2.Placement.Base = Vector(10000, 5000, 0)
    if App.GuiUp:
        poly_h_2.ViewObject.Visibility = False

    bspline_path_2 = Draft.make_bspline([Vector(10500, 4500, 0),
                                         Vector(11000, 6800, 0),
                                         Vector(11500, 6000, 0),
                                         Vector(12000, 5200, 0)])

    Draft.make_path_array(poly_h_2, bspline_path_2, 6,
                          use_link=True)
    t_ypos += 2000
    _set_text(["Path link array"], Vector(t_xpos, t_ypos, 0))

    # Point array
    _msg(16 * "-")
    _msg("Point array")
    poly_h = Draft.make_polygon(3, 250)
    poly_h.Placement.Base = Vector(12500, 2500, 0)

    point_1 = Draft.make_point(13000, 3000, 0)
    point_2 = Draft.make_point(13000, 3500, 0)
    point_3 = Draft.make_point(14000, 2500, 0)
    point_4 = Draft.make_point(14000, 3000, 0)

    add_list, delete_list = Draft.upgrade([point_1, point_2,
                                           point_3, point_4])
    compound = add_list[0]
    if App.GuiUp:
        compound.ViewObject.PointSize = 5

    Draft.make_point_array(poly_h, compound)
    t_xpos = 13000
    t_ypos = 2200
    _set_text(["Point array"], Vector(t_xpos, t_ypos, 0))

    # Clone and mirror
    _msg(16 * "-")
    _msg("Clone")
    wire_h = Draft.make_wire([Vector(15000, 2500, 0),
                              Vector(15200, 3000, 0),
                              Vector(15500, 2500, 0),
                              Vector(15200, 2300, 0)])

    Draft.make_clone(wire_h, Vector(0, 1000, 0))
    t_xpos = 15000
    t_ypos = 2100
    _set_text(["Clone"], Vector(t_xpos, t_ypos, 0))

    _msg(16 * "-")
    _msg("Mirror")
    wire_h = Draft.make_wire([Vector(17000, 2500, 0),
                              Vector(16500, 4000, 0),
                              Vector(16000, 2700, 0),
                              Vector(16500, 2500, 0),
                              Vector(16700, 2700, 0)])

    Draft.mirror(wire_h,
                 Vector(17100, 2000, 0),
                 Vector(17100, 4000, 0))
    t_xpos = 17000
    t_ypos = 2200
    _set_text(["Mirror"], Vector(t_xpos, t_ypos, 0))

    _msg(16 * "-")
    _msg("Layer")
    layer = Draft.make_layer("Custom layer",
                             line_color=(0.33, 0.0, 0.49),
                             shape_color=(0.56, 0.89, 0.56),
                             line_width=4,
                             transparency=50)
    cube = doc.addObject('Part::Box')
    cube.Length = 350
    cube.Width = 300
    cube.Height = 250
    cube.Placement.Base = Vector(14000, 5500, 0)

    cone = doc.addObject('Part::Cone')
    cone.Radius1 = 400
    cone.Height = 600
    cone.Angle = 270
    cone.Placement.Base = Vector(15000, 6000, 0)

    sphere = doc.addObject('Part::Sphere')
    sphere.Radius = 450
    sphere.Angle1 = -45
    sphere.Angle2 = 45
    sphere.Angle3 = 300
    sphere.Placement.Base = Vector(14000, 7000, 0)

    layer.Proxy.addObject(layer, cube)
    layer.Proxy.addObject(layer, cone)
    layer.Proxy.addObject(layer, sphere)

    t_xpos = 14000
    t_ypos = 5000
    _set_text(["Layer"], Vector(t_xpos, t_ypos, 0))
    doc.recompute()


def create_test_file(file_name="draft_test_objects",
                     file_path=os.environ["HOME"],
                     save=False,
                     font_file="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"):
    """Create a complete test file of Draft objects.

    It draws a frame with information on the software used to create
    the test document, and fills it with every object that can be created.

    Parameters
    ----------
    file_name: str, optional
        It defaults to `'draft_test_objects'`.
        It is the name of the document that is created.

        The `file_name` will be appended to `file_path`
        to determine the actual path to save. The extension `.FCStd`
        will be added automatically.

    file_path: str, optional
        It defaults to the value of `os.environ['HOME']`
        which in Linux is usually `'/home/user'`.

        If it is the empty string `''` it will use the value
        returned by `App.getUserAppDataDir()`,
        for example, `'/home/user/.FreeCAD/'`.

    save: bool, optional
        It defaults to `False`. If it is `True` the new document
        will be saved to disk after creating all objects.

    font_file: str, optional
        It defaults to `'/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'`.
        It is the full path of a font in the system to be used
        to create a `Draft ShapeString`.
        If the font is not found, this object is not created.

    Returns
    -------
    App::Document
        A reference to the test document that was created.

    To Do
    -----
    Find a reliable way of getting a default font to be able to create
    the `Draft ShapeString`.
    """
    doc = App.newDocument(file_name)
    _msg(16 * "-")
    _msg("Filename: {}".format(file_name))
    _msg("If the units tests fail, this script may fail as well")

    _create_frame(doc=doc)
    _create_objects(doc=doc, font_file=font_file)

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

## @}


if __name__ == "__main__":
    create_test_file()
