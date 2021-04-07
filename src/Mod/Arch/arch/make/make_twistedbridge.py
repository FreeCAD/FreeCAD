# ***************************************************************************
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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
"""Provides functions to create TwistedBridge objects.

The copies of frames will be placed along a path like a polyline,
spline, or bezier curve, and create a tunnel and walkway.
"""
## @package make_twistedbridge
# \ingroup archmake
# \brief Provides functions to create PathArray objects.

## \addtogroup archmake
# @{
import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftutils.messages import _msg, _err
from draftutils.translate import _tr
from arch.objects.twistedbridge import TwistedBridge

if App.GuiUp:
    from arch.viewproviders.view_twistedbridge import ViewProviderTwistedBridge


def make_twisted_bridge(base_object, path_object,
                        count=15, rot_factor=0.25,
                        width=100, thickness=10,
                        full_bridge=True):
    """Create a twisted bridge."""
    _name = "make_twisted_bridge"
    utils.print_header(_name, "Twisted bridge")

    found, doc = utils.find_doc(App.activeDocument())
    if not found:
        _err(_tr("No active document. Aborting."))
        return None

    if isinstance(base_object, str):
        base_object_str = base_object

    found, base_object = utils.find_object(base_object, doc)
    if not found:
        _msg("base_object: {}".format(base_object_str))
        _err(_tr("Wrong input: object not in document."))
        return None

    _msg("base_object: {}".format(base_object.Label))

    if isinstance(path_object, str):
        path_object_str = path_object

    found, path_object = utils.find_object(path_object, doc)
    if not found:
        _msg("path_object: {}".format(path_object_str))
        _err(_tr("Wrong input: object not in document."))
        return None

    _msg("path_object: {}".format(path_object.Label))

    _msg("count: {}".format(count))
    try:
        utils.type_check([(count, (int, float))],
                         name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a number."))
        return None
    count = int(count)

    _msg("rot_factor: {}".format(rot_factor))
    _msg("width: {}".format(width))
    _msg("thickness: {}".format(thickness))
    try:
        utils.type_check([(rot_factor, (int, float)),
                          (width, (int, float)),
                          (thickness, (int, float))],
                         name=_name)
    except TypeError:
        _err(_tr("Wrong input: must be a number."))
        return None

    full_bridge = bool(full_bridge)
    _msg("full_bridge: {}".format(full_bridge))

    new_obj = doc.addObject("Part::FeaturePython", "TwistedBridge")
    TwistedBridge(new_obj)

    new_obj.Base = base_object
    new_obj.PathObject = path_object
    new_obj.Count = count
    new_obj.RotationFactor = rot_factor
    new_obj.Width = width
    new_obj.Thickness = thickness
    new_obj.FullBridge = full_bridge

    if App.GuiUp:
        ViewProviderTwistedBridge(new_obj.ViewObject)
        gui_utils.formatObject(new_obj, new_obj.Base)

        new_obj.Base.ViewObject.hide()
        gui_utils.select(new_obj)

    return new_obj

## @}
