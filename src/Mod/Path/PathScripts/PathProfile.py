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
import TechDraw
from FreeCAD import Vector
from PathScripts import PathUtils
from PathScripts.PathUtils import depth_params

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    # Qt tanslation handling
    try:
        _encoding = QtGui.QApplication.UnicodeUTF8
        def translate(context, text, disambig=None):
            return QtGui.QApplication.translate(context, text, disambig, _encoding)
    except AttributeError:
        def translate(context, text, disambig=None):
            return QtGui.QApplication.translate(context, text, disambig)
else:
    def translate(ctxt, txt):
        return txt

__title__ = "Path Profile Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

"""Path Profile object and FreeCAD command"""

class ObjectProfile:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLinkSubList", "Base", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The base geometry of this toolpath"))
        obj.addProperty("App::PropertyBool", "Active", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString", "Comment", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","An optional comment for this profile"))
        obj.addProperty("App::PropertyString", "UserLabel", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","User Assigned Label"))

        obj.addProperty("App::PropertyIntegerConstraint", "ToolNumber", "Tool", "The tool number in use")
        obj.ToolNumber = (0, 0, 1000, 1)
        obj.setEditorMode('ToolNumber', 1)  # make this read only
        obj.addProperty("App::PropertyString", "ToolDescription", "Tool", "The description of the tool ")
        obj.setEditorMode('ToolDescription', 1) # make this read only

        # Depth Properties
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Rapid Safety Height between locations."))
        obj.addProperty("App::PropertyFloatConstraint", "StepDown", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Incremental Step Down of Tool"))
        obj.StepDown = (1, 0.01, 1000, 0.5)
        obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Final Depth of Tool- lowest value in Z"))

        # Start Point Properties
        obj.addProperty("App::PropertyVector", "StartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property","The start point of this path"))
        obj.addProperty("App::PropertyBool", "UseStartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property","make True, if specifying a Start Point"))
        obj.addProperty("App::PropertyLength", "ExtendAtStart", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property","extra length of tool path before start of part edge"))
        obj.addProperty("App::PropertyLength", "LeadInLineLen", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property","length of straight segment of toolpath that comes in at angle to first part edge"))

        # End Point Properties
        obj.addProperty("App::PropertyBool", "UseEndPoint", "End Point", QtCore.QT_TRANSLATE_NOOP("App::Property","make True, if specifying an End Point"))
        obj.addProperty("App::PropertyLength", "ExtendAtEnd", "End Point", QtCore.QT_TRANSLATE_NOOP("App::Property","extra length of tool path after end of part edge"))
        obj.addProperty("App::PropertyLength", "LeadOutLineLen", "End Point", QtCore.QT_TRANSLATE_NOOP("App::Property","length of straight segment of toolpath that comes in at angle to last part edge"))
        obj.addProperty("App::PropertyVector", "EndPoint", "End Point", QtCore.QT_TRANSLATE_NOOP("App::Property","The end point of this path"))

        # Profile Properties
        obj.addProperty("App::PropertyEnumeration", "Side", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property","Side of edge that tool should cut"))
        obj.Side = ['Left', 'Right', 'On']  # side of profile that cutter is on in relation to direction of profile
        obj.addProperty("App::PropertyEnumeration", "Direction", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property","The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.Direction = ['CW', 'CCW']  # this is the direction that the profile runs
        obj.addProperty("App::PropertyBool", "UseComp", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property","make True, if using Cutter Radius Compensation"))

        obj.addProperty("App::PropertyDistance", "RollRadius", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property","Radius at start and end"))
        obj.addProperty("App::PropertyDistance", "OffsetExtra", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property","Extra value to stay away from final profile- good for roughing toolpath"))
        obj.addProperty("App::PropertyLength", "SegLen", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property","Tesselation  value for tool paths made from beziers, bsplines, and ellipses"))
        obj.addProperty("App::PropertyAngle", "PlungeAngle", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property","Plunge angle with which the tool enters the work piece. Straight down is 90 degrees, if set small enough or zero the tool will descent exactly one layer depth down per turn"))
        obj.addProperty("App::PropertyBool", "processHoles", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property","Profile holes as well as the outline"))
        obj.addProperty("App::PropertyBool", "processPerimeter", "Profile", QtCore.QT_TRANSLATE_NOOP("App::Property","Profile the outline"))

        obj.addProperty("App::PropertyVectorList", "locs", "Tags", QtCore.QT_TRANSLATE_NOOP("App::Property","List of holding tag locations"))

        obj.addProperty("App::PropertyFloatList", "angles", "Tags", QtCore.QT_TRANSLATE_NOOP("App::Property","List of angles for the holding tags"))
        obj.addProperty("App::PropertyFloatList", "heights", "Tags", QtCore.QT_TRANSLATE_NOOP("App::Property","List of angles for the holding tags"))
        obj.addProperty("App::PropertyFloatList", "lengths", "Tags", QtCore.QT_TRANSLATE_NOOP("App::Property","List of angles for the holding tags"))
        locations = []
        angles = []
        lengths = []
        heights = []

        obj.locs = locations
        obj.angles = angles
        obj.lengths = lengths
        obj.heights = heights

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, obj, prop):
        if prop == "UserLabel":
            obj.Label = obj.UserLabel + " :" + obj.ToolDescription

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
                else: #catch all
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

    def _buildPathLibarea(self, obj, edgelist, isHole):
        import PathScripts.PathKurveUtils as PathKurveUtils
        import math
        import area
        output = ""
        if obj.Comment != "":
            output += '(' + str(obj.Comment)+')\n'

        if obj.StartPoint and obj.UseStartPoint:
            startpoint = obj.StartPoint
        else:
            startpoint = None

        if obj.EndPoint and obj.UseEndPoint:
            endpoint = obj.EndPoint
        else:
            endpoint = None

        PathKurveUtils.output('mem')
        PathKurveUtils.feedrate_hv(self.horizFeed, self.vertFeed)

        # Reverse the direction for holes
        if isHole:
            direction = "CW" if obj.Direction == "CCW" else "CCW"
        else:
            direction = obj.Direction

        output = ""
        output += "G0 Z" + str(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) + "\n"
        curve = PathKurveUtils.makeAreaCurve(edgelist, direction, startpoint, endpoint)

        '''The following line uses a profile function written for use with FreeCAD.  It's clean but incomplete.  It doesn't handle
print "x = " + str(point.x)
print "y - " + str(point.y)
            holding tags
            start location
            CRC
            or probably other features in heekscnc'''

        '''The following calls the original procedure from h
        toolLoad = obj.activeTCeekscnc profile function.  This, in turn, calls many other procedures to modify the profile.
            This procedure is hacked together from heekscnc and has not been thoroughly reviewed or understood for FreeCAD.  It can probably be
            thoroughly optimized and improved but it'll take a smarter mind than mine to do it.  -sliptonic Feb16'''
        roll_radius = 2.0
        extend_at_start = 0.0
        extend_at_end = 0.0
        lead_in_line_len = 0.0
        lead_out_line_len = 0.0

        '''

        Right here, I need to know the Holding Tags group from the tree that refers to this profile operation and build up the tags for PathKurve Utils.
        I need to access the location vector, length, angle in radians and height.

        '''
        PathKurveUtils.clear_tags()
        for i in range(len(obj.locs)):
            tag = obj.locs[i]
            h = obj.heights[i]
            l = obj.lengths[i]
            a = math.radians(obj.angles[i])
            PathKurveUtils.add_tag(area.Point(tag.x, tag.y), l, a, h)

        depthparams = depth_params(
            obj.ClearanceHeight.Value,
            obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown, 0.0,
            obj.FinalDepth.Value, None)

        PathKurveUtils.profile2(
            curve, obj.Side, self.radius, self.vertFeed, self.horizFeed,
            self.vertRapid, self.horizRapid, obj.OffsetExtra.Value, roll_radius,
            None, None, depthparams, extend_at_start, extend_at_end,
            lead_in_line_len, lead_out_line_len)

        output += PathKurveUtils.retrieve_gcode()
        return output

    def execute(self, obj):
        import Part  # math #DraftGeomUtils
        output = ""

        toolLoad = PathUtils.getLastToolLoad(obj)

        if toolLoad is None or toolLoad.ToolNumber == 0:
            self.vertFeed = 100
            self.horizFeed = 100
            self.vertRapid = 100
            self.horizRapid = 100
            self.radius = 0.25
            obj.ToolNumber = 0
            obj.ToolDescription = "UNDEFINED"
        else:
            self.vertFeed = toolLoad.VertFeed.Value
            self.horizFeed = toolLoad.HorizFeed.Value
            self.vertRapid = toolLoad.VertRapid.Value
            self.horizRapid = toolLoad.HorizRapid.Value
            tool = PathUtils.getTool(obj, toolLoad.ToolNumber)
            if tool.Diameter == 0:
                self.radius = 0.25
            else:
                self.radius = tool.Diameter/2
            obj.ToolNumber = toolLoad.ToolNumber
            obj.ToolDescription = toolLoad.Name

        if obj.UserLabel == "":
            obj.Label = obj.Name + " :" + obj.ToolDescription
        else:
            obj.Label = obj.UserLabel + " :" + obj.ToolDescription

        output += "(" + obj.Label + ")"
        if obj.Side != "On":
            output += "(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")"
        else:
            output += "(Uncompensated Tool Path)"

        if obj.Base:
            holes = []
            faces = []
            for b in obj.Base:
                for sub in b[1]:
                    shape = getattr(b[0].Shape, sub)
                    if isinstance (shape, Part.Face):
                        faces.append(shape)
                        if numpy.isclose(abs(shape.normalAt(0, 0).z), 1):  # horizontal face
                            holes += shape.Wires[1:]

                    else:
                        print ("found a base object which is not a face.  Can't continue.")
                        return
            profileshape = Part.makeCompound(faces)
            profilewire = TechDraw.findShapeOutline(profileshape, 1, Vector(0,0,1))

            if obj.processHoles:
                for wire in holes:
                    edgelist = wire.Edges
                    edgelist = Part.__sortEdges__(edgelist)
                    output += self._buildPathLibarea(obj, edgelist, True)

            if obj.processPerimeter:
                edgelist = profilewire.Edges
                edgelist = Part.__sortEdges__(edgelist)
                output += self._buildPathLibarea(obj, edgelist, False)

        if obj.Active:
            path = Path.Path(output)
            obj.Path = path
            obj.ViewObject.Visibility = True

        else:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            obj.ViewObject.Visibility = False


class _ViewProviderProfile:

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
        return ":/icons/Path-Profile-Face.svg"

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _CommandAddTag:
    def GetResources(self):
        return {'Pixmap': 'Path-Holding',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Profile", "Add Holding Tag"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Profile", "Add Holding Tag")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def setpoint(self, point, o):
        obj = FreeCADGui.Selection.getSelection()[0]
        obj.StartPoint.x = point.x
        obj.StartPoint.y = point.y
        loc = obj.locs
        h = obj.heights
        l = obj.lengths
        a = obj.angles

        x = point.x
        y = point.y
        z = float(0.0)
        loc.append(Vector(x, y, z))
        h.append(4.0)
        l.append(5.0)
        a.append(45.0)

        obj.locs = loc
        obj.heights = h
        obj.lengths = l
        obj.angles = a

    def Activated(self):

        FreeCADGui.Snapper.getPoint(callback=self.setpoint)


class _CommandSetStartPoint:
    def GetResources(self):
        return {'Pixmap': 'Path-Holding',
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


class _CommandSetEndPoint:
    def GetResources(self):
        return {'Pixmap': 'Path-Holding',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Profile", "Pick End Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Profile", "Pick End Point")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def setpoint(self, point, o):
        obj = FreeCADGui.Selection.getSelection()[0]
        obj.EndPoint.x = point.x
        obj.EndPoint.y = point.y

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
        FreeCADGui.doCommand('obj.Direction = "CCW"')
        FreeCADGui.doCommand('obj.UseComp = False')
        FreeCADGui.doCommand('obj.processHoles = False')
        FreeCADGui.doCommand('obj.processPerimeter = True')
        FreeCADGui.doCommand('obj.PlungeAngle = 90.0')
        FreeCADGui.doCommand('PathScripts.PathProfile._ViewProviderProfile(obj.ViewObject)')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')


class TaskPanel:
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ProfileEdit.ui")
        self.updating = False

    def accept(self):
        self.getFields()

        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)

    def getFields(self):
        if self.obj:
            if hasattr(self.obj, "StartDepth"):
                self.obj.StartDepth = self.form.startDepth.text()
            if hasattr(self.obj, "FinalDepth"):
                self.obj.FinalDepth = self.form.finalDepth.text()
            if hasattr(self.obj, "SafeHeight"):
                self.obj.SafeHeight = self.form.safeHeight.text()
            if hasattr(self.obj, "ClearanceHeight"):
                self.obj.ClearanceHeight = self.form.clearanceHeight.text()
            if hasattr(self.obj, "StepDown"):
                self.obj.StepDown = self.form.stepDown.value()
            if hasattr(self.obj, "OffsetExtra"):
                self.obj.OffsetExtra = self.form.extraOffset.value()
            if hasattr(self.obj, "SegLen"):
                self.obj.SegLen = self.form.segLen.value()
            if hasattr(self.obj, "RollRadius"):
                self.obj.RollRadius = self.form.rollRadius.value()
            if hasattr(self.obj, "PlungeAngle"):
                self.obj.PlungeAngle = str(self.form.plungeAngle.value())
            if hasattr(self.obj, "UseComp"):
                self.obj.UseComp = self.form.useCompensation.isChecked()
            if hasattr(self.obj, "UseStartPoint"):
                self.obj.UseStartPoint = self.form.useStartPoint.isChecked()
            if hasattr(self.obj, "UseEndPoint"):
                self.obj.UseEndPoint = self.form.useEndPoint.isChecked()
            if hasattr(self.obj, "Side"):
                self.obj.Side = str(self.form.cutSide.currentText())
            if hasattr(self.obj, "Direction"):
                self.obj.Direction = str(self.form.direction.currentText())
            if hasattr(self.obj, "processHoles"):
                self.obj.processHoles = self.form.processHoles.isChecked()
            if hasattr(self.obj, "processPerimeter"):
                self.obj.processPerimeter = self.form.processPerimeter.isChecked()
        self.obj.Proxy.execute(self.obj)

    def setFields(self):
        self.form.startDepth.setText(str(self.obj.StartDepth.Value))
        self.form.finalDepth.setText(str(self.obj.FinalDepth.Value))
        self.form.safeHeight.setText(str(self.obj.SafeHeight.Value))
        self.form.clearanceHeight.setText(str(self.obj.ClearanceHeight.Value))
        self.form.stepDown.setValue(self.obj.StepDown)
        self.form.extraOffset.setValue(self.obj.OffsetExtra.Value)
        self.form.segLen.setValue(self.obj.SegLen.Value)
        self.form.rollRadius.setValue(self.obj.RollRadius.Value)
        self.form.plungeAngle.setValue(self.obj.PlungeAngle.Value)
        self.form.useCompensation.setChecked(self.obj.UseComp)
        self.form.useStartPoint.setChecked(self.obj.UseStartPoint)
        self.form.useEndPoint.setChecked(self.obj.UseEndPoint)
        self.form.processHoles.setChecked(self.obj.processHoles)
        self.form.processPerimeter.setChecked(self.obj.processPerimeter)

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

        self.form.baseList.blockSignals(True)
        for i in self.obj.Base:
            for sub in i[1]:
                self.form.baseList.addItem(i[0].Name + "." + sub)
        self.form.baseList.blockSignals(False)

        self.form.tagTree.blockSignals(True)
        for i in range(len(self.obj.locs)):
            item = QtGui.QTreeWidgetItem(self.form.tagTree)
            item.setText(0, str(i+1))
            l = self.obj.locs[i]
            item.setText(1, str(l.x)+", " + str(l.y) + ", " + str(l.z))
            item.setText(2, str(self.obj.heights[i]))
            item.setText(3, str(self.obj.lengths[i]))
            item.setText(4, str(self.obj.angles[i]))
            item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
            item.setTextAlignment(0, QtCore.Qt.AlignLeft)
        self.form.tagTree.blockSignals(False)
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
        return int(QtGui.QDialogButtonBox.Ok)

    def edit(self, item, column):
        if not self.updating:
            self.resetObject()

    def resetObject(self, remove=None):
        "transfers the values from the widget to the object"
        loc = []
        h = []
        l = []
        a = []

        for i in range(self.form.tagTree.topLevelItemCount()):
            it = self.form.tagTree.findItems(
                    str(i+1), QtCore.Qt.MatchExactly, 0)[0]
            if (remove is None) or (remove != i):
                if it.text(1):
                    x = float(it.text(1).split()[0].rstrip(","))
                    y = float(it.text(1).split()[1].rstrip(","))
                    z = float(it.text(1).split()[2].rstrip(","))
                    loc.append(Vector(x, y, z))

                else:
                    loc.append(0.0)
                if it.text(2):
                    h.append(float(it.text(2)))
                else:
                    h.append(4.0)
                if it.text(3):
                    l.append(float(it.text(3)))
                else:
                    l.append(5.0)
                if it.text(4):
                    a.append(float(it.text(4)))
                else:
                    a.append(45.0)

        self.obj.locs = loc
        self.obj.heights = h
        self.obj.lengths = l
        self.obj.angles = a

        self.obj.touch()
        FreeCAD.ActiveDocument.recompute()

    def addElement(self):
        self.updating = True

        item = QtGui.QTreeWidgetItem(self.form.tagTree)
        item.setText(0, str(self.form.tagTree.topLevelItemCount()))
        item.setText(1, "0.0, 0.0, 0.0")
        item.setText(2, str(float(4.0)))
        item.setText(3, str(float(10.0)))
        item.setText(4, str(float(45.0)))
        item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
        self.updating = False

        self.resetObject()

    def removeElement(self):
        it = self.form.tagTree.currentItem()
        if it:
            nr = int(it.text(0))-1
            self.resetObject(remove=nr)
            self.update()

    def update(self):
        'fills the treewidget'
        self.updating = True
        self.form.tagTree.clear()
        if self.obj:
            for i in range(len(self.obj.locs)):
                item = QtGui.QTreeWidgetItem(self.form.tagTree)
                item.setText(0, str(i+1))
                l = self.obj.locs[i]
                item.setText(1, str(l.x) + ", " + str(l.y) + ", " + str(l.z))
                item.setText(2, str(self.obj.heights[i]))
                item.setText(3, str(self.obj.lengths[i]))
                item.setText(4, str(self.obj.angles[i]))
                item.setFlags(item.flags() | QtCore.Qt.ItemIsEditable)
                item.setTextAlignment(0, QtCore.Qt.AlignLeft)
        self.updating = False
        return


    def setupUi(self):

        # Connect Signals and Slots
        # Base Controls
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.form.addBase.clicked.connect(self.addBase)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.reorderBase.clicked.connect(self.reorderBase)

        # Depths
        self.form.startDepth.editingFinished.connect(self.getFields)
        self.form.finalDepth.editingFinished.connect(self.getFields)
        self.form.stepDown.editingFinished.connect(self.getFields)

        # Heights
        self.form.safeHeight.editingFinished.connect(self.getFields)
        self.form.clearanceHeight.editingFinished.connect(self.getFields)

        # operation
        self.form.cutSide.currentIndexChanged.connect(self.getFields)
        self.form.direction.currentIndexChanged.connect(self.getFields)
        self.form.useCompensation.clicked.connect(self.getFields)
        self.form.useStartPoint.clicked.connect(self.getFields)
        self.form.useEndPoint.clicked.connect(self.getFields)
        self.form.extraOffset.editingFinished.connect(self.getFields)
        self.form.segLen.editingFinished.connect(self.getFields)
        self.form.rollRadius.editingFinished.connect(self.getFields)
        self.form.processHoles.clicked.connect(self.getFields)
        self.form.processPerimeter.clicked.connect(self.getFields)

        # Tag Form
        QtCore.QObject.connect(
                self.form.tagTree,
                QtCore.SIGNAL("itemChanged(QTreeWidgetItem *, int)"),
                self.edit)
        self.form.addTag.clicked.connect(self.addElement)
        self.form.deleteTag.clicked.connect(self.removeElement)

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
    FreeCADGui.addCommand('Add_Tag', _CommandAddTag())
    FreeCADGui.addCommand('Set_StartPoint', _CommandSetStartPoint())
    FreeCADGui.addCommand('Set_EndPoint', _CommandSetEndPoint())

FreeCAD.Console.PrintLog("Loading PathProfile... done\n")
