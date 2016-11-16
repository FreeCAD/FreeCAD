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
from PySide import QtCore, QtGui
from PathScripts import PathUtils
import Part
import PathScripts.PathKurveUtils
import area
import TechDraw
from FreeCAD import Vector

FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui

"""Path Face object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectFace:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLinkSubList", "Base", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The base geometry of this object"))
        obj.addProperty("App::PropertyBool", "Active", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString", "Comment", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","An optional comment for this profile"))
        obj.addProperty("App::PropertyString", "UserLabel", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","User Assigned Label"))

        # Tool Properties
        obj.addProperty("App::PropertyIntegerConstraint", "ToolNumber", "Tool", QtCore.QT_TRANSLATE_NOOP("App::Property","The tool number in use"))
        obj.ToolNumber = (0, 0, 1000, 0)
        obj.setEditorMode('ToolNumber', 1)  # make this read only
        obj.addProperty("App::PropertyString", "ToolDescription", "Tool", QtCore.QT_TRANSLATE_NOOP("App::Property","The description of the tool "))
        obj.setEditorMode('ToolDescription', 1)  # make this read only

        # Depth Properties
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Rapid Safety Height between locations."))
        obj.addProperty("App::PropertyFloatConstraint", "StepDown", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Incremental Step Down of Tool"))
        obj.StepDown = (0.0, 0.01, 100.0, 0.5)
        obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyDistance", "FinishDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Maximum material removed on final pass."))

        # Face Properties
        obj.addProperty("App::PropertyEnumeration", "CutMode", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property","The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.CutMode = ['Climb', 'Conventional']
        obj.addProperty("App::PropertyDistance", "PassExtension", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property","How far the cutter should extend past the boundary"))
        obj.addProperty("App::PropertyEnumeration", "StartAt", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property","Start Faceing at center or boundary"))
        obj.StartAt = ['Center', 'Edge']
        obj.addProperty("App::PropertyPercent", "StepOver", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property","Percent of cutter diameter to step over on each pass"))
        #obj.StepOver = (1, 1, 100, 1)
        obj.addProperty("App::PropertyBool", "KeepToolDown", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property","Attempts to avoid unnecessary retractions."))
        obj.addProperty("App::PropertyBool", "ZigUnidirectional", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property","Lifts tool at the end of each pass to respect cut mode."))
        obj.addProperty("App::PropertyBool", "UseZigZag", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property","Use Zig Zag pattern to clear area."))
        obj.addProperty("App::PropertyFloat", "ZigZagAngle", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property","Angle of the zigzag pattern"))
        obj.addProperty("App::PropertyEnumeration", "BoundaryShape", "Face", QtCore.QT_TRANSLATE_NOOP("App::Property","Shape to use for calculating Boundary"))
        obj.BoundaryShape = ['Perimeter', 'Boundbox']


        # Start Point Properties
        obj.addProperty("App::PropertyVector", "StartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property","The start point of this path"))
        obj.addProperty("App::PropertyBool", "UseStartPoint", "Start Point", QtCore.QT_TRANSLATE_NOOP("App::Property","make True, if specifying a Start Point"))

        obj.Proxy = self

    def onChanged(self, obj, prop):

        if prop == "UserLabel":
            obj.Label = obj.UserLabel + " :" + obj.ToolDescription
        if prop == "StepOver":
            if obj.StepOver == 0:
                obj.StepOver = 1

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def _guessDepths(self, obj, ss, sub=""):
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


    def addFacebase(self, obj, ss, sub=""):
        baselist = obj.Base
        if baselist is None:
            baselist = []
        if len(baselist) == 0:  # When adding the first base object, guess at heights
            self._guessDepths(obj, ss, sub)

        item = (ss, sub)
        if item in baselist:
            FreeCAD.Console.PrintWarning(translate("Path", "this object already in the list" + "\n"))
        elif  PathUtils.findParentJob(obj).Base.Name != ss.Name:
            FreeCAD.Console.PrintWarning(translate("Path", "Please select features from the Job model object" +"\n"))
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

    def buildpathlibarea(self, obj, a):
        """Build the face path using libarea algorithm"""
        import PathScripts.PathAreaUtils as PathAreaUtils
        from PathScripts.PathUtils import depth_params

        FreeCAD.Console.PrintMessage(translate("PathFace", "Generating toolpath with libarea offsets.\n"))

        depthparams = depth_params(
                obj.ClearanceHeight.Value,
                obj.SafeHeight.Value,
                obj.StartDepth.Value,
                obj.StepDown,
                obj.FinishDepth.Value,
                obj.FinalDepth.Value)

        extraoffset = - obj.PassExtension.Value
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

    # To reload this from FreeCAD, use: import PathScripts.PathFace; reload(PathScripts.PathFace)
    def execute(self, obj):

        if not obj.Active:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            obj.ViewObject.Visibility = False
            return

        #Tool may have changed.  Refresh data
        toolLoad = PathUtils.getLastToolLoad(obj)
        if toolLoad is None or toolLoad.ToolNumber == 0:
            self.vertFeed = 100
            self.horizFeed = 100
            self.vertRapid = 100
            self.horiRrapid = 100
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

        #Build preliminary comments
        output = ""
        output += "(" + obj.Label + ")"

        if obj.UserLabel == "":
            obj.Label = obj.Name + " :" + obj.ToolDescription
        else:
            obj.Label = obj.UserLabel + " :" + obj.ToolDescription

        #Facing is done either against base objects
        if obj.Base:
            faces = []
            for b in obj.Base:
                for sub in b[1]:
                    shape = getattr(b[0].Shape, sub)
                    if isinstance (shape, Part.Face):
                        faces.append(shape)
                    else:
                        print ('falling out')
                        return
            planeshape = Part.makeCompound(faces)

        #If no base object, do planing of top surface of entire model
        else:
            parentJob = PathUtils.findParentJob(obj)
            if parentJob is None:
                return
            baseobject = parentJob.Base
            if baseobject is None:
                return
            planeshape = baseobject.Shape

        #if user wants the boundbox, calculate that
        if obj.BoundaryShape == 'Boundbox':
            bb = planeshape.BoundBox
            bbperim = Part.makeBox(bb.XLength, bb.YLength, 1, Vector(bb.XMin, bb.YMin, bb.ZMin), Vector(0,0,1))
            contourwire = TechDraw.findShapeOutline(bbperim, 1, Vector(0,0,1))
        else:
            contourwire = TechDraw.findShapeOutline(planeshape, 1, Vector(0,0,1))

        edgelist = contourwire.Edges
        edgelist = Part.__sortEdges__(edgelist)

        #use libarea to build the pattern
        a = area.Area()
        c = PathScripts.PathKurveUtils.makeAreaCurve(edgelist, 'CW')
        a.append(c)
        a.Reorder()
        output += self.buildpathlibarea(obj, a)

        path = Path.Path(output)
        obj.Path = path
        obj.ViewObject.Visibility = True


class _CommandSetFaceStartPoint:
    def GetResources(self):
        return {'Pixmap': 'Path-StartPoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathFace", "Pick Start Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathFace", "Pick Start Point")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def setpoint(self, point, o):
        obj = FreeCADGui.Selection.getSelection()[0]
        obj.StartPoint.x = point.x
        obj.StartPoint.y = point.y

    def Activated(self):
        FreeCADGui.Snapper.getPoint(callback=self.setpoint)


class ViewProviderFace:

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel()
        taskd.obj = vobj.Object
        taskd.setupUi()
        FreeCADGui.Control.showDialog(taskd)
        return True

    def getIcon(self):
        return ":/icons/Path-Face.svg"

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class CommandPathMillFace:

    def GetResources(self):
        return {'Pixmap': 'Path-Face',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathFace", "Face"),
                'Accel': "P, O",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathFace", "Create a Facing Operation from a model or face")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):

       # zbottom = 0.0
        ztop = 10.0

        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction(translate("PathFace", "Create Face"))
        FreeCADGui.addModule("PathScripts.PathMillFace")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Face")')
        FreeCADGui.doCommand('PathScripts.PathMillFace.ObjectFace(obj)')
        FreeCADGui.doCommand('obj.Active = True')
        FreeCADGui.doCommand('PathScripts.PathMillFace.ViewProviderFace(obj.ViewObject)')
        FreeCADGui.doCommand('from PathScripts import PathUtils')
        FreeCADGui.doCommand('obj.StepOver = 50')
        FreeCADGui.doCommand('obj.ClearanceHeight = 10')  # + str(bb.ZMax + 2.0))
        FreeCADGui.doCommand('obj.StepDown = 1.0')
        FreeCADGui.doCommand('obj.StartDepth = ' + str(ztop + 1))
        FreeCADGui.doCommand('obj.FinalDepth =' + str(ztop))
        FreeCADGui.doCommand('obj.ZigZagAngle = 0.0')
        FreeCADGui.doCommand('obj.UseZigZag = True')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        snippet = '''
parentJob = PathUtils.findParentJob(obj)
if parentJob is None:
    pass
else:
    baseobject = parentJob.Base
    if baseobject is None:
        pass
    else:
        obj.StartDepth = str(baseobject.Shape.BoundBox.ZMax + 1)
        obj.FinalDepth = str(baseobject.Shape.BoundBox.ZMax)
'''
        FreeCADGui.doCommand(snippet)

        FreeCAD.ActiveDocument.commitTransaction()

        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')


class TaskPanel:
    def __init__(self):
        #self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/MillFaceEdit.ui")
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/MillFaceEdit.ui")
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
            if hasattr(self.obj, "FinishDepth"):
                self.obj.FinishDepth = self.form.finishDepth.text()
            if hasattr(self.obj, "SafeHeight"):
                self.obj.SafeHeight = self.form.safeHeight.text()
            if hasattr(self.obj, "ClearanceHeight"):
                self.obj.ClearanceHeight = self.form.clearanceHeight.text()
            if hasattr(self.obj, "StepDown"):
                self.obj.StepDown = self.form.stepDown.value()
            if hasattr(self.obj, "PassExtension"):
                self.obj.PassExtension = self.form.extraOffset.value()
            # if hasattr(self.obj, "UseStartPoint"):
            #     self.obj.UseStartPoint = self.form.useStartPoint.isChecked()
            if hasattr(self.obj, "CutMode"):
                self.obj.CutMode = str(self.form.cutMode.currentText())
            if hasattr(self.obj, "UseZigZag"):
                self.obj.UseZigZag = self.form.useZigZag.isChecked()
            if hasattr(self.obj, "ZigUnidirectional"):
                self.obj.ZigUnidirectional = self.form.zigZagUnidirectional.isChecked()
            if hasattr(self.obj, "ZigZagAngle"):
                self.obj.ZigZagAngle = self.form.zigZagAngle.value()
            if hasattr(self.obj, "StepOver"):
                self.obj.StepOver = self.form.stepOverPercent.value()
            if hasattr(self.obj, "BoundaryShape"):
                self.obj.BoundaryShape = str(self.form.boundaryShape.currentText())

        self.obj.Proxy.execute(self.obj)

    def setFields(self):
        self.form.startDepth.setText(str(self.obj.StartDepth.Value))
        self.form.finalDepth.setText(str(self.obj.FinalDepth.Value))
        self.form.finishDepth.setText(str(self.obj.FinishDepth.Value))
        self.form.stepDown.setValue(self.obj.StepDown)
        self.form.safeHeight.setText(str(self.obj.SafeHeight.Value))
        self.form.clearanceHeight.setText(str(self.obj.ClearanceHeight.Value))
        self.form.stepOverPercent.setValue(self.obj.StepOver)
        self.form.useZigZag.setChecked(self.obj.UseZigZag)
        self.form.zigZagUnidirectional.setChecked(self.obj.ZigUnidirectional)
        self.form.zigZagAngle.setValue(self.obj.ZigZagAngle)
        #self.form.useStartPoint.setChecked(self.obj.UseStartPoint)
        self.form.extraOffset.setValue(self.obj.PassExtension.Value)

        index = self.form.cutMode.findText(
                self.obj.CutMode, QtCore.Qt.MatchFixedString)
        if index >=0:

            self.form.cutMode.blockSignals(True)
            self.form.cutMode.setCurrentIndex(index)
            self.form.cutMode.blockSignals(False)

        index = self.form.boundaryShape.findText(
                self.obj.BoundaryShape, QtCore.Qt.MatchFixedString)
        if index >=0:
            self.form.boundaryShape.blockSignals(True)
            self.form.boundaryShape.setCurrentIndex(index)
            self.form.boundaryShape.blockSignals(False)


        for i in self.obj.Base:
            for sub in i[1]:
                self.form.baseList.addItem(i[0].Name + "." + sub)

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
            self.obj.Proxy.addFacebase(self.obj, sel.Object, i)

        self.setFields()  # defaults may have changed.  Reload.
        self.form.baseList.clear()

        for i in self.obj.Base:
            for sub in i[1]:
                self.form.baseList.addItem(i[0].Name + "." + sub)

    def deleteBase(self):
        dlist = self.form.baseList.selectedItems()
        newlist = []
        for d in dlist:
            deletebase = d.text().partition(".")[0]
            deletesub = d.text().partition(".")[2]

            for i in self.obj.Base:
                sublist = []
                #baseobj = i[0]
                basesubs = i[1]
                for sub in basesubs:
                    if sub != deletesub:
                        sublist.append(sub)
                if len(sublist) >= 1:
                    newlist.append ((deletebase, tuple(sublist)))

                if i[0].Name != d.text().partition(".")[0] and d.text().partition(".")[2] not in i[1]:
                    newlist.append(i)
            self.form.baseList.takeItem(self.form.baseList.row(d))
        self.obj.Base = newlist
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def itemActivated(self):
        print self.form.baseList.selectedItems()[0].text()
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

        self.obj.touch()
        FreeCAD.ActiveDocument.recompute()

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
        self.form.finishDepth.editingFinished.connect(self.getFields)
        self.form.stepDown.editingFinished.connect(self.getFields)

        # Heights
        self.form.safeHeight.editingFinished.connect(self.getFields)
        self.form.clearanceHeight.editingFinished.connect(self.getFields)

        # operation
        self.form.cutMode.currentIndexChanged.connect(self.getFields)
        #self.form.useStartPoint.clicked.connect(self.getFields)
        self.form.extraOffset.editingFinished.connect(self.getFields)
        self.form.boundaryShape.currentIndexChanged.connect(self.getFields)
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
    FreeCADGui.addCommand('Path_MillFace', CommandPathMillFace())
    FreeCADGui.addCommand('Set_FaceStartPoint', _CommandSetFaceStartPoint())


FreeCAD.Console.PrintLog("Loading PathMillFace... done\n")
