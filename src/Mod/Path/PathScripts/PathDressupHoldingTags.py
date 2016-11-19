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

"""Holding Tags Dressup object and FreeCAD command"""

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
    if v.y < 0:
        return -a
    return a

class Side:
    Left  = +1
    Right = -1
    Straight = 0
    On = 0

    @classmethod
    def toString(cls, side):
        if side == cls.Left:
            return 'Left'
        if side == cls.Right:
            return 'Right'
        return 'On'

    @classmethod
    def of(cls, ptRef, pt):
        d = -ptRef.x*pt.y + ptRef.y*pt.x
        if d < 0:
            return cls.Left
        if d > 0:
            return cls.Right
        return cls.Straight

def testPrintAngle(v):
    print("(%+.2f, %+.2f, %+.2f): %+.2f" % (v.x, v.y, v.z, getAngle(v)/math.pi))

def testAngle(x=1, y=1):
    testPrintAngle(FreeCAD.Vector( 1*x, 0*y, 0))
    testPrintAngle(FreeCAD.Vector( 1*x, 1*y, 0))
    testPrintAngle(FreeCAD.Vector( 0*x, 1*y, 0))
    testPrintAngle(FreeCAD.Vector(-1*x, 1*y, 0))
    testPrintAngle(FreeCAD.Vector(-1*x, 0*y, 0))
    testPrintAngle(FreeCAD.Vector(-1*x,-1*y, 0))
    testPrintAngle(FreeCAD.Vector( 0*x,-1*y, 0))
    testPrintAngle(FreeCAD.Vector( 1*x,-1*y, 0))


def testPrintSide(pt1, pt2):
    print('(%.2f, %.2f) - (%.2f, %.2f) -> %s' % (pt1.x, pt1.y, pt2.x, pt2.y, Side.toString(Side.of(pt1, pt2))))

def testSide():
    testPrintSide(FreeCAD.Vector( 1, 0, 0), FreeCAD.Vector( 1, 0, 0))
    testPrintSide(FreeCAD.Vector( 1, 0, 0), FreeCAD.Vector(-1, 0, 0))
    testPrintSide(FreeCAD.Vector( 1, 0, 0), FreeCAD.Vector( 0, 1, 0))
    testPrintSide(FreeCAD.Vector( 1, 0, 0), FreeCAD.Vector( 0,-1, 0))

def pathCommandForEdge(edge):
    pt = edge.valueAt(edge.LastParameter)
    params = {'X': pt.x, 'Y': pt.y, 'Z': pt.z}
    if type(edge.Curve) == Part.Line:
        return Part.Command('G1', params)

    p1 = edge.valueAt(edge.FirstParameter)
    p2 = edge.valueAt((edge.FirstParameter + edge.LastParameter)/2)
    p3 = pt
    if Side.Left == Side.of(p2 - p1, p3 - p2):
        cmd = 'G3'
    else:
        cmd = 'G2'
    offset = pt1 - edge.Curve.Center
    params.update({'I': offset.x, 'J': offset.y, 'K': offset.z})
    return Part.Command(cmd, params)


class Tag:
    def __init__(self, x, y, width, height, angle, enabled=True):
        self.x = x
        self.y = y
        self.width = math.fabs(width)
        self.height = math.fabs(height)
        self.angle = math.fabs(angle)
        self.enabled = enabled

    def toString(self):
        return str((self.x, self.y, self.width, self.height, self.angle, self.enabled))

    def originAt(self, z):
        return FreeCAD.Vector(self.x, self.y, z)

    def createSolidsAt(self, z):
        r1 = self.width / 2
        height = self.height
        if self.angle == 90 and height > 0:
            self.solid = Part.makeCylinder(r1, height)
            self.core  = self.solid.copy()
        elif self.angle > 0.0 and height > 0.0:
            tangens = math.tan(math.radians(self.angle))
            dr = height / tangens
            if dr < r1:
                r2 = r1 - dr
                self.core = Part.makeCylinder(r2, height)
            else:
                r2 = 0
                height = r1 * tangens
                self.core = None
            self.solid = Part.makeCone(r1, r2, height)
        else:
            # degenerated case - no tag
            self.solid = Part.makeSphere(r1 / 10000)
            self.core = None
        self.solid.translate(self.originAt(z))
        if self.core:
            self.core.translate(self.originAt(z))

    @classmethod
    def FromString(cls, string):
        try:
            t = eval(string)
            return Tag(t[0], t[1], t[2], t[3], t[4], t[5])
        except:
            return None


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
                    d = -B.x * A.y + B.y * A.x

                    if d == 0:
                        # we're dealing with half an circle here
                        angle = getAngle(A) + math.pi/2
                        if cmd.Name in movecw:
                            angle -= math.pi
                    else:
                        #print("(%.2f, %.2f) (%.2f, %.2f)" % (A.x, A.y, B.x, B.y))
                        C = A + B
                        angle = getAngle(C)

                    R = (lastPt - center).Length
                    ptm = center + FreeCAD.Vector(math.cos(angle), math.sin(angle), 0) * R

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
            return Part.Wire(self.sortedBase(bottom))
        # if we get here there are already holding tags, or we're not looking at a profile
        # let's try and insert the missing pieces - another day
        raise ValueError("Selected path doesn't seem to be a Profile operation.")

    def sortedBase(self, base):
        # first find the exit point, where base wire is closed
        edges = [e for e in self.edges if e.valueAt(e.FirstParameter).z == self.minZ and e.valueAt(e.LastParameter).z != self.maxZ]
        exit = sorted(edges, key=lambda e: -e.valueAt(e.LastParameter).z)[0]
        pt = exit.valueAt(exit.FirstParameter)
        # then find the first base edge, and sort them until done
        ordered = []
        while base:
            edge = [e for e in base if e.valueAt(e.FirstParameter) == pt][0]
            ordered.append(edge)
            base.remove(edge)
            pt = edge.valueAt(edge.LastParameter)
        return ordered


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

    def shortestAndLongestPathEdge(self):
        edges = sorted(self.base.Edges, key=lambda e: e.Length)
        return (edges[0], edges[-1])

    def generateTags(self, obj, count=None, width=None, height=None, angle=90, spacing=None):
        #print("generateTags(%s, %s, %s, %s, %s)" % (count, width, height, angle, spacing))
        #for e in self.base.Edges:
        #    debugMarker(e.Vertexes[0].Point, 'base', (0.0, 1.0, 1.0), 0.2)

        if spacing:
            tagDistance = spacing
        else:
            if count:
                tagDistance = self.base.Length / count
            else:
                tagDistance = self.base.Length / 4
        if width:
            W = width
        else:
            W = self.tagWidth()
        if height:
            H = height
        else:
            H = self.tagHeight()


        # start assigning tags on the longest segment
        (shortestEdge, longestEdge) = self.shortestAndLongestPathEdge()
        startIndex = 0
        for i in range(0, len(self.base.Edges)):
            edge = self.base.Edges[i]
            if edge.Length == longestEdge.Length:
                startIndex = i
                break

        startEdge = self.base.Edges[startIndex]
        startCount = int(startEdge.Length / tagDistance)
        if (longestEdge.Length - shortestEdge.Length) > shortestEdge.Length:
            startCount = int(startEdge.Length / tagDistance) + 1

        lastTagLength = (startEdge.Length + (startCount - 1) * tagDistance) / 2
        currentLength = startEdge.Length

        minLength = min(2. * W, longestEdge.Length)

        #print("length=%.2f shortestEdge=%.2f(%.2f) longestEdge=%.2f(%.2f)" % (self.base.Length, shortestEdge.Length, shortestEdge.Length/self.base.Length, longestEdge.Length, longestEdge.Length / self.base.Length))
        #print("   start: index=%-2d count=%d (length=%.2f, distance=%.2f)" % (startIndex, startCount, startEdge.Length, tagDistance))
        #print("               -> lastTagLength=%.2f)" % lastTagLength)
        #print("               -> currentLength=%.2f)" % currentLength)

        edgeDict = { startIndex: startCount }

        for i in range(startIndex + 1, len(self.base.Edges)):
            edge = self.base.Edges[i]
            (currentLength, lastTagLength) = self.processEdge(i, edge, currentLength, lastTagLength, tagDistance, minLength, edgeDict)
        for i in range(0, startIndex):
            edge = self.base.Edges[i]
            (currentLength, lastTagLength) = self.processEdge(i, edge, currentLength, lastTagLength, tagDistance, minLength, edgeDict)

        tags = []

        for (i, count) in edgeDict.iteritems():
            edge = self.base.Edges[i]
            #print(" %d: %d" % (i, count))
            #debugMarker(edge.Vertexes[0].Point, 'base', (1.0, 0.0, 0.0), 0.2)
            #debugMarker(edge.Vertexes[1].Point, 'base', (0.0, 1.0, 0.0), 0.2)
            distance = (edge.LastParameter - edge.FirstParameter) / count
            for j in range(0, count):
                tag = edge.Curve.value((j+0.5) * distance)
                tags.append(Tag(tag.x, tag.y, W, H, angle, True))

        return tags

    def processEdge(self, index, edge, currentLength, lastTagLength, tagDistance, minLength, edgeDict):
        tagCount = 0
        currentLength += edge.Length
        if edge.Length > minLength:
            while lastTagLength + tagDistance < currentLength:
                tagCount += 1
                lastTagLength += tagDistance
            if tagCount > 0:
                #print("      index=%d -> count=%d" % (index, tagCount))
                edgeDict[index] = tagCount
        #else:
            #print("      skipping=%-2d (%.2f)" % (index, edge.Length))

        return (currentLength, lastTagLength)

    def tagHeight(self):
        return self.maxZ - self.minZ

    def tagWidth(self):
        return self.shortestAndLongestPathEdge()[1].Length / 10

    def tagAngle(self):
        return 90

    def pathLength(self):
        return self.base.Length

    def sortedTags(self, tags):
        ordered = []
        for edge in self.base.Edges:
            ts = [t for t in tags if DraftGeomUtils.isPtOnEdge(t.originAt(self.minZ), edge)]
            for t in sorted(ts, key=lambda t: (t.originAt(self.minZ) - edge.valueAt(edge.FirstParameter)).Length):
                tags.remove(t)
                ordered.append(t)
        if tags:
            raise ValueError("There's something really wrong here")
        return ordered


class ObjectDressup:

    def __init__(self, obj):
        self.obj = obj
        obj.addProperty("App::PropertyLink", "Base","Base", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "The base path to modify"))
        obj.addProperty("App::PropertyStringList", "Tags", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_holdingTags", "Inserted tags"))
        obj.setEditorMode("Tags", 2)
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def generateTags(self, obj, count=None, width=None, height=None, angle=90, spacing=None):
        return self.pathData.generateTags(obj, count, width, height, angle, spacing)


    def tagIntersection(self, face, edge):
        p1 = edge.valueAt(edge.FirstParameter)
        pts = edge.Curve.intersect(face.Surface)
        if pts[0]:
            closest = sorted(pts[0], key=lambda pt: (pt - p1).Length)[0]
            return closest
        return None

    def createPath(self, edges, tagSolids):
        commands = []
        i = 0
        while i != len(edges):
            edge = edges[i]
            while edge:
                for solid in tagSolids:
                    for face in solid.Faces:
                        pt = self.tagIntersection(face, edge)
                        if pt:
                            if pt == edge.valueAt(edge.FirstParameter):
                                pt
                            elif pt != edge.valueAt(edge.LastParameter):
                                parameter = edge.Curve.parameter(pt)
                                wire = edge.split(parameter)
                                commands.append(pathCommandForEdge(wire.Edges[0]))
                                edge = wire.Edges[1]
                                break;
                            else:
                                commands.append(pathCommandForEdge(edge))
                                edge = None
                                i += 1
                                break
                    if not edge:
                        break
                if edge:
                    commands.append(pathCommandForEdge(edge))
                    edge = None
        return self.obj.Path


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
            print("execute - no pathData")
            return

        if hasattr(obj, 'Tags') and obj.Tags:
            if self.fingerprint == obj.Tags:
                print("execute - cache valid")
                return
            print("execute - tags from property")
            tags = [Tag.FromString(tag) for tag in obj.Tags]
        else:
            print("execute - default tags")
            tags = self.generateTags(obj, 4.)

        if not tags:
            print("execute - no tags")
            self.tags = []
            obj.Path = obj.Base.Path
            return

        tagID = 0
        for tag in tags:
            tagID += 1
            if tag.enabled:
                #print("x=%s, y=%s, z=%s" % (tag.x, tag.y, pathData.minZ))
                debugMarker(FreeCAD.Vector(tag.x, tag.y, pathData.minZ), "tag-%02d" % tagID , (1.0, 0.0, 1.0), 0.5)

        tags = pathData.sortedTags(tags)
        for tag in tags:
            tag.createSolidsAt(pathData.minZ)

        self.fingerprint = [tag.toString() for tag in tags]
        self.tags = tags

        #obj.Path = self.createPath(pathData.edges, tags)
        obj.Path = self.Base.Path

    def setTags(self, obj, tags):
        obj.Tags = [tag.toString() for tag in tags]
        self.execute(obj)

    def getTags(self, obj):
        if hasattr(self, 'tags'):
            return self.tags
        return self.setup(obj).generateTags(obj, 4)

    def setup(self, obj):
        if not hasattr(self, "pathData") or not self.pathData:
            try:
                pathData = PathData(obj)
            except ValueError:
                FreeCAD.Console.PrintError(translate("PathDressup_HoldingTags", "Cannot insert holding tags for this path - please select a Profile path\n"))
                return None

            ## setup the object's properties, in case they're not set yet
            #obj.Count = self.tagCount(obj)
            #obj.Angle = self.tagAngle(obj)
            #obj.Blacklist = self.tagBlacklist(obj)

            # if the heigt isn't set, use the height of the path
            #if not hasattr(obj, "Height") or not obj.Height:
            #    obj.Height = pathData.maxZ - pathData.minZ
            # try and take an educated guess at the width
            #if not hasattr(obj, "Width") or not obj.Width:
            #    width = sorted(pathData.base.Edges, key=lambda e: -e.Length)[0].Length / 10
            #    while obj.Count > len([e for e in pathData.base.Edges if e.Length > 3*width]):
            #        width = widht / 2
            #    obj.Width = width

            # and the tool radius, not sure yet if it's needed
            #self.toolRadius = 5
            #toolLoad = PathUtils.getLastToolLoad(obj)
            #if toolLoad is None or toolLoad.ToolNumber == 0:
            #    self.toolRadius = 5
            #else:
            #    tool = PathUtils.getTool(obj, toolLoad.ToolNumber)
            #    if not tool or tool.Diameter == 0:
            #        self.toolRadius = 5
            #    else:
            #        self.toolRadius = tool.Diameter / 2
            self.pathData = pathData
        return self.pathData

    def getHeight(self, obj):
        return self.pathData.tagHeight()

    def getWidth(self, obj):
        return self.pathData.tagWidth()

    def getAngle(self, obj):
        return self.pathData.tagAngle()

    def getPathLength(self, obj):
        return self.pathData.pathLength()

class TaskPanel:
    DataTag   = QtCore.Qt.ItemDataRole.UserRole
    DataValue = QtCore.Qt.ItemDataRole.DisplayRole

    def __init__(self, obj):
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/HoldingTagsEdit.ui")
        FreeCAD.ActiveDocument.openTransaction(translate("PathDressup_HoldingTags", "Edit HoldingTags Dress-up"))

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

    def tableWidgetItem(self, tag, val):
        item = QtGui.QTableWidgetItem()
        item.setTextAlignment(QtCore.Qt.AlignRight)
        item.setData(self.DataTag, tag)
        item.setData(self.DataValue, val)
        return item

    def getFields(self):
        tags = []
        for row in range(0, self.form.twTags.rowCount()):
            x = self.form.twTags.item(row, 0).data(self.DataValue)
            y = self.form.twTags.item(row, 1).data(self.DataValue)
            w = self.form.twTags.item(row, 2).data(self.DataValue)
            h = self.form.twTags.item(row, 3).data(self.DataValue)
            a = self.form.twTags.item(row, 4).data(self.DataValue)
            tags.append(Tag(x, y, w, h, a, True))
        print("getFields: %d" % (len(tags)))
        self.obj.Proxy.setTags(self.obj, tags)

    def updateTags(self):
        self.tags = self.obj.Proxy.getTags(self.obj)
        self.form.twTags.blockSignals(True)
        self.form.twTags.setSortingEnabled(False)
        self.form.twTags.clearSpans()
        print("updateTags: %d" % (len(self.tags)))
        self.form.twTags.setRowCount(len(self.tags))
        for row, tag in enumerate(self.tags):
            self.form.twTags.setItem(row, 0, self.tableWidgetItem(tag, tag.x))
            self.form.twTags.setItem(row, 1, self.tableWidgetItem(tag, tag.y))
            self.form.twTags.setItem(row, 2, self.tableWidgetItem(tag, tag.width))
            self.form.twTags.setItem(row, 3, self.tableWidgetItem(tag, tag.height))
            self.form.twTags.setItem(row, 4, self.tableWidgetItem(tag, tag.angle))
        self.form.twTags.setSortingEnabled(True)
        self.form.twTags.blockSignals(False)

    def cleanupUI(self):
        print("cleanupUI")
        if debugDressup:
            for obj in FreeCAD.ActiveDocument.Objects:
                if obj.Name.startswith('tag'):
                    FreeCAD.ActiveDocument.removeObject(obj.Name)

    def updateUI(self):
        print("updateUI")
        self.cleanupUI()
        self.getFields()
        if debugDressup:
            FreeCAD.ActiveDocument.recompute()


    def whenApplyClicked(self):
        print("whenApplyClicked")
        self.cleanupUI()

        count = self.form.sbCount.value()
        spacing = self.form.dsbSpacing.value()
        width = self.form.dsbWidth.value()
        height = self.form.dsbHeight.value()
        angle = self.form.dsbAngle.value()

        tags = self.obj.Proxy.generateTags(self.obj, count, width, height, angle, spacing * 0.99)

        self.obj.Proxy.setTags(self.obj, tags)
        self.updateTags()
        if debugDressup:
            # this causes a big of an echo and a double click on the spin buttons, don't know why though
            FreeCAD.ActiveDocument.recompute()

    def autoApply(self):
        print("autoApply")
        if self.form.cbAutoApply.checkState() == QtCore.Qt.CheckState.Checked:
            self.whenApplyClicked()

    def updateTagSpacing(self, count):
        print("updateTagSpacing")
        if count == 0:
            spacing = 0
        else:
            spacing = self.pathLength / count
        self.form.dsbSpacing.blockSignals(True)
        self.form.dsbSpacing.setValue(spacing)
        self.form.dsbSpacing.blockSignals(False)

    def whenCountChanged(self):
        print("whenCountChanged")
        self.updateTagSpacing(self.form.sbCount.value())
        self.autoApply()

    def whenSpacingChanged(self):
        print("whenSpacingChanged")
        if self.form.dsbSpacing.value() == 0:
            count = 0
        else:
            count = int(self.pathLength / self.form.dsbSpacing.value())
        self.form.sbCount.blockSignals(True)
        self.form.sbCount.setValue(count)
        self.form.sbCount.blockSignals(False)
        self.autoApply()

    def whenOkClicked(self):
        print("whenOkClicked")
        self.whenApplyClicked()
        self.form.toolBox.setCurrentWidget(self.form.tbpTags)

    def setupSpinBox(self, widget, val, decimals = 2):
        widget.setMinimum(0)
        if decimals:
            widget.setDecimals(decimals)
        widget.setValue(val)

    def setFields(self):
        self.pathLength = self.obj.Proxy.getPathLength(self.obj)
        vHeader = self.form.twTags.verticalHeader()
        vHeader.setResizeMode(QtGui.QHeaderView.Fixed)
        vHeader.setDefaultSectionSize(20)
        self.updateTags()
        self.setupSpinBox(self.form.sbCount, self.form.twTags.rowCount(), None)
        self.setupSpinBox(self.form.dsbSpacing, 0)
        self.setupSpinBox(self.form.dsbHeight, self.obj.Proxy.getHeight(self.obj))
        self.setupSpinBox(self.form.dsbWidth, self.obj.Proxy.getWidth(self.obj))
        self.setupSpinBox(self.form.dsbAngle, self.obj.Proxy.getAngle(self.obj))
        self.updateTagSpacing(self.form.twTags.rowCount())

    def setupUi(self):
        self.setFields()
        self.form.sbCount.valueChanged.connect(self.whenCountChanged)
        self.form.dsbSpacing.valueChanged.connect(self.whenSpacingChanged)
        self.form.dsbHeight.valueChanged.connect(self.autoApply)
        self.form.dsbWidth.valueChanged.connect(self.autoApply)
        self.form.dsbAngle.valueChanged.connect(self.autoApply)
        #self.form.pbAdd.clicked.connect(self.)
        self.form.buttonBox.button(QtGui.QDialogButtonBox.Apply).clicked.connect(self.whenApplyClicked)
        self.form.buttonBox.button(QtGui.QDialogButtonBox.Ok).clicked.connect(self.whenOkClicked)
        self.form.twTags.itemChanged.connect(self.updateUI)

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

class CommandPathDressupHoldingTags:

    def GetResources(self):
        return {'Pixmap': 'Path-Dressup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "HoldingTags Dress-up"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Creates a HoldingTags Dress-up object from a selected path")}

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
            FreeCAD.Console.PrintError(translate("PathDressup_HoldingTags", "Please select one path object\n"))
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            FreeCAD.Console.PrintError(translate("PathDressup_HoldingTags", "The selected object is not a path\n"))
            return
        if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
            FreeCAD.Console.PrintError(translate("PathDressup_HoldingTags", "Please select a Profile object"))
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate("PathDressup_HoldingTags", "Create HoldingTags Dress-up"))
        FreeCADGui.addModule("PathScripts.PathDressupHoldingTags")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "HoldingTagsDressup")')
        FreeCADGui.doCommand('dbo = PathScripts.PathDressupHoldingTags.ObjectDressup(obj)')
        FreeCADGui.doCommand('obj.Base = FreeCAD.ActiveDocument.' + selection[0].Name)
        FreeCADGui.doCommand('PathScripts.PathDressupHoldingTags.ViewProviderDressup(obj.ViewObject)')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('Gui.ActiveDocument.getObject(obj.Base.Name).Visibility = False')
        FreeCADGui.doCommand('dbo.setup(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('PathDressup_HoldingTags', CommandPathDressupHoldingTags())

FreeCAD.Console.PrintLog("Loading PathDressupHoldingTags... done\n")
