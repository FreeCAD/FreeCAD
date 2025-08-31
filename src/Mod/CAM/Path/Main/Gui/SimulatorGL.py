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
"""
Command and task window handler for the OpenGL based CAM simulator
"""


import math
import os
import FreeCAD
import Path.Base.Util as PathUtil
import Path.Dressup.Utils as PathDressup
import Path.Main.Job as PathJob
from PathScripts import PathUtils
import CAMSimulator

from FreeCAD import Vector, Placement, Rotation


# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Mesh = LazyLoader("Mesh", globals(), "Mesh")
Part = LazyLoader("Part", globals(), "Part")

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui, QtCore
    from PySide.QtGui import QDialogButtonBox

_filePath = os.path.dirname(os.path.abspath(__file__))


def IsSame(x, y):
    """Check if two floats are the same within an epsilon"""
    return abs(x - y) < 0.0001


def RadiusAt(edge, p):
    """Find the tool radius within a point on its circumference"""
    x = edge.valueAt(p).x
    y = edge.valueAt(p).y
    return math.sqrt(x * x + y * y)


class CAMSimTaskUi:
    """Handles the simulator task panel"""

    def __init__(self, parent):
        # this will create a Qt widget from our ui file
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/TaskCAMSimulator.ui")
        self.parent = parent

    def getStandardButtons(self, *_args):
        """Task panel needs only Close button"""
        return QDialogButtonBox.Close

    def reject(self):
        """User Pressed the Close button"""
        self.parent.cancel()
        FreeCADGui.Control.closeDialog()


def TSError(msg):
    """Display error message"""
    QtGui.QMessageBox.information(None, "Path Simulation", msg)


class CAMSimulation:
    """Handles and prepares CAM jobs for simulation"""

    def __init__(self):
        self.debug = False
        self.stdrot = FreeCAD.Rotation(Vector(0, 0, 1), 0)
        self.iprogress = 0
        self.numCommands = 0
        self.simperiod = 20
        self.quality = 10
        self.resetSimulation = False
        self.jobs = []
        self.initdone = False
        self.taskForm = None
        self.disableAnim = False
        self.firstDrill = True
        self.millSim = None
        self.job = None
        self.activeOps = []
        self.ioperation = 0
        self.stock = None
        self.busy = False
        self.operations = []
        self.baseShape = None

    def Connect(self, but, sig):
        """Connect task panel buttons"""
        QtCore.QObject.connect(but, QtCore.SIGNAL("clicked()"), sig)

    def FindClosestEdge(self, edges, px, pz):
        """Convert tool shape to tool profile needed by GL simulator"""
        for edge in edges:
            p1 = edge.FirstParameter
            p2 = edge.LastParameter
            rad1 = RadiusAt(edge, p1)
            z1 = edge.valueAt(p1).z
            if IsSame(px, rad1) and IsSame(pz, z1):
                return edge, p1, p2
            rad2 = RadiusAt(edge, p2)
            z2 = edge.valueAt(p2).z
            if IsSame(px, rad2) and IsSame(pz, z2):
                return edge, p2, p1
            # sometimes a flat circle is without edge, so return edge with
            # same height and later a connecting edge will be interpolated
            if IsSame(pz, z1):
                return edge, p1, p2
            if IsSame(pz, z2):
                return edge, p2, p1
        return None, 0.0, 0.0

    def FindTopMostEdge(self, edges):
        """Examine tool solid edges and find the top most one"""
        maxz = -99999999.0
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

    def GetToolProfile(self, tool, resolution):
        """Get the edge profile of a tool solid. Basically locating the
        side edge that OCC creates on any revolved object
        """
        originalPlacement = tool.Placement
        tool.Placement = Placement(Vector(0, 0, 0), Rotation(Vector(0, 0, 1), 0), Vector(0, 0, 0))
        shape = tool.Shape
        tool.Placement = originalPlacement
        sideEdgeList = []
        for _i, edge in enumerate(shape.Edges):
            if not edge.isClosed():
                # v1 = edge.firstVertex()
                # v2 = edge.lastVertex()
                # tp = "arc" if type(edge.Curve) is Part.Circle else "line"
                sideEdgeList.append(edge)

        # sort edges as a single 3d line on the x-z plane

        # first find the topmost edge
        edge, p1, p2 = self.FindTopMostEdge(sideEdgeList)
        profile = [RadiusAt(edge, p1), edge.valueAt(p1).z]
        endrad = 0.0
        # one by one find all connecting edges
        while edge is not None:
            sideEdgeList.remove(edge)
            if isinstance(edge.Curve, Part.Circle):
                # if edge is curved, approximate it with lines based on resolution
                nsegments = int(edge.Length / resolution) + 1
                step = (p2 - p1) / nsegments
                location = p1 + step
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
            edge, p1, p2 = self.FindClosestEdge(sideEdgeList, endrad, endz)
            if edge is None:
                break
            startrad = RadiusAt(edge, p1)
            if not IsSame(startrad, endrad):
                profile.append(startrad)
                startz = edge.valueAt(p1).z
                profile.append(startz)

        return profile

    def Activate(self):
        """Invoke the simulator task panel"""
        self.initdone = False
        self.taskForm = CAMSimTaskUi(self)
        form = self.taskForm.form
        self.Connect(form.toolButtonPlay, self.SimPlay)
        form.sliderAccuracy.valueChanged.connect(self.onAccuracyBarChange)
        self.onAccuracyBarChange()
        self._populateJobSelection(form)
        form.comboJobs.currentIndexChanged.connect(self.onJobChange)
        self.onJobChange()
        form.listOperations.itemChanged.connect(self.onOperationItemChange)
        FreeCADGui.Control.showDialog(self.taskForm)
        self.disableAnim = False
        self.firstDrill = True
        self.millSim = CAMSimulator.PathSim()
        self.initdone = True
        self.job = self.jobs[self.taskForm.form.comboJobs.currentIndex()]
        # self.SetupSimulation()

    def _populateJobSelection(self, form):
        """Make Job selection combobox"""
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
            if hasattr(sel.Object, "Proxy") and isinstance(sel.Object.Proxy, PathJob.ObjectJob):
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

        # Preselect GUI-selected job in the combobox
        if jobName or jCnt == 1:
            form.comboJobs.setCurrentIndex(setJobIdx)
        else:
            form.comboJobs.setCurrentIndex(0)

    def SetupSimulation(self):
        """Prepare all selected job operations for simulation"""
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
        """When a new job is selected from the drop-down, update job operation list"""
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
        if len(j.Model.OutList) > 0:
            self.baseShape = j.Model.OutList[0].Shape
        else:
            self.baseShape = None

    def onAccuracyBarChange(self):
        """Update simulation quality"""
        form = self.taskForm.form
        self.quality = form.sliderAccuracy.value()
        qualText = QtCore.QT_TRANSLATE_NOOP("CAM_Simulator", "High")
        if self.quality < 4:
            qualText = QtCore.QT_TRANSLATE_NOOP("CAM_Simulator", "Low")
        elif self.quality < 9:
            qualText = QtCore.QT_TRANSLATE_NOOP("CAM_Simulator", "Medium")
        form.labelAccuracy.setText(qualText)

    def onOperationItemChange(self, _item):
        """Check if at least one operation is selected to enable the Play button"""
        playvalid = False
        form = self.taskForm.form
        for i in range(form.listOperations.count()):
            if form.listOperations.item(i).checkState() == QtCore.Qt.CheckState.Checked:
                playvalid = True
                break
        form.toolButtonPlay.setEnabled(playvalid)

    def SimPlay(self):
        """Activate the simulation"""
        self.SetupSimulation()
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
        if self.baseShape is not None:
            self.millSim.SetBaseShape(self.baseShape, 1)

    def cancel(self):
        """Cancel the simulation"""


class CommandCAMSimulate:
    """FreeCAD invoke simulation task panel command"""

    def GetResources(self):
        """Command info"""
        return {
            "Pixmap": "CAM_SimulatorGL",
            "MenuText": QtCore.QT_TRANSLATE_NOOP("CAM_Simulator", "CAM Simulator"),
            "Accel": "P, N",
            "ToolTip": QtCore.QT_TRANSLATE_NOOP("CAM_Simulator", "Simulates G-code on stock"),
        }

    def IsActive(self):
        """Command is active if at least one CAM job exists"""
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):
        """Activate the simulation"""
        CamSimulation = CAMSimulation()
        CamSimulation.Activate()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_SimulatorGL", CommandCAMSimulate())
    FreeCAD.Console.PrintLog("Loading PathSimulator Gui… done\n")
