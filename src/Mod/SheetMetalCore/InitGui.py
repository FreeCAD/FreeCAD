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

"""Gui initialization of the SheetMetal module (proof of concept)."""


class SheetMetalWorkbench(Workbench):
    """Sheet metal workbench on the explicit sheet model."""

    MenuText = "Sheet Metal (PoC)"
    ToolTip = "Sheet metal design with a linked flat pattern"

    def Initialize(self):
        import SheetMetalCore  # noqa: F401  Registers the document object types.
        import SheetMetalCommands

        commands = SheetMetalCommands.register()
        self.appendToolbar("Sheet Metal", commands)
        self.appendMenu("Sheet Metal", commands)

    def GetClassName(self):
        return "Gui::PythonWorkbench"


Gui.addWorkbench(SheetMetalWorkbench())
