# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""This module provides the object code for Draft Polygon.
"""
## @package poligon
# \ingroup DRAFT
# \brief This module provides the object code for Draft Polygon.

import math

import FreeCAD as App

import Part

import DraftGeomUtils

from PySide.QtCore import QT_TRANSLATE_NOOP

from draftutils.utils import get_param

from draftobjects.base import DraftObject


class Polygon(DraftObject):
    """The Polygon object"""

    def __init__(self, obj):
        super(Polygon, self).__init__(obj, "Polygon")

        _tip = "Number of edges of the polygon"
        obj.addProperty("App::PropertyInteger", "Edges",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Radius of the control circle"
        obj.addProperty("App::PropertyLength", "Radius",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))
        
        _tip = "Relation between the polygon and its control circle"
        obj.addProperty("App::PropertyEnumeration", "DrawMode",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))
        
        _tip = "Fillet corners with this radius"
        obj.addProperty("App::PropertyLength", "FilletRadius",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))
        
        _tip = "Chamfer corners with this length"
        obj.addProperty("App::PropertyLength", "ChamferSize",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))
        
        _tip = "Create a face"
        obj.addProperty("App::PropertyBool", "MakeFace",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))
        
        _tip = "Area of the polygon"
        obj.addProperty("App::PropertyArea", "Area",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))
        
        obj.MakeFace = get_param("fillmode",True)
        obj.DrawMode = ['inscribed','circumscribed']
        obj.Edges = 0
        obj.Radius = 1

    def onDocumentRestored(self, obj):
        """Needed for backward compatibitlity"""
        if not hasattr(obj, "Edges"):
            _msg = "Upgrading " + obj.Name + " properties to FreeCAD v 0.19.\n"
            App.Console.PrintMessage(_msg)

            _tip = "Number of edges of the polygon"
            obj.addProperty("App::PropertyInteger", "Edges",
                            "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))
            if hasattr(obj, "FacesNumber"):
                obj.Edges = obj.FacesNumber
                obj.removeProperty("FacesNumber")

    def execute(self, obj):
        if (obj.Edges < 3) or (obj.Radius.Value <= 0):
            obj.positionBySupport()
            return

        import Part

        plm = obj.Placement

        angle = (math.pi * 2) / obj.Edges

        if obj.DrawMode == 'inscribed':
            delta = obj.Radius.Value
        else:
            delta = obj.Radius.Value / math.cos(angle / 2.0)

        pts = [App.Vector(delta, 0, 0)]

        for i in range(obj.Edges - 1):
            ang = (i + 1) * angle
            pts.append(App.Vector(delta * math.cos(ang),
                                    delta*math.sin(ang),
                                    0))
        pts.append(pts[0])
        shape = Part.makePolygon(pts)
        if "ChamferSize" in obj.PropertiesList:
            if obj.ChamferSize.Value != 0:
                w = DraftGeomUtils.filletWire(shape,
                                              obj.ChamferSize.Value,
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