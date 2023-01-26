# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
"""Provides functions to create Point objects."""
## @package make_point
# \ingroup draftmake
# \brief Provides functions to create Point objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.gui_utils as gui_utils

from draftobjects.point import Point

if App.GuiUp:
    import FreeCADGui as Gui
    from draftviewproviders.view_point import ViewProviderPoint


def make_point(X=0, Y=0, Z=0, color=None, name="Point", point_size=5):
    """ make_point(x, y, z, [color(r, g, b), point_size]) or
        make_point(Vector, color(r, g, b), point_size])

    Creates a Draft Point in the current document.

    Parameters
    ----------
    X :
        float -> X coordinate of the point
        Base.Vector -> Ignore Y and Z coordinates and create the point
            from the vector.

    Y : float
        Y coordinate of the point

    Z : float
        Z coordinate of the point

    color : (R, G, B)
        Point color as RGB
        example to create a colored point:
        make_point(0, 0, 0, (1, 0, 0)) # color = red
        example to change the color, make sure values are floats:
        p1.ViewObject.PointColor = (0.0, 0.0, 1.0)
    """
    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return

    obj = App.ActiveDocument.addObject("Part::FeaturePython", name)

    if isinstance(X, App.Vector):
        Z = X.z
        Y = X.y
        X = X.x

    Point(obj, X, Y, Z)
    obj.Placement.Base = App.Vector(X, Y, Z)

    if App.GuiUp:
        ViewProviderPoint(obj.ViewObject)
        if hasattr(Gui,"draftToolBar") and color is None:
            color = Gui.draftToolBar.getDefaultColor("line")
        if color is not None:
            obj.ViewObject.PointColor = (float(color[0]), float(color[1]), float(color[2]))
        obj.ViewObject.PointSize = point_size
        gui_utils.select(obj)

    return obj


makePoint = make_point

## @}
