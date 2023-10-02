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
"""Provides support functions to edit Arch objects."""
## @package gui_edit_arch_objects
# \ingroup draftguitools
# \brief Provides support functions to edit Arch objects.

__title__ = "FreeCAD Draft Edit Tool"
__author__ = ("Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, "
              "Dmitry Chigrin, Carlo Pavan")
__url__ = "https://www.freecad.org"

## \addtogroup draftguitools
# @{
import math
import FreeCAD as App
import DraftVecUtils

from draftutils.translate import translate
import draftutils.utils as utils

from draftguitools.gui_edit_base_object import GuiTools



class ArchWallGuiTools(GuiTools):

    def __init__(self):
        pass

    def get_edit_points(self, obj):
        """Return the list of edipoints for the given Arch Wall object.

        0 : height of the wall
        1-to end : base object editpoints, in place with the wall
        """

        editpoints = []
        # height of the wall
        editpoints.append(App.Vector(0, 0, obj.Height))
        return editpoints

    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        '''            if node_idx == 0:
            edit_arch.updateWall(obj, node_idx, v)
        elif node_idx > 0:
            if obj.Base:
                if utils.get_type(obj.Base) in ["Wire", "Circle", "Rectangle",
                                                "Polygon", "Sketch"]:
                    self.update(obj.Base, node_idx - 1, v)'''
        if node_idx == 0:
            vz = DraftVecUtils.project(v, App.Vector(0, 0, 1))
            if vz.Length > 0:
                obj.Height = vz.Length


class ArchWindowGuiTools(GuiTools):

    def __init__(self):
        pass

    def get_edit_points(self, obj):
        editpoints = []
        pos = obj.Base.Placement.Base
        h = float(obj.Height) + pos.z
        normal = obj.Normal
        angle = normal.getAngle(App.Vector(1, 0, 0))
        editpoints.append(pos)
        editpoints.append(App.Vector(pos.x + float(obj.Width) * math.cos(angle-math.pi / 2.0),
                                                pos.y + float(obj.Width) * math.sin(angle-math.pi / 2.0),
                                                pos.z))
        editpoints.append(App.Vector(pos.x, pos.y, h))
        return editpoints

    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        pos = obj.Base.Placement.Base
        if node_idx == 0:
            obj.Base.Placement.Base = v
            obj.Base.recompute()
        if node_idx == 1:
            obj.Width = pos.sub(v).Length
            obj.Base.recompute()
        if node_idx == 2:
            obj.Height = pos.sub(v).Length
            obj.Base.recompute()
        for obj in obj.Hosts:
            obj.recompute()


class ArchStructureGuiTools(GuiTools):

    def __init__(self):
        pass

    def get_edit_points(self, obj):
        if obj.Nodes:
            editpoints = []
            for p in obj.Nodes:
                editpoints.append(p)
            return editpoints
        else:
            return None

    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        nodes = obj.Nodes
        nodes[node_idx] = v
        obj.Nodes = nodes

    def get_object_style(self, obj):
        return (obj.ViewObject.DisplayMode,
                obj.ViewObject.NodeSize,
                obj.ViewObject.ShowNodes)

    def set_object_editing_style(self, obj):
        obj.ViewObject.DisplayMode = "Wireframe"
        obj.ViewObject.NodeSize = 1
        obj.ViewObject.ShowNodes = True

    def restore_object_style(self, obj, modes):
        obj.ViewObject.DisplayMode = modes[0]
        obj.ViewObject.NodeSize = modes[1]
        obj.ViewObject.ShowNodes = modes[2]


class ArchSpaceGuiTools(GuiTools):

    def __init__(self):
        pass

    def get_edit_points(self, obj):
        try:
            editpoints = []
            editpoints.append(obj.ViewObject.Proxy.getTextPosition(obj.ViewObject))
            return editpoints
        except Exception:
            pass

    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        if node_idx == 0:
            obj.ViewObject.TextPosition = v


class ArchPanelCutGuiTools(GuiTools):

    def __init__(self):
        pass

    def get_edit_points(self, obj):
        editpoints = []
        if obj.TagPosition.Length == 0:
            pos = obj.Shape.BoundBox.Center
        else:
            pos = obj.TagPosition
        editpoints.append(pos)
        return editpoints

    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        if node_idx == 0:
            obj.TagPosition = v


class ArchPanelSheetGuiTools(GuiTools):

    def __init__(self):
        pass

    def get_edit_points(self, obj):
        editpoints = []
        editpoints.append(obj.TagPosition)
        for o in obj.Group:
            editpoints.append(o.Placement.Base)
        return editpoints

    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        if node_idx == 0:
            obj.TagPosition = v
        else:
            obj.Group[node_idx-1].Placement.Base = v

## @}
