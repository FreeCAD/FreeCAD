#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *  
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

# FreeCAD modules
import FreeCAD as App
import FreeCADGui as Gui
# Qt library
from PyQt4 import QtGui,QtCore
# Module
from Instance import *
from TankInstance import *
from shipUtils import Paths, Translator

class TaskPanel:
    def __init__(self):
        self.ui = Paths.modulePath() + "/tankGZ/TaskPanel.ui"
        self.ship  = None
        self.tanks = {}

    def accept(self):
        if not self.ship:
            return False
        return True

    def reject(self):
        if not self.ship:
            return False
        return True

    def clicked(self, index):
        pass

    def open(self):
        pass

    def needsFullSpace(self):
        return True

    def isAllowedAlterSelection(self):
        return False

    def isAllowedAlterView(self):
        return True

    def isAllowedAlterDocument(self):
        return False

    def helpRequested(self):
        pass

    def setupUi(self):
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.tanks = form.findChild(QtGui.QListWidget, "Tanks")
        form.disp  = form.findChild(QtGui.QLabel, "DisplacementLabel")
        form.draft = form.findChild(QtGui.QLabel, "DraftLabel")
        self.form = form
        # Initial values
        if self.initValues():
            return True
        self.retranslateUi()
        self.onTanksSelection()
        # Connect Signals and Slots
        QtCore.QObject.connect(form.tanks,QtCore.SIGNAL("itemSelectionChanged()"),self.onTanksSelection)
        return False

    def getMainWindow(self):
        "returns the main window"
        # using QtGui.qApp.activeWindow() isn't very reliable because if another
        # widget than the mainwindow is active (e.g. a dialog) the wrong widget is
        # returned
        toplevel = QtGui.qApp.topLevelWidgets()
        for i in toplevel:
            if i.metaObject().className() == "Gui::MainWindow":
                return i
        raise Exception("No main window found")

    def initValues(self):
        """ Get selected geometry.
        @return False if sucessfully values initialized.
        """
        # Get selected objects
        selObjs  = FreeCADGui.Selection.getSelection()
        if not selObjs:
            msg = Translator.translate("Ship instance must be selected (no object selected)\n")
            App.Console.PrintError(msg)
            return True
        for i in range(0,len(selObjs)):
            obj = selObjs[i]
            # Test if is a ship instance
            props = obj.PropertiesList
            try:
                props.index("IsShip")
            except ValueError:
                continue
            if obj.IsShip:
                # Test if another ship already selected
                if self.ship:
                    msg = Translator.translate("More than one ship selected (extra ship will be neglected)\n")
                    App.Console.PrintWarning(msg)
                    break
                self.ship = obj
        # Test if any valid ship was selected
        if not self.ship:
            msg = Translator.translate("Ship instance must be selected (no valid ship found at selected objects)\n")
            App.Console.PrintError(msg)
            return True
        props = self.ship.PropertiesList
        try:
            props.index("WeightNames")
        except:
            msg = Translator.translate("Ship weights has not been set. You need to set weights before use this tool.\n")
            App.Console.PrintError(msg)
            return True
        # Setup available tanks list
        objs = App.ActiveDocument.Objects
        iconPath  = Paths.iconsPath() + "/Tank.xpm"
        icon      = QtGui.QIcon(QtGui.QPixmap(iconPath))
        for obj in objs:
            # Try to get valid tank property
            props = obj.PropertiesList
            try:
                props.index("IsShipTank")
            except ValueError:
                continue
            if not obj.IsShipTank:
                continue
            # Add tank to list
            name  = obj.Name
            label = obj.Label
            tag   = label + ' (' + name + ')'
            self.tanks[tag] = name
            # self.tanks.append([name, tag])
            item  = QtGui.QListWidgetItem(tag)
            item.setIcon(icon)
            self.form.tanks.addItem(item)
        msg = Translator.translate("Ready to work\n")
        App.Console.PrintMessage(msg)
        return False

    def retranslateUi(self):
        """ Set user interface locale strings. 
        """
        self.form.setWindowTitle(Translator.translate("GZ curve computation"))
        self.form.findChild(QtGui.QGroupBox, "LoadConditionGroup").setTitle(Translator.translate("Loading condition."))

    def onTanksSelection(self):
        """ Called when tanks are selected or deselected.
        """
        # Set displacement label
        disp = self.computeDisplacement()
        self.form.disp.setText(Translator.translate("Displacement") + ' %g [kg]' % (disp[0]))
        
    def getTanks(self):
        """ Get the selected tanks objects list.
        @return Selected tanks list.
        """
        items = self.form.tanks.selectedItems()
        tanks = []
        for item in items:
            tag  = str(item.text())
            name = self.tanks[tag]
            t    = App.ActiveDocument.getObject('Tank')
            if not t:
                continue
            tanks.append(t)
        return tanks

    def computeDisplacement(self):
        """ Computes ship displacement.
        @return Ship displacement and center of gravity. None if errors detected.
        """
        if not self.ship:
            return None
        # Test if is a ship instance
        obj   = self.ship
        props = obj.PropertiesList
        try:
            props.index("IsShip")
        except ValueError:
            return None
        if not obj.IsShip:
            return None
        # Test if properties already exist
        try:
            props.index("WeightNames")
        except:
            return None
        # Get ship structure weights
        W = [0.0, 0.0, 0.0, 0.0]
        sWeights = weights(obj)
        for w in sWeights:
            W[0] = W[0] + w[1]
            W[1] = W[1] + w[1]*w[2][0]
            W[2] = W[2] + w[1]*w[2][1]
            W[3] = W[3] + w[1]*w[2][2]
        # Get selected tanks weights
        tanks = self.getTanks()
        for t in tanks:
            w = tankWeight(t)
            W[0] = W[0] + w[0]
            W[1] = W[1] + w[0]*w[1]
            W[2] = W[2] + w[0]*w[2]
            W[3] = W[3] + w[0]*w[3]
        return [W[0], W[1]/W[0], W[2]/W[0], W[3]/W[0]]

def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
