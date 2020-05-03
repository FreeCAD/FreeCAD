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
"""This module provides the object code for Draft Ellipse.
"""
## @package ellipse
# \ingroup DRAFT
# \brief This module provides the object code for Draft Ellipse.

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App

from draftutils.utils import get_param

from draftobjects.base import DraftObject


class Ellipse(DraftObject):
    """The Circle object"""

    def __init__(self, obj):
        super(Ellipse, self).__init__(obj, "Ellipse")

        _tip = "Start angle of the elliptical arc"
        obj.addProperty("App::PropertyAngle", "FirstAngle",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "End angle of the elliptical arc \n\
                (for a full circle, give it same value as First Angle)"
        obj.addProperty("App::PropertyAngle", "LastAngle",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Minor radius of the ellipse"
        obj.addProperty("App::PropertyLength", "MinorRadius",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Major radius of the ellipse"
        obj.addProperty("App::PropertyLength", "MajorRadius",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Create a face"
        obj.addProperty("App::PropertyBool", "MakeFace",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Area of this object"
        obj.addProperty("App::PropertyArea", "Area",
                        "Draft",QT_TRANSLATE_NOOP("App::Property", _tip))

        obj.MakeFace = get_param("fillmode",True)

    def execute(self, obj):
        import Part
        plm = obj.Placement
        if obj.MajorRadius.Value < obj.MinorRadius.Value:
            _err = "Error: Major radius is smaller than the minor radius"
            App.Console.PrintMessage(QT_TRANSLATE_NOOP("Draft", _err))
            return
        if obj.MajorRadius.Value and obj.MinorRadius.Value:
            ell = Part.Ellipse(App.Vector(0, 0, 0),
                               obj.MajorRadius.Value,
                               obj.MinorRadius.Value)
            shape = ell.toShape()
            if hasattr(obj,"FirstAngle"):
                if obj.FirstAngle.Value != obj.LastAngle.Value:
                    a1 = obj.FirstAngle.getValueAs(App.Units.Radian)
                    a2 = obj.LastAngle.getValueAs(App.Units.Radian)
                    shape = Part.ArcOfEllipse(ell, a1, a2).toShape()
            shape = Part.Wire(shape)
            if shape.isClosed():
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


_Ellipse = Ellipse