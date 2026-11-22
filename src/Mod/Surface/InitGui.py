# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2014 Nathan Miller
# SPDX-FileCopyrightText: 2014 Balázs Bámer
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

# -*- coding: utf8 -*-

"""The Surface Workbench GUI initialization."""

import os

import FreeCAD as App
import FreeCADGui as Gui


class SurfaceWorkbench(Gui.Workbench):
    """Surface workbench object."""

    Icon = os.path.join(
        App.getResourceDir(), "Mod", "Surface", "Resources", "icons", "Surface_Workbench.svg"
    )
    MenuText = "Surface"
    ToolTip = "Surface workbench: Create and edit complex surfaces"

    def Initialize(self):
        """Initialize the module."""
        import Surface
        import SurfaceGui

    def GetClassName(self):
        return "SurfaceGui::Workbench"


Gui.addWorkbench(SurfaceWorkbench())
