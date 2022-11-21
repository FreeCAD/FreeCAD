# ***************************************************************************
# *   Copyright (c) 2002,2003 Juergen Riegel <juergen.riegel@web.de>        *
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
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************/

# TechDraw gui init module
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up


class TechDrawWorkbench(Workbench):
    "Technical Drawing workbench object"

    def __init__(self):
        self.__class__.Icon = (
            FreeCAD.getResourceDir()
            + "Mod/TechDraw/Resources/icons/preferences-techdraw.svg"
        )
        self.__class__.MenuText = "TechDraw"
        self.__class__.ToolTip = "Technical Drawing workbench"

    def Initialize(self):
        # load the module

        import TechDrawGui

        try:
            import TechDrawTools
        except ImportError as err:
            FreeCAD.Console.PrintError(
                "Features from TechDrawTools package cannot be loaded. {err}\n".format(
                    err=str(err)
                )
            )

    def GetClassName(self):
        return "TechDrawGui::Workbench"


Gui.addWorkbench(TechDrawWorkbench())

# Append the export handler
FreeCAD.addExportType("Technical Drawing (*.svg *.dxf *.pdf)", "TechDrawGui")

FreeCAD.__unit_test__ += ["TestTechDrawGui"]

