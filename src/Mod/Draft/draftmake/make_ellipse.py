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
"""Provides functions to create Ellipse objects."""
## @package make_ellipse
# \ingroup draftmake
# \brief Provides functions to create Ellipse objects.

## \addtogroup draftmake
# @{
import FreeCAD as App
import draftutils.gui_utils as gui_utils

from draftobjects.ellipse import Ellipse

if App.GuiUp:
    from draftviewproviders.view_base import ViewProviderDraft


def make_ellipse(majradius, minradius, placement=None, face=None, support=None):
    """make_ellipse(majradius, minradius, [placement], [face], [support])

    Makes an ellipse with the given major and minor radius, and optionally
    a placement.

    Parameters
    ----------
    majradius :
        Major radius of the ellipse.

    minradius :
        Minor radius of the ellipse.

    placement : Base.Placement
        If a placement is given, it is used.

    face : Bool
        If face is False, the rectangle is shown as a wireframe,
        otherwise as a face.

    support :
        TODO: Describe.
    """

    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return

    obj = App.ActiveDocument.addObject("Part::Part2DObjectPython", "Ellipse")
    Ellipse(obj)

    if minradius > majradius:
        majradius, minradius = minradius, majradius
    obj.MajorRadius = majradius
    obj.MinorRadius = minradius
    obj.Support = support

    if face is not None:
        obj.MakeFace = face

    if placement:
        obj.Placement = placement

    if App.GuiUp:
        ViewProviderDraft(obj.ViewObject)
        gui_utils.format_object(obj)
        gui_utils.select(obj)

    return obj


makeEllipse = make_ellipse

## @}
