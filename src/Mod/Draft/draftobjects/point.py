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
"""This module provides the object code for Draft Point.
"""
## @package point
# \ingroup DRAFT
# \brief This module provides the object code for Draft Point.

import math

import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

from draftutils.utils import get_param

from draftobjects.base import DraftObject


class Point(DraftObject):
    """The Draft Point object.
    """
    def __init__(self, obj, x=0, y=0, z=0):
        super(Point, self).__init__(obj, "Point")

        obj.addProperty("App::PropertyDistance", 
                        "X",
                        "Draft",
                        QT_TRANSLATE_NOOP("App::Property","X Location"))

        obj.addProperty("App::PropertyDistance",
                        "Y",
                        "Draft",
                        QT_TRANSLATE_NOOP("App::Property","Y Location"))

        obj.addProperty("App::PropertyDistance",
                        "Z",
                        "Draft", 
                        QT_TRANSLATE_NOOP("App::Property","Z Location"))
        
        obj.X = x
        obj.Y = y
        obj.Z = z

        mode = 2
        obj.setEditorMode('Placement',mode)

    def execute(self, obj):
        import Part
        shape = Part.Vertex(App.Vector(0, 0, 0))
        obj.Shape = shape
        obj.Placement.Base = App.Vector(obj.X.Value,
                                        obj.Y.Value,
                                        obj.Z.Value)


_Point = Point