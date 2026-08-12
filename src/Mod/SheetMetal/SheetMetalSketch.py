# -*- coding: utf-8 -*-
########################################################################
#
#  SheetMetalJunction.py
#
#  Copyright 2015 Shai Seger <shaise at gmail dot com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
########################################################################

import os

import FreeCAD

import SheetMetalTools

###################################################################################################
# Gui code
###################################################################################################
if SheetMetalTools.isGuiLoaded():
    translate = FreeCAD.Qt.translate
    Gui = FreeCAD.Gui

    def smActivatePDSketch():
        Gui.runCommand("PartDesign_NewSketch")

    def smActivatePartSketch():
        Gui.runCommand("Sketcher_NewSketch")

    def smActivatePDSketchAndClose():
        smActivatePDSketch()
        SMSelectSketchDlg.hide()    

    def smActivatePartSketchAndClose():
        smActivatePartSketch()
        SMSelectSketchDlg.hide()    


    SMSelectSketchDlg = SheetMetalTools.taskLoadUI("SketchSelectionWidget.ui")
    SMSelectSketchDlg.setModal(True)
    SMSelectSketchDlg.pushPDSketch.clicked.connect(smActivatePDSketchAndClose)
    SMSelectSketchDlg.pushSketcherSketch.clicked.connect(smActivatePartSketchAndClose)


    class SheetMetalSketchCommand:
        """Activate Part/PartDesign sketcher."""

        def GetResources(self):
            icon = os.path.join(SheetMetalTools.icons_path, "Sketcher_NewSketch.svg")
            return {
                "Pixmap": icon,  # the name of a svg file available in the resources
                "MenuText": translate("SheetMetal", "New Sketch"),
                "ToolTip": translate("SheetMetal", "Create a new sketch in the active body"),
            }

        def Activated(self):
            sel = Gui.Selection.getSelectionEx()
            if len(sel) == 0:
                if Gui.ActiveDocument.ActiveView.getActiveObject("pdbody") is None:
                    smActivatePartSketch()
                else:
                    SMSelectSketchDlg.show()
                return
            
            if SheetMetalTools.smIsPartDesign(sel[0].Object):
                smActivatePDSketch()
                return

            smActivatePartSketch()
            return

        def IsActive(self):
            return True


    Gui.addCommand("SheetMetal_NewSketch", SheetMetalSketchCommand())

