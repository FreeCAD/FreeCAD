# ***************************************************************************
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2021 FreeCAD Developers                                 *
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
    """Draw a frame with information on the version of the software."""
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

    p1 = Vector(-1000, -4000, 0)
    p2 = Vector(17000, -4000, 0)
    p3 = Vector(17000, 8000, 0)
    p4 = Vector(-1000, 8000, 0)

    poly = Part.makePolygon([p1, p2, p3, p4, p1])
    frame = doc.addObject("Part::Feature", "Frame")
    frame.Shape = poly


def _create_objects(doc=None,
                    font_file=None,
                    hatch_file=None,
                    hatch_name=None):
    """Create the objects of the test file."""
    if not doc:
        doc = App.activeDocument()
    if not doc:
        doc = App.newDocument()

    # Drafting ##############################################################

    # Line
    _msg(16 * "-")
    _msg("Line")
    Draft.make_line(Vector(0, 0, 0), Vector(500, 500, 0))
    _set_text(["Line"], Vector(0, -200, 0))

    # Wire
    _msg(16 * "-")
    _msg("Wire")
    Draft.make_wire([Vector(1000, 0, 0),
                     Vector(1500, 250, 0),
                     Vector(1500, 500, 0)])
    _set_text(["Wire"], Vector(1000, -200, 0))

    # Fillet
    _msg(16 * "-")
    _msg("Fillet")
    line_1 = Draft.make_line(Vector(2000, 0, 0), Vector(2000, 500, 0))
    line_2 = Draft.make_line(Vector(2000, 500, 0), Vector(2500, 500, 0))
    if App.GuiUp:
        line_1.ViewObject.DrawStyle = "Dotted"
        line_2.ViewObject.DrawStyle = "Dotted"
    doc.recompute()
    Draft.make_fillet([line_1, line_2], 400)
    _set_text(["Fillet"], Vector(2000, -200, 0))

    # Circular arc
    _msg(16 * "-")
    _msg("Circular arc")
    arc = Draft.make_circle(250, startangle=90, endangle=270)
    arc.Placement.Base = Vector(3250, 250, 0)
    _set_text(["Circular arc"], Vector(3000, -200, 0))

    # Circular arc 3 points
    _msg(16 * "-")
    _msg("Circular arc 3 points")
    Draft.make_arc_3points([Vector(4250, 0, 0),
                            Vector(4000, 250, 0),
                            Vector(4250, 500, 0)])
    _set_text(["Circular arc 3 points"], Vector(4000, -200, 0))

    # Circle
    _msg(16 * "-")
    _msg("Circle")
    circle = Draft.make_circle(250)
    circle.Placement.Base = Vector(5250, 250, 0)
    _set_text(["Circle"], Vector(5000, -200, 0))

    # Ellipse
    _msg(16 * "-")
    _msg("Ellipse")
    ellipse = Draft.make_ellipse(250, 150)
    ellipse.Placement.Base = Vector(6250, 150, 0)
    _set_text(["Ellipse"], Vector(6000, -200, 0))

    # Rectangle
    _msg(16 * "-")
    _msg("Rectangle")
    rectangle = Draft.make_rectangle(500, 300, 0)
    rectangle.Placement.Base = Vector(7000, 0, 0)
    _set_text(["Rectangle"], Vector(7000, -200, 0))

    # Polygon
    _msg(16 * "-")
    _msg("Polygon")
    polygon = Draft.make_polygon(5, 250)
    polygon.Placement.Base = Vector(8250, 250, 0)
    _set_text(["Polygon"], Vector(8000, -200, 0))

    # BSpline
    _msg(16 * "-")
    _msg("BSpline")
    Draft.make_bspline([Vector(9000, 0, 0),
                        Vector(9100, 200, 0),
                        Vector(9400, 300, 0),
                        Vector(9500, 500, 0)])
    _set_text(["BSpline"], Vector(9000, -200, 0))

    # Cubic bezier
    _msg(16 * "-")
    _msg("Cubic bezier")
    Draft.make_bezcurve([Vector(10000, 0, 0),
                         Vector(10000, 500, 0),
                         Vector(10500, 0, 0),
                         Vector(10500, 500, 0)], degree=3)
    _set_text(["Cubic bezier"], Vector(10000, -200, 0))

    # N-degree bezier
    _msg(16 * "-")
    _msg("N-degree bezier")
    Draft.make_bezcurve([Vector (11000, 0, 0),
                         Vector (11100, 400, 0),
                         Vector (11250, 250, 0),
                         Vector (11400, 100, 0),
                         Vector (11500, 500, 0)])
    _set_text(["N-degree bezier"], Vector(11000, -200, 0))

    # Point
    _msg(16 * "-")
    _msg("Point")
    point = Draft.make_point(12000, 0, 0)
    if App.GuiUp:
        point.ViewObject.PointSize = 10
    _set_text(["Point"], Vector(12000, -200, 0))

    # Facebinder
    _msg(16 * "-")
    _msg("Facebinder")
    box = doc.addObject("Part::Box", "Box")
    box.Length = 200
    box.Width = 500
    box.Height = 100
    box.Placement.Base = Vector(13000, 0, 0)
    if App.GuiUp:
        box.ViewObject.Visibility = False
    facebinder = Draft.make_facebinder([(box, ("Face1", "Face3", "Face6"))])
    facebinder.Extrusion = 10
    _set_text(["Facebinder"], Vector(13000, -200, 0))

    # Shapestring
    _msg(16 * "-")
    _msg("Shapestring")
    try:
        shape_string = Draft.make_shapestring("Testing",
                                              font_file,
                                              100)
        shape_string.Placement.Base = Vector(14000, 0)
    except Exception:
        _wrn("Shapestring could not be created")
        _wrn("Possible cause: the font file may not exist")
        _wrn(font_file)
    _set_text(["Shapestring"], Vector(14000, -200, 0))

    # Hatch
    _msg(16 * "-")
    _msg("Hatch")
    rectangle = Draft.make_rectangle(500, 300, 0)
    rectangle.Placement.Base = Vector(15000, 0, 0)
    rectangle.MakeFace = True
    if App.GuiUp:
        rectangle.ViewObject.Visibility = False
    try:
        Draft.make_hatch(rectangle,
                         hatch_file,
                         hatch_name,
                         scale=10,
                         rotation=45)
    except Exception:
        _wrn("Hatch could not be created")
        _wrn("Possible cause: the hatch file may not exist")
        _wrn(hatch_file)
    _set_text(["Hatch"], Vector(15000, -200, 0))

    # Annotation ############################################################

    # Text
    _msg(16 * "-")
    _msg("Text")
    text = Draft.make_text(["Testing", "text"], Vector(0, 2100, 0))
    if App.GuiUp:
        text.ViewObject.FontSize = 100
    _set_text(["Text"], Vector(0, 1800, 0))

    # Linear dimension
    _msg(16 * "-")
    _msg("Linear dimension")
    dimension = Draft.make_linear_dimension(Vector(1500, 2000, 0),
                                            Vector(1500, 2400, 0),
                                            Vector(1000, 2200, 0))
    if App.GuiUp:
        dimension.ViewObject.ArrowSize = 15
        dimension.ViewObject.ExtLines = 0
        dimension.ViewObject.ExtOvershoot = 50
        dimension.ViewObject.DimOvershoot = 25
        dimension.ViewObject.FontSize = 50
        dimension.ViewObject.Decimals = 1
        dimension.ViewObject.ShowUnit = False

    line = Draft.make_wire([Vector(1500, 2600, 0),
                            Vector(1500, 3000, 0)])
    doc.recompute()
    dimension = Draft.make_linear_dimension_obj(line, 1, 2,
                                              Vector(1000, 2800, 0))
    if App.GuiUp:
        dimension.ViewObject.ArrowSize = 15
        dimension.ViewObject.ArrowType = "Arrow"
        dimension.ViewObject.ExtLines = -50
        dimension.ViewObject.ExtOvershoot = 50
        dimension.ViewObject.DimOvershoot = 25
        dimension.ViewObject.FontSize = 50
        dimension.ViewObject.Decimals = 1
        dimension.ViewObject.ShowUnit = False

    _set_text(["Dimension"], Vector(1000, 1800, 0))

    # Radius and diameter dimension
    _msg(16 * "-")
    _msg("Radius and diameter dimension")
    circle = Draft.make_circle(200)
    circle.Placement.Base = Vector(2200, 2200, 0)
    circle.MakeFace = False
    doc.recompute()
    dimension = Draft.make_radial_dimension_obj(circle,
                                                1,
                                                "radius",
                                                Vector(2300, 2300, 0))
    if App.GuiUp:
        dimension.ViewObject.ArrowSize = 15
        dimension.ViewObject.FontSize = 50
        dimension.ViewObject.Decimals = 1
        dimension.ViewObject.ShowUnit = False

    circle = Draft.make_circle(200)
    circle.Placement.Base = Vector(2200, 2800, 0)
    circle.MakeFace = False
    doc.recompute()
    dimension = Draft.make_radial_dimension_obj(circle,
                                                1,
                                                "diameter",
                                                Vector(2300, 2900, 0))
    if App.GuiUp:
        dimension.ViewObject.ArrowSize = 15
        dimension.ViewObject.FontSize = 50
        dimension.ViewObject.Decimals = 1
        dimension.ViewObject.ShowUnit = False
    _set_text(["Radius dimension",
               "Diameter dimension"], Vector(2000, 1800, 0))

    # Angular dimension
    _msg(16 * "-")
    _msg("Angular dimension")
    Draft.make_line(Vector(3000, 2000, 0), Vector(3500, 2000, 0))
    Draft.make_line(Vector(3000, 2000, 0), Vector(3500, 2500, 0))
    dimension = Draft.make_angular_dimension(Vector(3000, 2000, 0),
                                             [0, 45],
                                             Vector(3250, 2250, 0))
    if App.GuiUp:
        dimension.ViewObject.ArrowSize = 15
        dimension.ViewObject.FontSize = 50
        dimension.ViewObject.Decimals = 1
    _set_text(["Angle dimension"], Vector(3000, 1800, 0))

    # Label
    _msg(16 * "-")
    _msg("Label")
    place = App.Placement(Vector(4250, 2250, 0), App.Rotation())
    label = Draft.make_label(target_point=Vector(4000, 2000, 0),
                             placement=place,
                             custom_text="Example label",
                             distance=-100)
    label.Text = "Testing"
    if App.GuiUp:
        label.ViewObject.ArrowSize = 15
        label.ViewObject.TextSize = 50
    doc.recompute()
    _set_text(["Label"], Vector(4000, 1800, 0))

    # Array #################################################################

    # Orthogonal array
    _msg(16 * "-")
    _msg("Orthogonal array")
    rectangle = Draft.make_rectangle(100, 50)
    rectangle.Placement.Base = Vector(0, 4000, 0)
    if App.GuiUp:
        rectangle.ViewObject.Visibility = False
    Draft.make_ortho_array(rectangle,
                           Vector(200, 0, 0),
                           Vector(0, 150, 0),
                           Vector(0, 0, 0),
                           3,
                           2,
                           1,
                           use_link=False)
    _set_text(["Orthogonal array"], Vector(0, 3800, 0))

    # Orthogonal link array
    _msg(16 * "-")
    _msg("Orthogonal link array")
    rectangle = Draft.make_rectangle(50, 50)
    rectangle.Placement.Base = Vector(1000, 4000, 0)
    if App.GuiUp:
        rectangle.ViewObject.Visibility = False
    Draft.make_ortho_array(rectangle,
                           Vector(200, 0, 0),
                           Vector(0, 150, 0),
                           Vector(0, 0, 0),
                           3,
                           2,
                           1,
                           use_link=True)
    _set_text(["Orthogonal link array"], Vector(1000, 3800, 0))

    # Polar array
    _msg(16 * "-")
    _msg("Polar array")
    wire = Draft.make_wire([Vector(2000, 4050, 0),
                            Vector(2000, 4000, 0),
                            Vector(2100, 4000, 0)])
    if App.GuiUp:
        wire.ViewObject.Visibility = False
    Draft.make_polar_array(wire,
                           4,
                           90,
                           Vector(2000, 4250, 0),
                           use_link=False)
    _set_text(["Polar array"], Vector(2000, 3800, 0))

    # Polar link array
    _msg(16 * "-")
    _msg("Polar link array")
    wire = Draft.make_wire([Vector(3000, 4050, 0),
                            Vector(3000, 4000, 0),
                            Vector(3050, 4000, 0)])
    if App.GuiUp:
        wire.ViewObject.Visibility = False
    Draft.make_polar_array(wire,
                           4,
                           90,
                           Vector(3000, 4250, 0),
                           use_link=True)
    _set_text(["Polar link array"], Vector(3000, 3800, 0))

    # Circular array
    _msg(16 * "-")
    _msg("Circular array")
    polygon = Draft.make_polygon(5, 30)
    polygon.Placement.Base = Vector(4250, 4250, 0)
    if App.GuiUp:
        polygon.ViewObject.Visibility = False
    Draft.make_circular_array(polygon,
                              110,
                              100,
                              3,
                              1,
                              Vector(0, 0, 1),
                              Vector(0, 0, 0),
                              use_link=False)
    _set_text(["Circular array"], Vector(4000, 3800, 0))

    # Circular link array
    _msg(16 * "-")
    _msg("Circular link array")
    polygon = Draft.make_polygon(6, 30)
    polygon.Placement.Base = Vector(5250, 4250, 0)
    if App.GuiUp:
        polygon.ViewObject.Visibility = False
    Draft.make_circular_array(polygon,
                              110,
                              100,
                              3,
                              1,
                              Vector(0, 0, 1),
                              Vector(0, 0, 0),
                              use_link=True)
    _set_text(["Circular link array"], Vector(5000, 3800, 0))

    # Path array
    _msg(16 * "-")
    _msg("Path array")
    polygon = Draft.make_polygon(3, 30)
    polygon.Placement.Base = Vector(6000, 4000, 0)
    if App.GuiUp:
        polygon.ViewObject.Visibility = False
    spline = Draft.make_bspline([Vector(6000, 4000, 0),
                                 Vector(6100, 4200, 0),
                                 Vector(6400, 4300, 0),
                                 Vector(6500, 4500, 0)])
    Draft.make_path_array(polygon, spline, 5, use_link=False)
    _set_text(["Path array"], Vector(6000, 3800, 0))

    # Path link array
    _msg(16 * "-")
    _msg("Path link array")
    polygon = Draft.make_polygon(4, 30)
    polygon.Placement.Base = Vector(7000, 4000, 0)
    if App.GuiUp:
        polygon.ViewObject.Visibility = False
    spline = Draft.make_bspline([Vector(7000, 4000, 0),
                                 Vector(7100, 4200, 0),
                                 Vector(7400, 4300, 0),
                                 Vector(7500, 4500, 0)])
    Draft.make_path_array(polygon, spline, 5, use_link=True)
    _set_text(["Path link array"], Vector(7000, 3800, 0))

    # Point array
    _msg(16 * "-")
    _msg("Point array")
    polygon = Draft.make_polygon(3, 30)
    polygon.Placement.Base = Vector(8000, 4000, 0)
    point_1 = Draft.make_point(8030, 4030, 0)
    point_2 = Draft.make_point(8030, 4250, 0)
    point_3 = Draft.make_point(8470, 4250, 0)
    point_4 = Draft.make_point(8470, 4470, 0)
    add_list, delete_list = Draft.upgrade([point_1, point_2,
                                           point_3, point_4])
    compound = add_list[0]
    if App.GuiUp:
        compound.ViewObject.PointSize = 5
    Draft.make_point_array(polygon, compound, use_link=False)
    _set_text(["Point array"], Vector(8000, 3800, 0))

    # Point link array
    _msg(16 * "-")
    _msg("Point link array")
    polygon = Draft.make_polygon(4, 30)
    polygon.Placement.Base = Vector(9000, 4000, 0)
    point_1 = Draft.make_point(9030, 4030, 0)
    point_2 = Draft.make_point(9030, 4250, 0)
    point_3 = Draft.make_point(9470, 4250, 0)
    point_4 = Draft.make_point(9470, 4470, 0)
    add_list, delete_list = Draft.upgrade([point_1, point_2,
                                           point_3, point_4])
    compound = add_list[0]
    if App.GuiUp:
        compound.ViewObject.PointSize = 5
    Draft.make_point_array(polygon, compound, use_link=True)
    _set_text(["Point link array"], Vector(9000, 3800, 0))

    # Miscellaneous #########################################################

    # Mirror
    _msg(16 * "-")
    _msg("Mirror")
    wire = Draft.make_wire([Vector(0, 6000, 0),
                            Vector(150, 6200, 0),
                            Vector(500, 6000, 0)])
    Draft.mirror(wire,
                 Vector(0, 6250, 0),
                 Vector(500, 6250, 0))
    _set_text(["Mirror"], Vector(0, 5800, 0))

    # Clone
    _msg(16 * "-")
    _msg("Clone")
    wire = Draft.make_wire([Vector(1000, 6000, 0),
                            Vector(1150, 6200, 0),
                            Vector(1500, 6000, 0)])
    Draft.make_clone(wire, Vector(0, 300, 0))
    _set_text(["Clone"], Vector(1000, 5800, 0))

    # Shape2DView
    _msg(16 * "-")
    _msg("Shape2DView")
    place = App.Placement(Vector(2000, 6000, 0),
                          App.Rotation(Vector(0, 0, 1), Vector(1, 2, 3)))
    box = doc.addObject("Part::Box", "Box")
    box.Length = 200
    box.Width = 500
    box.Height = 100
    box.Placement = place
    if App.GuiUp:
        box.ViewObject.Visibility = False
    Draft.make_shape2dview(box)
    _set_text(["Shape2DView"], Vector(2000, 5800, 0))

    # WorkingPlaneProxy
    _msg(16 * "-")
    _msg("WorkingPlaneProxy")
    place = App.Placement(Vector(3250, 6250, 0), App.Rotation())
    proxy = Draft.make_workingplaneproxy(place)
    if App.GuiUp:
        proxy.ViewObject.DisplaySize = 500
        proxy.ViewObject.ArrowSize = 50
    _set_text(["WorkingPlaneProxy"], Vector(3000, 5800, 0))

    # Layer
    _msg(16 * "-")
    _msg("Layer")
    layer = Draft.make_layer("Custom layer",
                             line_color=(0.33, 0.0, 0.49),
                             shape_color=(0.56, 0.89, 0.56),
                             line_width=4,
                             transparency=50)
    box = doc.addObject("Part::Box", "Box")
    box.Length = 200
    box.Width = 500
    box.Height = 100
    box.Placement.Base = Vector(4000, 6000, 0)
    sphere = doc.addObject("Part::Sphere", "Sphere")
    sphere.Radius = 100
    sphere.Placement.Base = Vector(4400, 6250, 0)
    layer.Proxy.addObject(layer, box)
    layer.Proxy.addObject(layer, sphere)
    _set_text(["Layer"], Vector(4000, 5800, 0))

    doc.recompute()


def create_test_file(font_file=App.getHomePath()+"data/Mod/TechDraw/Resources/fonts/osifont-lgpl3fe.ttf",
                     hatch_file=App.getHomePath()+"data/Mod/TechDraw/PAT/FCPAT.pat",
                     hatch_name="Horizontal5"):
    """Create a complete test file of Draft objects.

    It draws a frame with information on the software used to create
    the test document, and fills it with every object that can be created.

    Parameters
    ----------
    font_file: str, optional
        It defaults to `App.getHomePath()+"data/Mod/TechDraw/Resources/fonts/osifont-lgpl3fe.ttf"`
        It is the full path of a font file to be used to create a `Draft ShapeString`.
        If the file is not found, this object is not created.

    hatch_file: str, optional
        It defaults to `App.getHomePath()+"data/Mod/TechDraw/PAT/FCPAT.pat"`
        It is the full path of a PAT file to be used to create a `Draft Hatch`.
        If the file is not found, this object is not created.

    hatch_name: str, optional
        It defaults to `"Horizontal5"`
        It is the name of a hatch pattern in the hatch_file.

    Returns
    -------
    App::Document
        A reference to the test document that was created.
    """
    _msg(16 * "-")
    _msg("If the units tests fail, this script may fail as well")
    doc = App.newDocument()

    _create_frame(doc=doc)
    _create_objects(doc=doc, font_file=font_file, hatch_file=hatch_file, hatch_name=hatch_name)

    if App.GuiUp:
        Gui.runCommand("Std_ViewFitAll")
        Gui.Selection.clearSelection()

    return doc

## @}


if __name__ == "__main__":
    create_test_file()
