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
from PySide2.QtWidgets import QDialogButtonBox

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Mesh = LazyLoader("Mesh", globals(), "Mesh")
Part = LazyLoader("Part", globals(), "Part")

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui, QtCore

_filePath = os.path.dirname(os.path.abspath(__file__))

def IsSame(x, y):
    return abs(x - y) < 0.0001

def RadiusAt(edge, p):
    x = edge.valueAt(p).x
    y = edge.valueAt(p).y
    return math.sqrt(x * x + y * y)


class CAMSimTaskUi:
    def __init__(self, parent):
        # this will create a Qt widget from our ui file
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/TaskCAMSimulator.ui")
        self.parent = parent

    def getStandardButtons(self, *args):
        return QDialogButtonBox.Close

    def reject(self):
        self.parent.cancel()
        FreeCADGui.Control.closeDialog()


def TSError(msg):
    QtGui.QMessageBox.information(None, "Path Simulation", msg)


class CAMSimulation:
    def __init__(self):
        self.debug = False
        self.stdrot = FreeCAD.Rotation(Vector(0, 0, 1), 0)
        self.iprogress = 0
        self.numCommands = 0
        self.simperiod = 20
        self.quality = 10
        self.resetSimulation = False
        self.jobs = []

    def Connect(self, but, sig):
        QtCore.QObject.connect(but, QtCore.SIGNAL("clicked()"), sig)

    ## Convert tool shape to tool profile needed by GL simulator
    def FindClosestEdge(self, edges, px, pz):
        for edge in edges:
            p1 = edge.FirstParameter
            p2 = edge.LastParameter
            rad = RadiusAt(edge, p1)
            z = edge.valueAt(p1).z
            if IsSame(px, rad) and IsSame(pz, z):
                return edge, p1, p2
            rad = RadiusAt(edge, p2)
            z = edge.valueAt(p2).z
            if IsSame(px, rad) and IsSame(pz, z):
                return edge, p2, p1
        return None, 0.0, 0.0

    def FindTopMostEdge(self, edges):
        maxz = 0.0
        topedge = None
        top_p1 = 0.0
        top_p2 = 0.0
        for edge in edges:
            p1 = edge.FirstParameter
            p2 = edge.LastParameter
            z = edge.valueAt(p1).z
            if z > maxz:
                topedge = edge
                top_p1 = p1
                top_p2 = p2
                maxz = z
            z = edge.valueAt(p2).z
            if z > maxz:
                topedge = edge
                top_p1 = p2
                top_p2 = p1
                maxz = z
        return topedge, top_p1, top_p2

    #the algo is based on locating the side edge that OCC creates on any revolved object 
    def GetToolProfile(self, tool, resolution):
        shape = tool.Shape
        sideEdgeList = []
        for i in range(len(shape.Edges)):
            edge = shape.Edges[i]
            if not edge.isClosed():
                v1 = edge.firstVertex()
                v2 = edge.lastVertex()
                tp = "arc" if type(edge.Curve) is Part.Circle else "line"
                sideEdgeList.append(edge)

        # sort edges as a single 3d line on the x-z plane
        profile = [0.0, 0.0]

        # first find the topmost edge
        edge, p1, p2 = self.FindTopMostEdge(sideEdgeList)
        profile = [RadiusAt(edge, p1), edge.valueAt(p1).z]
        endrad = 0.0
        # one by one find all connecting edges
        while edge is not None:
            sideEdgeList.remove(edge)
            if type(edge.Curve) is Part.Circle:
                # if edge is curved, aproximate it with lines based on resolution
                nsegments = int(edge.Length / resolution) + 1
                step = (p2 - p1) / nsegments
                location = p1 + step
                print (edge.Length, nsegments, step)
                while nsegments > 0:
                    endrad = RadiusAt(edge, location)
                    endz = edge.valueAt(location).z
                    profile.append(endrad)
                    profile.append(endz)
                    location += step
                    nsegments -= 1
            else:
                endrad = RadiusAt(edge, p2)
                endz = edge.valueAt(p2).z
                profile.append(endrad)
                profile.append(endz)
            edge, p1, p2 =  self.FindClosestEdge(sideEdgeList, endrad, endz)
            if edge is None:
                break
        return profile

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
        self.millSim = CAMSimulator.PathSim()
        self.initdone = True
        self.job = self.jobs[self.taskForm.form.comboJobs.currentIndex()]
        self.SetupSimulation()

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
        self.busy = False

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
        self.quality = form.sliderAccuracy.value()
        qualText = QtCore.QT_TRANSLATE_NOOP("CAM_Simulator", "High")
        if (self.quality < 4):
            qualText = QtCore.QT_TRANSLATE_NOOP("CAM_Simulator", "Low")
        elif (self.quality < 9):
            qualText = QtCore.QT_TRANSLATE_NOOP("CAM_Simulator", "Medium")
        form.labelAccuracy.setText(qualText)

    def SimPlay(self):
        self.millSim.ResetSimulation()
        for op in self.activeOps:
            tool = PathDressup.toolController(op).Tool
            toolNumber = PathDressup.toolController(op).ToolNumber
            toolProfile = self.GetToolProfile(tool, 0.5)
            self.millSim.AddTool(toolProfile, toolNumber, tool.Diameter, 1)
            opCommands = PathUtils.getPathWithPlacement(op).Commands
            for cmd in opCommands:
                self.millSim.AddCommand(cmd)
        self.millSim.BeginSimulation(self.stock, self.quality)

    def cancel(self):
        #self.EndSimulation()
        pass


class CommandCAMSimulate:
    def GetResources(self):
        return {
            "Pixmap": "CAM_SimulatorGL",
            "MenuText": QtCore.QT_TRANSLATE_NOOP("CAM_Simulator", "New CAM Simulator"),
            "Accel": "P, N",
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
