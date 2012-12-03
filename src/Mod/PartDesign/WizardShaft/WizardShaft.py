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
#import os
from PyQt4 import QtCore, QtGui
from WizardShaftTable import WizardShaftTable
from Shaft import Shaft

class TaskWizardShaft:
    "Shaft Wizard"
    # GUI
    App = FreeCAD
    Gui = FreeCADGui
    # Table and widget
    table = 0
    form = 0
    # Shaft
    shaft = 0
    # Feature
    featureWindow = 0

    def __init__(self, doc):
        mw = QtGui.qApp.activeWindow()
        cw = mw.centralWidget() # This is a qmdiarea widget
        self.doc = doc

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

        # Assign a table widget to the dock window
        self.table = WizardShaftTable(self, self.shaft)
        self.form = self.table.widget
        self.form.setWindowTitle("Shaft wizard")

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def accept(self):
        if self.table:
            del self.table
        if self.shaft:
            del self.shaft
        if self.form:
            del self.form
        return True

class WizardShaftGui:
    def Activated(self):
        FreeCADGui.Control.showDialog(TaskWizardShaft(FreeCAD.ActiveDocument))

    def GetResources(self):
        IconPath = FreeCAD.ConfigGet("AppHomePath") + "Mod/PartDesign/WizardShaft/WizardShaft.svg"
        MenuText = 'Shaft design wizard...'
        ToolTip  = 'Start the shaft design wizard'
        return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip}

    def IsActive(self):
        return FreeCAD.ActiveDocument != None

FreeCADGui.addCommand('PartDesign_WizardShaft', WizardShaftGui())

#Note: Start wizard in Python Console with
# Gui.runCommand('PartDesign_WizardShaft')
