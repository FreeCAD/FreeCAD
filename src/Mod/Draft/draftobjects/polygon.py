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
"""Provides the object code for the Polygon object."""
## @package polygon
# \ingroup draftobjects
# \brief Provides the object code for the Polygon object.

## \addtogroup draftobjects
# @{
import math
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import DraftGeomUtils

from draftutils.utils import get_param
from draftobjects.base import DraftObject


class Polygon(DraftObject):
    """The Polygon object"""

    def __init__(self, obj):
        super(Polygon, self).__init__(obj, "Polygon")

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Number of faces")
        obj.addProperty("App::PropertyInteger", "FacesNumber", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Radius of the control circle")
        obj.addProperty("App::PropertyLength", "Radius", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "How the polygon must be drawn from the control circle")
        obj.addProperty("App::PropertyEnumeration", "DrawMode", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Radius to use to fillet the corners")
        obj.addProperty("App::PropertyLength", "FilletRadius", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Size of the chamfer to give to the corners")
        obj.addProperty("App::PropertyLength", "ChamferSize", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "Create a face")
        obj.addProperty("App::PropertyBool", "MakeFace", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The area of this object")
        obj.addProperty("App::PropertyArea", "Area", "Draft", _tip)

        obj.MakeFace = get_param("fillmode",True)
        obj.DrawMode = ['inscribed','circumscribed']
        obj.FacesNumber = 0
        obj.Radius = 1

    def execute(self, obj):
        if self.props_changed_placement_only():
            obj.positionBySupport()
            self.props_changed_clear()
            return

        if (obj.FacesNumber >= 3) and (obj.Radius.Value > 0):
            import Part
            plm = obj.Placement
            angle = (math.pi * 2) / obj.FacesNumber
            if obj.DrawMode == 'inscribed':
                delta = obj.Radius.Value
            else:
                delta = obj.Radius.Value / math.cos(angle / 2.0)
            pts = [App.Vector(delta, 0, 0)]
            for i in range(obj.FacesNumber - 1):
                ang = (i + 1) * angle
                pts.append(App.Vector(delta * math.cos(ang),
                                      delta*math.sin(ang),
                                      0))
            pts.append(pts[0])
            shape = Part.makePolygon(pts)
            if "ChamferSize" in obj.PropertiesList:
                if obj.ChamferSize.Value != 0:
                    w = DraftGeomUtils.filletWire(shape,obj.ChamferSize.Value,
                                                  chamfer=True)
                    if w:
                        shape = w
            if "FilletRadius" in obj.PropertiesList:
                if obj.FilletRadius.Value != 0:
                    w = DraftGeomUtils.filletWire(shape,
                                                  obj.FilletRadius.Value)
                    if w:
                        shape = w
            if hasattr(obj,"MakeFace"):
                if obj.MakeFace:
                    shape = Part.Face(shape)
            else:
                shape = Part.Face(shape)
            obj.Shape = shape
            if hasattr(obj, "Area") and hasattr(shape, "Area"):
                obj.Area = shape.Area
            obj.Placement = plm
        obj.positionBySupport()
        self.props_changed_clear()

    def onChanged(self, obj, prop):
        self.props_changed_store(prop)


# Alias for compatibility with v0.18 and earlier
_Polygon = Polygon

## @}
