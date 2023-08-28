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
"""Provides the object code for the Circle object."""
## @package circle
# \ingroup draftobjects
# \brief Provides the object code for the Circle object.

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import draftutils.utils as utils

from draftobjects.base import DraftObject


class Circle(DraftObject):
    """The Circle object"""

    def __init__(self, obj):
        super(Circle, self).__init__(obj, "Circle")

        _tip = QT_TRANSLATE_NOOP("App::Property", "Start angle of the arc")
        obj.addProperty("App::PropertyAngle", "FirstAngle",
                        "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "End angle of the arc (for a full circle, \
                give it same value as First Angle)")
        obj.addProperty("App::PropertyAngle","LastAngle",
                        "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "Radius of the circle")
        obj.addProperty("App::PropertyLength", "Radius",
                        "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "Create a face")
        obj.addProperty("App::PropertyBool", "MakeFace",
                        "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "The area of this object")
        obj.addProperty("App::PropertyArea", "Area",
                        "Draft", _tip)

        obj.MakeFace = utils.get_param("fillmode", True)

    def execute(self, obj):
        """This method is run when the object is created or recomputed."""
        if self.props_changed_placement_only():
            obj.positionBySupport()
            self.props_changed_clear()
            return

        import Part

        plm = obj.Placement

        shape = Part.makeCircle(obj.Radius.Value,
                                App.Vector(0,0,0),
                                App.Vector(0,0,1),
                                obj.FirstAngle.Value,
                                obj.LastAngle.Value)

        if obj.FirstAngle.Value == obj.LastAngle.Value:
            shape = Part.Wire(shape)
            if getattr(obj,"MakeFace",True):
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
_Circle = Circle

## @}
