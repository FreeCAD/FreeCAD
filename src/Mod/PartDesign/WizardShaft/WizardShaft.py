# -*- coding: utf-8 -*-
#/******************************************************************************
# *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
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
import traceback
from PySide import QtCore, QtGui
from .WizardShaftTable import WizardShaftTable
from .Shaft import Shaft

translate = FreeCAD.Qt.translate

class TaskWizardShaft:
    "Shaft Wizard"
    App = FreeCAD
    Gui = FreeCADGui

    def __init__(self, doc):
        mw = QtGui.QApplication.activeWindow()
        #cw = mw.centralWidget() # This is a qmdiarea widget
        cw = mw.findChild(QtGui.QMdiArea)
        self.doc = doc

        # Get active document or create a new one
        # Important because when setting the default data in WizardShaftTable() the
        # updateSketch() slot will be activated and it relies on finding a valid document
        if self.doc is None:
            self.Gui.activateWorkbench("PartDesignWorkbench")
            self.doc = self.App.newDocument()
            # Grab the newly created feature window
            featureWindow = cw.subWindowList()[-1]
        else:
            featureWindow = cw.activeSubWindow()

        # Buttons for diagram display
        buttonLayout = QtGui.QGridLayout()
        all = translate("TaskWizardShaft", "All")
        bnames = [[f"{all} [x]", f"{all} [y]", f"{all} [z]" ],
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
        mdi=FreeCADGui.getMainWindow().findChild(QtGui.QMdiArea)
        cw.setActiveSubWindow(featureWindow)

    def showDiagram(self, diagram):
        try:
            self.shaft.showDiagram(diagram)
        except ImportError as e:
            msgBox = QtGui.QMessageBox()
            msgBox.setIcon(msgBox.Information)
            msgBox.setWindowTitle(translate("TaskWizardShaft", "Missing module"))
            msgBox.setText(translate("TaskWizardShaft", "You may have to install the Plot add-on"))
            msgBox.setDetailedText(traceback.format_exc())
            msgBox.exec_()
    def slotAllx(self):
        self.showDiagram("Allx")
    def slotAlly(self):
        self.showDiagram("Ally")
    def slotAllz(self):
        self.showDiagram("Allz")

    def slotFx(self):
        self.showDiagram("Nx")
    def slotQy(self):
        self.showDiagram("Qy")
    def slotQz(self):
        self.showDiagram("Qz")

    def slotMx(self):
        self.showDiagram("Mx")
    def slotMz(self):
        self.showDiagram("Mz")
    def slotMy(self):
        self.showDiagram("My")

    def slotNone(self):
        pass
    def slotWy(self):
        self.showDiagram("wy")
    def slotWz(self):
        self.showDiagram("wz")

    def slotSigmax(self):
        self.showDiagram("sigmax")
    def slotSigmay(self):
        self.showDiagram("sigmay")
    def slotSigmaz(self):
        self.showDiagram("sigmaz")

    def slotTaut(self):
        self.showDiagram("taut")
    def slotSigmabz(self):
        self.showDiagram("sigmabz")
    def slotSigmaby(self):
        self.showDiagram("sigmaby")

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

# Workaround to allow a callback
# Problem: From the FemConstraint ViewProvider, we need to tell the Shaft instance that the user finished editing the constraint
# We can find the Shaft Wizard dialog object from C++, but there is no way to reach the Shaft instance
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
        MenuText = QtCore.QT_TRANSLATE_NOOP("PartDesign_WizardShaft", "Shaft design wizard...")
        ToolTip  = QtCore.QT_TRANSLATE_NOOP("PartDesign_WizardShaft", "Start the shaft design wizard")
        return {'Pixmap': IconPath,
                'MenuText': MenuText,
                'ToolTip': ToolTip}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def __del__(self):
        global WizardShaftDlg
        WizardShaftDlg = None

class WizardShaftGuiCallback:
    def Activated(self):
        global WizardShaftDlg
        if WizardShaftDlg is not None and WizardShaftDlg.table is not None:
            WizardShaftDlg.table.finishEditConstraint()

    def isActive(self):
        global WizardShaftDlg
        return (WizardShaftDlg is not None)

    def GetResources(self):
        IconPath = FreeCAD.ConfigGet("AppHomePath") + "Mod/PartDesign/WizardShaft/WizardShaft.svg"
        MenuText = QtCore.QT_TRANSLATE_NOOP("PartDesign_WizardShaftCallBack", "Shaft design wizard...")
        ToolTip  = QtCore.QT_TRANSLATE_NOOP("PartDesign_WizardShaftCallBack", "Start the shaft design wizard")
        return {'Pixmap': IconPath,
                'MenuText': MenuText,
                'ToolTip': ToolTip}

FreeCADGui.addCommand('PartDesign_WizardShaft', WizardShaftGui())
FreeCADGui.addCommand('PartDesign_WizardShaftCallBack', WizardShaftGuiCallback())

#Note: Start wizard in Python Console with
# Gui.runCommand('PartDesign_WizardShaft')
