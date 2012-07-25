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
# pyOpenCL
import pyopencl as cl
# Module
import SimInstance
from shipUtils import Paths, Translator
from Simulation import FreeCADShipSimulation as Sim

import time

class TaskPanel:
    def __init__(self):
        self.ui  = Paths.modulePath() + "/simRun/TaskPanel.ui"
        self.sim = False

    def accept(self):
        msg = Translator.translate("Building data...\n")
        App.Console.PrintMessage(msg)
        # Get GUI data
        endTime = self.form.time.value()
        output  = []
        output.append(self.form.output.value())
        output.append(self.form.outputType.currentIndex())
        devId   = self.form.device.currentIndex()
        # Get OpenCL device
        count = 0
        platforms = cl.get_platforms()
        for p in platforms:
            devs = p.get_devices()
            for d in devs:
                if count == devId:
                    device = d
                count = count + 1
        # Get free surfaces data
        FSMesh = SimInstance.FSMesh(self.sim)
        wData  = self.sim.Waves
        wDir   = self.sim.Waves_Dir
        waves  = []
        for i in range(0,len(wData)):
            waves.append([wData[i].x, wData[i].y, wData[i].z, wDir[i]])
        msg = Translator.translate("Launching simulation...\n")
        App.Console.PrintMessage(msg)
        # Build simulation thread
        simulator = Sim(device, endTime, output, FSMesh, waves)
        simulator.start()
        msg = Translator.translate("Done!\n")
        App.Console.PrintMessage(msg)
        return True

    def reject(self):
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
        form.time       = form.findChild(QtGui.QDoubleSpinBox, "SimTime")
        form.output     = form.findChild(QtGui.QDoubleSpinBox, "Output")
        form.outputType = form.findChild(QtGui.QComboBox, "OutputType")
        form.device = form.findChild(QtGui.QComboBox, "Device")
        self.form = form
        # Initial values
        if self.initValues():
            return True
        self.retranslateUi()
        # Connect Signals and Slots
        # QtCore.QObject.connect(form.time, QtCore.SIGNAL("valueChanged(double)"), self.onData)

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
        """ Set initial values for fields
        """
        # Get objects
        selObjs  = Gui.Selection.getSelection()
        if not selObjs:
            msg = Translator.translate("Ship simulation instance must be selected (no object selected)\n")
            App.Console.PrintError(msg)
            return True
        for i in range(0,len(selObjs)):
            obj = selObjs[i]
            # Test if is a ship instance
            props = obj.PropertiesList
            try:
                props.index("IsShipSimulation")
            except ValueError:
                continue
            if obj.IsShipSimulation:
                # Test if another ship already selected
                if self.sim:
                    msg = Translator.translate("More than one ship simulation selected (extra simulations will be neglected)\n")
                    App.Console.PrintWarning(msg)
                    break
                self.sim = obj
        # Test if any valid ship was selected
        if not self.sim:
            msg = Translator.translate("Ship simulation instance must be selected (no valid simulation found at selected objects)\n")
            App.Console.PrintError(msg)
            return True
        # Get the list of devices
        devices = []
        platforms = cl.get_platforms()
        for p in platforms:
            devs = p.get_devices()
            for d in devs:
                devices.append([p,d])
                dname = d.get_info(cl.device_info.NAME)
                pname = p.get_info(cl.platform_info.NAME)
                self.form.device.addItem(dname + " (" + pname + ")")
        if not len(devices):
            msg = Translator.translate("This tool requires an active OpenCL context to work\n")
            App.Console.PrintError(msg)
            return True
        msg = Translator.translate("Ready to work\n")
        App.Console.PrintMessage(msg)
        return False

    def retranslateUi(self):
        """ Set user interface locale strings. 
        """
        self.form.setWindowTitle(Translator.translate("Run the simulation"))
        self.form.findChild(QtGui.QLabel, "SimTimeLabel").setText(Translator.translate("Simulation time"))
        self.form.findChild(QtGui.QLabel, "OutputLabel").setText(Translator.translate("Output"))
        self.form.findChild(QtGui.QLabel, "DeviceLabel").setText(Translator.translate("OpenCL device"))

def createTask():
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel

def stopSimulation():
    try:
        simulator = Sim()
        if not simulator.isRunning():
            msg = Translator.translate("Simulation already stopped\n")
            App.Console.PrintWarning(msg)
            return
    except:
        msg = Translator.translate("Any active simulation to stop!\n")
        App.Console.PrintError(msg)
        return
    simulator.stop()
    msg = Translator.translate("Simulation will stop at the end of actual iteration\n")
    App.Console.PrintMessage(msg)
