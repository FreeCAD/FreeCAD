#/******************************************************************************
# * Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net>   *
# *                                                                            *
# * This file is part of the FreeCAD CAx development system.                   *
# *                                                                            *
# * This library is free software; you can redistribute it and/or              *
# * modify it under the terms of the GNU Library General Public                *
# * License as published by the Free Software Foundation; either               *
# * version 2 of the License, or (at your option) any later version.           *
# *                                                                            *
# * This library is distributed in the hope that it will be useful,            *
# * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
# * GNU Library General Public License for more details.                       *
# *                                                                            *
# * You should have received a copy of the GNU Library General Public          *
# * License along with this library; see the file COPYING.LIB. If not,         *
# * write to the Free Software Foundation, Inc., 59 Temple Place,              *
# * Suite 330, Boston, MA 02111-1307, USA                                      *
# *                                                                            *
# ******************************************************************************/

import FreeCAD, FreeCADGui
import PartDesignGui
from PySide import QtCore, QtGui
from TaskHole import TaskHole
from FeatureHole import Hole
from ViewProviderHole import ViewProviderHole

class HoleGui:
    def getMainWindow(self):
        "returns the main window"
        # using QtGui.QApplication.activeWindow() isn't very reliable because if another
        # widget than the mainwindow is active (e.g. a dialog) the wrong widget is
        # returned
        toplevel = QtGui.QApplication.topLevelWidgets()
        for i in toplevel:
            if i.metaObject().className() == "Gui::MainWindow":
                return i
        raise Exception("No main window found")

    "Create a new hole feature"
    def Activated(self):
        # Get main window
        mw = self.getMainWindow()

        # Get active document
        doc = FreeCAD.activeDocument()
        if doc is None:
            QtGui.QMessageBox.critical(mw, "No document", "A document must be open in order to create a hole feature")
            return

        # Check for valid position selection
        selection = FreeCADGui.Selection.getSelectionEx()
        if len(selection) != 1:
            QtGui.QMessageBox.critical(mw, "No position defined", "Please select a face to create the hole feature on")
            return
        if selection[0].DocumentName != doc.Name:
            QtGui.QMessageBox.critical(mw, "Wrong document", "Please select a face in the active document")
            return
        # Note: For some reason setting the Support property here breaks all sorts of things.
        #       It is done in TaskHole.updateUI() instead

        # Show feature preview
        body = FreeCADGui.activeView().getActiveObject("pdbody");
        if body is None:
            QtGui.QMessageBox.critical(mw, "No active body", "Please create a body or make a body active")
            return

        feature = doc.addObject("Part::FeaturePython","Hole")
        hole = Hole(feature)
        body.addFeature(feature)

        vp = ViewProviderHole(feature.ViewObject)
        feature.touch()
        FreeCAD.ActiveDocument.recompute()
        # Fit view (remove after the testing phase)
        FreeCADGui.SendMsgToActiveView("ViewFit")

        vp.setEdit(vp,  1)

    def GetResources(self):
        IconPath = FreeCAD.ConfigGet("AppHomePath") + "Mod/PartDesign/FeatureHole/PartDesign_Hole.svg"
        MenuText = 'Create a hole feature'
        ToolTip = 'Create a hole feature'
        return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip}

FreeCADGui.addCommand('PartDesign_Hole', HoleGui())
