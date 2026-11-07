# SPDX-License-Identifier: LGPL-2.1-or-later

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
import Path
import Path.Base.Util as PathUtil
import Path.Dressup.Utils as PathDressup
from Machine.models.machine import AxisRole
from Path.Base.Generator import rotation
from Path.Post import Utils as PostUtils
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


def TablePose(machine, placement, current=None):
    """TablePose(machine, placement, current=None) ... Return (rotation, positions): how the
    machine's rotary table turns the part to cut on placement's work plane, and the rotary axis
    positions that give it, solved the way the post solves them. current holds the previous
    operation's positions, which the solver stays nearest to.

    The simulator turns the part by the rotation, so on a trunnion the tool stays vertical and
    the part tilts under it; a tilting head leaves the part alone. Without a machine that
    reaches the plane, the part is turned the shortest way that makes the tool axis vertical and
    positions is None."""
    z = Vector(0, 0, 1)
    axis = placement.Rotation.multVec(z)
    rotary = machine is not None and machine.has_rotary_axes
    chain = rotation.build_kinematic_chain(machine) if rotary else []
    if axis.isEqual(z, 1e-6):
        return Rotation(), {ax.name: 0.0 for ax in chain} if rotary else None
    if rotary:
        result = rotation.solve_orientation(machine, axis, current_state=current)
        if result.success:
            table = [ax for ax in chain if ax.role == AxisRole.TABLE_ROTARY]
            return rotation.compute_rotation_matrix(table, result.angles), result.angles
    return Rotation(axis, z), None


def RotaryAxes(machine):
    """Return the machine's (table, head) rotary axes, in the order compute_rotation_matrix
    applies them: each one's axes from the one nearest the part or tool out to the one on the
    machine, as they are mounted on each other; mounted on neither, a table's azimuth (about Z)
    first and a head's tilt first."""
    if machine is None or not machine.has_rotary_axes:
        return [], []

    def applied(axes, table):
        if all(axes[i].parent == axes[i - 1].name for i in range(1, len(axes))):
            return axes[::-1]
        return sorted(axes, key=lambda ax: (abs(ax.rotation_vector.z) > 0.9) != table)

    chain = rotation.build_kinematic_chain(machine)
    table = [ax for ax in chain if ax.role == AxisRole.TABLE_ROTARY]
    head = [ax for ax in chain if ax.role == AxisRole.HEAD_ROTARY]
    return applied(table, True), applied(head, False)


def LastRotaryPositions(op, axes):
    """LastRotaryPositions(op, axes) ... Return {axis name: position} for each axis op's path
    moves, as the path leaves it. Used for operations that turn the rotaries themselves, such
    as a 4th-axis surface."""
    positions = {}
    commands = op.Path.Commands if op.Path else []
    for ax in axes:
        for cmd in reversed(commands):
            if ax.name in cmd.Parameters:
                positions[ax.name] = float(cmd.Parameters[ax.name])
                break
    return positions


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
        shape = tool.Shape.copy()
        shape.Placement = Placement()
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

        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
        if prefs.GetBool("SimulatorFollowsVisibility"):
            form.followsVisibility.setCheckState(QtCore.Qt.CheckState.Checked)
        form.followsVisibility.clicked.connect(self.followsVisibilityChange)

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
        # Get list of Job objects in active document
        jobList = FreeCAD.ActiveDocument.findObjects("Path::FeaturePython", "Job.*")

        # Get name of selected Job
        jobName = ""
        selection = FreeCADGui.Selection.getSelection()
        if selection:  #  Identify job selected by user
            job = PathUtils.findParentJob(selection[0])
            if job:
                jobName = job.Name

        # Prepare combobox
        form.comboJobs.blockSignals(True)
        form.comboJobs.clear()
        form.comboJobs.blockSignals(False)

        # Get index of selected Job
        setJobIdx = 0
        for i, job in enumerate(jobList):
            # Populate the job selection combobox
            form.comboJobs.addItem(job.ViewObject.Icon, job.Label)
            self.jobs.append(job)
            if job.Name == jobName:
                setJobIdx = i

        # Preselect GUI-selected job in the combobox
        form.comboJobs.setCurrentIndex(setJobIdx)

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
        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
        followsVisibility = prefs.GetBool("SimulatorFollowsVisibility")
        form = self.taskForm.form
        j = self.jobs[form.comboJobs.currentIndex()]
        self.job = j
        form.listOperations.clear()
        self.operations = []
        allhidden = all(
            not op.Visibility for op in j.Operations.OutList if PathUtil.opProperty(op, "Active")
        )
        for op in j.Operations.OutList:
            if PathUtil.opProperty(op, "Active"):
                listItem = QtGui.QListWidgetItem(op.ViewObject.Icon, op.Label)
                listItem.setFlags(listItem.flags() | QtCore.Qt.ItemIsUserCheckable)
                if followsVisibility and not op.Visibility and not allhidden:
                    listItem.setCheckState(QtCore.Qt.CheckState.Unchecked)
                else:
                    listItem.setCheckState(QtCore.Qt.CheckState.Checked)
                self.operations.append(op)
                form.listOperations.addItem(listItem)
        if len(j.Model.OutList) > 0:
            self.baseShape = Part.makeCompound([o.Shape for o in j.Model.OutList])
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

    def followsVisibilityChange(self):
        """Update job list in accordance with operations visibility"""
        form = self.taskForm.form
        state = form.followsVisibility.isChecked()
        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
        prefs.SetBool("SimulatorFollowsVisibility", state)
        self.onJobChange()

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
        self.millSim.ResetSimulation(FreeCADGui.getDocument(self.job.Document))
        machine = self.job.Proxy.getMachine()
        # each axis turns at its own rate and in the machine's sequence: the table's turn the
        # part, the head's tilt the tool
        table, head = RotaryAxes(machine)
        axes = table + head
        # degrees per second the rotaries turn to a new pose; the slowest axis sets the pace
        indexRate = min((ax.max_velocity for ax in axes), default=0.0) / 60.0
        self.millSim.SetRotaryAxes(
            [
                (ax.name, ax.rotation_vector, ax.max_velocity / 60.0, ax.sequence, ax in head)
                for ax in axes
            ]
        )
        positions = None
        for op in self.activeOps:
            self.millSim.BeginOperation(op.Label)
            tc = PathDressup.toolController(op)
            tool = tc.Tool
            toolNumber = tc.ToolNumber
            # arcs in the profile become chords: finer at high quality, where the cut surface
            # is fine enough to show them
            toolProfile = self.GetToolProfile(tool, 0.25 if self.quality >= 9 else 0.5)
            self.millSim.AddTool(toolProfile, toolNumber, tool.Diameter, 1)
            # The simulation runs at the programmed feeds. As in the cycle time estimate, a G0
            # without F moves at the tool controller's rapid rate (its feed if none is set) and a
            # feed move without F at its horizontal feed; a G0 or G1 with only F sets them.
            hFeed = tc.HorizFeed.Value
            rapid = tc.HorizRapid.Value or hFeed
            if rapid > 0:
                self.millSim.AddCommand(Path.Command("G0", {"F": rapid}))
            if hFeed > 0:
                self.millSim.AddCommand(Path.Command("G1", {"F": hFeed}))
            # An operation's path is stored in its work plane's frame, the tool along the
            # frame's Z, and its Placement positions it. The simulator takes path and frame as
            # they are, so a tilted operation cuts with a tilted tool and its arcs stay arcs.
            # The table pose turns the part the way the machine does.
            frame = getattr(op, "Placement", Placement())
            pose, solved = TablePose(machine, frame, positions)
            positions = solved or positions
            angles = [float(solved[ax.name]) for ax in axes] if solved else []
            self.millSim.SetFrame(frame, pose, indexRate, angles)
            # the drill cycles' G98, G99 and G80 as the post writes them
            opCommands = PostUtils.cannedCycleTerminator(op.Path).Commands
            for cmd in opCommands:
                self.millSim.AddCommand(cmd)
            # an operation that turns the rotaries itself leaves them where it ends; solve the
            # next operation's pose from there
            ended = LastRotaryPositions(op, axes)
            if ended:
                positions = dict(positions or {}, **ended)
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
            "MenuText": QtCore.QT_TRANSLATE_NOOP("CAM_SimulatorGL", "CAM Simulator"),
            "Accel": "P, N",
            "ToolTip": QtCore.QT_TRANSLATE_NOOP("CAM_SimulatorGL", "Simulates G-code on stock"),
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
