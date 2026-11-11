# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2016 triplus
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Persistent toolbars for FreeCAD."""

import FreeCAD as App


def addTop(name, toolbars):
    """addTop("name", ["toolbars"])

    Example for Sketcher workbench (InitGui.py):

    try:
        import PersistentToolbars

        toolbars = ["Sketcher constraints",
                    "Break",
                    "Sketcher geometries"]

        PersistentToolbars.addRight("SketcherWorkbench", toolbars)
    except ImportError:
        pass

    Description:
    Provides ability to preset toolbar position for any workbench.
    Settings are deleted when FreeCAD exits normally. After user
    customizes workbench toolbar position this setting has no
    effect anymore. Customized toolbar position set by the user is
    used instead."""

    p = App.ParamGet("User parameter:Tux/PersistentToolbars/System")
    p.GetGroup(name).SetBool("Saved", 1)
    p.GetGroup(name).SetString("Top", ",".join(toolbars))


def addRight(name, toolbars):
    """addRight("name", ["toolbars"])

    Description:
    Look at addTop for more information."""

    p = App.ParamGet("User parameter:Tux/PersistentToolbars/System")
    p.GetGroup(name).SetBool("Saved", 1)
    p.GetGroup(name).SetString("Right", ",".join(toolbars))


def addLeft(name, toolbars):
    """addLeft("name", ["toolbars"])

    Description:
    Look at addTop for more information."""

    p = App.ParamGet("User parameter:Tux/PersistentToolbars/System")
    p.GetGroup(name).SetBool("Saved", 1)
    p.GetGroup(name).SetString("Left", ",".join(toolbars))


def addBottom(name, toolbars):
    """addBottom("name", ["toolbars"])

    Description:
    Look at addTop for more information."""

    p = App.ParamGet("User parameter:Tux/PersistentToolbars/System")
    p.GetGroup(name).SetBool("Saved", 1)
    p.GetGroup(name).SetString("Bottom", ",".join(toolbars))


def clear():
    """Delete all user and system toolbar position data."""

    p = App.ParamGet("User parameter:Tux")
    p.RemGroup("PersistentToolbars")
