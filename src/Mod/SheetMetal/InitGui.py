########################################################################
#
#  InitGui.py
#
#  Copyright 2015 Shai Seger <shaise at gmail dot com>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
########################################################################

import os

import FreeCAD

import SheetMetalTools
from engineering_mode import engineering_mode_enabled

Gui = FreeCAD.Gui
SMWBPath = SheetMetalTools.mod_path
SMIconPath = SheetMetalTools.icons_path

# Add translations path.
LanguagePath = os.path.join(SMWBPath, "Resources", "translations")
Gui.addLanguagePath(LanguagePath)
Gui.updateLocale()


class SMWorkbench(Workbench):
    global SHEETMETALWB_VERSION
    global SMIconPath
    global SMWBPath
    global engineering_mode_enabled

    MenuText = FreeCAD.Qt.translate("SheetMetal", "Sheet Metal")
    ToolTip = FreeCAD.Qt.translate(
        "SheetMetal",
        "Sheet Metal workbench allows for designing and unfolding sheet metal parts",
        )
    Icon = os.path.join(SMIconPath, "SMLogo.svg")

    def Initialize(self):
        """Execute when FreeCAD starts."""
        import os.path

        # Import all the needed files that create the workbench commands.
        import ExtrudedCutout
        import SheetMetalBaseCmd
        import SheetMetalBaseShapeCmd
        import SheetMetalBend
        import SheetMetalCmd
        import SheetMetalHem
        import SheetMetalCornerReliefCmd
        import SheetMetalExtendCmd
        import SheetMetalFoldCmd
        import SheetMetalFormingCmd
        import SheetMetalJunction
        import SheetMetalRelief
        import SheetMetalUnfoldCmd
        import SheetMetalUnfolder
        import SketchOnSheetMetalCmd
        import SheetMetalSketch
        import SheetMetalFromSolid

        self.list = [
            "SheetMetal_BaseShape",
            "SheetMetal_NewSketch",
            "SheetMetal_AddBase",
            "SheetMetal_FromSolid",
            "SheetMetal_AddWall",
            "SheetMetal_AddHem",
            "SheetMetal_Extrude",
            "SheetMetal_ExtendBySketch",
            "SheetMetal_AddFoldWall",
            "SheetMetal_Unfold",
            "SheetMetal_UnfoldUpdate",
            "SheetMetal_AddCornerRelief",
            "SheetMetal_AddRelief",
            "SheetMetal_AddJunction",
            "SheetMetal_AddBend",
            "SheetMetal_SketchOnSheet",
            "SheetMetal_AddCutout",
            "SheetMetal_Forming",
            ]

        if engineering_mode_enabled():
            self.list.insert(self.list.index("SheetMetal_Unfold") + 1,
                             "SheetMetal_UnattendedUnfold")

        # Create a new toolbar with commands.
        self.appendToolbar(FreeCAD.Qt.translate("SheetMetal", "Sheet Metal"), self.list)
        # Create a new menu.
        self.appendMenu(FreeCAD.Qt.translate("SheetMetal", "&Sheet Metal"), self.list)
        # # Append a submenu to an existing menu.
        # self.appendMenu(["An existing Menu","My submenu"],self.list)
        Gui.addPreferencePage(os.path.join(SMWBPath, "Resources/panels/SMprefs.ui"), "SheetMetal")
        Gui.addIconPath(SMIconPath)

    def Activated(self):
        """Execute when the workbench is activated."""
        return

    def Deactivated(self):
        """Execute when the workbench is deactivated."""
        return

    def ContextMenu(self, recipient):
        """Execute whenever the user right-clicks on screen."""
        # `recipient` will be either `view` or `tree`.
        #
        # Add commands to the context menu.
        self.appendContextMenu(FreeCAD.Qt.translate("SheetMetal", "Sheet Metal"), self.list)

    def GetClassName(self):
        # This function is mandatory if this is a full python workbench.
        return "Gui::PythonWorkbench"


Gui.addWorkbench(SMWorkbench())
