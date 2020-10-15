# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 sliptonic <shopinthewoods@gmail.com>               *
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
import Part
import Path
import PathScripts.PathEngraveBase as PathEngraveBase
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils
import PathScripts.PathGeom as PathGeom
import PathScripts.PathPreferences as PathPreferences

import traceback

import math

from PySide import QtCore

__doc__ = "Class and implementation of Path Vcarve operation"

PRIMARY = 0
EXTERIOR1 = 1
EXTERIOR2 = 4
TWIN = 2
COLINEAR = 3
SECONDARY = 5

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


VD = []


class ObjectVcarve(PathEngraveBase.ObjectOp):
    '''Proxy class for Vcarve operation.'''

    def opFeatures(self, obj):
        '''opFeatures(obj) ... return all standard features and edges based geomtries'''
        return PathOp.FeatureTool | PathOp.FeatureHeights | PathOp.FeatureBaseFaces | PathOp.FeatureCoolant

    def setupAdditionalProperties(self, obj):
        if not hasattr(obj, 'BaseShapes'):
            obj.addProperty("App::PropertyLinkList", "BaseShapes", "Path",
                            QtCore.QT_TRANSLATE_NOOP("PathVcarve",
                                "Additional base objects to be engraved"))
        obj.setEditorMode('BaseShapes', 2)  # hide
        if not hasattr(obj, 'BaseObject'):
            obj.addProperty("App::PropertyLink", "BaseObject", "Path",
                            QtCore.QT_TRANSLATE_NOOP("PathVcarve",
                            "Additional base objects to be engraved"))
        obj.setEditorMode('BaseObject', 2)  # hide

    def initOperation(self, obj):
        '''initOperation(obj) ... create vcarve specific properties.'''
        obj.addProperty("App::PropertyFloat", "Discretize", "Path",
                        QtCore.QT_TRANSLATE_NOOP("PathVcarve",
                        "The deflection value for discretizing arcs"))
        obj.addProperty("App::PropertyFloat", "Threshold", "Path",
                        QtCore.QT_TRANSLATE_NOOP("PathVcarve",
                        "cutoff for removing colinear segments (degrees). \
                        default=10.0."))
        obj.addProperty("App::PropertyFloat", "Tolerance", "Path",
                QtCore.QT_TRANSLATE_NOOP("PathVcarve", ""))
        obj.Threshold = 10.0
        obj.Discretize = 0.01
        obj.Tolerance = PathPreferences.defaultGeometryTolerance()
        self.setupAdditionalProperties(obj)

    def opOnDocumentRestored(self, obj):
        # upgrade ...
        self.setupAdditionalProperties(obj)

    def buildPathMedial(self, obj, Faces):
        '''constructs a medial axis path using openvoronoi'''

        def insert_many_wires(vd, wires):
            for wire in wires:
                PathLog.debug('discretize value: {}'.format(obj.Discretize))
                pts = wire.discretize(QuasiDeflection=obj.Discretize)
                ptv = [FreeCAD.Vector(p.x, p.y) for p in pts]
                ptv.append(ptv[0])

                for i in range(len(pts)):
                    vd.addSegment(ptv[i], ptv[i+1])

        def calculate_depth(MIC, baselevel=0):
            # given a maximum inscribed circle (MIC) and tool angle,
            # return depth of cut relative to baselevel.

            r = obj.ToolController.Tool.Diameter / 2
            toolangle = obj.ToolController.Tool.CuttingEdgeAngle
            maxdepth = baselevel - r / math.tan(math.radians(toolangle/2))

            d = baselevel - round(MIC / math.tan(math.radians(toolangle / 2)), 4)
            PathLog.debug('baselevel value: {} depth: {}'.format(baselevel, d))
            return d if d <= maxdepth else maxdepth

        def getEdges(vd, color=[PRIMARY]):
            if type(color) == int:
                color = [color]
            geomList = []
            bblevel = self.model[0].Shape.BoundBox.ZMin
            for e in vd.Edges:
                if e.Color not in color:
                    continue
                if e.toGeom() is None:
                    continue
                p1 = e.Vertices[0].toGeom(calculate_depth(e.getDistances()[0], bblevel))
                p2 = e.Vertices[-1].toGeom(calculate_depth(e.getDistances()[-1], bblevel))
                newedge = Part.Edge(Part.Vertex(p1), Part.Vertex(p2))

                newedge.fixTolerance(obj.Tolerance, Part.Vertex)
                geomList.append(newedge)

            return geomList

        def sortEm(mywire, unmatched):
            remaining = []
            wireGrowing = False

            # end points of existing wire
            wireverts = [mywire.Edges[0].valueAt(mywire.Edges[0].FirstParameter),
                mywire.Edges[-1].valueAt(mywire.Edges[-1].LastParameter)]

            for i, candidate in enumerate(unmatched):

                # end points of candidate edge
                cverts = [candidate.Edges[0].valueAt(candidate.Edges[0].FirstParameter),
                    candidate.Edges[-1].valueAt(candidate.Edges[-1].LastParameter)]

                # ignore short segments below tolerance level
                if PathGeom.pointsCoincide(cverts[0], cverts[1], obj.Tolerance):
                    continue

                # iterate the combination of endpoints. If a match is found,
                # make an edge from the common endpoint to the other end of
                # the candidate wire. Add the edge to the wire and return it.

                # This generates a new edge rather than using the candidate to
                # avoid vertexes with close but different vectors
                for wvert in wireverts:
                    for idx, cvert in enumerate(cverts):
                        if PathGeom.pointsCoincide(wvert, cvert, obj.Tolerance):
                            wireGrowing = True
                            elist = mywire.Edges
                            otherIndex = int(not(idx))

                            newedge = Part.Edge(Part.Vertex(wvert),
                                Part.Vertex(cverts[otherIndex]))

                            elist.append(newedge)
                            mywire = Part.Wire(Part.__sortEdges__(elist))
                            remaining.extend(unmatched[i+1:])
                            return mywire, remaining, wireGrowing

                # if not matched, add to remaining list to test later
                remaining.append(candidate)

            return mywire, remaining, wireGrowing

        def getWires(candidateList):

            chains = []
            while len(candidateList) > 0:
                cur_wire = Part.Wire(candidateList.pop(0))

                wireGrowing = True
                while wireGrowing:
                    cur_wire, candidateList, wireGrowing = sortEm(cur_wire,
                            candidateList)

                chains.append(cur_wire)

            return chains

        def cutWire(w):
            path = []
            path.append(Path.Command("G0 Z{}".format(obj.SafeHeight.Value)))
            e = w.Edges[0]
            p = e.valueAt(e.FirstParameter)
            path.append(Path.Command("G0 X{} Y{} Z{}".format(p.x, p.y,
                obj.SafeHeight.Value)))
            c = Path.Command("G1 X{} Y{} Z{} F{}".format(p.x, p.y, p.z,
                obj.ToolController.HorizFeed.Value))
            path.append(c)
            for e in w.Edges:
                path.extend(PathGeom.cmdsForEdge(e,
                    hSpeed=obj.ToolController.HorizFeed.Value))

            return path

        VD.clear()
        pathlist = []
        pathlist.append(Path.Command("(starting)"))
        for f in Faces:
            vd = Path.Voronoi()
            insert_many_wires(vd, f.Wires)

            vd.construct()

            for e in vd.Edges:
                e.Color = PRIMARY if e.isPrimary() else SECONDARY
            vd.colorExterior(EXTERIOR1)
            vd.colorExterior(EXTERIOR2,
                lambda v: not f.isInside(v.toGeom(f.BoundBox.ZMin),
                obj.Tolerance, True))
            vd.colorColinear(COLINEAR, obj.Threshold)
            vd.colorTwins(TWIN)

            edgelist = getEdges(vd)

            for wire in getWires(edgelist):
                pathlist.extend(cutWire(wire))
            VD.append((f, vd, getWires(edgelist)))

        self.commandlist = pathlist

    def opExecute(self, obj):
        '''opExecute(obj) ... process engraving operation'''
        PathLog.track()

        if not hasattr(obj.ToolController.Tool, "CuttingEdgeAngle"):
            FreeCAD.Console.PrintError(
                translate("Path_Vcarve", "VCarve requires an engraving \
                           cutter with CuttingEdgeAngle") + "\n")

        if obj.ToolController.Tool.CuttingEdgeAngle >= 180.0:
            FreeCAD.Console.PrintError(
                translate("Path_Vcarve",
                    "Engraver Cutting Edge Angle must be < 180 degrees.") + "\n")
            return
        try:
            if obj.Base:
                PathLog.track()
                for base in obj.Base:
                    faces = []
                    for sub in base[1]:
                        shape = getattr(base[0].Shape, sub)
                        if isinstance(shape, Part.Face):
                            faces.append(shape)

                modelshape = Part.makeCompound(faces)

            elif len(self.model) == 1 and self.model[0].isDerivedFrom('Sketcher::SketchObject') or \
                    self.model[0].isDerivedFrom('Part::Part2DObject'):
                PathLog.track()

                modelshape = self.model[0].Shape
            self.buildPathMedial(obj, modelshape.Faces)

        except Exception as e:
            PathLog.error(e)
            traceback.print_exc()
            PathLog.error(translate('PathVcarve', 'The Job Base Object has \
                                    no engraveable element. Engraving \
                                    operation will produce no output.'))

    def opUpdateDepths(self, obj, ignoreErrors=False):
        '''updateDepths(obj) ... engraving is always done at \
                the top most z-value'''
        job = PathUtils.findParentJob(obj)
        self.opSetDefaultValues(obj, job)


def SetupProperties():
    return ["Discretize"]


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Vcarve operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    ObjectVcarve(obj, name)
    return obj
