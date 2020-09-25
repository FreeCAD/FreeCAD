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
"""Provides the object code for the DrawingView object (OBSOLETE).

This module is obsolete, since the Drawing Workbench stopped
being developed in v0.17.
The TechDraw Workbench replaces Drawing, and it no longer requires
a `DrawingView` object to display objects in a drawing sheet.

This module is still provided in order to be able to open older files
that use this `DrawingView` object. However, a GUI tool to create
this object should no longer be available.
"""
## @package drawingview
# \ingroup draftobjects
# \brief Provides the object code for the DrawingView object (OBSOLETE).

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import draftfunctions.svg as get_svg
import draftfunctions.dxf as get_dxf
import draftutils.utils as utils
import draftutils.groups as groups

from draftobjects.base import DraftObject


class DrawingView(DraftObject):
    """The DrawingView object. This class is OBSOLETE.

    This object was used with the Drawing Workbench, but since this workbench
    because obsolete in v0.17, the object should no longer be used.
    It is retained for compatibility purposes, that is, to open older
    files that may contain this object.

    To produce 2D drawings, use TechDraw Workbench.
    """

    def __init__(self, obj):
        super(DrawingView, self).__init__(obj, "DrawingView")

        _tip = QT_TRANSLATE_NOOP("App::Property",
                                 "The linked object")
        obj.addProperty("App::PropertyLink",
                        "Source",
                        "Base",
                        _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                                 "Projection direction")
        obj.addProperty("App::PropertyVector",
                        "Direction",
                        "Shape View",
                        _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                                 "The width of the lines inside this object")
        obj.addProperty("App::PropertyFloat",
                        "LineWidth",
                        "View Style",
                        _tip)
        obj.LineWidth = 0.35

        _tip = QT_TRANSLATE_NOOP("App::Property",
                                 "The size of the texts inside this object")
        obj.addProperty("App::PropertyLength",
                        "FontSize",
                        "View Style",
                        _tip)
        obj.FontSize = 12

        _tip = QT_TRANSLATE_NOOP("App::Property",
                                 "The spacing between lines of text")
        obj.addProperty("App::PropertyLength",
                        "LineSpacing",
                        "View Style",
                        _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                                 "The color of the projected objects")
        obj.addProperty("App::PropertyColor",
                        "LineColor",
                        "View Style",
                        _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property",
                                 "Shape Fill Style")
        obj.addProperty("App::PropertyEnumeration",
                        "FillStyle",
                        "View Style",
                        _tip)
        obj.FillStyle = ['shape color'] + list(utils.svgpatterns().keys())

        _tip = QT_TRANSLATE_NOOP("App::Property",
                                 "Line Style")
        obj.addProperty("App::PropertyEnumeration",
                        "LineStyle",
                        "View Style",
                        _tip)
        obj.LineStyle = ['Solid', 'Dashed', 'Dotted', 'Dashdot']

        _tip = QT_TRANSLATE_NOOP("App::Property",
                                 "If checked, source objects are displayed "
                                 "regardless of being visible in the 3D model")
        obj.addProperty("App::PropertyBool",
                        "AlwaysOn",
                        "View Style",
                        _tip)

    def execute(self, obj):
        """Execute when the object is created or recomputed."""
        result = ""
        if hasattr(obj, "Source") and obj.Source:
            if hasattr(obj, "LineStyle"):
                ls = obj.LineStyle
            else:
                ls = None
            if hasattr(obj, "LineColor"):
                lc = obj.LineColor
            else:
                lc = None
            if hasattr(obj, "LineSpacing"):
                lp = obj.LineSpacing
            else:
                lp = None

            if obj.Source.isDerivedFrom("App::DocumentObjectGroup"):
                svg = ""
                objs = groups.get_group_contents([obj.Source])
                for o in objs:
                    v = o.ViewObject.isVisible()
                    if hasattr(obj, "AlwaysOn") and obj.AlwaysOn:
                        v = True
                    if v:
                        svg += get_svg.get_svg(o,
                                               obj.Scale,
                                               obj.LineWidth,
                                               obj.FontSize.Value,
                                               obj.FillStyle,
                                               obj.Direction, ls, lc, lp)
            else:
                svg = get_svg.get_svg(obj.Source,
                                      obj.Scale,
                                      obj.LineWidth,
                                      obj.FontSize.Value,
                                      obj.FillStyle,
                                      obj.Direction, ls, lc, lp)

            result += '<g id="' + obj.Name + '"'
            result += ' transform="'
            result += 'rotate(' + str(obj.Rotation) + ','
            result += str(obj.X) + ',' + str(obj.Y)
            result += ') '
            result += 'translate(' + str(obj.X) + ',' + str(obj.Y) + ') '
            result += 'scale(' + str(obj.Scale) + ',' + str(-obj.Scale)
            result += ')'
            result += '">'
            result += svg
            result += '</g>'
        obj.ViewResult = result

    def getDXF(self, obj):
        """Return a DXF fragment."""
        return get_dxf.get_dxf(obj)


# Alias for compatibility with v0.18 and earlier
_DrawingView = DrawingView

## @}
