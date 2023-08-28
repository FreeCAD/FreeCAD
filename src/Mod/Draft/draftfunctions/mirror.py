# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 Carlo Pavan <carlopav@gmail.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Provides functions to produce a mirrored object.

It just creates a `Part::Mirroring` object, and sets the appropriate
`Source` and `Normal` properties.
"""
## @package mirror
# \ingroup draftfunctions
# \brief Provides functions to produce a mirrored object.

## \addtogroup draftfunctions
# @{
import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftutils.messages import _err
from draftutils.translate import translate

if App.GuiUp:
    import FreeCADGui as Gui


def mirror(objlist, p1, p2):
    """Create a mirror object from the provided list and line.

    It creates a `Part::Mirroring` object from the given `objlist` using
    a plane that is defined by the two given points `p1` and `p2`,
    and either

    - the Draft working plane normal, or
    - the negative normal provided by the camera direction
      if the working plane normal does not exist and the graphical interface
      is available.

    If neither of these two is available, it uses as normal the +Z vector.

    Parameters
    ----------
    objlist: single object or a list of objects
        A single object or a list of objects.

    p1: Base::Vector3
        Point 1 of the mirror plane. It is also used as the `Placement.Base`
        of the resulting object.

    p2: Base::Vector3
        Point 1 of the mirror plane.

    Returns
    -------
    None
        If the operation fails.

    list
        List of `Part::Mirroring` objects, or a single one
        depending on the input `objlist`.

    To Do
    -----
    Implement a mirror tool specific to the workbench that does not
    just use `Part::Mirroring`. It should create a derived object,
    that is, it should work similar to `Draft.offset`.
    """
    utils.print_header('mirror', "Create mirror")

    if not objlist:
        _err(translate("draft","No object given"))
        return

    if p1 == p2:
        _err(translate("draft","The two points are coincident"))
        return

    if not isinstance(objlist, list):
        objlist = [objlist]

    if hasattr(App, "DraftWorkingPlane"):
        norm = App.DraftWorkingPlane.getNormal()
    elif App.GuiUp:
        norm = Gui.ActiveDocument.ActiveView.getViewDirection().negative()
    else:
        norm = App.Vector(0, 0, 1)

    pnorm = p2.sub(p1).cross(norm).normalize()

    result = []

    for obj in objlist:
        mir = App.ActiveDocument.addObject("Part::Mirroring", "Mirror")
        mir.Label = obj.Label + " (" + translate("draft", "mirrored") + ")"
        mir.Source = obj
        mir.Base = p1
        mir.Normal = pnorm
        gui_utils.format_object(mir, obj)
        result.append(mir)

    if len(result) == 1:
        result = result[0]
        gui_utils.select(result)

    return result

## @}
