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
from PyQt4 import QtCore, QtGui
from WizardShaftTable import WizardShaftTable
from Shaft import Shaft

class TaskWizardShaft:
    "Shaft Wizard"
    App = FreeCAD
    Gui = FreeCADGui
    
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
        
        # Buttons for diagram display
        buttonLayout = QtGui.QGridLayout() 
        bnames = [["All [x]", "All [y]", "All [z]" ], 
                           ["N [x]", "Q [y]", "Q [z]"], 
                           ["Mt [x]",  "Mb [z]", "Mb [y]"], 
                           ["",  "w [y]",  "w [z]"], 
                           ["sigma [x]", "sigma [y]", "sigma [z]"], 
                           ["tau [x]",  "sigmab [z]", "sigmab [y]"]]
        slots = [[self.slotAllx,  self.slotAlly,  self.slotAllz], 
                      [self.slotFx,  self.slotQy,  self.slotQz], 
                      [self.slotMx,  self.slotMz,  self.slotMy], 
                      [self.slotNone,  self.slotWy, self.slotWz], 
                      [self.slotSigmax,  self.slotSigmay,  self.slotSigmaz], 
                      [self.slotTaut,  self.slotSigmabz,  self.slotSigmaby]]
        self.buttons = [[None,  None,  None],  [None,  None,  None],  [None,  None,  None],  [None,  None,  None],  [None,  None,  None],  [None,  None,  None]]

        for col in range(3):
            for row in range(6):
                button = QtGui.QPushButton(bnames[row][col])
                buttonLayout.addWidget(button,  row,  col)
                self.buttons[row][col] = button
                button.clicked.connect(slots[row][col])
                
        # Create Shaft object
        self.shaft = Shaft(self)
        # Create table widget
        self.form = QtGui.QWidget()
        self.table = WizardShaftTable(self, self.shaft)        
        
        # The top layout will contain the Shaft Wizard layout plus the elements of the FEM constraints dialog
        layout = QtGui.QVBoxLayout()
        layout.setObjectName("ShaftWizard") # Do not change or translate: Required to detect whether Shaft Wizard is running in FemGui::ViewProviderFemConstraintXXX
        sublayout = QtGui.QVBoxLayout()
        sublayout.setObjectName("ShaftWizardLayout") # Do not change or translate        
        sublayout.addWidget(self.table.widget)
        sublayout.addLayout(buttonLayout)
        layout.addLayout(sublayout)
        self.form.setLayout(layout)
        
        # Switch to feature window
        mdi=QtGui.qApp.activeWindow().findChild(QtGui.QMdiArea)
        cw.setActiveSubWindow(featureWindow)
        
    def slotAllx(self):
        self.shaft.showDiagram("Allx")
    def slotAlly(self):
        self.shaft.showDiagram("Ally")
    def slotAllz(self):
        self.shaft.showDiagram("Allz")
        
    def slotFx(self):
        self.shaft.showDiagram("Nx")        
    def slotQy(self):
        self.shaft.showDiagram("Qy")        
    def slotQz(self):
        self.shaft.showDiagram("Qz")
    
    def slotMx(self):
        self.shaft.showDiagram("Mx")
    def slotMz(self):
        self.shaft.showDiagram("Mz")        
    def slotMy(self):
        self.shaft.showDiagram("My")
        
    def slotNone(self):
        pass
    def slotWy(self):
        self.shaft.showDiagram("wy")
    def slotWz(self):
        self.shaft.showDiagram("wz")
        
    def slotSigmax(self):
        self.shaft.showDiagram("sigmax")
    def slotSigmay(self):
        self.shaft.showDiagram("sigmay")        
    def slotSigmaz(self):
        self.shaft.showDiagram("sigmaz")
        
    def slotTaut(self):
        self.shaft.showDiagram("taut")
    def slotSigmabz(self):
        self.shaft.showDiagram("sigmabz")        
    def slotSigmaby(self):
        self.shaft.showDiagram("sigmaby")
        
    def updateButton(self,  row,  col,  flag):
        self.buttons[row][col].setEnabled(flag)
        
    def updateButtons(self,  col,  flag):
        for row in range(len(self.buttons)):
            self.updateButton(row,  col,  flag)
        
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
        
    def isAllowedAlterDocument(self):
        return False

# Work-around to allow a callback
# Problem: From the FemConstraint ViewProvider, we need to tell the Shaft instance that the user finished editing the constraint
# We can find the Shaft Wizard dialog object from C++, but there is not way to reach the Shaft instance
# Also it seems to be impossible to access the active dialog from Python, so Gui::Command::runCommand() is not an option either
# Note: Another way would be to create a hidden widget in the Shaft Wizard dialog and write some data to it, triggering a slot 
# in the python code
WizardShaftDlg = None

class WizardShaftGui:    
    def Activated(self):
        global WizardShaftDlg
        WizardShaftDlg = TaskWizardShaft(FreeCAD.ActiveDocument)
        FreeCADGui.Control.showDialog(WizardShaftDlg)
        
    def GetResources(self):
        IconPath = FreeCAD.ConfigGet("AppHomePath") + "Mod/PartDesign/WizardShaft/WizardShaft.svg"
        MenuText = 'Shaft design wizard...'
        ToolTip  = 'Start the shaft design wizard'
        return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip}

    def IsActive(self):
        return FreeCAD.ActiveDocument != None
        
    def __del__(self):
        global WizardShaftDlg
        WizardShaftDlg = None
        
class WizardShaftGuiCallback:    
    def Activated(self):
        global WizardShaftDlg
        if WizardShaftDlg != None and WizardShaftDlg.table != None:
            WizardShaftDlg.table.finishEditConstraint()
                
    def isActive(self):
        global WizardShaftDlg
        return (WizardShaftDlg is not None)
        
    def GetResources(self):
        IconPath = FreeCAD.ConfigGet("AppHomePath") + "Mod/PartDesign/WizardShaft/WizardShaft.svg"
        MenuText = 'Shaft design wizard...'
        ToolTip  = 'Start the shaft design wizard'
        return {'Pixmap' : IconPath, 'MenuText': MenuText, 'ToolTip': ToolTip}

FreeCADGui.addCommand('PartDesign_WizardShaft', WizardShaftGui())
FreeCADGui.addCommand('PartDesign_WizardShaftCallBack', WizardShaftGuiCallback())

#Note: Start wizard in Python Console with
# Gui.runCommand('PartDesign_WizardShaft')
