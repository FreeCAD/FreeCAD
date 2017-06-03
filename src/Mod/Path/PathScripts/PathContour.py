# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts.PathLog as PathLog
from PathScripts import PathUtils
from PathScripts.PathUtils import depth_params
from PySide import QtCore
import ArchPanel
import Part
from PathScripts.PathUtils import waiting_effects
from PathScripts.PathUtils import makeWorkplane

LOG_MODULE = 'PathContour'
PathLog.setLevel(PathLog.Level.INFO, LOG_MODULE)
#PathLog.trackModule('PathContour')
FreeCAD.setLogLevel('Path.Area', 0)

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

__title__ = "Path Contour Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

"""Path Contour object and FreeCAD command"""


class ObjectContour:

    def __init__(self, obj):
        obj.addProperty("App::PropertyBool", "Active", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString", "Comment", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "An optional comment for this Contour"))
        obj.addProperty("App::PropertyString", "UserLabel", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "User Assigned Label"))

        # Tool Properties
        obj.addProperty("App::PropertyLink", "ToolController", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tool controller that will be used to calculate the path"))

        # Depth Properties
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Rapid Safety Height between locations."))
        obj.addProperty("App::PropertyDistance", "StepDown", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Incremental Step Down of Tool"))
        obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Final Depth of Tool- lowest value in Z"))

        # Start Point Properties
        obj.addProperty("App::PropertyVector", "StartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property", "The start point of this path"))

        # Contour Properties
        obj.addProperty("App::PropertyEnumeration", "Direction", "Contour", QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.Direction = ['CW', 'CCW']  # this is the direction that the Contour runs
        obj.addProperty("App::PropertyBool", "UseComp", "Contour", QtCore.QT_TRANSLATE_NOOP("App::Property", "make True, if using Cutter Radius Compensation"))
        obj.addProperty("App::PropertyEnumeration", "Side", "Contour", QtCore.QT_TRANSLATE_NOOP("App::Property", "Side of edge that tool should cut"))
        obj.Side = ['Left', 'Right', 'On']  # side of profile that cutter is on in relation to direction of profile
        obj.setEditorMode('Side', 2)  # hide

        obj.addProperty("App::PropertyDistance", "OffsetExtra", "Contour", QtCore.QT_TRANSLATE_NOOP("App::Property", "Extra value to stay away from final Contour- good for roughing toolpath"))

        obj.Proxy = self
        self.endVector = None

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, obj, prop):
        pass
        # if prop in ['ClearanceHeight', 'StartPoint']:
        #     obj.StartPoint.z = obj.ClearanceHeight.Value

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

    @waiting_effects
    def _buildPathArea(self, obj, baseobject, start=None):
        PathLog.track()
        profile = Path.Area()
        profile.setPlane(makeWorkplane(baseobject))
        profile.add(baseobject)

        profileparams = {'Fill': 0,
                         'Coplanar': 0}

        if obj.UseComp is False:
            profileparams['Offset'] = 0.0
        else:
            profileparams['Offset'] = self.radius+obj.OffsetExtra.Value

        depthparams = depth_params(
                clearance_height=obj.ClearanceHeight.Value,
                rapid_safety_space=obj.SafeHeight.Value,
                start_depth=obj.StartDepth.Value,
                step_down=obj.StepDown.Value,
                z_finish_step=0.0,
                final_depth=obj.FinalDepth.Value,
                user_depths=None)

        PathLog.debug('depths: {}'.format(depthparams.get_depths()))
        profile.setParams(**profileparams)
        PathLog.debug("Contour with params: {}".format(profile.getParams()))

        sections = profile.makeSections(mode=0, project=True, heights=depthparams.get_depths())
        shapelist = [sec.getShape() for sec in sections]

        params = {'shapes': shapelist,
                  'feedrate': self.horizFeed,
                  'feedrate_v': self.vertFeed,
                  'verbose': True,
                  'resume_height': obj.StepDown.Value,
                  'retraction': obj.ClearanceHeight.Value,
                  'return_end': True}

        if obj.Direction == 'CCW':
            params['orientation'] = 1
        else:
            params['orientation'] = 0

        if self.endVector is not None:
            params['start'] = self.endVector
        elif start is not None:
            params['start'] = start

        (pp, end_vector) = Path.fromShapes(**params)
        PathLog.debug("Generating Path with params: {}".format(params))
        PathLog.debug('pp: {}, end vector: {}'.format(pp, end_vector))
        self.endVector = end_vector

        return pp

    def execute(self, obj):
        PathLog.track()
        self.endVector = None

        commandlist = []
        toolLoad = obj.ToolController

        if toolLoad is None or toolLoad.ToolNumber == 0:
            FreeCAD.Console.PrintError("No Tool Controller is selected. We need a tool to build a Path.")
            return
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

        commandlist.append(Path.Command("(" + obj.Label + ")"))

        if obj.UseComp:
            commandlist.append(Path.Command("(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")"))
        else:
            commandlist.append(Path.Command("(Uncompensated Tool Path)"))

        parentJob = PathUtils.findParentJob(obj)
        if parentJob is None:
            return
        baseobject = parentJob.Base
        if baseobject is None:
            return

        # Let's always start by rapid to clearance...just for safety
        commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

        if hasattr(baseobject, "Proxy"):
            if isinstance(baseobject.Proxy, ArchPanel.PanelSheet):  # process the sheet
                baseobject.Proxy.execute(baseobject)
                shapes = baseobject.Proxy.getOutlines(baseobject, transform=True)
                for shape in shapes:
                    f = Part.makeFace([shape], 'Part::FaceMakerSimple')
                    thickness = baseobject.Group[0].Source.Thickness
                    contourshape = f.extrude(FreeCAD.Vector(0, 0, thickness))
                    try:
                        commandlist.extend(self._buildPathArea(obj, contourshape, start=obj.StartPoint).Commands)
                    except Exception as e:
                        print(e)
                        FreeCAD.Console.PrintError("Something unexpected happened. Unable to generate a contour path. Check project and tool config.")
        else:
            bb = baseobject.Shape.BoundBox
            env = PathUtils.getEnvelope(baseobject.Shape, bb.ZLength + (obj.StartDepth.Value-bb.ZMax))
            try:
                commandlist.extend(self._buildPathArea(obj, env, start=obj.StartPoint).Commands)
            except Exception as e:
                print(e)
                FreeCAD.Console.PrintError("Something unexpected happened. Unable to generate a contour path. Check project and tool config.")

        if obj.Active:
            path = Path.Path(commandlist)
            obj.Path = path
            if obj.ViewObject:
                obj.ViewObject.Visibility = True
        else:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            obj.ViewObject.Visibility = False


class _ViewProviderContour:

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel()
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()
        return True

    def getIcon(self):
        return ":/icons/Path-Contour.svg"

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _CommandSetStartPoint:
    def GetResources(self):
        return {'Pixmap': 'Path-StartPoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Contour", "Pick Start Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Contour", "Pick Start Point")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def setpoint(self, point, o):
        obj = FreeCADGui.Selection.getSelection()[0]
        obj.StartPoint.x = point.x
        obj.StartPoint.y = point.y
        obj.StartPoint.z = obj.ClearanceHeight.Value

    def Activated(self):
        FreeCADGui.Snapper.getPoint(callback=self.setpoint)


class CommandPathContour:
    def GetResources(self):
        return {'Pixmap': 'Path-Contour',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathContour", "Contour"),
                'Accel': "P, C",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathContour", "Creates a Contour Path for the Base Object ")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):
        ztop = 10.0
        zbottom = 0.0

        FreeCAD.ActiveDocument.openTransaction(translate("Path", "Create a Contour"))
        FreeCADGui.addModule("PathScripts.PathContour")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Contour")')
        FreeCADGui.doCommand('PathScripts.PathContour.ObjectContour(obj)')
        FreeCADGui.doCommand('PathScripts.PathContour._ViewProviderContour(obj.ViewObject)')

        FreeCADGui.doCommand('obj.Active = True')

        FreeCADGui.doCommand('obj.ClearanceHeight = ' + str(ztop + 10.0))
        FreeCADGui.doCommand('obj.StepDown = 1.0')
        FreeCADGui.doCommand('obj.StartDepth= ' + str(ztop))
        FreeCADGui.doCommand('obj.FinalDepth=' + str(zbottom))

        FreeCADGui.doCommand('obj.SafeHeight = ' + str(ztop + 2.0))
        FreeCADGui.doCommand('obj.OffsetExtra = 0.0')
        FreeCADGui.doCommand('obj.Direction = "CW"')
        FreeCADGui.doCommand('obj.UseComp = True')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')

        FreeCADGui.doCommand('PathScripts.PathContour.ObjectContour.setDepths(obj.Proxy, obj)')
        FreeCADGui.doCommand('obj.ToolController = PathScripts.PathUtils.findToolController(obj)')

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')


class TaskPanel:
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ContourEdit.ui")
        # self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/ContourEdit.ui")
        self.updating = False

    def accept(self):
        self.getFields()

        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Selection.removeObserver(self.s)
        FreeCAD.ActiveDocument.recompute()

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Selection.removeObserver(self.s)
        FreeCAD.ActiveDocument.recompute()

    def getFields(self):
        PathLog.track()
        if self.obj:
            if hasattr(self.obj, "StartDepth"):
                self.obj.StartDepth = FreeCAD.Units.Quantity(self.form.startDepth.text()).Value
            if hasattr(self.obj, "FinalDepth"):
                self.obj.FinalDepth = FreeCAD.Units.Quantity(self.form.finalDepth.text()).Value
            if hasattr(self.obj, "SafeHeight"):
                self.obj.SafeHeight = FreeCAD.Units.Quantity(self.form.safeHeight.text()).Value
            if hasattr(self.obj, "ClearanceHeight"):
                self.obj.ClearanceHeight = FreeCAD.Units.Quantity(self.form.clearanceHeight.text()).Value
            if hasattr(self.obj, "StepDown"):
                self.obj.StepDown = FreeCAD.Units.Quantity(self.form.stepDown.text()).Value
            if hasattr(self.obj, "OffsetExtra"):
                self.obj.OffsetExtra = FreeCAD.Units.Quantity(self.form.extraOffset.text()).Value
            if hasattr(self.obj, "UseComp"):
                self.obj.UseComp = self.form.useCompensation.isChecked()
            # if hasattr(self.obj, "UseStartPoint"):
            #     self.obj.UseStartPoint = self.form.useStartPoint.isChecked()
            if hasattr(self.obj, "Direction"):
                self.obj.Direction = str(self.form.direction.currentText())
            if hasattr(self.obj, "ToolController"):
                tc = PathUtils.findToolController(self.obj, self.form.uiToolController.currentText())
                self.obj.ToolController = tc
        self.obj.Proxy.execute(self.obj)

    def setFields(self):
        PathLog.track()
        self.form.startDepth.setText(FreeCAD.Units.Quantity(self.obj.StartDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.finalDepth.setText(FreeCAD.Units.Quantity(self.obj.FinalDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.safeHeight.setText(FreeCAD.Units.Quantity(self.obj.SafeHeight.Value, FreeCAD.Units.Length).UserString)
        self.form.clearanceHeight.setText(FreeCAD.Units.Quantity(self.obj.ClearanceHeight.Value,  FreeCAD.Units.Length).UserString)
        self.form.stepDown.setText(FreeCAD.Units.Quantity(self.obj.StepDown.Value, FreeCAD.Units.Length).UserString)
        self.form.extraOffset.setText(FreeCAD.Units.Quantity(self.obj.OffsetExtra.Value, FreeCAD.Units.Length).UserString)
        self.form.useCompensation.setChecked(self.obj.UseComp)
        # self.form.useStartPoint.setChecked(self.obj.UseStartPoint)

        index = self.form.direction.findText(
                self.obj.Direction, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.direction.blockSignals(True)
            self.form.direction.setCurrentIndex(index)
            self.form.direction.blockSignals(False)

        controllers = PathUtils.getToolControllers(self.obj)
        labels = [c.Label for c in controllers]
        self.form.uiToolController.blockSignals(True)
        self.form.uiToolController.addItems(labels)
        self.form.uiToolController.blockSignals(False)

        if self.obj.ToolController is None:
            self.obj.ToolController = PathUtils.findToolController(self.obj)

        if self.obj.ToolController is not None:
            index = self.form.uiToolController.findText(
                self.obj.ToolController.Label, QtCore.Qt.MatchFixedString)
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

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def setupUi(self):
        PathLog.track()
        # Connect Signals and Slots
        # Depths
        self.form.startDepth.editingFinished.connect(self.getFields)
        self.form.finalDepth.editingFinished.connect(self.getFields)
        self.form.stepDown.editingFinished.connect(self.getFields)

        # Heights
        self.form.safeHeight.editingFinished.connect(self.getFields)
        self.form.clearanceHeight.editingFinished.connect(self.getFields)

        # operation
        self.form.direction.currentIndexChanged.connect(self.getFields)
        self.form.uiToolController.currentIndexChanged.connect(self.getFields)
        self.form.useCompensation.clicked.connect(self.getFields)
        self.form.useStartPoint.clicked.connect(self.getFields)
        self.form.extraOffset.editingFinished.connect(self.getFields)

        self.setFields()


class SelObserver:
    def __init__(self):
        import PathScripts.PathSelection as PST
        PST.contourselect()

    def __del__(self):
        import PathScripts.PathSelection as PST
        PST.clear()

    def addSelection(self, doc, obj, sub, pnt):
        FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj + ')')
        FreeCADGui.updateGui()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Contour', CommandPathContour())
    FreeCADGui.addCommand('Set_StartPoint', _CommandSetStartPoint())

FreeCAD.Console.PrintLog("Loading PathContour... done\n")
