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
__url__ = "https://www.freecadweb.org"

## \addtogroup draftguitools
# @{
import math
import FreeCAD as App
import DraftVecUtils

from draftutils.translate import translate
import draftutils.utils as utils


def get_supported_arch_objects():
    return ["Wall", "Window", "Structure", "Space", "PanelCut", "PanelSheet"]


# WALL---------------------------------------------------------------------

def getWallPts(obj):
    """Return the list of edipoints for the given Arch Wall object.

    0 : height of the wall
    1-to end : base object editpoints, in place with the wall
    """
    editpoints = []
    # height of the wall
    editpoints.append(App.Vector(0, 0, obj.Height))
    return editpoints


def updateWall(obj, nodeIndex, v):
    if nodeIndex == 0:
        vz = DraftVecUtils.project(v, App.Vector(0, 0, 1))
        if vz.Length > 0:
            obj.Height = vz.Length
    obj.recompute()


# WINDOW-------------------------------------------------------------------

def getWindowPts(obj):
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


def updateWindow(obj, nodeIndex, v):
    pos = obj.Base.Placement.Base
    if nodeIndex == 0:
        obj.Base.Placement.Base = v
        obj.Base.recompute()
    if nodeIndex == 1:
        obj.Width = pos.sub(v).Length
        obj.Base.recompute()
    if nodeIndex == 2:
        obj.Height = pos.sub(v).Length
        obj.Base.recompute()
    for obj in obj.Hosts:
        obj.recompute()
    obj.recompute()


# STRUCTURE----------------------------------------------------------------
def get_structure_format(obj):
    return (obj.ViewObject.DisplayMode,
            obj.ViewObject.NodeSize,
            obj.ViewObject.ShowNodes)

def set_structure_editing_format(obj):
    obj.ViewObject.DisplayMode = "Wireframe"
    obj.ViewObject.NodeSize = 1
    obj.ViewObject.ShowNodes = True

def restore_structure_format(obj, modes):
    obj.ViewObject.DisplayMode = modes[0]
    obj.ViewObject.NodeSize = modes[1]
    obj.ViewObject.ShowNodes = modes[2]

def getStructurePts(obj):
    if obj.Nodes:
        editpoints = []
        for p in obj.Nodes:
            editpoints.append(p)
        return editpoints
    else:
        return None


def updateStructure(obj, nodeIndex, v):
    nodes = obj.Nodes
    nodes[nodeIndex] = v
    obj.Nodes = nodes


# SPACE--------------------------------------------------------------------

def getSpacePts(obj):
    try:
        editpoints = []
        editpoints.append(obj.ViewObject.Proxy.getTextPosition(obj.ViewObject))
        return editpoints
    except:
        pass


def updateSpace(obj, nodeIndex, v):
    if nodeIndex == 0:
        obj.ViewObject.TextPosition = v


# PANELS-------------------------------------------------------------------

def getPanelCutPts(obj):
    editpoints = []
    if obj.TagPosition.Length == 0:
        pos = obj.Shape.BoundBox.Center
    else:
        pos = obj.TagPosition
    editpoints.append(pos)
    return editpoints


def updatePanelCut(obj, nodeIndex, v):
    if nodeIndex == 0:
        obj.TagPosition = v


def getPanelSheetPts(obj):
    editpoints = []
    editpoints.append(obj.TagPosition)
    for o in obj.Group:
        editpoints.append(o.Placement.Base)
    return editpoints


def updatePanelSheet(obj, nodeIndex, v):
    if nodeIndex == 0:
        obj.TagPosition = v
    else:
        obj.Group[nodeIndex-1].Placement.Base = v

## @}
