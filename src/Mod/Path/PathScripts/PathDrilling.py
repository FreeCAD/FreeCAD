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

from __future__ import print_function

import ArchPanel
import FreeCAD
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils
import string
import sys

from PathScripts.PathUtils import fmt, waiting_effects
from PySide import QtCore

# xrange is not available in python3
if sys.version_info.major >= 3:
    xrange = range

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

"""Path Drilling object and FreeCAD command"""

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectDrilling(PathOp.ObjectOp):

    def opFeatures(self, obj):
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureHeights | PathOp.FeatureBaseGeometry

    def initOperation(self, obj):

        obj.addProperty("App::PropertyStringList", "Disabled", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "List of disabled features"))
        obj.addProperty("App::PropertyLength", "PeckDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Incremental Drill depth before retracting to clear chips"))
        obj.addProperty("App::PropertyBool", "PeckEnabled", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable pecking"))
        obj.addProperty("App::PropertyFloat", "DwellTime", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "The time to dwell between peck cycles"))
        obj.addProperty("App::PropertyBool", "DwellEnabled", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Enable dwell"))
        obj.addProperty("App::PropertyBool", "AddTipLength", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Calculate the tip length and subtract from final depth"))
        obj.addProperty("App::PropertyEnumeration", "ReturnLevel", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "Controls how tool retracts Default=G98"))
        obj.ReturnLevel = ['G98', 'G99']  # this is the direction that the Contour runs

        obj.addProperty("App::PropertyDistance", "RetractHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property", "The height where feed starts and height during retract tool when path is finished"))

        self.vertFeed = 0.0
        self.horizFeed = 0.0
        self.vertRapid = 0.0
        self.horizRapid = 0.0

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

        tiplength = 0.0
        if obj.AddTipLength:
            tiplength = PathUtils.drillTipLength(tool)

        if len(obj.Base) == 0:
            job = PathUtils.findParentJob(obj)
            if not job or not job.Base:
                return
            baseobject = job.Base

            # Arch PanelSheet
            features = []
            fbb = None
            if self.baseIsArchPanel(obj, baseobject):
                holeshapes = baseobject.Proxy.getHoles(baseobject, transform=True)
                tooldiameter = obj.ToolController.Proxy.getTool(obj.ToolController).Diameter
                for holeNr, hole in enumerate(holeshapes):
                    PathLog.debug('Entering new HoleShape')
                    for wireNr, wire in enumerate(hole.Wires):
                        PathLog.debug('Entering new Wire')
                        for edgeNr, edge in enumerate(wire.Edges):
                            if PathUtils.isDrillable(baseobject, edge, tooldiameter):
                                PathLog.debug('Found drillable hole edges: {}'.format(edge))
                                features.append((baseobject, "%d.%d.%d" % (holeNr, wireNr, edgeNr)))

            else:
                features = self.findHoles(obj, baseobject)
                for base,sub in features:
                    shape = base.Shape.getElement(sub)
                    fbb = fbb.united(shape.BoundBox) if fbb else shape.BoundBox
            obj.Base = features
            self.setDepths(obj, obj.Base[0][0].Shape.BoundBox, fbb)
            obj.Disabled = []

        locations = []
        self.commandlist.append(Path.Command("(Begin Drilling)"))

        for base, subs in obj.Base:
            for sub in subs:
                if self.isHoleEnabled(obj, base, sub):
                    pos = self.holePosition(obj, base, sub)
                    locations.append({'x': pos.x, 'y': pos.y})

        if len(locations) > 0:
            locations = PathUtils.sort_jobs(locations, ['x', 'y'])
            self.commandlist.append(Path.Command('G90'))
            self.commandlist.append(Path.Command(obj.ReturnLevel))
            # rapid to clearance height
            self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
            # rapid to first hole location, with spindle still retracted:

            p0 = locations[0]
            self.commandlist.append(Path.Command('G0', {'X': p0['x'], 'Y': p0['y'], 'F': self.horizRapid}))
            # move tool to clearance plane
            self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))

            cmd = "G81"
            cmdParams = {}
            cmdParams['Z'] = obj.FinalDepth.Value - tiplength
            cmdParams['F'] = self.vertFeed
            cmdParams['R'] = obj.RetractHeight.Value
            if obj.PeckEnabled and obj.PeckDepth.Value > 0:
                cmd = "G83"
                cmdParams['Q'] = obj.PeckDepth.Value
            elif obj.DwellEnabled and obj.DwellTime > 0:
                cmd = "G82"
                cmdParams['P'] = obj.DwellTime

            for p in locations:
                params = {}
                params['X'] = p['x']
                params['Y'] = p['y']
                params.update(cmdParams)
                self.commandlist.append(Path.Command(cmd, params))

            self.commandlist.append(Path.Command('G80'))

    def setDepths(self, obj, bb, fbb):
        if bb and fbb:
            obj.StartDepth = bb.ZMax
            obj.ClearanceHeight = bb.ZMax + 5.0
            obj.SafeHeight = bb.ZMax + 3.0
            obj.RetractHeight = bb.ZMax + 1.0

            if fbb.ZMax < bb.ZMax:
                obj.FinalDepth = fbb.ZMax
            else:
                obj.FinalDepth = bb.ZMin
        else:
            obj.StartDepth = 5.0
            obj.ClearanceHeight = 10.0
            obj.SafeHeight = 8.0
            obj.RetractHeight = 6.0

    def findHoles(self, obj, baseobject):
        import DraftGeomUtils as dgu
        shape = baseobject.Shape
        PathLog.track('obj: {} shape: {}'.format(obj, shape))
        holelist = []
        features = []
        # tooldiameter = obj.ToolController.Proxy.getTool(obj.ToolController).Diameter
        tooldiameter = None
        PathLog.debug('search for holes larger than tooldiameter: {}: '.format(tooldiameter))
        if dgu.isPlanar(shape):
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

    def opSetDefaultValues(self, obj):
        obj.RetractHeight = 10

def Create(name):
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectDrilling(obj)
    return obj
