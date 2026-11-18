# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2002 Juergen Riegel
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

# Sketcher gui init module
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up


class SketcherWorkbench(Workbench):
    "Sketcher workbench object"

    def __init__(self):
        self.__class__.Icon = (
            FreeCAD.getResourceDir() + "Mod/Sketcher/Resources/icons/SketcherWorkbench.svg"
        )
        self.__class__.MenuText = "Sketcher"
        self.__class__.ToolTip = "Sketcher workbench"

    def Initialize(self):
        # load the module
        import SketcherGui
        import Sketcher

        try:
            import Profiles
        except ImportError:
            print("Error in Profiles module")

    def GetClassName(self):
        return "SketcherGui::Workbench"


Gui.addWorkbench(SketcherWorkbench())

FreeCAD.__unit_test__ += ["TestSketcherGui"]
