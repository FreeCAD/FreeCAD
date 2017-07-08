# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
from PySide import QtCore, QtGui
from PathScripts import PathUtils
import PathScripts.PathLog as PathLog
from PathScripts.PathUtils import waiting_effects, depth_params
import Part

FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui

"""Path Pocket object and FreeCAD command"""

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectPocket:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLinkSubList", "Base", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The base geometry of this object"))
        obj.addProperty("App::PropertyBool", "Active", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString", "Comment", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "An optional comment for this profile"))
        obj.addProperty("App::PropertyString", "UserLabel", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "User Assigned Label"))

        # Tool Properties
        obj.addProperty("App::PropertyLink", "ToolController", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tool controller that will be used to calculate the path"))

        # Depth Properties
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Rapid Safety Height between locations."))
        obj.addProperty("App::PropertyDistance", "StepDown", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Incremental Step Down of Tool"))
        obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyDistance", "FinishDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Maximum material removed on final pass."))

        # Pocket Properties
        obj.addProperty("App::PropertyEnumeration", "CutMode", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.CutMode = ['Climb', 'Conventional']
        obj.addProperty("App::PropertyDistance", "MaterialAllowance", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Amount of material to leave"))
        obj.addProperty("App::PropertyEnumeration", "StartAt", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Start pocketing at center or boundary"))
        obj.StartAt = ['Center', 'Edge']
        obj.addProperty("App::PropertyPercent", "StepOver", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Percent of cutter diameter to step over on each pass"))
        obj.addProperty("App::PropertyFloat", "ZigZagAngle", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Angle of the zigzag pattern"))
        obj.addProperty("App::PropertyEnumeration", "OffsetPattern", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property", "clearing pattern to use"))
        obj.OffsetPattern = ['ZigZag', 'Offset', 'Spiral', 'ZigZagOffset', 'Line', 'Grid', 'Triangle']
        obj.addProperty("App::PropertyBool", "MinTravel", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Use 3D Sorting of Path"))

        # Start Point Properties
        obj.addProperty("App::PropertyVector", "StartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property", "The start point of this path"))
        obj.addProperty("App::PropertyBool", "UseStartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property", "make True, if specifying a Start Point"))

        # Debug Parameters
        obj.addProperty("App::PropertyString", "AreaParams", "Path")
        obj.setEditorMode('AreaParams', 2)  # hide
        obj.addProperty("App::PropertyString", "PathParams", "Path")
        obj.setEditorMode('PathParams', 2)  # hide
        obj.addProperty("Part::PropertyPartShape", "removalshape", "Path")
        obj.setEditorMode('removalshape', 2)  # hide
        if FreeCAD.GuiUp:
            ViewProviderPocket(obj.ViewObject)

        obj.Proxy = self

    def onChanged(self, obj, prop):
        if prop in ['AreaParams', 'PathParams', 'removalshape']:
            obj.setEditorMode(prop, 2)

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def setDepths(proxy, obj):
        PathLog.track()
        parentJob = PathUtils.findParentJob(obj)
        if parentJob is None:
            return
        baseobject = parentJob.Base
        if baseobject is None:
            return

        try:
            bb = baseobject.Shape.BoundBox  # parent boundbox
            obj.StartDepth = bb.ZMax
            obj.ClearanceHeight = bb.ZMax + 5.0
            obj.SafeHeight = bb.ZMax + 3.0
            obj.FinalDepth = bb.ZMin

        except:
            obj.StartDepth = 5.0
            obj.ClearanceHeight = 10.0
            obj.SafeHeight = 8.0

    def addpocketbase(self, obj, ss, sub=""):
        PathLog.track()
        baselist = obj.Base
        if baselist is None:
            baselist = []
        if len(baselist) == 0:  # When adding the first base object, guess at heights

            try:
                bb = ss.Shape.BoundBox  # parent boundbox
                subobj = ss.Shape.getElement(sub)
                fbb = subobj.BoundBox  # feature boundbox
                obj.StartDepth = bb.ZMax
                obj.ClearanceHeight = bb.ZMax + 5.0
                obj.SafeHeight = bb.ZMax + 3.0

                if fbb.ZMax == fbb.ZMin and fbb.ZMax == bb.ZMax:  # top face
                    obj.FinalDepth = bb.ZMin
                elif fbb.ZMax > fbb.ZMin and fbb.ZMax == bb.ZMax:  # vertical face, full cut
                    obj.FinalDepth = fbb.ZMin
                elif fbb.ZMax > fbb.ZMin and fbb.ZMin > bb.ZMin:  # internal vertical wall
                    obj.FinalDepth = fbb.ZMin
                elif fbb.ZMax == fbb.ZMin and fbb.ZMax > bb.ZMin:  # face/shelf
                    obj.FinalDepth = fbb.ZMin
                else:  # catch all
                    obj.FinalDepth = bb.ZMin
            except:
                obj.StartDepth = 5.0
                obj.ClearanceHeight = 10.0
                obj.SafeHeight = 8.0

        item = (ss, sub)
        if item in baselist:
            FreeCAD.Console.PrintWarning(translate("Path", "this object already in the list" + "\n"))
        else:
            baselist.append(item)
        obj.Base = baselist
        self.execute(obj)

    def getStock(self, obj):
        """find and return a stock object from hosting project if any"""
        for o in obj.InList:
            if hasattr(o, "Group"):
                for g in o.Group:
                    if hasattr(g, "Height_Allowance"):
                        return o
        # not found? search one level up
        for o in obj.InList:
            return self.getStock(o)
        return None

    @waiting_effects
    def _buildPathArea(self, obj, envelopeshape, getsim=False):
        PathLog.track()
        pocket = Path.Area()
        pocket.setPlane(Part.makeCircle(10))
        pocket.add(envelopeshape)

        stepover = (self.radius * 2) * (float(obj.StepOver)/100)

        pocketparams = {'Fill': 0,
                        'Coplanar': 0,
                        'PocketMode': 1,
                        'SectionCount': -1,
                        'Angle': obj.ZigZagAngle,
                        'FromCenter': (obj.StartAt == "Center"),
                        'PocketStepover': stepover,
                        'PocketExtraOffset': obj.MaterialAllowance.Value}

        offsetval = self.radius
        pocketparams['ToolRadius'] = offsetval

        Pattern = ['ZigZag', 'Offset', 'Spiral', 'ZigZagOffset', 'Line', 'Grid', 'Triangle']
        pocketparams['PocketMode'] = Pattern.index(obj.OffsetPattern) + 1

        pocket.setParams(**pocketparams)
        obj.AreaParams = str(pocket.getParams())
        PathLog.debug("Pocketing with params: {}".format(pocket.getParams()))

        heights = [i for i in self.depthparams]
        PathLog.debug('pocket section heights: {}'.format(heights))
        sections = pocket.makeSections(mode=0, project=False, heights=heights)

        shapelist = [sec.getShape() for sec in sections]

        params = {'shapes': shapelist,
                  'feedrate': self.horizFeed,
                  'feedrate_v': self.vertFeed,
                  'verbose': True,
                  'resume_height': obj.StepDown.Value,
                  'retraction': obj.ClearanceHeight.Value}

        if obj.UseStartPoint and obj.StartPoint is not None:
            params['start'] = obj.StartPoint

            # if MinTravel is turned on, set path sorting to 3DSort
            # 3DSort shouldn't be used without a valid start point. Can cause
            # tool crash without it.
            if obj.MinTravel:
                params['sort_mode'] = 2

        obj.PathParams = str({key: value for key, value in params.items() if key != 'shapes'})

        pp = Path.fromShapes(**params)
        PathLog.debug("Generating Path with params: {}".format(params))
        PathLog.debug(pp)

        simobj = None
        if getsim:
            pocketparams['Thicken'] = True
            pocketparams['ToolRadius'] = self.radius - self.radius * .005
            pocketparams['Stepdown'] = -1
            pocket.setParams(**pocketparams)
            simobj = pocket.getShape().extrude(FreeCAD.Vector(0, 0, obj.StepDown.Value))

        return pp, simobj

    def execute(self, obj, getsim=False):
        PathLog.track()
        commandlist = []
        simlist = []
        commandlist.append(Path.Command("(" + obj.Label + ")"))
        if not obj.Active:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            obj.ViewObject.Visibility = False
            return

        parentJob = PathUtils.findParentJob(obj)
        if parentJob is None:
            return
        baseobject = parentJob.Base
        if baseobject is None:
            return

        self.depthparams = depth_params(
                clearance_height=obj.ClearanceHeight.Value,
                safe_height=obj.SafeHeight.Value,
                start_depth=obj.StartDepth.Value,
                step_down=obj.StepDown.Value,
                z_finish_step=0.0,
                final_depth=obj.FinalDepth.Value,
                user_depths=None)

        toolLoad = obj.ToolController
        if toolLoad is None or toolLoad.ToolNumber == 0:
            FreeCAD.Console.PrintError("No Tool Controller is selected. We need a tool to build a Path.")
        else:
            self.vertFeed = toolLoad.VertFeed.Value
            self.horizFeed = toolLoad.HorizFeed.Value
            self.vertRapid = toolLoad.VertRapid.Value
            self.horizRapid = toolLoad.HorizRapid.Value
            tool = toolLoad.Proxy.getTool(toolLoad)
            if not tool or tool.Diameter == 0:
                FreeCAD.Console.PrintError("No Tool found or diameter is zero. We need a tool to build a Path.")
                return
            else:
                self.radius = tool.Diameter/2

        if obj.Base:
            PathLog.debug("base items exist.  Processing...")
            for b in obj.Base:
                PathLog.debug("Base item: {}".format(b))
                for sub in b[1]:
                    if "Face" in sub:
                        shape = Part.makeCompound([getattr(b[0].Shape, sub)])
                    else:
                        edges = [getattr(b[0].Shape, sub) for sub in b[1]]
                        shape = Part.makeFace(edges, 'Part::FaceMakerSimple')

                    env = PathUtils.getEnvelope(baseobject.Shape, subshape=shape, depthparams=self.depthparams)
                    obj.removalshape = env.cut(baseobject.Shape)

                    try:
                        (pp, sim) = self._buildPathArea(obj, obj.removalshape, getsim=getsim)
                        if sim is not None:
                            simlist.append(sim)
                        commandlist.extend(pp.Commands)
                    except Exception as e:
                        FreeCAD.Console.PrintError(e)
                        FreeCAD.Console.PrintError("Something unexpected happened. Unable to generate a pocket path. Check project and tool config.")
        else:  # process the job base object as a whole
            PathLog.debug("processing the whole job base object")

            env = PathUtils.getEnvelope(baseobject.Shape, subshape=None, depthparams=self.depthparams)
            obj.removalshape = env.cut(baseobject.Shape)
            try:
                (pp, sim) = self._buildPathArea(obj, obj.removalshape, getsim=getsim)
                commandlist.extend(pp.Commands)
                if sim is not None:
                    simlist.append(sim)

            except Exception as e:
                FreeCAD.Console.PrintError(e)
                FreeCAD.Console.PrintError("Something unexpected happened. Unable to generate a pocket path. Check project and tool config.")

        # Let's finish by rapid to clearance...just for safety
        commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

        path = Path.Path(commandlist)
        obj.Path = path

        PathLog.debug(simlist)
        simshape = None
        if len(simlist) > 1:
            simshape = simlist[0].fuse(simlist[1:])
        elif len(simlist) == 1:
            simshape = simlist[0]

        if simshape is not None and PathLog.getLevel(PathLog.thisModule()) == PathLog.Level.DEBUG:
            sim = FreeCAD.ActiveDocument.addObject("Part::Feature", "simshape")
            sim.Shape = simshape
        return simshape


class _CommandSetPocketStartPoint:
    def GetResources(self):
        return {'Pixmap': 'Path-StartPoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathPocket", "Pick Start Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathPocket", "Pick Start Point")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def setpoint(self, point, o):
        obj = FreeCADGui.Selection.getSelection()[0]
        obj.StartPoint.x = point.x
        obj.StartPoint.y = point.y

    def Activated(self):
        FreeCADGui.Snapper.getPoint(callback=self.setpoint)


class ViewProviderPocket:

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def deleteObjectsOnReject(self):
        return hasattr(self, 'deleteOnReject') and self.deleteOnReject

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel(vobj.Object, self.deleteObjectsOnReject())
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()
        self.deleteOnReject = False
        return True

    def getIcon(self):
        return ":/icons/Path-Pocket.svg"

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class CommandPathPocket:

    def GetResources(self):
        return {'Pixmap': 'Path-Pocket',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathPocket", "Pocket"),
                'Accel': "P, O",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathPocket", "Creates a Path Pocket object from a face or faces")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):
        PathLog.track()

        zbottom = 0.0
        ztop = 10.0

        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction(translate("PathPocket", "Create Pocket"))
        FreeCADGui.addModule("PathScripts.PathPocket")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Pocket")')
        FreeCADGui.doCommand('PathScripts.PathPocket.ObjectPocket(obj)')
        FreeCADGui.doCommand('obj.Active = True')
        FreeCADGui.doCommand('obj.ViewObject.Proxy.deleteOnReject = True')
        FreeCADGui.doCommand('from PathScripts import PathUtils')
        FreeCADGui.doCommand('obj.StepOver = 100')
        FreeCADGui.doCommand('obj.ClearanceHeight = 10')  # + str(bb.ZMax + 2.0))
        FreeCADGui.doCommand('obj.StepDown = 1.0')
        FreeCADGui.doCommand('obj.StartDepth = ' + str(ztop))
        FreeCADGui.doCommand('obj.FinalDepth =' + str(zbottom))
        FreeCADGui.doCommand('obj.ZigZagAngle = 45')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('PathScripts.PathPocket.ObjectPocket.setDepths(obj.Proxy, obj)')
        FreeCADGui.doCommand('obj.ToolController = PathScripts.PathUtils.findToolController(obj)')
        FreeCAD.ActiveDocument.commitTransaction()

        # FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')


class TaskPanel:
    def __init__(self, obj, deleteOnReject):
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Pocket", "Pocket Operation"))
        # self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/PocketEdit.ui")
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/PocketEdit.ui")
        self.deleteOnReject = deleteOnReject
        self.isDirty = True

    def accept(self):
        FreeCADGui.Control.closeDialog()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.Selection.removeObserver(self.s)
        if self.isDirty:
            FreeCAD.ActiveDocument.recompute()

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCADGui.Selection.removeObserver(self.s)
        if self.deleteOnReject:
            FreeCAD.ActiveDocument.openTransaction(translate("Path_Pocket", "Uncreate Pocket Operation"))
            FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

    def clicked(self, button):
        if button == QtGui.QDialogButtonBox.Apply:
            self.getFields()
            self.obj.Proxy.execute(self.obj)
            self.isDirty = False

    def getFields(self):
        if self.obj:
            if hasattr(self.obj, "StartDepth"):
                self.obj.StartDepth = FreeCAD.Units.Quantity(self.form.startDepth.text()).Value
            if hasattr(self.obj, "FinalDepth"):
                self.obj.FinalDepth = FreeCAD.Units.Quantity(self.form.finalDepth.text()).Value
            if hasattr(self.obj, "FinishDepth"):
                self.obj.FinishDepth = FreeCAD.Units.Quantity(self.form.finishDepth.text()).Value
            if hasattr(self.obj, "StepDown"):
                self.obj.StepDown = FreeCAD.Units.Quantity(self.form.stepDown.text()).Value
            if hasattr(self.obj, "SafeHeight"):
                self.obj.SafeHeight = FreeCAD.Units.Quantity(self.form.safeHeight.text()).Value
            if hasattr(self.obj, "ClearanceHeight"):
                self.obj.ClearanceHeight = FreeCAD.Units.Quantity(self.form.clearanceHeight.text()).Value
            if hasattr(self.obj, "MaterialAllowance"):
                self.obj.MaterialAllowance = FreeCAD.Units.Quantity(self.form.extraOffset.text()).Value
            if hasattr(self.obj, "UseStartPoint"):
                self.obj.UseStartPoint = self.form.useStartPoint.isChecked()
            if hasattr(self.obj, "CutMode"):
                self.obj.CutMode = str(self.form.cutMode.currentText())
            if hasattr(self.obj, "OffsetPattern"):
                self.obj.OffsetPattern = str(self.form.offsetpattern.currentText())
            if hasattr(self.obj, "ZigZagAngle"):
                self.obj.ZigZagAngle = FreeCAD.Units.Quantity(self.form.zigZagAngle.text()).Value
            if hasattr(self.obj, "StepOver"):
                self.obj.StepOver = self.form.stepOverPercent.value()
            if hasattr(self.obj, "ToolController"):
                PathLog.debug("name: {}".format(self.form.uiToolController.currentText()))
                tc = PathUtils.findToolController(self.obj, self.form.uiToolController.currentText())
                self.obj.ToolController = tc
        self.isDirty = True

    def setFields(self):
        self.form.startDepth.setText(FreeCAD.Units.Quantity(self.obj.StartDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.finalDepth.setText(FreeCAD.Units.Quantity(self.obj.FinalDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.finishDepth.setText(FreeCAD.Units.Quantity(self.obj.FinishDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.stepDown.setText(FreeCAD.Units.Quantity(self.obj.StepDown.Value, FreeCAD.Units.Length).UserString)
        self.form.safeHeight.setText(FreeCAD.Units.Quantity(self.obj.SafeHeight.Value, FreeCAD.Units.Length).UserString)
        self.form.clearanceHeight.setText(FreeCAD.Units.Quantity(self.obj.ClearanceHeight.Value,  FreeCAD.Units.Length).UserString)
        self.form.extraOffset.setText(FreeCAD.Units.Quantity(self.obj.MaterialAllowance.Value, FreeCAD.Units.Length).UserString)
        self.form.useStartPoint.setChecked(self.obj.UseStartPoint)
        self.form.zigZagAngle.setText(FreeCAD.Units.Quantity(self.obj.ZigZagAngle, FreeCAD.Units.Angle).UserString)
        self.form.stepOverPercent.setValue(self.obj.StepOver)

        index = self.form.offsetpattern.findText(
                self.obj.OffsetPattern, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.offsetpattern.blockSignals(True)
            self.form.offsetpattern.setCurrentIndex(index)
            self.form.offsetpattern.blockSignals(False)

        index = self.form.cutMode.findText(
                self.obj.CutMode, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.cutMode.blockSignals(True)
            self.form.cutMode.setCurrentIndex(index)
            self.form.cutMode.blockSignals(False)

        self.form.baseList.blockSignals(True)
        for i in self.obj.Base:
            for sub in i[1]:
                self.form.baseList.addItem(i[0].Name + "." + sub)
        self.form.baseList.blockSignals(False)

        controllers = PathUtils.getToolControllers(self.obj)
        labels = [c.Label for c in controllers]
        self.form.uiToolController.blockSignals(True)
        self.form.uiToolController.addItems(labels)
        self.form.uiToolController.blockSignals(False)
        if self.obj.ToolController is not None:
            index = self.form.uiToolController.findText(
                self.obj.ToolController.Label, QtCore.Qt.MatchFixedString)
            PathLog.debug("searching for TC label {}. Found Index: {}".format(self.obj.ToolController.Label, index))
            if index >= 0:
                self.form.uiToolController.blockSignals(True)
                self.form.uiToolController.setCurrentIndex(index)
                self.form.uiToolController.blockSignals(False)
        else:
            self.obj.ToolController = PathUtils.findToolController(self.obj)

    def open(self):
        self.s = SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)

    def addBase(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()

        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate("PathProject", "Please select only faces from one solid\n"))
            return
        sel = selection[0]
        if not sel.HasSubObjects:
            FreeCAD.Console.PrintError(translate("PathProject", "Please select faces from one solid\n"))
            return
        if not selection[0].SubObjects[0].ShapeType == "Face":
            FreeCAD.Console.PrintError(translate("PathProject", "Please select faces from one solid\n"))
            return
        for i in sel.SubElementNames:
            self.obj.Proxy.addpocketbase(self.obj, sel.Object, i)

        self.setFields()  # defaults may have changed.  Reload.
        self.form.baseList.clear()

        for i in self.obj.Base:
            for sub in i[1]:
                self.form.baseList.addItem(i[0].Name + "." + sub)

    def deleteBase(self):
        dlist = self.form.baseList.selectedItems()
        newlist = []
        for d in dlist:
            for i in self.obj.Base:
                if i[0].Name != d.text().partition(".")[0] or i[1] != d.text().partition(".")[2]:
                    newlist.append(i)
            self.form.baseList.takeItem(self.form.baseList.row(d))
        self.obj.Base = newlist
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def itemActivated(self):
        FreeCADGui.Selection.clearSelection()
        slist = self.form.baseList.selectedItems()
        for i in slist:
            objstring = i.text().partition(".")
            obj = FreeCAD.ActiveDocument.getObject(objstring[0])
            if objstring[2] != "":
                FreeCADGui.Selection.addSelection(obj, objstring[2])
            else:
                FreeCADGui.Selection.addSelection(obj)

        FreeCADGui.updateGui()

    def reorderBase(self):
        newlist = []
        for i in range(self.form.baseList.count()):
            s = self.form.baseList.item(i).text()
            objstring = s.partition(".")

            obj = FreeCAD.ActiveDocument.getObject(objstring[0])
            item = (obj, str(objstring[2]))
            newlist.append(item)
        self.obj.Base = newlist

        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Apply | QtGui.QDialogButtonBox.Cancel)

    def setupUi(self):

        # Connect Signals and Slots
        # Base Controls
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.form.addBase.clicked.connect(self.addBase)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.reorderBase.clicked.connect(self.reorderBase)
        self.form.uiToolController.currentIndexChanged.connect(self.getFields)

        # Depths
        self.form.startDepth.editingFinished.connect(self.getFields)
        self.form.finalDepth.editingFinished.connect(self.getFields)
        self.form.finishDepth.editingFinished.connect(self.getFields)
        self.form.stepDown.editingFinished.connect(self.getFields)

        # Heights
        self.form.safeHeight.editingFinished.connect(self.getFields)
        self.form.clearanceHeight.editingFinished.connect(self.getFields)

        # operation
        self.form.cutMode.currentIndexChanged.connect(self.getFields)
        self.form.useStartPoint.clicked.connect(self.getFields)

        # Pattern
        self.form.offsetpattern.currentIndexChanged.connect(self.getFields)
        self.form.stepOverPercent.editingFinished.connect(self.getFields)
        self.form.zigZagAngle.editingFinished.connect(self.getFields)
        self.form.extraOffset.editingFinished.connect(self.getFields)
        self.form.uiToolController.currentIndexChanged.connect(self.getFields)

        self.setFields()

        sel = FreeCADGui.Selection.getSelectionEx()
        if len(sel) != 0 and sel[0].HasSubObjects:
                self.addBase()


class SelObserver:
    def __init__(self):
        import PathScripts.PathSelection as PST
        PST.pocketselect()

    def __del__(self):
        import PathScripts.PathSelection as PST
        PST.clear()

    def addSelection(self, doc, obj, sub, pnt):
        FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj + ')')
        FreeCADGui.updateGui()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Pocket', CommandPathPocket())
    FreeCADGui.addCommand('Set_PocketStartPoint', _CommandSetPocketStartPoint())


FreeCAD.Console.PrintLog("Loading PathPocket... done\n")
