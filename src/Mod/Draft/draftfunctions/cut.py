# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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
"""Provides functions to create a cut object from two objects."""
## @package cut
# \ingroup draftfunctions
# \brief Provides functions to create a cut object from two objects.

## \addtogroup draftfunctions
# @{
import FreeCAD as App
import draftutils.gui_utils as gui_utils

from draftutils.translate import translate
from draftutils.messages import _err


def cut(object1, object2):
    """Return a cut object made from the difference of the 2 given objects.

    Parameters
    ----------
    object1: Part::Feature
        Any object with a `Part::TopoShape`.

    object2: Part::Feature
        Any object with a `Part::TopoShape`.

    Returns
    -------
    Part::Cut
        The resulting cut object.

    None
        If there is a problem and the new object can't be created.
    """
    if not App.activeDocument():
        _err(translate("draft","No active document. Aborting."))
        return

    obj = App.activeDocument().addObject("Part::Cut", "Cut")
    obj.Base = object1
    obj.Tool = object2

    if App.GuiUp:
        gui_utils.format_object(obj, object1)
        gui_utils.select(obj)
        object1.ViewObject.Visibility = False
        object2.ViewObject.Visibility = False

    return obj

## @}
