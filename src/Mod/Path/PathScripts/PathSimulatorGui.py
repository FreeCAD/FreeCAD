import FreeCAD
import Mesh
import Part
import Path
import PathScripts.PathDressup as PathDressup
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathSimulator
import math
import os

from FreeCAD import Vector, Base

_filePath = os.path.dirname(os.path.abspath(__file__))

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui, QtCore

# compiled with pyrcc4 -py3 Resources\CAM_Sim.qrc -o CAM_Sim_rc.py


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

    def Connect(self, but, sig):
        QtCore.QObject.connect(but, QtCore.SIGNAL("clicked()"), sig)

    def UpdateProgress(self):
        if self.numCommands > 0:
            self.taskForm.form.progressBar.setValue(self.iprogress * 100 / self.numCommands)

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
        form.comboJobs.currentIndexChanged.connect(self.onJobChange)
        jobList = FreeCAD.ActiveDocument.findObjects("Path::FeaturePython", "Job.*")
        form.comboJobs.clear()
        self.jobs = []
        for j in jobList:
            self.jobs.append(j)
            form.comboJobs.addItem(j.ViewObject.Icon, j.Label)
        FreeCADGui.Control.showDialog(self.taskForm)
        self.disableAnim = False
        self.isVoxel = True
        self.firstDrill = True
        self.voxSim = PathSimulator.PathSim()
        self.SimulateMill()
        self.initdone = True

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
        if (self.isVoxel):
            maxlen = self.stock.BoundBox.XLength
            if (maxlen < self.stock.BoundBox.YLength):
                maxlen = self.stock.BoundBox.YLength
            self.voxSim.BeginSimulation(self.stock, 0.01 * self.accuracy * maxlen)
            (self.cutMaterial.Mesh, self.cutMaterialIn.Mesh) = self.voxSim.GetResultMesh()
        else:
            self.cutMaterial.Shape = self.stock
        self.busy = False
        self.tool = None
        for i in range(len(self.activeOps)):
            self.SetupOperation(0)
            if (self.tool is not None):
                break
        self.iprogress = 0
        self.UpdateProgress()

    def SetupOperation(self, itool):
        self.operation = self.activeOps[itool]
        try:
            self.tool = PathDressup.toolController(self.operation).Tool
        except Exception:
            self.tool = None

        # if hasattr(self.operation, "ToolController"):
        #     self.tool = self.operation.ToolController.Tool
        if (self.tool is not None):
            toolProf = self.CreateToolProfile(self.tool, Vector(0, 1, 0), Vector(0, 0, 0), self.tool.Diameter / 2.0)
            self.cutTool.Shape = Part.makeSolid(toolProf.revolve(Vector(0, 0, 0), Vector(0, 0, 1)))
            self.cutTool.ViewObject.show()
            self.voxSim.SetCurrentTool(self.tool)
        self.icmd = 0
        self.curpos = FreeCAD.Placement(self.initialPos, self.stdrot)
        # self.cutTool.Placement = FreeCAD.Placement(self.curpos, self.stdrot)
        self.cutTool.Placement = self.curpos
        self.opCommands =  self.operation.Path.Commands

    def SimulateMill(self):
        self.job = self.jobs[self.taskForm.form.comboJobs.currentIndex()]
        self.busy = False
        # self.timer.start(100)
        self.height = 10
        self.skipStep = False
        self.initialPos = Vector(0, 0, self.job.Stock.Shape.BoundBox.ZMax)
        # Add cut tool
        self.cutTool = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "CutTool")
        self.cutTool.ViewObject.Proxy = 0
        self.cutTool.ViewObject.hide()

        # Add cut material
        if self.isVoxel:
            self.cutMaterial = FreeCAD.ActiveDocument.addObject("Mesh::FeaturePython", "CutMaterial")
            self.cutMaterialIn = FreeCAD.ActiveDocument.addObject("Mesh::FeaturePython", "CutMaterialIn")
            self.cutMaterialIn.ViewObject.Proxy = 0
            self.cutMaterialIn.ViewObject.show()
            self.cutMaterialIn.ViewObject.ShapeColor = (1.0, 0.85, 0.45, 0.0)
        else:
            self.cutMaterial = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "CutMaterial")
            self.cutMaterial.Shape = self.job.Stock.Shape
        self.cutMaterial.ViewObject.Proxy = 0
        self.cutMaterial.ViewObject.show()
        self.cutMaterial.ViewObject.ShapeColor = (0.5, 0.25, 0.25, 0.0)

        # Add cut path solid for debug
        if self.debug:
            self.cutSolid = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "CutDebug")
            self.cutSolid.ViewObject.Proxy = 0
            self.cutSolid.ViewObject.hide()

        self.SetupSimulation()
        self.resetSimulation = True
        FreeCAD.ActiveDocument.recompute()

    # def SkipStep(self):
    #     self.skipStep = True
    #     self.PerformCut()

    def PerformCutBoolean(self):
        if self.resetSimulation:
            self.resetSimulation = False
            self.SetupSimulation()

        if self.busy:
            return
        self.busy = True

        cmd = self.operation.Path.Commands[self.icmd]
        # for cmd in job.Path.Commands:
        pathSolid = None

        if cmd.Name in ['G0']:
            self.curpos = self.RapidMove(cmd, self.curpos)
        if cmd.Name in ['G1', 'G2', 'G3']:
            if self.skipStep:
                self.curpos = self.RapidMove(cmd, self.curpos)
            else:
                (pathSolid, self.curpos) = self.GetPathSolid(self.tool, cmd, self.curpos)
        if cmd.Name in ['G81', 'G82', 'G83']:
            if self.firstDrill:
                extendcommand = Path.Command('G0', {"X": 0.0, "Y": 0.0, "Z": cmd.r})
                self.curpos = self.RapidMove(extendcommand, self.curpos)
                self.firstDrill = False
            extendcommand = Path.Command('G0', {"X": cmd.x, "Y": cmd.y, "Z": cmd.r})
            self.curpos = self.RapidMove(extendcommand, self.curpos)
            extendcommand = Path.Command('G1', {"X": cmd.x, "Y": cmd.y, "Z": cmd.z})
            self.curpos = self.RapidMove(extendcommand, self.curpos)
            extendcommand = Path.Command('G1', {"X": cmd.x, "Y": cmd.y, "Z": cmd.r})
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
        if self.icmd >= len(self.operation.Path.Commands):
            # self.cutMaterial.Shape = self.stock.removeSplitter()
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
        if cmd.Name in ['G0', 'G1', 'G2', 'G3']:
            self.curpos = self.voxSim.ApplyCommand(self.curpos, cmd)
            if not self.disableAnim:
                self.cutTool.Placement = self.curpos  # FreeCAD.Placement(self.curpos, self.stdrot)
                (self.cutMaterial.Mesh, self.cutMaterialIn.Mesh) = self.voxSim.GetResultMesh()
        if cmd.Name in ['G81', 'G82', 'G83']:
            extendcommands = []
            if self.firstDrill:
                extendcommands.append(Path.Command('G0', {"X": 0.0, "Y": 0.0, "Z": cmd.r}))
                self.firstDrill = False
            extendcommands.append(Path.Command('G0', {"X": cmd.x, "Y": cmd.y, "Z": cmd.r}))
            extendcommands.append(Path.Command('G1', {"X": cmd.x, "Y": cmd.y, "Z": cmd.z}))
            extendcommands.append(Path.Command('G1', {"X": cmd.x, "Y": cmd.y, "Z": cmd.r}))
            for ecmd in extendcommands:
                self.curpos = self.voxSim.ApplyCommand(self.curpos, ecmd)
                if not self.disableAnim:
                    self.cutTool.Placement = self.curpos  # FreeCAD.Placement(self.curpos, self.stdrot)
                    (self.cutMaterial.Mesh, self.cutMaterialIn.Mesh) = self.voxSim.GetResultMesh()
        self.icmd += 1
        self.iprogress += 1
        self.UpdateProgress()
        if self.icmd >= len(self.opCommands):
            # self.cutMaterial.Shape = self.stock.removeSplitter()
            self.ioperation += 1
            if self.ioperation >= len(self.activeOps):
                self.EndSimulation()
                return
            else:
                self.SetupOperation(self.ioperation)
        self.busy = False

    def PerformCut(self):
        if (self.isVoxel):
            self.PerformCutVoxel()
        else:
            self.PerformCutBoolean()

    def RapidMove(self, cmd, curpos):
        path = PathGeom.edgeForCmd(cmd, curpos)  # hack to overcome occ bug
        if path is None:
            return curpos
        return path.valueAt(path.LastParameter)

    # def GetPathSolidOld(self, tool, cmd, curpos):
    #     e1 = PathGeom.edgeForCmd(cmd, curpos)
    #     # curpos = e1.valueAt(e1.LastParameter)
    #     n1 = e1.tangentAt(0)
    #     n1[2] = 0.0
    #     try:
    #         n1.normalize()
    #     except:
    #         return (None, e1.valueAt(e1.LastParameter))
    #     height = self.height
    #     rad = tool.Diameter / 2.0 - 0.001 * curpos[2]  # hack to overcome occ bug
    #     if type(e1.Curve) is Part.Circle and e1.Curve.Radius <= rad:  # hack to overcome occ bug
    #         rad = e1.Curve.Radius - 0.001
    #         # return (None, e1.valueAt(e1.LastParameter))
    #     xf = n1[0] * rad
    #     yf = n1[1] * rad
    #     xp = curpos[0]
    #     yp = curpos[1]
    #     zp = curpos[2]
    #     v1 = Vector(yf + xp, -xf + yp, zp)
    #     v2 = Vector(yf + xp, -xf + yp, zp + height)
    #     v3 = Vector(-yf + xp, xf + yp, zp + height)
    #     v4 = Vector(-yf + xp, xf + yp, zp)
    #     # vc1 = Vector(xf + xp, yf + yp, zp)
    #     # vc2 = Vector(xf + xp, yf + yp, zp + height)
    #     l1 = Part.makeLine(v1, v2)
    #     l2 = Part.makeLine(v2, v3)
    #     # l2 = Part.Edge(Part.Arc(v2, vc2, v3))
    #     l3 = Part.makeLine(v3, v4)
    #     l4 = Part.makeLine(v4, v1)
    #     # l4 = Part.Edge(Part.Arc(v4, vc1, v1))
    #     w1 = Part.Wire([l1, l2, l3, l4])
    #     w2 = Part.Wire(e1)
    #     try:
    #         ex1 = w2.makePipeShell([w1], True, True)
    #     except:
    #         # Part.show(w1)
    #         # Part.show(w2)
    #         return (None, e1.valueAt(e1.LastParameter))
    #     cyl1 = Part.makeCylinder(rad, height, curpos)
    #     curpos = e1.valueAt(e1.LastParameter)
    #     cyl2 = Part.makeCylinder(rad, height, curpos)
    #     ex1s = Part.Solid(ex1)
    #     f1 = ex1s.fuse([cyl1, cyl2]).removeSplitter()
    #     return (f1, curpos)

    # get a solid representation of a tool going along path
    def GetPathSolid(self, tool, cmd, pos):
        toolPath = PathGeom.edgeForCmd(cmd, pos)
        # curpos = e1.valueAt(e1.LastParameter)
        startDir = toolPath.tangentAt(0)
        startDir[2] = 0.0
        endPos = toolPath.valueAt(toolPath.LastParameter)
        endDir = toolPath.tangentAt(toolPath.LastParameter)
        try:
            startDir.normalize()
            endDir.normalize()
        except Exception:
            return (None, endPos)
        # height = self.height

        # hack to overcome occ bugs
        rad = tool.Diameter / 2.0 - 0.001 * pos[2]
        # rad = rad + 0.001 * self.icmd
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
        # rad = tool.Diameter / 2.0 - 0.001 * pos[2] # hack to overcome occ bug
        xf = dir[0] * rad
        yf = dir[1] * rad
        xp = pos[0]
        yp = pos[1]
        zp = pos[2]
        h = tool.CuttingEdgeHeight
        if h <= 0.0:  # set default if user fails to avoid freeze
            h = 1.0
            PathLog.error("SET Tool Length")
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
            listItem = QtGui.QListWidgetItem(op.ViewObject.Icon, op.Label)
            listItem.setFlags(listItem.flags() | QtCore.Qt.ItemIsUserCheckable)
            listItem.setCheckState(QtCore.Qt.CheckState.Checked)
            self.operations.append(op)
            form.listOperations.addItem(listItem)
        if  self.initdone:
          self.SetupSimulation()

    def onSpeedBarChange(self):
        form = self.taskForm.form
        self.simperiod = 1000 / form.sliderSpeed.value()
        form.labelGPerSec.setText(str(form.sliderSpeed.value()) + " G/s")
        # if (self.timer.isActive()):
        self.timer.setInterval(self.simperiod)

    def onAccuracyBarChange(self):
        form = self.taskForm.form
        self.accuracy = 1.1 - 0.1 * form.sliderAccuracy.value()
        form.labelAccuracy.setText(str(self.accuracy) + "%")

    def GuiBusy(self, isBusy):
        form = self.taskForm.form
        # form.toolButtonStop.setEnabled()
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
        if (self.tool == None):
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
            (self.cutMaterial.Mesh, self.cutMaterialIn.Mesh) = self.voxSim.GetResultMesh()
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
        return {'Pixmap': 'Path-Simulator',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Simulator", "CAM Simulator"),
                'Accel': "P, M",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Simulator", "Simulate Path G-Code on stock")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):
        pathSimulation.Activate()

pathSimulation = PathSimulation()
if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Simulator', CommandPathSimulate())
    FreeCAD.Console.PrintLog("Loading PathSimulator Gui... done\n")
