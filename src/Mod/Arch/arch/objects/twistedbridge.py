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
"""Provides the object code for the TwistedBridge object.

See https://forum.freecadweb.org/viewtopic.php?f=23&t=49617

A `twisted bridge` would consist of three parts:
 1. The ribcage composed of a twisted array generated from a frame
    and a path.
 2. The `tunnel` object produced by lofting or sweeping the internal twisted
    profiles of the ribcage along the path.
 3. The `walkway` object on which the person can stand; it is generated
    from the path, and the internal width of the ribcage profile.

See the array code as well in `draftobjects/pathtwistedarray.py`.
"""
## @package twistedbridge
# \ingroup archobjects
# \brief Provides the object code for the TwistedBridge object.

import draftgeoutils.geo_arrays as geo
from draftutils.translate import _tr

from draftobjects.base import DraftObject

## \addtogroup draftobjects
# @{


class TwistedBridge(DraftObject):
    """The PathTwistedArray object.

    This array distributes copies of an object along a path like a polyline,
    spline, or bezier curve, and the copies are twisted around the path
    by a given rotation parameter.
    Then a tunnel is created, and a walkway is also extruded from the path
    and the width of the tunnel.
    """

    def __init__(self, obj):
        super(TwistedBridge, self).__init__(obj, "TwistedBridge")
        self.set_properties(obj)
        obj.Proxy = self

    def set_properties(self, obj):
        """Set properties only if they don't exist."""
        if hasattr(obj, "PropertiesList"):
            properties = obj.PropertiesList
        else:
            properties = []

        if "Base" not in properties:
            _tip = _tr("The base object that will be duplicated.")
            obj.addProperty("App::PropertyLink",
                            "Base",
                            "Objects",
                            _tip)
            obj.Base = None

        if "PathObject" not in properties:
            _tip = _tr("The object along which "
                       "the copies will be distributed. "
                       "It must contain 'Edges'.")
            obj.addProperty("App::PropertyLink",
                            "PathObject",
                            "Objects",
                            _tip)
            obj.PathObject = None

        if "Count" not in properties:
            _tip = _tr("Number of copies to create.")
            obj.addProperty("App::PropertyInteger",
                            "Count",
                            "Objects",
                            _tip)
            obj.Count = 15

        if "RotationFactor" not in properties:
            _tip = _tr("Rotation factor of the twisted array.")
            obj.addProperty("App::PropertyFloat",
                            "RotationFactor",
                            "Objects",
                            _tip)
            obj.RotationFactor = 0.25

        if "Width" not in properties:
            _tip = _tr("Width of the walkway.")
            obj.addProperty("App::PropertyLength",
                            "Width",
                            "Walkway",
                            _tip)
            obj.Width = 100

        if "Thickness" not in properties:
            _tip = _tr("Thickness of the walkway.")
            obj.addProperty("App::PropertyLength",
                            "Thickness",
                            "Walkway",
                            _tip)
            obj.Thickness = 10

        if "FullBridge" not in properties:
            _tip = _tr("If True, it will build the entire bridge, including "
                       "tunnel and walkway.\n"
                       "Otherwise only the twisted array elements "
                       "will be produced.")
            obj.addProperty("App::PropertyBool",
                            "FullBridge",
                            "Objects",
                            _tip)
            obj.FullBridge = True

    def execute(self, obj):
        """Execute when the object is created or recomputed."""
        base = obj.Base
        path = obj.PathObject
        count = obj.Count
        rot_factor = obj.RotationFactor
        width = obj.Width
        thickness = obj.Thickness
        bridge = obj.FullBridge

        if not bridge:
            shape = geo.get_twisted_array_shape(base, path,
                                                count=count,
                                                rot_factor=rot_factor)
        else:
            shape = geo.get_twisted_bridge_shape(base, path,
                                                 count=count,
                                                 rot_factor=rot_factor,
                                                 width=width,
                                                 thickness=thickness)

        obj.Shape = shape

## @}
