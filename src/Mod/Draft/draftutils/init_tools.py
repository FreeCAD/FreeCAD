# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2021 FreeCAD Developers                                 *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Provides functions and lists of commands to set up Draft menus and toolbars."""

## @package init_tools
# \ingroup draftutils
# \brief Provides lists of commands to set up toolbars of the workbench.

## \addtogroup draftutils
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

from draftutils.translate import translate

# Comment out commands that aren't ready to be used


def get_draft_drawing_commands():
    """Return the drawing commands list."""
    return [
        "Draft_Line",
        "Draft_Wire",
        "Draft_Fillet",
        "Draft_ArcTools",
        "Draft_Circle",
        "Draft_Ellipse",
        "Draft_Rectangle",
        "Draft_Polygon",
        "Draft_BSpline",
        "Draft_BezierTools",
        "Draft_Point",
        "Draft_Facebinder",
        "Draft_ShapeString",
        "Draft_Hatch",
    ]


def get_draft_annotation_commands():
    """Return the annotation commands list."""
    # fmt: off
    return [
        "Draft_Text",
        "Draft_Dimension",
        "Draft_Label",
        "Draft_AnnotationStyleEditor",
    ]
    # fmt: on


def get_draft_modification_commands():
    """Return the modification commands list."""
    return [
        "Draft_Move",
        "Draft_Rotate",
        "Draft_Scale",
        "Draft_Mirror",
        "Draft_Offset",
        "Draft_Trimex",
        "Draft_Stretch",
        "Separator",
        "Draft_Clone",
        "Draft_ArrayTools",
        "Separator",
        "Draft_Edit",
        "Draft_SubelementHighlight",
        "Separator",
        "Draft_Join",
        "Draft_Split",
        "Draft_Upgrade",
        "Draft_Downgrade",
        "Separator",
        "Draft_WireToBSpline",
        "Draft_Draft2Sketch",
        "Draft_Slope",
        "Draft_FlipDimension",
        "Separator",
        "Draft_Shape2DView",
    ]


def get_draft_utility_commands_menu():
    """Return the utility commands list for the menu."""
    return [
        "Draft_SetStyle",
        "Draft_ApplyStyle",
        "Separator",
        "Draft_Layer",
        "Draft_LayerManager",
        "Draft_AddNamedGroup",
        "Draft_SelectGroup",
        "Draft_ToggleConstructionMode",
        "Separator",
        "Draft_AddToLayer",
        "Draft_AddToGroup",
        "Draft_AddConstruction",
        "Separator",
        "Draft_ToggleDisplayMode",
        "Draft_ToggleGrid",
        "Draft_SelectPlane",
        "Draft_WorkingPlaneProxy",
        "Separator",
        "Draft_Heal",
        "Draft_ShowSnapBar",
    ]


def get_draft_utility_commands_toolbar():
    """Return the utility commands list for the toolbar."""
    return [
        "Draft_LayerManager",
        "Draft_AddNamedGroup",
        "Draft_SelectGroup",
        "Draft_AddToLayer",
        "Draft_AddToGroup",
        "Draft_AddConstruction",
        "Draft_ToggleDisplayMode",
        "Draft_WorkingPlaneProxy",
    ]


def get_draft_snap_commands():
    """Return the snapping commands list."""
    return [
        "Draft_Snap_Lock",
        "Draft_Snap_Endpoint",
        "Draft_Snap_Midpoint",
        "Draft_Snap_Center",
        "Draft_Snap_Angle",
        "Draft_Snap_Intersection",
        "Draft_Snap_Perpendicular",
        "Draft_Snap_Extension",
        "Draft_Snap_Parallel",
        "Draft_Snap_Special",
        "Draft_Snap_Near",
        "Draft_Snap_Ortho",
        "Draft_Snap_Grid",
        "Draft_Snap_WorkingPlane",
        "Draft_Snap_Dimensions",
        "Separator",
        "Draft_ToggleGrid",
    ]


def get_draft_context_commands():
    """Return the context menu commands list."""
    return [
        "Draft_SetStyle",
        "Draft_ApplyStyle",
        "Separator",
        "Draft_Layer",
        "Draft_LayerManager",
        "Draft_AddNamedGroup",
        "Draft_SelectGroup",
        "Draft_ToggleConstructionMode",
        "Separator",
        "Draft_AddToLayer",
        "Draft_AddToGroup",
        "Draft_AddConstruction",
        "Separator",
        "Draft_ToggleDisplayMode",
        "Draft_ToggleGrid",
        "Draft_SelectPlane",
        "Draft_WorkingPlaneProxy",
    ]


## @}
