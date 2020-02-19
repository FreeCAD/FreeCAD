"""Run this file to create a standard test document for Part objects.

Use as input to the freecad executable.
    freecad part_test_objects.py

Or load it as a module and use the defined function.
    import parttests.part_test_objects as pto
    pto.create_test_file()
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
import FreeCAD as App
from FreeCAD import Vector
import Draft
from draftutils.messages import _msg


if App.GuiUp:
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

    record = Draft.makeText(["Part test file",
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


def create_test_file(file_name="part_test_objects",
                     file_path="",
                     save=False):
    """Create a complete test file of Part objects.

    It draws a frame with information on the software used to create
    the test document, and fills it with every object that can be created.

    Parameters
    ----------
    file_name: str, optional
        It defaults to `'part_test_objects'`.
        It is the name of document that is created.

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

    # Part primitives
    _msg(16 * "-")
    _msg("Plane")
    plane = App.ActiveDocument.addObject("Part::Plane", "Plane")
    plane.Length = 2000
    plane.Width = 2000

    _msg(16 * "-")
    _msg("Box")
    displacement = Vector(2500, 0, 0)
    box = App.ActiveDocument.addObject("Part::Box", "Box")
    box.Length = 1500
    box.Width = 1500
    box.Height = 1500
    box.Placement.Base = displacement

    _msg(16 * "-")
    _msg("Cylinder")
    displacement += Vector(3000, 750, 0)
    cylinder = App.ActiveDocument.addObject("Part::Cylinder", "Cylinder")
    cylinder.Radius = 500
    cylinder.Height = 1500
    cylinder.Placement.Base = displacement

    _msg(16 * "-")
    _msg("Cone")
    displacement += Vector(2000, 0, 0)
    cone = App.ActiveDocument.addObject("Part::Cone", "Cone")
    cone.Radius1 = 700
    cone.Height = 2000
    cone.Placement.Base = displacement

    _msg(16 * "-")
    _msg("Sphere")
    displacement += Vector(2750, 0, 0)
    sphere = App.ActiveDocument.addObject("Part::Sphere", "Sphere")
    sphere.Radius = 1000
    sphere.Placement.Base = displacement

    _msg(16 * "-")
    _msg("Ellipsoid")
    displacement += Vector(2700, 0, 0)
    ellipsoid = App.ActiveDocument.addObject("Part::Ellipsoid", "Ellipsoid")
    ellipsoid.Radius1 = 500
    ellipsoid.Radius2 = 1000
    ellipsoid.Placement.Base = displacement

    _msg(16 * "-")
    _msg("Torus")
    displacement += Vector(2700, 0, 0)
    torus = App.ActiveDocument.addObject("Part::Torus", "Torus")
    torus.Radius1 = 1000
    torus.Radius2 = 300
    torus.Placement.Base = displacement

    _msg(16 * "-")
    _msg("Prism")
    displacement += Vector(2700, 0, 0)
    prism = App.ActiveDocument.addObject("Part::Prism", "Prism")
    prism.Circumradius = 450
    prism.Height = 1200
    prism.Placement.Base = displacement

    _msg(16 * "-")
    _msg("Wedge")
    displacement = Vector(0, 4500, 0)
    wedge = App.ActiveDocument.addObject("Part::Wedge", "Wedge")
    wedge.Xmin = 0
    wedge.Ymin = 0
    wedge.Zmin = 0
    wedge.X2min = 0
    wedge.Z2min = 0

    wedge.Xmax = 1000
    wedge.X2max = 1800
    wedge.Ymax = 1000
    wedge.Zmax = 200
    wedge.Z2max = 1200
    wedge.Placement.Base = displacement

    _msg(16 * "-")
    _msg("Helix")
    displacement += Vector(3500, 0, 0)
    helix = App.ActiveDocument.addObject("Part::Helix", "Helix")
    helix.Pitch = 200
    helix.Height = 2000
    helix.Radius = 350
    helix.Placement.Base = displacement

    _msg(16 * "-")
    _msg("Spiral")
    displacement += Vector(2100, 0, 0)
    spiral = App.ActiveDocument.addObject("Part::Spiral", "Spiral")
    spiral.Growth = 200
    spiral.Radius = 100
    spiral.Rotations = 4
    spiral.Placement.Base = displacement

    _msg(16 * "-")
    _msg("Circle")
    displacement += Vector(2500, 0, 0)
    circle = App.ActiveDocument.addObject("Part::Circle", "Circle")
    circle.Radius = 500
    circle.Placement.Base = displacement

    _msg(16 * "-")
    _msg("Ellipse")
    displacement += Vector(2500, 0, 0)
    ellipse = App.ActiveDocument.addObject("Part::Ellipse", "Ellipse")
    ellipse.MajorRadius = 800
    ellipse.MinorRadius = 400
    ellipse.Placement.Base = displacement

    _msg(16 * "-")
    _msg("Vertex")
    displacement += Vector(2500, 0, 0)
    vertex = App.ActiveDocument.addObject("Part::Vertex", "Vertex")
    vertex.Placement.Base = displacement
    if App.GuiUp:
        vertex.ViewObject.PointSize = 8

    _msg(16 * "-")
    _msg("Line")
    displacement += Vector(2500, 0, 0)
    line = App.ActiveDocument.addObject("Part::Line", "Line")
    line.Z2 = 1500
    line.Placement.Base = displacement

    _msg(16 * "-")
    _msg("Regular polygon")
    displacement += Vector(2500, 0, 0)
    polygon = App.ActiveDocument.addObject("Part::RegularPolygon",
                                           "RegularPolygon")
    polygon.Circumradius = 750
    polygon.Placement.Base = displacement

    App.ActiveDocument.recompute()

    if App.GuiUp:
        Gui.runCommand("Std_ViewFitAll")
        Gui.runCommand("Std_ViewIsometric")

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
