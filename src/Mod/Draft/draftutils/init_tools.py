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
    from draftguitools import gui_arcs
    from draftguitools import gui_beziers
    arc_group = gui_arcs.ArcGroup
    bez_group = gui_beziers.BezierGroup

    return ["Draft_Line",
            "Draft_Wire",
            "Draft_Fillet",
            ([QT_TRANSLATE_NOOP("Workbench", "Arc tools")],
                list(arc_group.GetCommands(arc_group))), # tuple len=2: submenu
            ("Draft_ArcTools", ),                        # tuple len=1: toolbar flyout
            "Draft_Circle",
            "Draft_Ellipse",
            "Draft_Rectangle",
            "Draft_Polygon",
            "Draft_BSpline",
            ([QT_TRANSLATE_NOOP("Workbench", "Bézier tools")],
                list(bez_group.GetCommands(bez_group))),
            ("Draft_BezierTools", ),
            "Draft_Point",
            "Draft_Facebinder",
            "Draft_ShapeString",
            "Draft_Hatch"]


def get_draft_annotation_commands():
    """Return the annotation commands list."""
    return ["Draft_Text",
            "Draft_Dimension",
            "Draft_Label",
            "Draft_AnnotationStyleEditor"]


def get_draft_modification_commands():
    """Return the modification commands list."""
    from draftguitools import gui_arrays
    arr_group = gui_arrays.ArrayGroup

    return ["Draft_Move",
            "Draft_Rotate",
            "Draft_Scale",
            "Draft_Mirror",
            "Draft_Offset",
            "Draft_Trimex",
            "Draft_Stretch",
            "Separator",
            "Draft_Clone",
            ([QT_TRANSLATE_NOOP("Workbench", "Array tools")],
                list(arr_group.GetCommands(arr_group))), # tuple len=2: submenu
            ("Draft_ArrayTools", ),                      # tuple len=1: toolbar flyout
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
            "Draft_Shape2DView"]


def get_draft_utility_commands_menu():
    """Return the utility commands list for the menu."""
    return ["Draft_SetStyle",
            "Draft_ApplyStyle",
            "Separator",
            "Draft_Layer",
            "Draft_LayerManager",
            "Draft_AddNamedGroup",
            "Draft_AddToGroup",
            "Draft_SelectGroup",
            "Draft_ToggleConstructionMode",
            "Draft_AddConstruction",
            "Separator",
            "Draft_ToggleDisplayMode",
            "Draft_ToggleGrid",
            "Draft_SelectPlane",
            "Draft_WorkingPlaneProxy",
            "Separator",
            "Draft_Heal",
            "Draft_ToggleContinueMode",
            "Draft_ShowSnapBar"]


def get_draft_utility_commands_toolbar():
    """Return the utility commands list for the toolbar."""
    return ["Draft_LayerManager",
            "Draft_AddNamedGroup",
            "Draft_AddToGroup",
            "Draft_SelectGroup",
            "Draft_AddConstruction",
            "Draft_ToggleDisplayMode",
            "Draft_WorkingPlaneProxy"]


def get_draft_snap_commands():
    """Return the snapping commands list."""
    return ["Draft_Snap_Lock",
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
            # "Separator", # Removed: if the Python generated BIM snap toolbar
                           # is displayed in the Draft WB the separator appears
                           # after the last button. Can be reinstated when the
                           # BIM WB has a `normal` snap toolbar as well.
            "Draft_ToggleGrid"]


def get_draft_context_commands():
    """Return the context menu commands list."""
    return ["Draft_SetStyle",
            "Draft_ApplyStyle",
            "Separator",
            "Draft_Layer",
            "Draft_AddNamedGroup",
            "Draft_AddToGroup",
            "Draft_SelectGroup",
            "Draft_ToggleConstructionMode",
            "Draft_AddConstruction",
            "Separator",
            "Draft_ToggleDisplayMode",
            "Draft_ToggleGrid",
            "Draft_SelectPlane",
            "Draft_WorkingPlaneProxy"]


def init_toolbar(workbench, toolbar, cmd_list):
    """Initialize a toolbar.

    Parameters
    ----------
    workbench: Gui.Workbench
        The workbench. The commands from cmd_list must be available.

    toolbar: string
        The name of the toolbar.

    cmd_list: list of strings or list of strings and tuples
        See f.e. the return value of get_draft_drawing_commands.
    """
    for cmd in cmd_list:
        if isinstance(cmd, tuple):
            if len(cmd) == 1:
                workbench.appendToolbar(toolbar, [cmd[0]])
        else:
            workbench.appendToolbar(toolbar, [cmd])


def init_menu(workbench, menu_list, cmd_list):
    """Initialize a menu.

    Parameters
    ----------
    workbench: Gui.Workbench
        The workbench. The commands from cmd_list must be available.

    menu_list: list of strings
        The main and optional submenu(s). The commands, and additional
        submenus (if any), are added to the last (sub)menu in the list.

    cmd_list: list of strings or list of strings and tuples
        See f.e. the return value of get_draft_drawing_commands.
    """
    for cmd in cmd_list:
        if isinstance(cmd, tuple):
            if len(cmd) == 2:
                workbench.appendMenu(menu_list + cmd[0], cmd[1])
        else:
            workbench.appendMenu(menu_list, [cmd])

## @}
