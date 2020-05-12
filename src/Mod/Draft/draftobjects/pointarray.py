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
"""This module provides the object code for the Draft PointArray object.
"""
## @package pointarray
# \ingroup DRAFT
# \brief This module provides the object code for the Draft PointArray object.

import math

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import DraftVecUtils

import draftutils.utils as utils

from draftobjects.base import DraftObject


class PointArray(DraftObject):
    """The Draft Point Array object"""

    def __init__(self, obj, bobj, ptlst):
        super(PointArray, self).__init__(obj, "PointArray")
        
        _tip = "Base object"
        obj.addProperty("App::PropertyLink", "Base",
                        "Objects", QT_TRANSLATE_NOOP("App::Property", _tip))
        
        _tip = "List of points used to distribute the base object"
        obj.addProperty("App::PropertyLink", "PointList",
                        "Objects", QT_TRANSLATE_NOOP("App::Property", _tip))
        
        _tip = "Number of copies" # TODO: verify description of the tooltip
        obj.addProperty("App::PropertyInteger", "Count",
                        "Parameters", QT_TRANSLATE_NOOP("App::Property", _tip))

        obj.Base = bobj
        obj.PointList = ptlst
        obj.Count = 0

        obj.setEditorMode("Count", 1)

    def execute(self, obj):
        import Part
        pls = []
        opl = obj.PointList
        while utils.get_type(opl) == 'Clone':
            opl = opl.Objects[0]
        if hasattr(opl, 'Geometry'):
            place = opl.Placement
            for pts in opl.Geometry:
                if hasattr(pts, 'X') and hasattr(pts, 'Y') and hasattr(pts, 'Z'):
                    pn = pts.copy()
                    pn.translate(place.Base)
                    pn.rotate(place)
                    pls.append(pn)
        elif hasattr(opl, 'Links'):
            pls = opl.Links
        elif hasattr(opl, 'Components'):
            pls = opl.Components

        base = []
        i = 0
        if hasattr(obj.Base, 'Shape'):
            for pts in pls:
                #print pts # inspect the objects
                if hasattr(pts, 'X') and hasattr(pts, 'Y') and hasattr(pts, 'Z'):
                    nshape = obj.Base.Shape.copy()
                    if hasattr(pts, 'Placement'):
                        place = pts.Placement
                        nshape.translate(place.Base)
                        nshape.rotate(place.Base, place.Rotation.Axis, place.Rotation.Angle * 180 /  math.pi )
                    else:
                        nshape.translate(App.Vector(pts.X,pts.Y,pts.Z))
                    i += 1
                    base.append(nshape)
        obj.Count = i
        if i > 0:
            obj.Shape = Part.makeCompound(base)
        else:
            App.Console.PrintError(QT_TRANSLATE_NOOP("draft","No point found\n"))
            obj.Shape = obj.Base.Shape.copy()


_PointArray = PointArray
