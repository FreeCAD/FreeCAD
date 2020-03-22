"""Provides lists of commands for the Draft Workbench.

This module returns lists of commands, so that the toolbars
can be initialized by Draft, and by other workbenches.
These commands should be defined in `DraftTools`, and in the individual
modules in `draftguitools`.
"""
## @package init_tools
# \ingroup DRAFT
# \brief This module provides lists of commands for the Draft Workbench.

# ***************************************************************************
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
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

from PySide.QtCore import QT_TRANSLATE_NOOP

# Comment out commands that aren't ready to be used


def get_draft_drawing_commands():
    """Return the drawing commands list."""
    return ["Draft_Line", "Draft_Wire",  # "Draft_Fillet",
            "Draft_ArcTools",
            "Draft_Circle", "Draft_Ellipse", "Draft_Rectangle",
            "Draft_Polygon", "Draft_BSpline", "Draft_BezierTools",
            "Draft_Point", "Draft_Facebinder"]


def get_draft_annotation_commands():
    """Return the annotation commands list."""
    return ["Draft_Text", "Draft_ShapeString", "Draft_Dimension",
            "Draft_Label"]


def get_draft_array_commands():
    """Return the array commands list."""
    return ["Draft_ArrayTools"]


def get_draft_modification_commands():
    """Return the modification commands list."""
    lst = ["Draft_Move", "Draft_Rotate", "Draft_Offset",
           "Draft_Trimex", "Draft_Join", "Draft_Split",
           "Draft_Upgrade", "Draft_Downgrade", "Draft_Scale",
           "Draft_Edit", "Draft_SubelementHighlight",
           "Draft_WireToBSpline", "Draft_Draft2Sketch",
           "Draft_Shape2DView"]
    lst += get_draft_array_commands()
    lst += ["Draft_Clone",
            "Draft_Drawing", "Draft_Mirror", "Draft_Stretch"]
    return lst


def get_draft_context_commands():
    """Return the context menu commands list."""
    return ["Draft_ApplyStyle", "Draft_ToggleDisplayMode",
            "Draft_AddToGroup", "Draft_SelectGroup",
            "Draft_SelectPlane", "Draft_ShowSnapBar",
            "Draft_ToggleGrid", "Draft_AutoGroup"]


def get_draft_line_commands():
    """Return the line commands list."""
    return ["Draft_UndoLine", "Draft_FinishLine",
            "Draft_CloseLine"]


def get_draft_utility_commands():
    """Return the utility commands list."""
    return ["Draft_Layer", "Draft_Heal", "Draft_FlipDimension",
            "Draft_ToggleConstructionMode",
            "Draft_ToggleContinueMode", "Draft_Edit",
            "Draft_Slope", "Draft_SetWorkingPlaneProxy",
            "Draft_AddConstruction"]


def get_draft_snap_commands():
    """Return the snapping commands list."""
    return ['Draft_Snap_Lock', 'Draft_Snap_Midpoint',
            'Draft_Snap_Perpendicular',
            'Draft_Snap_Grid', 'Draft_Snap_Intersection',
            'Draft_Snap_Parallel',
            'Draft_Snap_Endpoint', 'Draft_Snap_Angle',
            'Draft_Snap_Center',
            'Draft_Snap_Extension', 'Draft_Snap_Near',
            'Draft_Snap_Ortho', 'Draft_Snap_Special',
            'Draft_Snap_Dimensions', 'Draft_Snap_WorkingPlane']


def init_draft_toolbars(workbench):
    """Initialize the Draft toolbars.

    Parameters
    ----------
    workbench : Gui.Workbench
        The workbench class on which the commands have to be available.
        If called from within the `Initialize` method
        of a workbench class defined inside `InitGui.py`,
        it can be used as `setup_draft_toolbars(self)`.
    """
    workbench.appendToolbar(QT_TRANSLATE_NOOP("Draft",
                                              "Draft creation tools"),
                            get_draft_drawing_commands())
    workbench.appendToolbar(QT_TRANSLATE_NOOP("Draft",
                                              "Draft annotation tools"),
                            get_draft_annotation_commands())
    workbench.appendToolbar(QT_TRANSLATE_NOOP("Draft",
                                              "Draft modification tools"),
                            get_draft_modification_commands())


def init_draft_menus(workbench):
    """Initialize the Draft menus.

    Parameters
    ----------
    workbench : Gui.Workbench
        The workbench class on which the commands have to be available.
        If called from within the `Initialize` method
        of a workbench class defined inside `InitGui.py`,
        it can be used as `setup_draft_menus(self)`.
    """
    workbench.appendMenu(QT_TRANSLATE_NOOP("Draft", "&Drafting"),
                         get_draft_drawing_commands())
    workbench.appendMenu(QT_TRANSLATE_NOOP("Draft", "&Annotation"),
                         get_draft_annotation_commands())
    workbench.appendMenu(QT_TRANSLATE_NOOP("Draft", "&Modification"),
                         get_draft_modification_commands())
    workbench.appendMenu(QT_TRANSLATE_NOOP("Draft", "&Utilities"),
                         get_draft_utility_commands()
                         + get_draft_context_commands())
