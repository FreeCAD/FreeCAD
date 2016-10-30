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
import FreeCADGui
import Path
from PathScripts import PathUtils
from PySide import QtCore, QtGui
import math
import Part
import DraftGeomUtils

"""Holding Tabs Dressup object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)

except AttributeError:

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

debugDressup = True

def debugMarker(vector, label, color = None, radius = 0.5):
    if debugDressup:
        obj = FreeCAD.ActiveDocument.addObject("Part::Sphere", label)
        obj.Label = label
        obj.Radius = radius
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        if color:
            obj.ViewObject.ShapeColor = color

movecommands = ['G0', 'G00', 'G1', 'G01', 'G2', 'G02', 'G3', 'G03']
movestraight = ['G1', 'G01']
movecw =       ['G2', 'G02']
moveccw =      ['G3', 'G03']
movearc = movecw + moveccw

def getAngle(v):
    a = v.getAngle(FreeCAD.Vector(1,0,0))
    if v.x < 0 and v.y < 0:
        return a - math.pi/2
    if v.y < 0:
        return -a
    return a

class PathData:
    def __init__(self, obj):
        self.obj = obj
        self.edges = []
        lastPt = FreeCAD.Vector(0, 0, 0)
        for cmd in obj.Base.Path.Commands:
            if cmd.Name in movecommands:
                pt = self.point(cmd, lastPt)
                if cmd.Name in movestraight:
                    self.edges.append(Part.Edge(Part.Line(lastPt, pt)))
                elif cmd.Name in movearc:
                    center = lastPt + self.point(cmd, FreeCAD.Vector(0,0,0), 'I', 'J', 'K')
                    A = lastPt - center
                    B = pt - center
                    a = getAngle(A)
                    b = getAngle(B)
                    if cmd.Name in movecw and a < 0 and math.fabs(math.pi - b) < 0.0000001:
                        angle = (a - b) / 2
                    elif cmd.Name in moveccw and math.fabs(math.pi - a) < 0.0000001 and b < 0:
                        angle = (b - a) / 2
                    else:
                        angle = (a + b) / 2.
                    d = -B.x * A.y + B.y * A.x

                    R = (lastPt - center).Length
                    ptm = center + FreeCAD.Vector(math.cos(angle), math.sin(angle), 0) * R
                    #if pt.z == 2.: # and pt.x == 10. and math.fabs(pt.y) == 12.5:
                    #    #print("%s (%.2f %.2f) -> (%.2f, %.2f): center=(%.2f, %.2f)" % (cmd, lastPt.x, lastPt.y, pt.x, pt.y, center.x, center.y))
                    #    print("   a = %+.2f b = %+.2f) %+.2f -> angle = %+.2f   r = %.2f" % (a/math.pi, b/math.pi, d, angle/math.pi, R))
                    #    debugMarker(lastPt, 'arc', (1.0, 1.0, 0.0), 0.3)
                    #    debugMarker(ptm,    'arc', (1.0, 1.0, 0.0), 0.3)
                    #    debugMarker(pt,     'arc', (1.0, 1.0, 0.0), 0.3)
                    self.edges.append(Part.Edge(Part.Arc(lastPt, ptm, pt)))
                lastPt = pt
        self.base = self.findBottomWire(self.edges)
        # determine overall length
        self.length = self.base.Length

    def point(self, cmd, pt, X='X', Y='Y', Z='Z'):
        x = cmd.Parameters.get(X, pt.x)
        y = cmd.Parameters.get(Y, pt.y)
        z = cmd.Parameters.get(Z, pt.z)
        return FreeCAD.Vector(x, y, z)

    def findBottomWire(self, edges):
        (minZ, maxZ) = self.findZLimits(edges)
        self.minZ = minZ
        self.maxZ = maxZ
        bottom = [e for e in edges if e.Vertexes[0].Point.z == minZ and e.Vertexes[1].Point.z == minZ]
        wire = Part.Wire(bottom)
        if wire.isClosed():
            return wire
        # if we get here there are already holding tabs, or we're not looking at a profile
        # let's try and insert the missing pieces - another day
        raise ValueError("Selected path doesn't seem to be a Profile operation.")


    def findZLimits(self, edges):
        # not considering arcs and spheres in Z direction, find the highes and lowest Z values
        minZ = edges[0].Vertexes[0].Point.z
        maxZ = minZ
        for e in edges:
            for v in e.Vertexes:
                if v.Point.z < minZ:
                    minZ = v.Point.z
                if v.Point.z > maxZ:
                    maxZ = v.Point.z
        return (minZ, maxZ)

class ObjectDressup:

    def __init__(self, obj):
        self.obj = obj
        obj.addProperty("App::PropertyLink", "Base","Base", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTabs", "The base path to modify"))
        obj.addProperty("App::PropertyInteger", "Count", "Tab", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTabs", "The number of holding tabs to be generated"))
        obj.addProperty("App::PropertyFloat", "Height", "Tab", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTabs", "Height of holding tabs measured from bottom of path"))
        obj.addProperty("App::PropertyFloat", "Width", "Tab", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTabs", "Width of each tab at its base"))
        obj.addProperty("App::PropertyFloat", "Angle", "Tab", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTabs", "Angle of plunge used for tabs"))
        obj.addProperty("App::PropertyIntegerList", "Blacklist", "Tab", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTabs", "IDs of disabled paths"))
        obj.setEditorMode("Blacklist", 2)
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def getFingerprint(self, obj):
        if hasattr(self, 'pathData'):
            return "%d-%.2f-%.2f-%.2f-%s" % (obj.Count, obj.Height, obj.Width, obj.Angle, str(obj.Blacklist))
        return ''

    def execute(self, obj):
        if not obj.Base:
            return
        if not obj.Base.isDerivedFrom("Path::Feature"):
            return
        if not obj.Base.Path:
            return
        if not obj.Base.Path.Commands:
            return

        pathData = self.setup(obj)
        if not pathData:
            return

        if hasattr(self, 'fingerprint') and self.fingerprint and self.fingerprint == self.getFingerprint(obj):
            return

        self.fingerprint = self.getFingerprint(obj)

        #for e in pathData.base.Edges:
        #    debugMarker(e.Vertexes[0].Point, 'base', (0.0, 1.0, 1.0), 0.2)

        if obj.Count == 0:
            obj.Path = obj.Base.Path
            return

        tabDistance = pathData.base.Length / obj.Count

        # start assigning tabs on the longest segment
        maxLen = sorted(pathData.base.Edges, key=lambda e: -e.Length)[0].Length
        startIndex = 0
        for i in range(0, len(pathData.base.Edges)):
            edge = pathData.base.Edges[i]
            if edge.Length == maxLen:
                startIndex = i
                break

        startEdge = pathData.base.Edges[startIndex]
        startCount = int(startEdge.Length / tabDistance) + 1

        lastTabLength = (startEdge.Length + (startCount - 1) * tabDistance) / 2
        if lastTabLength < 0 or lastTabLength > startEdge.Length:
            lastTabLength = startEdge.Length / 2
        currentLength = startEdge.Length
        minLength = 2 * obj.Width

        print("start index=%-2d -> count=%d (length=%.2f, distance=%.2f)" % (startIndex, startCount, startEdge.Length, tabDistance))
        print("               -> lastTabLength=%.2f)" % lastTabLength)
        print("               -> currentLength=%.2f)" % currentLength)

        tabs = { startIndex: startCount }

        for i in range(startIndex + 1, len(pathData.base.Edges)):
            edge = pathData.base.Edges[i]
            (currentLength, lastTabLength) = self.executeTabLength(i, edge, currentLength, lastTabLength, tabDistance, minLength, tabs)
        for i in range(0, startIndex):
            edge = pathData.base.Edges[i]
            (currentLength, lastTabLength) = self.executeTabLength(i, edge, currentLength, lastTabLength, tabDistance, minLength, tabs)

        self.tabs = tabs
        locs = {}

        tabID = 0
        for (i, count) in tabs.iteritems():
            edge = pathData.base.Edges[i]
            #debugMarker(edge.Vertexes[0].Point, 'base', (1.0, 0.0, 0.0), 0.2)
            #debugMarker(edge.Vertexes[1].Point, 'base', (0.0, 1.0, 0.0), 0.2)
            distance = (edge.LastParameter - edge.FirstParameter) / count
            for j in range(0, count):
                tab = edge.Curve.value((j+0.5) * distance)
                tabID += 1
                locs[(tab.x, tab.y)] = tabID
                if not tabID in obj.Blacklist:
                    debugMarker(tab, "tab-%02d" % tabID , (1.0, 0.0, 1.0), 0.5)

        self.tabLocations = locs
        #debugMarker(pathData.base.CenterOfMass, 'cog', (0., 0., 0.), 0.5)

        obj.Path = obj.Base.Path

    def executeTabLength(self, index, edge, currentLength, lastTabLength, tabDistance, minLength, tabs):
        tabCount = 0
        currentLength += edge.Length
        if edge.Length > minLength:
            while lastTabLength + tabDistance < currentLength:
                tabCount += 1
                lastTabLength += tabDistance
            if tabCount > 0:
                #print("      index=%d -> count=%d" % (index, tabCount))
                tabs[index] = tabCount
        else:
            print("      skipping=%-2d (%.2f)" % (index, edge.Length))
        return (currentLength, lastTabLength)

    def holdingTabsLocations(self, obj):
        if hasattr(self, "tabLocations") and self.tabLocations:
            return self.tabLocations
        return {}

    def setup(self, obj):
        if not hasattr(self, "pathData") or not self.pathData:
            try:
                pathData = PathData(obj)
            except ValueError:
                FreeCAD.Console.PrintError(translate("PathDressup_HoldingTabs", "Cannot insert holding tabs for this path - please select a Profile path\n"))
                return None

            # setup the object's properties, in case they're not set yet
            obj.Count = self.tabCount(obj)
            obj.Angle = self.tabAngle(obj)
            obj.Blacklist = self.tabBlacklist(obj)

            # if the heigt isn't set, use the height of the path
            if not hasattr(obj, "Height") or not obj.Height:
                obj.Height = pathData.maxZ - pathData.minZ
            # try and take an educated guess at the width
            if not hasattr(obj, "Width") or not obj.Width:
                width = sorted(pathData.base.Edges, key=lambda e: -e.Length)[0].Length / 10
                while obj.Count > len([e for e in pathData.base.Edges if e.Length > 3*width]):
                    width = widht / 2
                obj.Width = width

            # and the tool radius, not sure yet if it's needed
            self.toolRadius = 5
            toolLoad = PathUtils.getLastToolLoad(obj)
            if toolLoad is None or toolLoad.ToolNumber == 0:
                self.toolRadius = 5
            else:
                tool = PathUtils.getTool(obj, toolLoad.ToolNumber)
                if not tool or tool.Diameter == 0:
                    self.toolRadius = 5
                else:
                    self.toolRadius = tool.Diameter / 2
            self.pathData = pathData
        return self.pathData

    def tabCount(self, obj):
        if hasattr(obj, "Count") and obj.Count:
            return obj.Count
        return 4

    def tabHeight(self, obj):
        if hasattr(obj, "Height") and obj.Height:
            return obj.Height
        return 1

    def tabWidth(self, obj):
        if hasattr(obj, "Width") and obj.Width:
            return obj.Width
        return 3

    def tabAngle(self, obj):
        if hasattr(obj, "Angle") and obj.Angle:
            return obj.Angle
        return 90

    def tabBlacklist(self, obj):
        if hasattr(obj, "Blacklist") and obj.Blacklist:
            return obj.Blacklist
        return []

class TaskPanel:
    DataId = QtCore.Qt.ItemDataRole.UserRole

    def __init__(self, obj):
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/HoldingTabsEdit.ui")
        FreeCAD.ActiveDocument.openTransaction(translate("PathDressup_HoldingTabs", "Edit HoldingTabs Dress-up"))

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)

    def accept(self):
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)
        FreeCAD.ActiveDocument.recompute()

    def open(self):
        self.s = SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)

    def updateTabList(self):
        blacklist = self.obj.Proxy.tabBlacklist(self.obj)
        itemList = []
        for (x,y), id in self.obj.Proxy.holdingTabsLocations(self.obj).iteritems():
            label = "%d: (x=%.2f, y=%.2f)" % (id, x, y)
            item = QtGui.QListWidgetItem(label)
            if id in blacklist:
                item.setCheckState(QtCore.Qt.CheckState.Unchecked)
            else:
                item.setCheckState(QtCore.Qt.CheckState.Checked)
            item.setFlags(QtCore.Qt.ItemFlag.ItemIsSelectable | QtCore.Qt.ItemFlag.ItemIsEnabled | QtCore.Qt.ItemFlag.ItemIsUserCheckable)
            item.setData(self.DataId, id)
            itemList.append(item)
        self.form.lwHoldingTabs.clear()
        for item in sorted(itemList, key=lambda item: item.data(self.DataId)):
            self.form.lwHoldingTabs.addItem(item)

    def getFields(self):
        self.obj.Count = self.form.sbCount.value()
        self.obj.Height = self.form.dsbHeight.value()
        self.obj.Width = self.form.dsbWidth.value()
        self.obj.Angle = self.form.dsbAngle.value()
        blacklist = []
        for i in range(0, self.form.lwHoldingTabs.count()):
            item = self.form.lwHoldingTabs.item(i)
            if item.checkState() == QtCore.Qt.CheckState.Unchecked:
                blacklist.append(item.data(self.DataId))
        self.obj.Blacklist = sorted(blacklist)
        self.obj.Proxy.execute(self.obj)

    def update(self):
        if True or debugDressup:
            for obj in FreeCAD.ActiveDocument.Objects:
                if obj.Name.startswith('tab'):
                    FreeCAD.ActiveDocument.removeObject(obj.Name)
        self.getFields()
        self.updateTabList()
        FreeCAD.ActiveDocument.recompute()

    def setupSpinBox(self, widget, val, decimals = 2):
        widget.setMinimum(0)
        if decimals:
            widget.setDecimals(decimals)
        widget.setValue(val)


    def setFields(self):
        self.setupSpinBox(self.form.sbCount, self.obj.Proxy.tabCount(self.obj), None)
        self.setupSpinBox(self.form.dsbHeight, self.obj.Proxy.tabHeight(self.obj))
        self.setupSpinBox(self.form.dsbWidth, self.obj.Proxy.tabWidth(self.obj))
        self.setupSpinBox(self.form.dsbAngle, self.obj.Proxy.tabAngle(self.obj))
        self.updateTabList()

    def setupUi(self):
        self.setFields()
        self.form.sbCount.valueChanged.connect(self.update)
        self.form.dsbHeight.valueChanged.connect(self.update)
        self.form.dsbWidth.valueChanged.connect(self.update)
        self.form.dsbAngle.valueChanged.connect(self.update)
        self.form.lwHoldingTabs.itemChanged.connect(self.update)

class SelObserver:
    def __init__(self):
        import PathScripts.PathSelection as PST
        PST.eselect()

    def __del__(self):
        import PathScripts.PathSelection as PST
        PST.clear()

    def addSelection(self, doc, obj, sub, pnt):
        FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj + ')')
        FreeCADGui.updateGui()

class ViewProviderDressup:

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def claimChildren(self):
        for i in self.Object.Base.InList:
            if hasattr(i, "Group"):
                group = i.Group
                for g in group:
                    if g.Name == self.Object.Base.Name:
                        group.remove(g)
                i.Group = group
                print i.Group
        #FreeCADGui.ActiveDocument.getObject(obj.Base.Name).Visibility = False
        return [self.Object.Base]

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        panel = TaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(panel)
        panel.setupUi()
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onDelete(self, arg1=None, arg2=None):
        '''this makes sure that the base operation is added back to the project and visible'''
        FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
        PathUtils.addToJob(arg1.Object.Base)
        return True

class CommandPathDressupHoldingTabs:

    def GetResources(self):
        return {'Pixmap': 'Path-Dressup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTabs", "HoldingTabs Dress-up"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTabs", "Creates a HoldingTabs Dress-up object from a selected path")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate("PathDressup_HoldingTabs", "Please select one path object\n"))
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            FreeCAD.Console.PrintError(translate("PathDressup_HoldingTabs", "The selected object is not a path\n"))
            return
        if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
            FreeCAD.Console.PrintError(translate("PathDressup_HoldingTabs", "Please select a Profile object"))
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate("PathDressup_HoldingTabs", "Create HoldingTabs Dress-up"))
        FreeCADGui.addModule("PathScripts.PathDressupHoldingTabs")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "HoldingTabsDressup")')
        FreeCADGui.doCommand('dbo = PathScripts.PathDressupHoldingTabs.ObjectDressup(obj)')
        FreeCADGui.doCommand('obj.Base = FreeCAD.ActiveDocument.' + selection[0].Name)
        FreeCADGui.doCommand('PathScripts.PathDressupHoldingTabs.ViewProviderDressup(obj.ViewObject)')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('Gui.ActiveDocument.getObject(obj.Base.Name).Visibility = False')
        FreeCADGui.doCommand('dbo.setup(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('PathDressup_HoldingTabs', CommandPathDressupHoldingTabs())

FreeCAD.Console.PrintLog("Loading PathDressupHoldingTabs... done\n")
