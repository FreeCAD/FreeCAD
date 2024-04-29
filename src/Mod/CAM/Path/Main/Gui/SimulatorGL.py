# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2017 Shai Seger <shaise at gmail>                       *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Path
import Path.Base.Util as PathUtil
import Path.Dressup.Utils as PathDressup
import PathScripts.PathUtils as PathUtils
import Path.Main.Job as PathJob
import PathGui
import CAMSimulator
import math
import os

from FreeCAD import Vector, Base

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Mesh = LazyLoader("Mesh", globals(), "Mesh")
Part = LazyLoader("Part", globals(), "Part")

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui, QtCore

_filePath = os.path.dirname(os.path.abspath(__file__))


class CAMSimTaskUi:
    def __init__(self, parent):
        # this will create a Qt widget from our ui file
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/TaskCAMSimulator.ui")
        #self.form = FreeCADGui.PySideUic.loadUi(_filePath + "/TaskCAMSimulator.ui")
        self.parent = parent

    def accept(self):
        self.parent.accept()
        FreeCADGui.Control.closeDialog()

    def reject(self):
        self.parent.cancel()
        FreeCADGui.Control.closeDialog()


def TSError(msg):
    QtGui.QMessageBox.information(None, "Path Simulation", msg)


class CAMSimulation:
    def __init__(self):
        self.debug = False
        self.timer = QtCore.QTimer()
        QtCore.QObject.connect(self.timer, QtCore.SIGNAL("timeout()"), self.PerformCut)
        self.stdrot = FreeCAD.Rotation(Vector(0, 0, 1), 0)
        self.iprogress = 0
        self.numCommands = 0
        self.simperiod = 20
        self.accuracy = 0.1
        self.resetSimulation = False
        self.jobs = []

    def Connect(self, but, sig):
        QtCore.QObject.connect(but, QtCore.SIGNAL("clicked()"), sig)

    def UpdateProgress(self):
        if self.numCommands > 0:
            self.taskForm.form.progressBar.setValue(
                self.iprogress * 100 / self.numCommands
            )

    def Activate(self):
        self.initdone = False
        self.taskForm = CAMSimTaskUi(self)
        form = self.taskForm.form
        self.Connect(form.toolButtonPlay, self.SimPlay)
        form.sliderAccuracy.valueChanged.connect(self.onAccuracyBarChange)
        self.onAccuracyBarChange()
        self._populateJobSelection(form)
        form.comboJobs.currentIndexChanged.connect(self.onJobChange)
        self.onJobChange()
        FreeCADGui.Control.showDialog(self.taskForm)
        self.disableAnim = False
        self.firstDrill = True
        self.voxSim = CAMSimulator.PathSim()
        self.SimulateMill()
        self.initdone = True

    def _populateJobSelection(self, form):
        # Make Job selection combobox
        setJobIdx = 0
        jobName = ""
        jIdx = 0
        # Get list of Job objects in active document
        jobList = FreeCAD.ActiveDocument.findObjects("Path::FeaturePython", "Job.*")
        jCnt = len(jobList)

        # Check if user has selected a specific job for simulation
        guiSelection = FreeCADGui.Selection.getSelectionEx()
        if guiSelection:  #  Identify job selected by user
            sel = guiSelection[0]
            if hasattr(sel.Object, "Proxy") and isinstance(
                sel.Object.Proxy, PathJob.ObjectJob
            ):
                jobName = sel.Object.Name
                FreeCADGui.Selection.clearSelection()

        # populate the job selection combobox
        form.comboJobs.blockSignals(True)
        form.comboJobs.clear()
        form.comboJobs.blockSignals(False)
        for j in jobList:
            form.comboJobs.addItem(j.ViewObject.Icon, j.Label)
            self.jobs.append(j)
            if j.Name == jobName or jCnt == 1:
                setJobIdx = jIdx
            jIdx += 1

        # Pre-select GUI-selected job in the combobox
        if jobName or jCnt == 1:
            form.comboJobs.setCurrentIndex(setJobIdx)
        else:
            form.comboJobs.setCurrentIndex(0)

    def SetupSimulation(self):
        form = self.taskForm.form
        self.activeOps = []
        self.numCommands = 0
        self.ioperation = 0
        for i in range(form.listOperations.count()):
            if form.listOperations.item(i).checkState() == QtCore.Qt.CheckState.Checked:
                self.firstDrill = True
                self.activeOps.append(self.operations[i])
                self.numCommands += len(self.operations[i].Path.Commands)

        self.stock = self.job.Stock.Shape
        maxlen = self.stock.BoundBox.XLength
        if maxlen < self.stock.BoundBox.YLength:
            maxlen = self.stock.BoundBox.YLength
        self.resolution = 0.01 * self.accuracy * maxlen
        self.busy = False
        self.tool = None
        for i in range(len(self.activeOps)):
            self.SetupOperation(0)
            if self.tool is not None:
                break
        self.iprogress = 0
        self.UpdateProgress()

    def SetupOperation(self, itool):
        self.operation = self.activeOps[itool]
        try:
            self.tool = PathDressup.toolController(self.operation).Tool
        except Exception:
            self.tool = None

        if self.tool is not None:
            # GL: add tool table
            pass
        self.icmd = 0
        self.curpos = FreeCAD.Placement(self.initialPos, self.stdrot)
        self.opCommands = PathUtils.getPathWithPlacement(self.operation).Commands

    def SimulateMill(self):
        self.job = self.jobs[self.taskForm.form.comboJobs.currentIndex()]
        self.busy = False
        self.height = 10
        self.skipStep = False
        self.initialPos = Vector(0, 0, self.job.Stock.Shape.BoundBox.ZMax)

        # Add cut material - GL: add stock to wind

        self.SetupSimulation()
        self.resetSimulation = True
        FreeCAD.ActiveDocument.recompute()

    def PerformCutVoxel(self):
        if self.resetSimulation:
            self.resetSimulation = False
            self.SetupSimulation()

        if self.busy:
            return
        self.busy = True

        cmd = self.opCommands[self.icmd]
        # for cmd in job.Path.Commands:
        # GL: add all commands to simulator
        # GL: support all the following
        if cmd.Name in ["G0", "G1", "G2", "G3"]:
            pass
        if cmd.Name in ["G80"]:
            pass
        if cmd.Name in ["G73", "G81", "G82", "G83"]:
            pass
            extendcommands = []
            if self.firstDrill:
                extendcommands.append(Path.Command("G0", {"Z": cmd.r}))
                self.firstDrill = False
            extendcommands.append(
                Path.Command("G0", {"X": cmd.x, "Y": cmd.y, "Z": cmd.r})
            )
            extendcommands.append(
                Path.Command("G1", {"X": cmd.x, "Y": cmd.y, "Z": cmd.z})
            )
            extendcommands.append(
                Path.Command("G1", {"X": cmd.x, "Y": cmd.y, "Z": cmd.r})
            )
        self.icmd += 1
        self.iprogress += 1
        self.busy = False

    def PerformCut(self):
        self.PerformCutVoxel()

    def onJobChange(self):
        form = self.taskForm.form
        j = self.jobs[form.comboJobs.currentIndex()]
        self.job = j
        form.listOperations.clear()
        self.operations = []
        for op in j.Operations.OutList:
            if PathUtil.opProperty(op, "Active"):
                listItem = QtGui.QListWidgetItem(op.ViewObject.Icon, op.Label)
                listItem.setFlags(listItem.flags() | QtCore.Qt.ItemIsUserCheckable)
                listItem.setCheckState(QtCore.Qt.CheckState.Checked)
                self.operations.append(op)
                form.listOperations.addItem(listItem)
        if self.initdone:
            self.SetupSimulation()

    def onAccuracyBarChange(self):
        form = self.taskForm.form
        self.accuracy = 1.1 - 0.1 * form.sliderAccuracy.value()
        form.labelAccuracy.setText(str(round(self.accuracy, 1)) + "%")

    def EndSimulation(self):
        self.resetSimulation = True

    def SimStop(self):
        self.EndSimulation()

    def SimPlay(self):
        self.voxSim.BeginSimulation(self.stock, self.resolution)
        # GL: update simulator and open window 
        pass

    def accept(self):
        self.EndSimulation()

    def cancel(self):
        self.EndSimulation()


class CommandCAMSimulate:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Simulator",
            "MenuText": QtCore.QT_TRANSLATE_NOOP("CAM_Simulator", "CAM Simulator"),
            "Accel": "P, M",
            "ToolTip": QtCore.QT_TRANSLATE_NOOP(
                "CAM_Simulator", "Simulate G-code on stock"
            ),
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):
        CamSimulation = CAMSimulation()
        CamSimulation.Activate()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_SimulatorGL", CommandCAMSimulate())
    FreeCAD.Console.PrintLog("Loading PathSimulator Gui... done\n")
