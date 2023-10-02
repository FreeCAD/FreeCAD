# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2019, 2020 Carlo Pavan <carlopav@gmail.com>             *
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
"""Provides support functions to edit Sketch objects."""
## @package gui_edit_sketcher_objects
# \ingroup draftguitools
# \brief Provides support functions to edit Sketch objects.

__title__ = "FreeCAD Draft Edit Tool"
__author__ = ("Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, "
              "Dmitry Chigrin, Carlo Pavan")
__url__ = "https://www.freecad.org"

## \addtogroup draftguitools
# @{
import FreeCAD as App
from draftutils.translate import translate

from draftguitools.gui_edit_base_object import GuiTools


class SketcherSketchObjectGuiTools(GuiTools):

    def __init__(self):
        pass

    def get_edit_points(self, obj):
        """Return the list of edipoints for the given single line sketch.
        (WallTrace)
        0 : startpoint
        1 : endpoint
        """
        import Part
        editpoints = []
        if (obj.ConstraintCount == 0
                and obj.GeometryCount == 1
                and type(obj.Geometry[0]) == Part.LineSegment):
            editpoints.append(obj.getPoint(0,1))
            editpoints.append(obj.getPoint(0,2))
            return editpoints
        else:
            _wrn = translate("draft", "Sketch is too complex to edit: "
                                    "it is suggested to use sketcher default editor")
            App.Console.PrintWarning(_wrn + "\n")
            return None

    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        """Move a single line sketch vertex a certain displacement.

        (single segment sketch object, node index as Int, App.Vector)
        move a single line sketch (WallTrace) vertex according to a given App.Vector
        0 : startpoint
        1 : endpoint
        """
        line = obj.Geometry[0]
        if node_idx == 0:
            line.StartPoint = v
        elif node_idx == 1:
            line.EndPoint = v
        obj.Geometry = [line]
        obj.recompute()

## @}
