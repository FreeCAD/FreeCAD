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
import PathSimulator
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
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/TaskPathSimulator.ui")
        self.parent = parent

    def accept(self):
        self.parent.accept()
        FreeCADGui.Control.closeDialog()

    def reject(self):
        self.parent.cancel()
        FreeCADGui.Control.closeDialog()


def TSError(msg):
    QtGui.QMessageBox.information(None, "Path Simulation", msg)


class PathSimulation:
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
        self.Connect(form.toolButtonStop, self.SimStop)
        self.Connect(form.toolButtonPlay, self.SimPlay)
        self.Connect(form.toolButtonPause, self.SimPause)
        self.Connect(form.toolButtonStep, self.SimStep)
        self.Connect(form.toolButtonFF, self.SimFF)
        form.sliderSpeed.valueChanged.connect(self.onSpeedBarChange)
        self.onSpeedBarChange()
        form.sliderAccuracy.valueChanged.connect(self.onAccuracyBarChange)
        self.onAccuracyBarChange()
        self._populateJobSelection(form)
        form.comboJobs.currentIndexChanged.connect(self.onJobChange)
        self.onJobChange()
        FreeCADGui.Control.showDialog(self.taskForm)
        self.disableAnim = False
        self.isVoxel = True
        self.firstDrill = True
        self.voxSim = PathSimulator.PathSim()
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
        if self.isVoxel:
            maxlen = self.stock.BoundBox.XLength
            if maxlen < self.stock.BoundBox.YLength:
                maxlen = self.stock.BoundBox.YLength
            self.resolution = 0.01 * self.accuracy * maxlen
            self.voxSim.BeginSimulation(self.stock, self.resolution)
            (
                self.cutMaterial.Mesh,
                self.cutMaterialIn.Mesh,
            ) = self.voxSim.GetResultMesh()
        else:
            self.cutMaterial.Shape = self.stock
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
            self.cutTool.Shape = self.tool.Shape

            if not self.cutTool.Shape.isValid() or self.cutTool.Shape.isNull():
                self.EndSimulation()
                raise RuntimeError(
                    "Path Simulation: Error in tool geometry - {}".format(
                        self.tool.Name
                    )
                )

            self.cutTool.ViewObject.show()
            self.voxSim.SetToolShape(self.cutTool.Shape, 0.05 * self.accuracy)
        self.icmd = 0
        self.curpos = FreeCAD.Placement(self.initialPos, self.stdrot)
        self.cutTool.Placement = self.curpos
        self.opCommands = PathUtils.getPathWithPlacement(self.operation).Commands

    def SimulateMill(self):
        self.job = self.jobs[self.taskForm.form.comboJobs.currentIndex()]
        self.busy = False
        self.height = 10
        self.skipStep = False
        self.initialPos = Vector(0, 0, self.job.Stock.Shape.BoundBox.ZMax)
        # Add cut tool
        self.cutTool = FreeCAD.ActiveDocument.addObject(
            "Part::FeaturePython", "CutTool"
        )
        self.cutTool.ViewObject.Proxy = 0
        self.cutTool.ViewObject.hide()

        # Add cut material
        if self.isVoxel:
            self.cutMaterial = FreeCAD.ActiveDocument.addObject(
                "Mesh::FeaturePython", "CutMaterial"
            )
            self.cutMaterialIn = FreeCAD.ActiveDocument.addObject(
                "Mesh::FeaturePython", "CutMaterialIn"
            )
            self.cutMaterialIn.ViewObject.Proxy = 0
            self.cutMaterialIn.ViewObject.show()
            self.cutMaterialIn.ViewObject.ShapeColor = (1.0, 0.85, 0.45, 0.0)
        else:
            self.cutMaterial = FreeCAD.ActiveDocument.addObject(
                "Part::FeaturePython", "CutMaterial"
            )
            self.cutMaterial.Shape = self.job.Stock.Shape
        self.cutMaterial.ViewObject.Proxy = 0
        self.cutMaterial.ViewObject.show()
        self.cutMaterial.ViewObject.ShapeColor = (0.5, 0.25, 0.25, 0.0)

        # Add cut path solid for debug
        if self.debug:
            self.cutSolid = FreeCAD.ActiveDocument.addObject(
                "Part::FeaturePython", "CutDebug"
            )
            self.cutSolid.ViewObject.Proxy = 0
            self.cutSolid.ViewObject.hide()

        self.SetupSimulation()
        self.resetSimulation = True
        FreeCAD.ActiveDocument.recompute()

    def PerformCutBoolean(self):
        if self.resetSimulation:
            self.resetSimulation = False
            self.SetupSimulation()

        if self.busy:
            return
        self.busy = True

        cmd = self.opCommands[self.icmd]
        pathSolid = None

        if cmd.Name in ["G0"]:
            self.firstDrill = True
            self.curpos = self.RapidMove(cmd, self.curpos)
        if cmd.Name in ["G1", "G2", "G3"]:
            self.firstDrill = True
            if self.skipStep:
                self.curpos = self.RapidMove(cmd, self.curpos)
            else:
                (pathSolid, self.curpos) = self.GetPathSolid(
                    self.tool, cmd, self.curpos
                )

        if cmd.Name in ["G80"]:
            self.firstDrill = True
        if cmd.Name in ["G73", "G81", "G82", "G83"]:
            if self.firstDrill:
                extendcommand = Path.Command("G0", {"Z": cmd.r})
                self.curpos = self.RapidMove(extendcommand, self.curpos)
                self.firstDrill = False
            extendcommand = Path.Command("G0", {"X": cmd.x, "Y": cmd.y, "Z": cmd.r})
            self.curpos = self.RapidMove(extendcommand, self.curpos)
            extendcommand = Path.Command("G1", {"X": cmd.x, "Y": cmd.y, "Z": cmd.z})
            self.curpos = self.RapidMove(extendcommand, self.curpos)
            extendcommand = Path.Command("G1", {"X": cmd.x, "Y": cmd.y, "Z": cmd.r})
            self.curpos = self.RapidMove(extendcommand, self.curpos)
        self.skipStep = False
        if pathSolid is not None:
            if self.debug:
                self.cutSolid.Shape = pathSolid
            newStock = self.stock.cut([pathSolid], 1e-3)
            try:
                if newStock.isValid():
                    self.stock = newStock.removeSplitter()
            except Exception:
                if self.debug:
                    print("invalid cut at cmd #{}".format(self.icmd))
        if not self.disableAnim:
            self.cutTool.Placement = FreeCAD.Placement(self.curpos, self.stdrot)
        self.icmd += 1
        self.iprogress += 1
        self.UpdateProgress()
        if self.icmd >= len(self.opCommands):
            self.ioperation += 1
            if self.ioperation >= len(self.activeOps):
                self.EndSimulation()
                return
            else:
                self.SetupOperation(self.ioperation)
        if not self.disableAnim:
            self.cutMaterial.Shape = self.stock
        self.busy = False

    def PerformCutVoxel(self):
        if self.resetSimulation:
            self.resetSimulation = False
            self.SetupSimulation()

        if self.busy:
            return
        self.busy = True

        cmd = self.opCommands[self.icmd]
        # for cmd in job.Path.Commands:
        if cmd.Name in ["G0", "G1", "G2", "G3"]:
            self.firstDrill = True
            if cmd.Name in ["G2", "G3"] and (cmd.k or 0) == 0:
                cx = self.curpos.Base.x + (cmd.i or 0)
                cy = self.curpos.Base.y + (cmd.j or 0)
                a0 = math.atan2(self.curpos.Base.y - cy, self.curpos.Base.x - cx)
                a1 = math.atan2(cmd.y - cy, cmd.x - cx)
                da = a1 - a0
                if cmd.Name == "G3":
                    da = da % (2 * math.pi)
                else:
                    da = -((-da) % (2 * math.pi))
                r = math.sqrt((cmd.i or 0) ** 2 + (cmd.j or 0) ** 2)
                n = math.ceil(math.sqrt(r / self.resolution * da * da))
                da = da / n
                dz = (cmd.z - self.curpos.Base.z) / n
                cmd.Name = "G1"
                for i in range(n):
                    a0 += da
                    cmd.x = cx + r * math.cos(a0)
                    cmd.y = cy + r * math.sin(a0)
                    cmd.z = self.curpos.Base.z + dz
                    self.curpos = self.voxSim.ApplyCommand(self.curpos, cmd)
            else:
                self.curpos = self.voxSim.ApplyCommand(self.curpos, cmd)
            if not self.disableAnim:
                self.cutTool.Placement = self.curpos
                (
                    self.cutMaterial.Mesh,
                    self.cutMaterialIn.Mesh,
                ) = self.voxSim.GetResultMesh()
        if cmd.Name in ["G80"]:
            self.firstDrill = True
        if cmd.Name in ["G73", "G81", "G82", "G83"]:
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
            for ecmd in extendcommands:
                self.curpos = self.voxSim.ApplyCommand(self.curpos, ecmd)
                if not self.disableAnim:
                    self.cutTool.Placement = self.curpos
                    (
                        self.cutMaterial.Mesh,
                        self.cutMaterialIn.Mesh,
                    ) = self.voxSim.GetResultMesh()
        self.icmd += 1
        self.iprogress += 1
        self.UpdateProgress()
        if self.icmd >= len(self.opCommands):
            self.ioperation += 1
            if self.ioperation >= len(self.activeOps):
                self.EndSimulation()
                return
            else:
                self.SetupOperation(self.ioperation)
        self.busy = False

    def PerformCut(self):
        if self.isVoxel:
            self.PerformCutVoxel()
        else:
            self.PerformCutBoolean()

    def RapidMove(self, cmd, curpos):
        path = Path.Geom.edgeForCmd(cmd, curpos)  # hack to overcome occ bug
        if path is None:
            return curpos
        return path.valueAt(path.LastParameter)

    # get a solid representation of a tool going along path
    def GetPathSolid(self, tool, cmd, pos):
        toolPath = Path.Geom.edgeForCmd(cmd, pos)
        startDir = toolPath.tangentAt(0)
        startDir[2] = 0.0
        endPos = toolPath.valueAt(toolPath.LastParameter)
        endDir = toolPath.tangentAt(toolPath.LastParameter)
        try:
            startDir.normalize()
            endDir.normalize()
        except Exception:
            return (None, endPos)

        # hack to overcome occ bugs
        rad = float(tool.Diameter) / 2.0 - 0.001 * pos[2]
        if type(toolPath.Curve) is Part.Circle and toolPath.Curve.Radius <= rad:
            rad = toolPath.Curve.Radius - 0.01 * (pos[2] + 1)
            return (None, endPos)

        # create the path shell
        toolProf = self.CreateToolProfile(tool, startDir, pos, rad)
        rotmat = Base.Matrix()
        rotmat.move(pos.negative())
        rotmat.rotateZ(math.pi)
        rotmat.move(pos)
        mirroredProf = toolProf.transformGeometry(rotmat)
        fullProf = Part.Wire([toolProf, mirroredProf])
        pathWire = Part.Wire(toolPath)
        try:
            pathShell = pathWire.makePipeShell([fullProf], False, True)
        except Exception:
            if self.debug:
                Part.show(pathWire)
                Part.show(fullProf)
            return (None, endPos)

        # create the start cup
        startCup = toolProf.revolve(pos, Vector(0, 0, 1), -180)

        # create the end cup
        endProf = self.CreateToolProfile(tool, endDir, endPos, rad)
        endCup = endProf.revolve(endPos, Vector(0, 0, 1), 180)

        fullShell = Part.makeShell(startCup.Faces + pathShell.Faces + endCup.Faces)
        return (Part.makeSolid(fullShell).removeSplitter(), endPos)

    # create radial profile of the tool (90 degrees to the direction of the path)
    def CreateToolProfile(self, tool, dir, pos, rad):
        type = tool.ToolType
        xf = dir[0] * rad
        yf = dir[1] * rad
        xp = pos[0]
        yp = pos[1]
        zp = pos[2]
        h = tool.CuttingEdgeHeight
        if h <= 0.0:  # set default if user fails to avoid freeze
            h = 1.0
            Path.Log.error("SET Tool Length")
        # common to all tools
        vTR = Vector(xp + yf, yp - xf, zp + h)
        vTC = Vector(xp, yp, zp + h)
        vBC = Vector(xp, yp, zp)
        lT = Part.makeLine(vTR, vTC)
        res = None
        if type == "ChamferMill":
            ang = 90 - tool.CuttingEdgeAngle / 2.0
            if ang > 80:
                ang = 80
            if ang < 0:
                ang = 0
            h1 = math.tan(ang * math.pi / 180) * rad
            if h1 > (h - 0.1):
                h1 = h - 0.1
            vBR = Vector(xp + yf, yp - xf, zp + h1)
            lR = Part.makeLine(vBR, vTR)
            lB = Part.makeLine(vBC, vBR)
            res = Part.Wire([lB, lR, lT])

        elif type == "BallEndMill":
            h1 = rad
            if h1 >= h:
                h1 = h - 0.1
            vBR = Vector(xp + yf, yp - xf, zp + h1)
            r2 = h1 / 2.0
            h2 = rad - math.sqrt(rad * rad - r2 * r2)
            vBCR = Vector(xp + yf / 2.0, yp - xf / 2.0, zp + h2)
            cB = Part.Edge(Part.Arc(vBC, vBCR, vBR))
            lR = Part.makeLine(vBR, vTR)
            res = Part.Wire([cB, lR, lT])

        else:  # default: assume type == "EndMill"
            vBR = Vector(xp + yf, yp - xf, zp)
            lR = Part.makeLine(vBR, vTR)
            lB = Part.makeLine(vBC, vBR)
            res = Part.Wire([lB, lR, lT])

        return res

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

    def onSpeedBarChange(self):
        form = self.taskForm.form
        self.simperiod = 1000 / form.sliderSpeed.value()
        form.labelGPerSec.setText(str(form.sliderSpeed.value()) + " G/s")
        self.timer.setInterval(self.simperiod)

    def onAccuracyBarChange(self):
        form = self.taskForm.form
        self.accuracy = 1.1 - 0.1 * form.sliderAccuracy.value()
        form.labelAccuracy.setText(str(round(self.accuracy, 1)) + "%")

    def GuiBusy(self, isBusy):
        form = self.taskForm.form
        form.toolButtonPlay.setEnabled(not isBusy)
        form.toolButtonPause.setEnabled(isBusy)
        form.toolButtonStep.setEnabled(not isBusy)
        form.toolButtonFF.setEnabled(not isBusy)

    def EndSimulation(self):
        self.UpdateProgress()
        self.timer.stop()
        self.GuiBusy(False)
        self.ViewShape()
        self.resetSimulation = True

    def SimStop(self):
        self.cutTool.ViewObject.hide()
        self.iprogress = 0
        self.EndSimulation()

    def InvalidOperation(self):
        if len(self.activeOps) == 0:
            return True
        if self.tool is None:
            TSError("No tool assigned for the operation")
            return True
        return False

    def SimFF(self):
        if self.InvalidOperation():
            return
        self.GuiBusy(True)
        self.timer.start(1)
        self.disableAnim = True

    def SimStep(self):
        if self.InvalidOperation():
            return
        self.disableAnim = False
        self.PerformCut()

    def SimPlay(self):
        if self.InvalidOperation():
            return
        self.disableAnim = False
        self.GuiBusy(True)
        self.timer.start(self.simperiod)

    def ViewShape(self):
        if self.isVoxel:
            (
                self.cutMaterial.Mesh,
                self.cutMaterialIn.Mesh,
            ) = self.voxSim.GetResultMesh()
        else:
            self.cutMaterial.Shape = self.stock

    def SimPause(self):
        if self.disableAnim:
            self.ViewShape()
        self.GuiBusy(False)
        self.timer.stop()

    def RemoveTool(self):
        if self.cutTool is None:
            return
        FreeCAD.ActiveDocument.removeObject(self.cutTool.Name)
        self.cutTool = None

    def RemoveInnerMaterial(self):
        if self.cutMaterialIn is not None:
            if self.isVoxel and self.cutMaterial is not None:
                mesh = Mesh.Mesh()
                mesh.addMesh(self.cutMaterial.Mesh)
                mesh.addMesh(self.cutMaterialIn.Mesh)
                self.cutMaterial.Mesh = mesh
            FreeCAD.ActiveDocument.removeObject(self.cutMaterialIn.Name)
            self.cutMaterialIn = None

    def RemoveMaterial(self):
        if self.cutMaterial is not None:
            FreeCAD.ActiveDocument.removeObject(self.cutMaterial.Name)
            self.cutMaterial = None
        self.RemoveInnerMaterial()

    def accept(self):
        self.EndSimulation()
        self.RemoveInnerMaterial()
        self.RemoveTool()

    def cancel(self):
        self.EndSimulation()
        self.RemoveTool()
        self.RemoveMaterial()


class CommandPathSimulate:
    def GetResources(self):
        return {
            "Pixmap": "Path_Simulator",
            "MenuText": QtCore.QT_TRANSLATE_NOOP("Path_Simulator", "CAM Simulator"),
            "Accel": "P, M",
            "ToolTip": QtCore.QT_TRANSLATE_NOOP(
                "Path_Simulator", "Simulate G-code on stock"
            ),
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):
        pathSimulation = PathSimulation()
        pathSimulation.Activate()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("Path_Simulator", CommandPathSimulate())
    FreeCAD.Console.PrintLog("Loading PathSimulator Gui... done\n")
