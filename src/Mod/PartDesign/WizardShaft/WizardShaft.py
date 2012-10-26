#/******************************************************************************
# *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
# *                                                                            *
# *   This file is part of the FreeCAD CAx development system.                 *
# *                                                                            *
# *   This library is free software; you can redistribute it and/or            *
# *   modify it under the terms of the GNU Library General Public              *
# *   License as published by the Free Software Foundation; either             *
# *   version 2 of the License, or (at your option) any later version.         *
# *                                                                            *
# *   This library  is distributed in the hope that it will be useful,         *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
# *   GNU Library General Public License for more details.                     *
# *                                                                            *
# *   You should have received a copy of the GNU Library General Public        *
# *   License along with this library; see the file COPYING.LIB. If not,       *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
# *   Suite 330, Boston, MA  02111-1307, USA                                   *
# *                                                                            *
# ******************************************************************************/

import FreeCAD, FreeCADGui
import os
from PyQt4 import QtCore, QtGui
from WizardShaftTable import WizardShaftTable
from Shaft import Shaft

class WizardShaft:
    "Shaft Wizard"
    # GUI
    App = FreeCAD
    Gui = FreeCADGui
    doc = Gui.ActiveDocument
    table = 0
    # Shaft
    shaft = 0
    # Feature
    sketch = 0
    featureWindow = 0

    def __init__(self):
        #FreeCADGui.activeWorkbench()
        #
        "Initialize the user interface"
        ### Create a dock window
        wizardWidget = QtGui.QDockWidget("Shaft Wizard")
        wizardWidget.setObjectName("Shaft Wizard")
        mw = QtGui.qApp.activeWindow()
        mw.addDockWidget(QtCore.Qt.TopDockWidgetArea,wizardWidget)
        cw = mw.centralWidget() # This is a qmdiarea widget

        # Get active document or create a new one
        # Important because when setting the default data in WizardShaftTable() the
        # updateSketch() slot will be activated and it relies on finding a valid document
        if self.doc == None:
            self.Gui.activateWorkbench("PartDesignWorkbench")
            self.doc = self.App.newDocument()
            # Grab the newly created feature window
            featureWindow = cw.subWindowList()[-1]
        else:
            featureWindow = cw.activeSubWindow()

        # Create Shaft object
        self.shaft = Shaft(self.doc)
        #self.shaft.featureWindow = featureWindow

        # Assign a table widget to the dock window
        self.table = WizardShaftTable(self, self.shaft)
        wizardWidget.setWidget(self.table.widget)
        # Grab the newly created HTML window
        cw = mw.centralWidget()
        htmlWindow = cw.subWindowList()[-1]
        #self.shaft.htmlWindow = htmlWindow

    def updateEdge(self, column, start):
        App.Console.PrintMessage("Not implemented yet - waiting for robust references...")
        return
        if self.sketchClosed is not True:
            return
        # Create a chamfer or fillet at the start or end edge of the segment
        if start is True:
            row = rowStartEdgeType
            idx = 0
        else:
            row = rowEndEdgeType
            idx = 1

        edgeType = self.tableWidget.item(row, column).text().toAscii()[0].upper()
        if not ((edgeType == "C") or (edgeType == "F")):
            return # neither chamfer nor fillet defined

        if edgeType == "C":
            objName = self.doc.addObject("PartDesign::Chamfer","ChamferShaft%u" % (column * 2 + idx))
        else:
            objName = self.doc.addObject("PartDesign::Fillet","FilletShaft%u" % (column * 2 + idx))
        if objName == "":
            return

        edgeName = "Edge%u" % self.getEdgeIndex(column, idx, edgeType)
        self.doc.getObject(objName).Base = (self.doc.getObject("RevolutionShaft"),"[%s]" % edgeName)
        # etc. etc.

    def getEdgeIndex(self, column, startIdx):
        # FIXME: This is impossible without robust references anchored in the sketch!!!
        return

class WizardShaftGui:
    def Activated(self):
        WizardShaft()

    def GetResources(self):
        IconPath = FreeCAD.ConfigGet("AppHomePath") + "Mod/PartDesign/Gui/Resources/icons/WizardShaftIcon.png"
        MenuText = 'Start the shaft design wizard'
        ToolTip  = 'Start the shaft design wizard'
        return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip}

FreeCADGui.addCommand('PartDesign_WizardShaft', WizardShaftGui())

#Note: Start wizard in Python Console with
# Gui.runCommand('PartDesign_WizardShaft')
