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
"""Provides the object code for the Rectangle object."""
## @package rectangle
# \ingroup draftobjects
# \brief Provides the object code for the Rectangle object.

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import DraftGeomUtils

from draftutils.utils import get_param
from draftobjects.base import DraftObject


class Rectangle(DraftObject):
    """The Rectangle object"""

    def __init__(self, obj):
        super(Rectangle, self).__init__(obj, "Rectangle")

        _tip = QT_TRANSLATE_NOOP("App::Property", "Length of the rectangle")
        obj.addProperty("App::PropertyDistance", "Length", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "Height of the rectangle")
        obj.addProperty("App::PropertyDistance", "Height", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "Radius to use to fillet the corners")
        obj.addProperty("App::PropertyLength", "FilletRadius", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "Size of the chamfer to give to the corners")
        obj.addProperty("App::PropertyLength", "ChamferSize", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "Create a face")
        obj.addProperty("App::PropertyBool", "MakeFace", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "Horizontal subdivisions of this rectangle")
        obj.addProperty("App::PropertyInteger", "Rows", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "Vertical subdivisions of this rectangle")
        obj.addProperty("App::PropertyInteger", "Columns", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "The area of this object")
        obj.addProperty("App::PropertyArea", "Area", "Draft", _tip)

        obj.MakeFace = get_param("fillmode",True)
        obj.Length=1
        obj.Height=1
        obj.Rows=1
        obj.Columns=1

    def execute(self, obj):
        """This method is run when the object is created or recomputed."""
        if self.props_changed_placement_only():
            obj.positionBySupport()
            self.props_changed_clear()
            return

        import Part

        if (obj.Length.Value == 0) or (obj.Height.Value == 0):
            obj.positionBySupport()
            return

        plm = obj.Placement

        shape = None

        if hasattr(obj,"Rows") and hasattr(obj,"Columns"):
            # TODO: verify if this is needed:
            if obj.Rows > 1:
                rows = obj.Rows
            else:
                rows = 1
            if obj.Columns > 1:
                columns = obj.Columns
            else:
                columns = 1
            # TODO: till here

            if (rows > 1) or (columns > 1):
                shapes = []
                l = obj.Length.Value/columns
                h = obj.Height.Value/rows
                for i in range(columns):
                    for j in range(rows):
                        p1 = App.Vector(i*l,j*h,0)
                        p2 = App.Vector(p1.x+l,p1.y,p1.z)
                        p3 = App.Vector(p1.x+l,p1.y+h,p1.z)
                        p4 = App.Vector(p1.x,p1.y+h,p1.z)
                        p = Part.makePolygon([p1,p2,p3,p4,p1])
                        if "ChamferSize" in obj.PropertiesList:
                            if obj.ChamferSize.Value != 0:
                                w = DraftGeomUtils.filletWire(p,
                                                              obj.ChamferSize.Value,
                                                              chamfer=True)
                                if w:
                                    p = w
                        if "FilletRadius" in obj.PropertiesList:
                            if obj.FilletRadius.Value != 0:
                                w = DraftGeomUtils.filletWire(p,
                                                              obj.FilletRadius.Value)
                                if w:
                                    p = w
                        if hasattr(obj,"MakeFace"):
                            if obj.MakeFace:
                                p = Part.Face(p)
                        shapes.append(p)
                if shapes:
                    shape = Part.makeCompound(shapes)

        if not shape:
            p1 = App.Vector(0,0,0)
            p2 = App.Vector(p1.x+obj.Length.Value,
                            p1.y,
                            p1.z)
            p3 = App.Vector(p1.x+obj.Length.Value,
                            p1.y+obj.Height.Value,
                            p1.z)
            p4 = App.Vector(p1.x,
                            p1.y+obj.Height.Value,
                            p1.z)
            shape = Part.makePolygon([p1, p2, p3, p4, p1])
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

        if hasattr(obj,"Area") and hasattr(shape,"Area"):
            obj.Area = shape.Area

        obj.Placement = plm
        obj.positionBySupport()
        self.props_changed_clear()

    def onChanged(self, obj, prop):
        self.props_changed_store(prop)


# Alias for compatibility with v0.18 and earlier
_Rectangle = Rectangle

## @}
