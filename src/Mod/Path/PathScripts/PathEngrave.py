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

import ArchPanel
import DraftGeomUtils
import FreeCAD
import Part
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathGeom as PathGeom
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils
import TechDraw
import traceback

from DraftGeomUtils import geomType
from PathScripts.PathPreferences import PathPreferences
from PySide import QtCore

__doc__ = "Class and implementation of Path Engrave operation"

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

def adjustWirePlacement(obj, shape, wires):
    job = PathUtils.findParentJob(obj)
    if hasattr(shape, 'MapMode') and 'Deactivated' != shape.MapMode:
        if hasattr(shape, 'Support') and 1 == len(shape.Support) and 1 == len(shape.Support[0][1]):
            pmntShape   = shape.Placement
            pmntSupport = shape.Support[0][0].getGlobalPlacement()
            #pmntSupport = shape.Support[0][0].Placement
            pmntBase    = job.Base.Placement
            pmnt = pmntBase.multiply(pmntSupport.inverse().multiply(pmntShape))
            #PathLog.debug("pmnt = %s" % pmnt)
            newWires = []
            for w in wires:
                edges = []
                for e in w.Edges:
                    e = e.copy()
                    e.Placement = FreeCAD.Placement()
                    edges.append(e)
                w = Part.Wire(edges)
                w.Placement = pmnt
                newWires.append(w)
            wires = newWires
        else:
            PathLog.warning(translate("PathEngrave", "Attachment not supported by engraver"))
    else:
        PathLog.debug("MapMode: %s" % (shape.MapMode if hasattr(shape, 'MapMode') else 'None')) 
    return wires

class ObjectEngrave(PathOp.ObjectOp):
    '''Proxy class for Engrave operation.'''

    def opFeatures(self, obj):
        '''opFeatures(obj) ... return all standard features and edges based geomtries'''
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureHeights | PathOp.FeatureStepDown | PathOp.FeatureBaseEdges;

    def setupAdditionalProperties(self, obj):
        if not hasattr(obj, 'BaseShapes'):
            obj.addProperty("App::PropertyLinkList", "BaseShapes", "Path", QtCore.QT_TRANSLATE_NOOP("PathEngrave", "Additional base objects to be engraved"))
            obj.setEditorMode('BaseShapes', 2) # hide
        if not hasattr(obj, 'BaseObject'):
            obj.addProperty("App::PropertyLink", "BaseObject", "Path", QtCore.QT_TRANSLATE_NOOP("PathEngrave", "Additional base objects to be engraved"))
            obj.setEditorMode('BaseObject', 2) # hide

    def initOperation(self, obj):
        '''initOperation(obj) ... create engraving specific properties.'''
        obj.addProperty("App::PropertyInteger", "StartVertex", "Path", QtCore.QT_TRANSLATE_NOOP("PathEngrave", "The vertex index to start the path from"))
        self.setupAdditionalProperties(obj)

    def onDocumentRestored(self, obj):
        # upgrade ...
        super(self.__class__, self).onDocumentRestored(obj)
        self.setupAdditionalProperties(obj)

    def opExecute(self, obj):
        '''opExecute(obj) ... process engraving operation'''
        PathLog.track()

        job = PathUtils.findParentJob(obj)
        if job and job.Base:
            obj.BaseObject = job.Base

        zValues = []
        if obj.StepDown.Value != 0:
            z = obj.StartDepth.Value - obj.StepDown.Value

            while z > obj.FinalDepth.Value:
                zValues.append(z)
                z -= obj.StepDown.Value
        zValues.append(obj.FinalDepth.Value)
        self.zValues = zValues

        try:
            if self.baseobject.isDerivedFrom('Sketcher::SketchObject') or \
                    self.baseobject.isDerivedFrom('Part::Part2DObject') or \
                    hasattr(self.baseobject, 'ArrayType'):
                PathLog.track()

                self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))

                # we only consider the outer wire if this is a Face
                wires = []
                for w in self.baseobject.Shape.Wires:
                    wires.append(Part.Wire(w.Edges))
                self.buildpathocc(obj, wires, zValues)
                self.wires = wires

            elif isinstance(self.baseobject.Proxy, ArchPanel.PanelSheet):  # process the sheet
                PathLog.track()
                wires = []
                for tag in self.baseobject.Proxy.getTags(self.baseobject, transform=True):
                    self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
                    tagWires = []
                    for w in tag.Wires:
                        tagWires.append(Part.Wire(w.Edges))
                    self.buildpathocc(obj, tagWires, zValues)
                    wires.extend(tagWires)
                self.wires = wires
            elif obj.Base:
                PathLog.track()
                wires = []
                for base, subs in obj.Base:
                    edges = []
                    #for sub in subs:
                    #    edges.extend(base.Shape.getElement(sub).Edges)
                    #shapeWires = adjustWirePlacement(obj, base, TechDraw.edgeWalker(edges))
                    #wires.extend(shapeWires)
                    for feature in subs:
                        sub = base.Shape.getElement(feature)
                        if type(sub) == Part.Edge:
                            edges.append(sub)
                        elif sub.Wires:
                            wires.extend(sub.Wires)
                        else:
                            wires.append(Part.Wire(sub.Edges))

                    for edgelist in Part.sortEdges(edges):
                        wires.append(Part.Wire(edgelist))
                wires = adjustWirePlacement(obj, base, wires)
                self.buildpathocc(obj, wires, zValues)
                self.wires = wires
            elif not obj.BaseShapes:
                PathLog.track()
                raise ValueError(translate('PathEngrave', "Unknown baseobject type for engraving (%s)") % (obj.Base))

            if obj.BaseShapes:
                PathLog.track()
                wires = []
                for shape in obj.BaseShapes:
                    shapeWires = adjustWirePlacement(obj, shape, shape.Shape.Wires)
                    self.buildpathocc(obj, shapeWires, zValues)
                    wires.extend(shapeWires)
                self.wires = wires

        except Exception as e:
            PathLog.error(e)
            traceback.print_exc()
            PathLog.error(translate('PathEngrave', 'The Job Base Object has no engraveable element.  Engraving operation will produce no output.'))

    def buildpathocc(self, obj, wires, zValues):
        '''buildpathocc(obj, wires, zValues) ... internal helper function to generate engraving commands.'''
        PathLog.track(obj.Label, len(wires), zValues)

        for wire in wires:
            offset = wire

            # reorder the wire
            offset = DraftGeomUtils.rebaseWire(offset, obj.StartVertex)

            last = None
            for z in zValues:
                if last:
                    self.commandlist.append(Path.Command('G1', {'X': last.x, 'Y': last.y, 'Z': z, 'F': self.vertFeed}))

                for edge in offset.Edges:
                    if not last:
                        # we set the first move to our first point
                        last = edge.Vertexes[0].Point
                        if len(offset.Edges) > 1:
                            e2 = offset.Edges[1]
                            if not PathGeom.pointsCoincide(edge.Vertexes[-1].Point, e2.Vertexes[0].Point) and not PathGeom.pointsCoincide(edge.Vertexes[-1].Point, e2.Vertexes[-1].Point):
                                PathLog.debug("flip first edge")
                                last = edge.Vertexes[-1].Point
                            else:
                                PathLog.debug("original first edge")
                        else:
                            PathLog.debug("not enough edges to flip")
                        self.commandlist.append(Path.Command('G0', {'X': last.x, 'Y': last.y, 'Z': obj.ClearanceHeight.Value, 'F': self.horizRapid}))
                        self.commandlist.append(Path.Command('G0', {'X': last.x, 'Y': last.y, 'Z': obj.SafeHeight.Value, 'F': self.vertRapid}))
                        self.commandlist.append(Path.Command('G0', {'X': last.x, 'Y': last.y, 'Z': z, 'F': self.vertFeed}))

                    if PathGeom.pointsCoincide(last, edge.Vertexes[0].Point):
                        for cmd in PathGeom.cmdsForEdge(edge):
                            params = cmd.Parameters
                            params.update({'Z': z, 'F': self.horizFeed})
                            self.commandlist.append(Path.Command(cmd.Name, params))
                        last = edge.Vertexes[-1].Point
                    else:
                        for cmd in PathGeom.cmdsForEdge(edge, True):
                            params = cmd.Parameters
                            params.update({'Z': z, 'F': self.horizFeed})
                            self.commandlist.append(Path.Command(cmd.Name, params))
                        last = edge.Vertexes[0].Point
            self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))
        if self.commandlist:
            self.commandlist.pop()


    def opSetDefaultValues(self, obj):
        '''opSetDefaultValues(obj) ... set depths for engraving'''
        job = PathUtils.findParentJob(obj)
        if job and job.Base:
            bb = job.Base.Shape.BoundBox
            obj.OpStartDepth = bb.ZMax
            obj.OpFinalDepth = bb.ZMax - max(obj.StepDown.Value, 0.1)
        else:
            obj.OpFinalDepth = -0.1

    def updateDepths(self, obj, ignoreErrors=False):
        '''updateDepths(obj) ... engraving is always done at the top most z-value'''
        self.opSetDefaultValues(obj)

def Create(name):
    '''Create(name) ... Creates and returns a Engrave operation.'''
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectEngrave(obj)
    return obj

