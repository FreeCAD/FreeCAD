# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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

"""Gui initialization of the SheetDesign module."""

import os

import FreeCAD


class SheetDesignWorkbench(Workbench):
    """Sheet Design workbench: folded sheet parts on an explicit sheet model."""

    MenuText = "Sheet Design"
    ToolTip = "Design of folded sheet parts with a linked flat pattern"
    Icon = os.path.join(
        FreeCAD.getResourceDir(), "Mod", "SheetDesign", "Resources", "icons", "SheetDesignWorkbench.svg"
    )

    def Initialize(self):
        import SheetDesign  # noqa: F401  Registers the document object types.
        import SheetDesignGui  # noqa: F401  Loads the icons and the task panel files.
        import SheetDesignCommands

        commands = SheetDesignCommands.register()
        self.appendToolbar("Sheet Design", commands)
        self.appendMenu("Sheet Design", commands)

    def GetClassName(self):
        return "Gui::PythonWorkbench"


Gui.addWorkbench(SheetDesignWorkbench())

FreeCAD.__unit_test__ += ["TestSheetDesignGui"]
