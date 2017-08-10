# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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
import DraftGeomUtils
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils
import string
import sys

from PySide import QtCore

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())

class ObjectOp(PathOp.ObjectOp):

    def opFeatures(self, obj):
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureHeights | PathOp.FeatureBaseFaces | self.circularHoleFeatures(obj)

    def initOperation(self, obj):
        obj.addProperty("App::PropertyStringList", "Disabled", "Base", QtCore.QT_TRANSLATE_NOOP("Path", "List of disabled features"))
        self.initCircularHoleOperation(obj)

    def baseIsArchPanel(self, obj, base):
        return hasattr(base, "Proxy") and isinstance(base.Proxy, ArchPanel.PanelSheet)

    def getArchPanelEdge(self, obj, base, sub):
        ids = string.split(sub, '.')
        holeId = int(ids[0])
        wireId = int(ids[1])
        edgeId = int(ids[2])

        for holeNr, hole in enumerate(base.Proxy.getHoles(base, transform=True)):
            if holeNr == holeId:
                for wireNr, wire in enumerate(hole.Wires):
                    if wireNr == wireId:
                        for edgeNr, edge in enumerate(wire.Edges):
                            if edgeNr == edgeId:
                                return edge

    def holeDiameter(self, obj, base, sub):
        if self.baseIsArchPanel(obj, base):
            edge = self.getArchPanelEdge(obj, base, sub)
            return edge.BoundBox.XLength

        shape = base.Shape.getElement(sub)
        if shape.ShapeType == 'Vertex':
            return 0

        # for all other shapes the diameter is just the dimension in X
        return shape.BoundBox.XLength

    def holePosition(self, obj, base, sub):
        if self.baseIsArchPanel(obj, base):
            edge = self.getArchPanelEdge(obj, base, sub)
            center = edge.Curve.Center
            return FreeCAD.Vector(center.x, center.y, 0)

        shape = base.Shape.getElement(sub)
        if shape.ShapeType == 'Vertex':
            return FreeCAD.Vector(shape.X, shape.Y, 0)

        if shape.ShapeType == 'Edge':
            return FreeCAD.Vector(shape.Curve.Center.x, shape.Curve.Center.y, 0)

        if shape.ShapeType == 'Face':
            return FreeCAD.Vector(shape.Surface.Center.x, shape.Surface.Center.y, 0)

        PathLog.error('This is bad')

    def isHoleEnabled(self, obj, base, sub):
        name = "%s.%s" % (base.Name, sub)
        return not name in obj.Disabled

    def opExecute(self, obj):
        PathLog.track()

        if len(obj.Base) == 0:
            # Arch PanelSheet
            features = []
            if self.baseIsArchPanel(obj, self.baseobject):
                holeshapes = self.baseobject.Proxy.getHoles(self.baseobject, transform=True)
                tooldiameter = obj.ToolController.Proxy.getTool(obj.ToolController).Diameter
                for holeNr, hole in enumerate(holeshapes):
                    PathLog.debug('Entering new HoleShape')
                    for wireNr, wire in enumerate(hole.Wires):
                        PathLog.debug('Entering new Wire')
                        for edgeNr, edge in enumerate(wire.Edges):
                            if PathUtils.isDrillable(self.baseobject, edge, tooldiameter):
                                PathLog.debug('Found drillable hole edges: {}'.format(edge))
                                features.append((self.baseobject, "%d.%d.%d" % (holeNr, wireNr, edgeNr)))

                self.setDepths(obj, None, None, self.baseobject.Shape.BoundBox)
            else:
                features = self.findHoles(obj, self.baseobject)
                self.setupDepthsFrom(obj, features, self.baseobject)
            obj.Base = features
            obj.Disabled = []

        holes = []

        for base, subs in obj.Base:
            for sub in subs:
                if self.isHoleEnabled(obj, base, sub):
                    pos = self.holePosition(obj, base, sub)
                    holes.append({'x': pos.x, 'y': pos.y, 'r': self.holeDiameter(obj, base, sub)})

        if len(holes) > 0:
            self.circularHoleExecute(obj, holes)

    def opOnChanged(self, obj, prop):
        if 'Base' == prop and not 'Restore' in obj.State and obj.Base:
            features = []
            for base, subs in obj.Base:
                for sub in subs:
                    features.append((base, sub))

            job = PathUtils.findParentJob(obj)
            if not job or not job.Base:
                return

            self.setupDepthsFrom(obj, features, job.Base)

    def setupDepthsFrom(self, obj, features, baseobject):
        zmax = None
        zmin = None
        for base,sub in features:
            shape = base.Shape.getElement(sub)
            bb = shape.BoundBox
            # find the highes zmax and the highes zmin levels, those provide
            # the safest values for StartDepth and FinalDepth
            if zmax is None or zmax < bb.ZMax:
                zmax = bb.ZMax
            if zmin is None or zmin < bb.ZMin:
                zmin = bb.ZMin
        self.setDepths(obj, zmax, zmin, baseobject.Shape.BoundBox)

    def setDepths(self, obj, zmax, zmin, bb):
        PathLog.track(obj.Label, zmax, zmin, bb)
        if zmax is None:
            zmax = 5
        if zmin is None:
            zmin = 0

        if zmin > zmax:
            zmax = zmin

        PathLog.debug("setDepths(%s): z=%.2f -> %.2f bb.z=%.2f -> %.2f" % (obj.Label, zmin, zmax, bb.ZMin, bb.ZMax))

        obj.StartDepth = zmax
        obj.ClearanceHeight = bb.ZMax + 5.0
        obj.SafeHeight = bb.ZMax + 3.0
        obj.FinalDepth = zmin

    def findHoles(self, obj, baseobject):
        shape = baseobject.Shape
        PathLog.track('obj: {} shape: {}'.format(obj, shape))
        holelist = []
        features = []
        # tooldiameter = obj.ToolController.Proxy.getTool(obj.ToolController).Diameter
        tooldiameter = None
        PathLog.debug('search for holes larger than tooldiameter: {}: '.format(tooldiameter))
        if DraftGeomUtils.isPlanar(shape):
            PathLog.debug("shape is planar")
            for i in range(len(shape.Edges)):
                candidateEdgeName = "Edge" + str(i + 1)
                e = shape.getElement(candidateEdgeName)
                if PathUtils.isDrillable(shape, e, tooldiameter):
                    PathLog.debug('edge candidate: {} (hash {})is drillable '.format(e, e.hashCode()))
                    x = e.Curve.Center.x
                    y = e.Curve.Center.y
                    diameter = e.BoundBox.XLength
                    holelist.append({'featureName': candidateEdgeName, 'feature': e, 'x': x, 'y': y, 'd': diameter, 'enabled': True})
                    features.append((baseobject, candidateEdgeName))
                    PathLog.debug("Found hole feature %s.%s" % (baseobject.Label, candidateEdgeName))
        else:
            PathLog.debug("shape is not planar")
            for i in range(len(shape.Faces)):
                candidateFaceName = "Face" + str(i + 1)
                f = shape.getElement(candidateFaceName)
                if PathUtils.isDrillable(shape, f, tooldiameter):
                    PathLog.debug('face candidate: {} is drillable '.format(f))
                    x = f.Surface.Center.x
                    y = f.Surface.Center.y
                    diameter = f.BoundBox.XLength
                    holelist.append({'featureName': candidateFaceName, 'feature': f, 'x': x, 'y': y, 'd': diameter, 'enabled': True})
                    features.append((baseobject, candidateFaceName))
                    PathLog.debug("Found hole feature %s.%s" % (baseobject.Label, candidateFaceName))

        PathLog.debug("holes found: {}".format(holelist))
        return features
