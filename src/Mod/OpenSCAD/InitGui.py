#***************************************************************************
#*   Copyright (c) 2002 Juergen Riegel <juergen.riegel@web.de>             *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Lesser General Public License for more details.                   *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************/

# OpenSCAD gui init module
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up

import FreeCAD
import sys


class OpenSCADWorkbench(Workbench):
    "OpenSCAD workbench object"

    def __init__(self):
        self.__class__.Icon = (
            FreeCAD.getResourceDir()
            + "Mod/OpenSCAD/Resources/icons/OpenSCADWorkbench.svg"
        )
        self.__class__.MenuText = "OpenSCAD"
        self.__class__.ToolTip = (
            "OpenSCAD is an application for creating solid 3D CAD.\n"
            "FreeCAD utizes OpenSCAD's capability as a script-only based modeller that uses its own description language\n"
            "Note: the Mesh workbench heavily uses the boolean operations of this workbench because they are quite robust"
        )

    def Initialize(self):
        def QT_TRANSLATE_NOOP(scope, text):
            return text

        import OpenSCAD_rc, OpenSCADCommands

        commands = [
            "OpenSCAD_ReplaceObject",
            "OpenSCAD_RemoveSubtree",
            "OpenSCAD_RefineShapeFeature",
            "OpenSCAD_MirrorMeshFeature",
            "OpenSCAD_ScaleMeshFeature",
            "OpenSCAD_ResizeMeshFeature",
            "OpenSCAD_IncreaseToleranceFeature",
            "OpenSCAD_Edgestofaces",
            "OpenSCAD_ExpandPlacements",
            "OpenSCAD_ExplodeGroup",
        ]
        toolbarcommands = [
            "OpenSCAD_ReplaceObject",
            "OpenSCAD_RemoveSubtree",
            "OpenSCAD_ExplodeGroup",
            "OpenSCAD_RefineShapeFeature",
            "OpenSCAD_IncreaseToleranceFeature",
        ]
        import PartGui

        parttoolbarcommands = [
            "Part_CheckGeometry",
            "Part_Primitives",
            "Part_Builder",
            "Part_Cut",
            "Part_Fuse",
            "Part_Common",
            "Part_Extrude",
            "Part_Revolve",
        ]
        import FreeCAD
        translate = FreeCAD.Qt.translate

        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD")
        openscadfilename = param.GetString("openscadexecutable")
        if not openscadfilename:
            import OpenSCADUtils

            openscadfilename = OpenSCADUtils.searchforopenscadexe()
            if openscadfilename:  # automatic search was successful
                FreeCAD.addImportType("OpenSCAD Format (*.scad)", "importCSG")
                param.SetString(
                    "openscadexecutable", openscadfilename
                )  # save the result
        if openscadfilename:
            commands.extend(
                [
                    "OpenSCAD_AddOpenSCADElement",
                    "OpenSCAD_MeshBoolean",
                    "OpenSCAD_Hull",
                    "OpenSCAD_Minkowski",
                ]
            )

            toolbarcommands.extend(
                [
                    "OpenSCAD_AddOpenSCADElement",
                    "OpenSCAD_MeshBoolean",
                    "OpenSCAD_Hull",
                    "OpenSCAD_Minkowski",
                ]
            )
        else:
            FreeCAD.Console.PrintWarning("OpenSCAD executable not found\n")

        transferMechanism = param.GetInt("transfermechanism", 0)
        if openscadfilename and transferMechanism == 0:
            # We are using the Python temp-directory creation function
            if "snap" in openscadfilename:
                FreeCAD.Console.PrintMessage(
                    translate(
                        "OpenSCAD",
                        "It looks like you may be using a Snap version of OpenSCAD.",
                    )
                    + " "
                    + translate(
                        "OpenSCAD",
                        "If OpenSCAD execution fails to load the temporary file, use FreeCAD's OpenSCAD Workbench Preferences to change the transfer mechanism.",
                    )
                    + "\n"
                )
            elif sys.executable.startswith("/tmp/"):  # Heuristic for AppImages
                FreeCAD.Console.PrintMessage(
                    translate(
                        "OpenSCAD",
                        "It looks like you may be using a sandboxed version of FreeCAD.",
                    )
                    + " "
                    + translate(
                        "OpenSCAD",
                        "If OpenSCAD execution fails to load the temporary file, use FreeCAD's OpenSCAD Workbench Preferences to change the transfer mechanism.",
                    )
                    + "\n"
                )

        self.appendToolbar(
            QT_TRANSLATE_NOOP("Workbench", "OpenSCAD Tools"), toolbarcommands
        )
        self.appendMenu("OpenSCAD", commands)
        self.appendToolbar(
            QT_TRANSLATE_NOOP("Workbench", "Frequently-used Part WB tools"), parttoolbarcommands
        )
        # self.appendMenu('OpenSCAD',["AddOpenSCADElement"])
        ###self.appendCommandbar("&Generic Tools",["ColorCodeShape"])
        FreeCADGui.addIconPath(":/icons")
        FreeCADGui.addLanguagePath(":/translations")
        FreeCADGui.addPreferencePage(":/ui/openscadprefs-base.ui", "OpenSCAD")

    def GetClassName(self):
        return "Gui::PythonWorkbench"


Gui.addWorkbench(OpenSCADWorkbench())

# Not all of the GUI tests will require an OpenSCAD binary (CSG import and export don't)
FreeCAD.__unit_test__ += ["TestOpenSCADGui"]
