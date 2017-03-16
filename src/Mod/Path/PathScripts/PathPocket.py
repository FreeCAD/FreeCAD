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

FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui

"""Path Pocket object and FreeCAD command"""

LOG_MODULE = 'PathPocket'
PathLog.setLevel(PathLog.Level.INFO, LOG_MODULE)
# PathLog.trackModule('PathPocket')

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectPocket:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLinkSubList", "Base", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The base geometry of this object"))
        obj.addProperty("App::PropertyBool", "Active", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString", "Comment", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "An optional comment for this profile"))
        obj.addProperty("App::PropertyString", "UserLabel", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "User Assigned Label"))

        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm", QtCore.QT_TRANSLATE_NOOP("App::Property", "The library to use to generate the path"))
        obj.Algorithm = ['OCC Native', 'libarea']

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
        obj.addProperty("App::PropertyBool", "KeepToolDown", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Attempts to avoid unnecessary retractions."))
        obj.addProperty("App::PropertyBool", "ZigUnidirectional", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Lifts tool at the end of each pass to respect cut mode."))
        obj.addProperty("App::PropertyBool", "UseZigZag", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Use Zig Zag pattern to clear area."))
        obj.addProperty("App::PropertyFloat", "ZigZagAngle", "Pocket", QtCore.QT_TRANSLATE_NOOP("App::Property", "Angle of the zigzag pattern"))

        # Entry Properties
        obj.addProperty("App::PropertyBool", "UseEntry", "Entry", QtCore.QT_TRANSLATE_NOOP("App::Property", "Allow Cutter enter material with a straight plunge."))
        obj.addProperty("App::PropertyFloatConstraint", "RampSize", "Entry", QtCore.QT_TRANSLATE_NOOP("App::Property", "The minimum fraction of tool diameter to use for ramp length"))
        obj.RampSize = (0.0, 0.01, 100.0, 0.5)
        obj.addProperty("App::PropertyFloatConstraint", "HelixSize", "Entry", QtCore.QT_TRANSLATE_NOOP("App::Property", "The fraction of tool diameter to use for calculating helix size."))
        obj.HelixSize = (0.0, 0.01, 100.0, 0.5)
        obj.addProperty("App::PropertyFloatConstraint", "RampAngle", "Entry", QtCore.QT_TRANSLATE_NOOP("App::Property", "The Angle of the ramp entry."))
        obj.RampAngle = (0.0, 0.01, 100.0, 0.5)

        # Start Point Properties
        obj.addProperty("App::PropertyVector", "StartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property", "The start point of this path"))
        obj.addProperty("App::PropertyBool", "UseStartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property", "make True, if specifying a Start Point"))

        obj.Proxy = self

    def onChanged(self, obj, prop):
        if prop == "UseEntry":
            if obj.UseEntry:
                obj.setEditorMode('HelixSize', 0)  # make this visible
                obj.setEditorMode('RampAngle', 0)  # make this visible
                obj.setEditorMode('RampSize', 0)   # make this visible
            else:
                obj.setEditorMode('HelixSize', 2)  # make this hidden
                obj.setEditorMode('RampAngle', 2)  # make this hidden
                obj.setEditorMode('RampSize', 2)   # make this hidden

        if prop == "UserLabel":
            obj.Label = obj.UserLabel + " :" + obj.ToolDescription

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def addpocketbase(self, obj, ss, sub=""):
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
        print("this base is: " + str(baselist))
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

    def buildpathlibarea(self, obj, a):
        """Build the pocket path using libarea algorithm"""
        import PathScripts.PathAreaUtils as PathAreaUtils
        from PathScripts.PathUtils import depth_params
        PathLog.debug("Generating toolpath with libarea offsets.\n")

        depthparams = depth_params(
                obj.ClearanceHeight.Value,
                obj.SafeHeight.Value,
                obj.StartDepth.Value,
                obj.StepDown,
                obj.FinishDepth.Value,
                obj.FinalDepth.Value)

        extraoffset = obj.MaterialAllowance.Value
        stepover = (self.radius * 2) * (float(obj.StepOver)/100)
        use_zig_zag = obj.UseZigZag
        zig_angle = obj.ZigZagAngle
        from_center = (obj.StartAt == "Center")
        keep_tool_down = obj.KeepToolDown
        zig_unidirectional = obj.ZigUnidirectional
        start_point = None
        cut_mode = obj.CutMode

        PathAreaUtils.flush_nc()
        PathAreaUtils.output('mem')
        PathAreaUtils.feedrate_hv(self.horizFeed, self.vertFeed)
        if obj.UseStartPoint:
            start_point = (obj.StartPoint.x, obj.StartPoint.y)

        # print "a," + str(self.radius) + "," + str(extraoffset) + "," + str(stepover) + ",depthparams, " + str(from_center) + "," + str(keep_tool_down) + "," + str(use_zig_zag) + "," + str(zig_angle) + "," + str(zig_unidirectional) + "," + str(start_point) + "," + str(cut_mode)

        PathAreaUtils.pocket(
                a,
                self.radius,
                extraoffset,
                stepover,
                depthparams,
                from_center,
                keep_tool_down,
                use_zig_zag,
                zig_angle,
                zig_unidirectional,
                start_point,
                cut_mode)
        return PathAreaUtils.retrieve_gcode()

    def buildpathocc(self, obj, shape):
        """Build pocket Path using Native OCC algorithm."""
        import Part
        import DraftGeomUtils
        from PathScripts.PathUtils import fmt, helicalPlunge, rampPlunge, depth_params

        PathLog.debug("Generating toolpath with OCC native offsets.\n")
        extraoffset = obj.MaterialAllowance.Value

        # Build up the offset loops
        output = ""
        if obj.Comment != "":
            output += '(' + str(obj.Comment)+')\n'
        output += 'G0 Z' + fmt(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) + "\n"

        offsets = []
        nextradius = self.radius + extraoffset
        result = DraftGeomUtils.pocket2d(shape, nextradius)
        while result:
            offsets.extend(result)
            nextradius += (self.radius * 2) * (float(obj.StepOver)/100)
            result = DraftGeomUtils.pocket2d(shape, nextradius)

        # revert the list so we start with the outer wires
        if obj.StartAt != 'Edge':
            offsets.reverse()

        plungePos = None
        rampEdge = None
        if obj.UseEntry:
            # Try to find an entry location
            toold = self.radius*2
            helixBounds = DraftGeomUtils.pocket2d(shape, self.radius * (1 + obj.HelixSize))

            if helixBounds:
                rampD = obj.RampSize

                if obj.StartAt == 'Edge':
                    plungePos = helixBounds[0].Edges[0].Vertexes[0].Point
                else:
                    plungePos = offsets[0].Edges[0].Vertexes[0].Point

                    # If it turns out this is invalid for some reason, nuke plungePos
                    [perp, idx] = DraftGeomUtils.findPerpendicular(plungePos, shape.Edges)
                    if not perp or perp.Length < self.radius * (1 + obj.HelixSize):
                        plungePos = None
                        FreeCAD.Console.PrintError(translate("PathPocket", "Helical Entry location not found.\n"))
                    # FIXME: Really need to do a point-in-polygon operation to make sure this is within helixBounds
                    # Or some math to prove that it has to be (doubt that's true)
                    # Maybe reverse helixBounds and pick off that?

            if plungePos is None:  # If we didn't find a place to helix, how about a ramp?
                FreeCAD.Console.PrintMessage(translate("PathPocket", "Attempting ramp entry.\n"))
                if (offsets[0].Edges[0].Length >= toold * rampD) and not (isinstance(offsets[0].Edges[0].Curve, Part.Circle)):
                    rampEdge = offsets[0].Edges[0]
                # The last edge also connects with the starting location- try that
                elif (offsets[0].Edges[-1].Length >= toold * rampD) and not (isinstance(offsets[0].Edges[-1].Curve, Part.Circle)):
                    rampEdge = offsets[0].Edges[-1]
                else:
                    FreeCAD.Console.PrintError(translate("PathPocket", "Ramp Entry location not found.\n"))
                    # print "Neither edge works: " + str(offsets[0].Edges[0]) + ", " + str(offsets[0].Edges[-1])
                    # FIXME: There's got to be a smarter way to find a place to ramp

        # For helix-ing/ramping, know where we were last time
        # FIXME: Can probably get this from the "machine"?
        lastZ = obj.ClearanceHeight.Value

        startPoint = None

        depthparams = depth_params(
                obj.ClearanceHeight.Value,
                obj.SafeHeight.Value,
                obj.StartDepth.Value,
                obj.StepDown,
                obj.FinishDepth.Value,
                obj.FinalDepth.Value)

        for vpos in depthparams.get_depths():

            first = True
            # loop over successive wires
            for currentWire in offsets:
                last = None
                for edge in currentWire.Edges:
                    if not last:
                        # we set the base GO to our fast move to our starting pos
                        if first:
                            # If we can helix, do so
                            if plungePos:
                                output += helicalPlunge(plungePos, obj.RampAngle, vpos, lastZ, self.radius*2, obj.HelixSize, self.horizFeed)
                                lastZ = vpos
                            # Otherwise, see if we can ramp
                            # FIXME: This could be a LOT smarter (eg, searching for a longer leg of the edge to ramp along)
                            elif rampEdge:
                                output += rampPlunge(rampEdge, obj.RampAngle, vpos, lastZ)
                                lastZ = vpos
                            # Otherwise, straight plunge... Don't want to, but sometimes you might not have a choice.
                            # FIXME: At least not with the lazy ramp programming above...
                            else:
                                print("WARNING: Straight-plunging... probably not good, but we didn't find a place to helix or ramp")
                                startPoint = edge.Vertexes[0].Point
                                output += "G0 Z" + fmt(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) + "\n"
                                output += "G0 X" + fmt(startPoint.x) + " Y" + fmt(startPoint.y) +\
                                          " Z" + fmt(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.horizRapid) + "\n"
                            first = False
                        # then move slow down to our starting point for our profile
                        last = edge.Vertexes[0].Point
                        output += "G1 X" + fmt(last.x) + " Y" + fmt(last.y) + " Z" + fmt(vpos) + " F" + fmt(self.vertFeed) + "\n"
                    if DraftGeomUtils.geomType(edge) == "Circle":
                        point = edge.Vertexes[-1].Point
                        if point == last:  # edges can come flipped
                            point = edge.Vertexes[0].Point
                        center = edge.Curve.Center
                        relcenter = center.sub(last)
                        v1 = last.sub(center)
                        v2 = point.sub(center)
                        if v1.cross(v2).z < 0:
                            output += "G2"
                        else:
                            output += "G3"
                        output += " X" + fmt(point.x) + " Y" + fmt(point.y) + " Z" + fmt(vpos)
                        output += " I" + fmt(relcenter.x) + " J" + fmt(relcenter.y) + " K" + fmt(relcenter.z) + " F" + fmt(self.horizFeed)
                        output += "\n"
                        last = point
                    else:
                        point = edge.Vertexes[-1].Point
                        if point == last:  # edges can come flipped
                            point = edge.Vertexes[0].Point
                        output += "G1 X" + fmt(point.x) + " Y" + fmt(point.y) + " Z" + fmt(vpos) + " F" + fmt(self.horizFeed) + "\n"
                        last = point

        # move back up
        output += "G0 Z" + fmt(obj.ClearanceHeight.Value) + "F " + PathUtils.fmt(self.vertRapid) + "\n"
        return output

    # To reload this from FreeCAD, use: import PathScripts.PathPocket; reload(PathScripts.PathPocket)
    def execute(self, obj):
        output = ""

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
            for b in obj.Base:
                for sub in b[1]:
                    import Part
                    import PathScripts.PathKurveUtils
                    if "Face" in sub:
                        shape = getattr(b[0].Shape, sub)
                        wire = shape.OuterWire
                        edges = wire.Edges
                    else:
                        edges = [getattr(b[0].Shape, sub) for sub in b[1]]
                        wire = Part.Wire(edges)
                        shape = None

                    # output = ""
                    if obj.Algorithm == "OCC Native":
                        if shape is None:
                            shape = wire
                        output += self.buildpathocc(obj, shape)
                    else:
                        try:
                            import area
                        except:
                            FreeCAD.Console.PrintError(translate("PathKurve", "libarea needs to be installed for this command to work.\n"))
                            return

                        a = area.Area()
                        if shape is None:
                            c = PathScripts.PathKurveUtils.makeAreaCurve(wire.Edges, 'CW')
                            a.append(c)
                        else:
                            for w in shape.Wires:
                                c = PathScripts.PathKurveUtils.makeAreaCurve(w.Edges, 'CW')
                                a.append(c)

                        a.Reorder()
                        output += self.buildpathlibarea(obj, a)

            if obj.Active:
                path = Path.Path(output)
                obj.Path = path
                obj.ViewObject.Visibility = True
            else:
                path = Path.Path("(inactive operation)")
                obj.Path = path
                obj.ViewObject.Visibility = False


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

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel()
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()
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

        zbottom = 0.0
        ztop = 10.0

        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction(translate("PathPocket", "Create Pocket"))
        FreeCADGui.addModule("PathScripts.PathPocket")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Pocket")')
        FreeCADGui.doCommand('PathScripts.PathPocket.ObjectPocket(obj)')
        FreeCADGui.doCommand('obj.Active = True')
        FreeCADGui.doCommand('PathScripts.PathPocket.ViewProviderPocket(obj.ViewObject)')
        FreeCADGui.doCommand('from PathScripts import PathUtils')
        FreeCADGui.doCommand('obj.Algorithm = "libarea"')
        FreeCADGui.doCommand('obj.StepOver = 100')
        FreeCADGui.doCommand('obj.ClearanceHeight = 10')  # + str(bb.ZMax + 2.0))
        FreeCADGui.doCommand('obj.StepDown = 1.0')
        FreeCADGui.doCommand('obj.StartDepth = ' + str(ztop))
        FreeCADGui.doCommand('obj.FinalDepth =' + str(zbottom))
        FreeCADGui.doCommand('obj.ZigZagAngle = 45')
        FreeCADGui.doCommand('obj.UseEntry = False')
        FreeCADGui.doCommand('obj.RampAngle = 3.0')
        FreeCADGui.doCommand('obj.RampSize = 0.75')
        FreeCADGui.doCommand('obj.HelixSize = 0.75')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('obj.ToolController = PathScripts.PathUtils.findToolController(obj)')
        FreeCAD.ActiveDocument.commitTransaction()

        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')


class TaskPanel:
    def __init__(self):
        # self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/PocketEdit.ui")
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/PocketEdit.ui")
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
                self.obj.StartDepth = FreeCAD.Units.Quantity(self.form.startDepth.text()).Value
            if hasattr(self.obj, "FinalDepth"):
                self.obj.FinalDepth = FreeCAD.Units.Quantity(self.form.finalDepth.text()).Value
            if hasattr(self.obj, "SafeHeight"):
                self.obj.SafeHeight = FreeCAD.Units.Quantity(self.form.safeHeight.text()).Value
            if hasattr(self.obj, "ClearanceHeight"):
                self.obj.ClearanceHeight = FreeCAD.Units.Quantity(self.form.clearanceHeight.text()).Value
            if hasattr(self.obj, "StepDown"):
                self.obj.StepDown = FreeCAD.Units.Quantity(self.form.stepDown.text()).Value
            if hasattr(self.obj, "MaterialAllowance"):
                self.obj.MaterialAllowance = FreeCAD.Units.Quantity(self.form.extraOffset.text()).Value
            if hasattr(self.obj, "UseStartPoint"):
                self.obj.UseStartPoint = self.form.useStartPoint.isChecked()
            if hasattr(self.obj, "Algorithm"):
                self.obj.Algorithm = str(self.form.algorithmSelect.currentText())
            if hasattr(self.obj, "CutMode"):
                self.obj.CutMode = str(self.form.cutMode.currentText())
            if hasattr(self.obj, "UseZigZag"):
                self.obj.UseZigZag = self.form.useZigZag.isChecked()
            if hasattr(self.obj, "ZigUnidirectional"):
                self.obj.ZigUnidirectional = self.form.zigZagUnidirectional.isChecked()
            if hasattr(self.obj, "ZigZagAngle"):
                self.obj.ZigZagAngle = FreeCAD.Units.Quantity(self.form.zigZagAngle.text()).Value
            if hasattr(self.obj, "StepOver"):
                self.obj.StepOver = self.form.stepOverPercent.value()
            if hasattr(self.obj, "ToolController"):
                PathLog.debug("name: {}".format(self.form.uiToolController.currentText()))
                tc = PathUtils.findToolController(self.obj, self.form.uiToolController.currentText())
                self.obj.ToolController = tc

        self.obj.Proxy.execute(self.obj)

    def setFields(self):
        self.form.startDepth.setText(FreeCAD.Units.Quantity(self.obj.StartDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.finalDepth.setText(FreeCAD.Units.Quantity(self.obj.FinalDepth.Value, FreeCAD.Units.Length).UserString)
        self.form.safeHeight.setText(FreeCAD.Units.Quantity(self.obj.SafeHeight.Value, FreeCAD.Units.Length).UserString)
        self.form.clearanceHeight.setText(FreeCAD.Units.Quantity(self.obj.ClearanceHeight.Value,  FreeCAD.Units.Length).UserString)
        self.form.stepDown.setText(FreeCAD.Units.Quantity(self.obj.StepDown.Value, FreeCAD.Units.Length).UserString)
        self.form.extraOffset.setText(FreeCAD.Units.Quantity(self.obj.MaterialAllowance, FreeCAD.Units.Length).UserString)
        self.form.useStartPoint.setChecked(self.obj.UseStartPoint)
        self.form.useZigZag.setChecked(self.obj.UseZigZag)
        self.form.zigZagUnidirectional.setChecked(self.obj.ZigUnidirectional)
        self.form.zigZagAngle.setText(FreeCAD.Units.Quantity(self.obj.ZigZagAngle, FreeCAD.Units.Angle).UserString)
        self.form.stepOverPercent.setValue(self.obj.StepOver)

        index = self.form.algorithmSelect.findText(self.obj.Algorithm, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.algorithmSelect.blockSignals(True)
            self.form.algorithmSelect.setCurrentIndex(index)
            self.form.algorithmSelect.blockSignals(False)

        index = self.form.cutMode.findText(
                self.obj.CutMode, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.cutMode.blockSignals(True)
            self.form.cutMode.setCurrentIndex(index)
            self.form.cutMode.blockSignals(False)

        # for i in self.obj.Base:
        #     self.form.baseList.addItem(i[0].Name + "." + i[1][0])
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

        # for i in self.obj.Base:
        #     self.form.baseList.addItem(i[0].Name + "." + i[1][0])

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
        self.form.algorithmSelect.currentIndexChanged.connect(self.getFields)
        self.form.cutMode.currentIndexChanged.connect(self.getFields)
        self.form.useStartPoint.clicked.connect(self.getFields)
        self.form.extraOffset.editingFinished.connect(self.getFields)

        # Pattern
        self.form.stepOverPercent.editingFinished.connect(self.getFields)
        self.form.useZigZag.clicked.connect(self.getFields)
        self.form.zigZagUnidirectional.clicked.connect(self.getFields)
        self.form.zigZagAngle.editingFinished.connect(self.getFields)

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
