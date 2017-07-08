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
import numpy
import ArchPanel
import Part

from PathScripts import PathUtils
from PathScripts.PathUtils import depth_params
import PathScripts.PathLog as PathLog

LOG_MODULE = 'PathProfile'
PathLog.setLevel(PathLog.Level.INFO, LOG_MODULE)
PathLog.trackModule('PathProfile')
FreeCAD.setLogLevel('Path.Area', 0)

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui


# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

__title__ = "Path Profile Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

"""Path Profile object and FreeCAD command"""


class ObjectProfile:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLinkSubList", "Base", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The base geometry of this toolpath"))
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

        # Start Point Properties
        obj.addProperty("App::PropertyVector", "StartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property", "The start point of this path"))
        obj.addProperty("App::PropertyBool", "UseStartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property", "make True, if specifying a Start Point"))

        # Profile Properties
        obj.addProperty("App::PropertyEnumeration", "Side", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Side of edge that tool should cut"))
        obj.Side = ['Left', 'Right']  # side of profile that cutter is on in relation to direction of profile
        obj.addProperty("App::PropertyEnumeration", "Direction", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.Direction = ['CW', 'CCW']  # this is the direction that the profile runs
        obj.addProperty("App::PropertyBool", "UseComp", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "make True, if using Cutter Radius Compensation"))
        obj.addProperty("App::PropertyDistance", "OffsetExtra", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Extra value to stay away from final profile- good for roughing toolpath"))
        obj.addProperty("App::PropertyBool", "processHoles", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile holes as well as the outline"))
        obj.addProperty("App::PropertyBool", "processPerimeter", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile the outline"))
        obj.addProperty("App::PropertyBool", "processCircles", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile round holes"))
        obj.addProperty("App::PropertyEnumeration", "JoinType", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Controls how tool moves around corners. Default=Round"))
        obj.JoinType = ['Round', 'Square', 'Miter']  # this is the direction that the Contour runs
        obj.addProperty("App::PropertyFloat", "MiterLimit", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property", "Maximum distance before a miter join is truncated"))

        # Debug Parameters
        obj.addProperty("App::PropertyString", "AreaParams", "Path")
        obj.setEditorMode('AreaParams', 2)  # hide
        obj.addProperty("App::PropertyString", "PathParams", "Path")
        obj.setEditorMode('PathParams', 2)  # hide
        obj.addProperty("Part::PropertyPartShape", "removalshape", "Path")
        obj.setEditorMode('removalshape', 2)  # hide

        if FreeCAD.GuiUp:
            _ViewProviderProfile(obj.ViewObject)

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, obj, prop):
        if prop == "UseComp":
            if not obj.UseComp:
                obj.setEditorMode('Side', 2)
            else:
                obj.setEditorMode('Side', 0)
        if prop in ['AreaParams', 'PathParams', 'removalshape']:
            obj.setEditorMode(prop, 2)

        obj.setEditorMode('MiterLimit', 2)
        if obj.JoinType == 'Miter':
            obj.setEditorMode('MiterLimit', 0)

    def addprofilebase(self, obj, ss, sub=""):
        baselist = obj.Base
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

            if bb.XLength == fbb.XLength and bb.YLength == fbb.YLength:
                obj.Side = "Left"
            else:
                obj.Side = "Right"

        item = (ss, sub)
        if item in baselist:
            FreeCAD.Console.PrintWarning("this object already in the list" + "\n")
        else:
            baselist.append(item)
        obj.Base = baselist
        self.execute(obj)

    def _buildPathArea(self, obj, baseobject, isHole=False, start=None, getsim=False):
        PathLog.track()
        profile = Path.Area()
        profile.setPlane(Part.makeCircle(10))
        profile.add(baseobject)

        profileparams = {'Fill': 0,
                         'Coplanar': 2,
                         'Offset': 0.0,
                         'SectionCount': -1}

        offsetval = 0

        if obj.UseComp:
            offsetval = self.radius+obj.OffsetExtra.Value

        if obj.Side == 'Right':
            offsetval = 0 - offsetval

        if isHole:
            offsetval = 0 - offsetval

        profileparams['Offset'] = offsetval

        jointype = ['Round', 'Square', 'Miter']
        profileparams['JoinType'] = jointype.index(obj.JoinType)

        if obj.JoinType == 'Miter':
            profileparams['MiterLimit'] = obj.MiterLimit

        profile.setParams(**profileparams)
        obj.AreaParams = str(profile.getParams())

        # PathLog.debug("About to profile with params: {}".format(profileparams))
        PathLog.debug("About to profile with params: {}".format(profile.getParams()))

        heights = [i for i in self.depthparams]

        sections = profile.makeSections(mode=0, project=True, heights=heights)
        shapelist = [sec.getShape() for sec in sections]

        params = {'shapes': shapelist,
                  'feedrate': self.horizFeed,
                  'feedrate_v': self.vertFeed,
                  'verbose': True,
                  'resume_height': obj.StepDown.Value,
                  'retraction': obj.ClearanceHeight.Value}

        # Reverse the direction for holes
        if isHole:
            direction = "CW" if obj.Direction == "CCW" else "CCW"
        else:
            direction = obj.Direction

        if direction == 'CCW':
            params['orientation'] = 1
        else:
            params['orientation'] = 0

        if obj.UseStartPoint is True and obj.StartPoint is not None:
            params['start'] = obj.StartPoint

        pp = Path.fromShapes(**params)

        obj.PathParams = str({key: value for key, value in params.items() if key != 'shapes'})

        PathLog.debug("Generating Path with params: {}".format(params))
        PathLog.debug(pp)

        simobj = None
        if getsim:
            profileparams['Thicken'] = True
            profileparams['ToolRadius'] = self.radius - self.radius * .005
            profile.setParams(**profileparams)
            sec = profile.makeSections(mode=0, project=False, heights=heights)[-1].getShape()
            simobj = sec.extrude(FreeCAD.Vector(0, 0, baseobject.BoundBox.ZMax))

        return pp, simobj

    def execute(self, obj, getsim=False):
        import Part

        if not obj.Active:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            obj.ViewObject.Visibility = False
            return

        self.depthparams = depth_params(
                clearance_height=obj.ClearanceHeight.Value,
                safe_height=obj.SafeHeight.Value,
                start_depth=obj.StartDepth.Value,
                step_down=obj.StepDown.Value,
                z_finish_step=0.0,
                final_depth=obj.FinalDepth.Value,
                user_depths=None)

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

        if obj.Base:  # The user has selected subobjects from the base.  Process each.
            holes = []
            faces = []
            for b in obj.Base:
                for sub in b[1]:
                    shape = getattr(b[0].Shape, sub)
                    if isinstance(shape, Part.Face):
                        faces.append(shape)
                        if numpy.isclose(abs(shape.normalAt(0, 0).z), 1):  # horizontal face
                            holes += shape.Wires[1:]
                    else:
                        FreeCAD.Console.PrintWarning("found a base object which is not a face.  Can't continue.")
                        return

            for wire in holes:
                f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                drillable = PathUtils.isDrillable(baseobject.Shape, wire)
                if (drillable and obj.processCircles) or (not drillable and obj.processHoles):
                    env = PathUtils.getEnvelope(baseobject.Shape, subshape=f, depthparams=self.depthparams)
                    try:
                        (pp, sim) = self._buildPathArea(obj, baseobject=env, isHole=True, start=None, getsim=getsim)
                        commandlist.extend(pp.Commands)
                    except Exception as e:
                        FreeCAD.Console.PrintError(e)
                        FreeCAD.Console.PrintError("Something unexpected happened. Unable to generate a contour path. Check project and tool config.")

            if len(faces) > 0:
                profileshape = Part.makeCompound(faces)

            if obj.processPerimeter:
                env = PathUtils.getEnvelope(baseobject.Shape, subshape=profileshape, depthparams=self.depthparams)
                try:
                    (pp, sim) = self._buildPathArea(obj, baseobject=env, start=None, getsim=getsim)
                    commandlist.extend(pp.Commands)
                except Exception as e:
                    FreeCAD.Console.PrintError(e)
                    FreeCAD.Console.PrintError("Something unexpected happened. Unable to generate a contour path. Check project and tool config.")

        else:  # Try to build targets from the job base
            if hasattr(baseobject, "Proxy"):
                if isinstance(baseobject.Proxy, ArchPanel.PanelSheet):  # process the sheet
                    if obj.processPerimeter:
                        shapes = baseobject.Proxy.getOutlines(baseobject, transform=True)
                        for shape in shapes:
                            for wire in shape.Wires:
                                f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                                env = PathUtils.getEnvelope(baseobject.Shape, subshape=f, depthparams=self.depthparams)
                                try:
                                    (pp, sim) = self._buildPathArea(obj, baseobject=env, isHole=False, start=None, getsim=getsim)
                                    commandlist.extend(pp.Commands)
                                except Exception as e:
                                    FreeCAD.Console.PrintError(e)
                                    FreeCAD.Console.PrintError("Something unexpected happened. Unable to generate a contour path. Check project and tool config.")

                    shapes = baseobject.Proxy.getHoles(baseobject, transform=True)
                    for shape in shapes:
                        for wire in shape.Wires:
                            drillable = PathUtils.isDrillable(baseobject.Proxy, wire)
                            if (drillable and obj.processCircles) or (not drillable and obj.processHoles):
                                f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                                env = PathUtils.getEnvelope(baseobject.Shape, subshape=f, depthparams=self.depthparams)
                                try:
                                    (pp, sim) = self._buildPathArea(obj, baseobject=env, isHole=True, start=None, getsim=getsim)
                                    commandlist.extend(pp.Commands)
                                except Exception as e:
                                    FreeCAD.Console.PrintError(e)
                                    FreeCAD.Console.PrintError("Something unexpected happened. Unable to generate a contour path. Check project and tool config.")

        # Let's finish by rapid to clearance...just for safety
        commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))

        path = Path.Path(commandlist)
        obj.Path = path


class _ViewProviderProfile:

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
        return ":/icons/Path-Profile-Face.svg"

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _CommandSetStartPoint:
    def GetResources(self):
        return {'Pixmap': 'Path-StartPoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Profile", "Pick Start Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Profile", "Pick Start Point")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def setpoint(self, point, o):
        obj = FreeCADGui.Selection.getSelection()[0]
        obj.StartPoint.x = point.x
        obj.StartPoint.y = point.y

    def Activated(self):

        FreeCADGui.Snapper.getPoint(callback=self.setpoint)


class CommandPathProfile:
    def GetResources(self):
        return {'Pixmap': 'Path-Profile-Face',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathProfile", "Face Profile"),
                'Accel': "P, F",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathProfile", "Profile based on face or faces")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):
        ztop = 10.0
        zbottom = 0.0

        FreeCAD.ActiveDocument.openTransaction(translate("Path", "Create a Profile"))
        FreeCADGui.addModule("PathScripts.PathProfile")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Profile")')
        FreeCADGui.doCommand('PathScripts.PathProfile.ObjectProfile(obj)')

        FreeCADGui.doCommand('obj.Active = True')

        FreeCADGui.doCommand('obj.ClearanceHeight = ' + str(ztop + 10.0))
        FreeCADGui.doCommand('obj.StepDown = 1.0')
        FreeCADGui.doCommand('obj.StartDepth= ' + str(ztop))
        FreeCADGui.doCommand('obj.FinalDepth=' + str(zbottom))

        FreeCADGui.doCommand('obj.SafeHeight = ' + str(ztop + 2.0))
        FreeCADGui.doCommand('obj.Side = "Left"')
        FreeCADGui.doCommand('obj.OffsetExtra = 0.0')
        FreeCADGui.doCommand('obj.Direction = "CW"')
        FreeCADGui.doCommand('obj.UseComp = True')
        FreeCADGui.doCommand('obj.processHoles = False')
        FreeCADGui.doCommand('obj.processPerimeter = True')
        FreeCADGui.doCommand('obj.JoinType = "Round"')
        FreeCADGui.doCommand('obj.MiterLimit =' + str(0.1))

        FreeCADGui.doCommand('obj.ViewObject.Proxy.deleteOnReject = True')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('obj.ToolController = PathScripts.PathUtils.findToolController(obj)')

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')


class TaskPanel:
    def __init__(self, obj, deleteOnReject):
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Profile", "Profile Operation"))
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ProfileEdit.ui")
        self.isDirty = True
        self.deleteOnReject = deleteOnReject
        self.obj = obj

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
            FreeCAD.ActiveDocument.openTransaction(translate("Path_Profile", "Uncreate Profile Operation"))
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
            if hasattr(self.obj, "StepDown"):
                self.obj.StepDown = FreeCAD.Units.Quantity(self.form.stepDown.text()).Value
            if hasattr(self.obj, "SafeHeight"):
                self.obj.SafeHeight = FreeCAD.Units.Quantity(self.form.safeHeight.text()).Value
            if hasattr(self.obj, "ClearanceHeight"):
                self.obj.ClearanceHeight = FreeCAD.Units.Quantity(self.form.clearanceHeight.text()).Value
            if hasattr(self.obj, "OffsetExtra"):
                self.obj.OffsetExtra = FreeCAD.Units.Quantity(self.form.extraOffset.text()).Value
            if hasattr(self.obj, "UseComp"):
                self.obj.UseComp = self.form.useCompensation.isChecked()
            if hasattr(self.obj, "UseStartPoint"):
                self.obj.UseStartPoint = self.form.useStartPoint.isChecked()
            if hasattr(self.obj, "Side"):
                self.obj.Side = str(self.form.cutSide.currentText())
            if hasattr(self.obj, "Direction"):
                self.obj.Direction = str(self.form.direction.currentText())
            if hasattr(self.obj, "processHoles"):
                self.obj.processHoles = self.form.processHoles.isChecked()
            if hasattr(self.obj, "processPerimeter"):
                self.obj.processPerimeter = self.form.processPerimeter.isChecked()
            if hasattr(self.obj, "processCircles"):
                self.obj.processCircles = self.form.processCircles.isChecked()
            if hasattr(self.obj, "ToolController"):
                PathLog.debug("name: {}".format(self.form.uiToolController.currentText()))
                tc = PathUtils.findToolController(self.obj, self.form.uiToolController.currentText())
                self.obj.ToolController = tc

        self.isDirty = True

    def setFields(self):
        self.form.startDepth.setText(FreeCAD.Units.Quantity(self.obj.StartDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.finalDepth.setText(FreeCAD.Units.Quantity(self.obj.FinalDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.stepDown.setText(FreeCAD.Units.Quantity(self.obj.StepDown.Value, FreeCAD.Units.Length).UserString)
        self.form.safeHeight.setText(FreeCAD.Units.Quantity(self.obj.SafeHeight.Value, FreeCAD.Units.Length).UserString)
        self.form.clearanceHeight.setText(FreeCAD.Units.Quantity(self.obj.ClearanceHeight.Value,  FreeCAD.Units.Length).UserString)
        self.form.extraOffset.setText(FreeCAD.Units.Quantity(self.obj.OffsetExtra.Value, FreeCAD.Units.Length).UserString)
        self.form.useCompensation.setChecked(self.obj.UseComp)
        self.form.useStartPoint.setChecked(self.obj.UseStartPoint)
        self.form.processHoles.setChecked(self.obj.processHoles)
        self.form.processPerimeter.setChecked(self.obj.processPerimeter)
        self.form.processCircles.setChecked(self.obj.processCircles)

        index = self.form.cutSide.findText(
                self.obj.Side, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.cutSide.blockSignals(True)
            self.form.cutSide.setCurrentIndex(index)
            self.form.cutSide.blockSignals(False)

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

        self.form.baseList.blockSignals(True)
        for i in self.obj.Base:
            for sub in i[1]:
                self.form.baseList.addItem(i[0].Name + "." + sub)
        self.form.baseList.blockSignals(False)

        self.form.update()

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
            self.obj.Proxy.addprofilebase(self.obj, sel.Object, i)

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
        self.form.stepDown.editingFinished.connect(self.getFields)

        # Heights
        self.form.safeHeight.editingFinished.connect(self.getFields)
        self.form.clearanceHeight.editingFinished.connect(self.getFields)

        # operation
        self.form.cutSide.currentIndexChanged.connect(self.getFields)
        self.form.uiToolController.currentIndexChanged.connect(self.getFields)
        self.form.direction.currentIndexChanged.connect(self.getFields)
        self.form.useCompensation.clicked.connect(self.getFields)
        self.form.useStartPoint.clicked.connect(self.getFields)
        self.form.extraOffset.editingFinished.connect(self.getFields)
        self.form.processHoles.clicked.connect(self.getFields)
        self.form.processPerimeter.clicked.connect(self.getFields)
        self.form.processCircles.clicked.connect(self.getFields)

        self.setFields()

        sel = FreeCADGui.Selection.getSelectionEx()
        if len(sel) != 0 and sel[0].HasSubObjects:
                self.addBase()


class SelObserver:
    def __init__(self):
        import PathScripts.PathSelection as PST
        PST.profileselect()

    def __del__(self):
        import PathScripts.PathSelection as PST
        PST.clear()

    def addSelection(self, doc, obj, sub, pnt):
        FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj + ')')
        FreeCADGui.updateGui()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Profile', CommandPathProfile())
    FreeCADGui.addCommand('Set_StartPoint', _CommandSetStartPoint())

FreeCAD.Console.PrintLog("Loading PathProfile... done\n")
