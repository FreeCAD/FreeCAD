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
import Part
import Path
import PathScripts.PathEngraveBase as PathEngraveBase
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils
import PathScripts.PathGeom as PathGeom
import traceback
import time
from PathScripts.PathOpTools import orientWire
import math

from PySide import QtCore

__doc__ = "Class and implementation of Path Vcarve operation"

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectVcarve(PathEngraveBase.ObjectOp):
    '''Proxy class for Vcarve operation.'''

    def opFeatures(self, obj):
        '''opFeatures(obj) ... return all standard features and edges based geomtries'''
        return PathOp.FeatureTool | PathOp.FeatureHeights | PathOp.FeatureBaseFaces

    def setupAdditionalProperties(self, obj):
        if not hasattr(obj, 'BaseShapes'):
            obj.addProperty("App::PropertyLinkList", "BaseShapes", "Path", QtCore.QT_TRANSLATE_NOOP("PathVcarve", "Additional base objects to be engraved"))
        obj.setEditorMode('BaseShapes', 2)  # hide
        if not hasattr(obj, 'BaseObject'):
            obj.addProperty("App::PropertyLink", "BaseObject", "Path", QtCore.QT_TRANSLATE_NOOP("PathVcarve", "Additional base objects to be engraved"))
        obj.setEditorMode('BaseObject', 2)  # hide

    def initOperation(self, obj):
        '''initOperation(obj) ... create vcarve specific properties.'''
        obj.addProperty("App::PropertyFloat", "Discretize", "Path", QtCore.QT_TRANSLATE_NOOP("PathVcarve", "The deflection value for discretizing arcs"))
        obj.addProperty("App::PropertyFloat", "Threshold", "Path", QtCore.QT_TRANSLATE_NOOP("PathVcarve", "cutoff threshold for removing extraneous segments (0-1.0). default=0.8. Larger numbers remove less."))
        obj.Threshold = 0.8
        obj.Discretize = 0.01
        self.setupAdditionalProperties(obj)

    def opOnDocumentRestored(self, obj):
        # upgrade ...
        self.setupAdditionalProperties(obj)

    def buildPathMedial(self, obj, Faces):
        '''constructs a medial axis path using openvoronoi'''
        #import openvoronoi as ovd

        # def insert_wire_points(vd, wire):
        #     pts = []
        #     for p in wire.Vertexes:
        #         pts.append(ovd.Point(p.X, p.Y))
        #         PathLog.debug('ovd.Point( {} ,{} )'.format(p.X, p.Y))
        #     id_list = []
        #     PathLog.debug("inserting {} openvoronoi point-sites".format(len(pts)))
        #     for p in pts:
        #         id_list.append(vd.addVertexSite(p))
        #     return id_list

        # def insert_wire_segments(vd, id_list):
        #     PathLog.debug('inserting {} segments into the voronoi diagram'.format(len(id_list)))
        #     for n in range(len(id_list)):
        #         n_nxt = n + 1
        #         if n == (len(id_list) - 1):
        #             n_nxt = 0
        #         vd.addLineSite(id_list[n], id_list[n_nxt])

        def insert_many_wires(vd, wires):
            #polygon_ids = []
            #t_before = time.time()
            for wire in wires:
                PathLog.debug('discretize value: {}'.format(obj.Discretize))
                pts = wire.discretize(QuasiDeflection=obj.Discretize)
                ptv = [FreeCAD.Vector(p[0], p[1]) for p in pts]
                ptv.append(ptv[0])

                for i in range(len(pts)):
                    vd.addSegment(ptv[i], ptv[i+1])

                # segwire = Part.Wire([Part.makeLine(p[0], p[1]) for p in zip(pointList, pointList[1:])])

                # if idx == 0:
                #     segwire = orientWire(segwire, forward=False)
                # else:
                #     segwire = orientWire(segwire, forward=True)

                # poly_id = insert_wire_points(vd, segwire)
                # polygon_ids.append(poly_id)
            # t_after = time.time()
            # pt_time = t_after - t_before

            # t_before = time.time()
            # for ids in polygon_ids:
                # insert_wire_segments(vd, ids)
            # t_after = time.time()
            # seg_time = t_after - t_before
            # return [pt_time, seg_time]

        def calculate_depth(MIC):
            # given a maximum inscribed circle (MIC) and tool angle,
            # return depth of cut.
            maxdepth = obj.ToolController.Tool.CuttingEdgeHeight
            toolangle = obj.ToolController.Tool.CuttingEdgeAngle
            d = MIC / math.tan(math.radians(toolangle / 2))
            return d if d <= maxdepth else maxdepth

        # def buildMedial(vd):
        #     safeheight = obj.SafeHeight.Value
        #     path = []
        #     maw = ovd.MedialAxisWalk(vd.getGraph())
        #     toolpath = maw.walk()
        #     for chain in toolpath:

        #         path.append(Path.Command("G0 Z{}".format(safeheight)))
        #         p = chain[0][0][0]
        #         z = -(chain[0][0][1])

        #         path.append(Path.Command("G0 X{} Y{} Z{}".format(p.x, p.y, safeheight)))

        #         for step in chain:
        #             for point in step:
        #                 p = point[0]
        #                 z = calculate_depth(-(point[1]))
        #                 path.append(Path.Command("G1 X{} Y{} Z{} F{}".format(p.x, p.y, z, obj.ToolController.HorizFeed.Value)))
#     path.append(Path.Command("G0 Z{}".format(safeheight))) return path pathlist = []
        def getEdges(vd, color=[0]):
            if type(color) == int:
                color = [color]
            geomList = []
            for e in vd.Edges:
                if e.Color not in color:
                    continue
                # geom = e.toGeom(8)
                if e.toGeom(8) is None:
                    continue
                p1 = e.Vertices[0].toGeom(calculate_depth(0-e.getDistances()[0]))
                p2 = e.Vertices[-1].toGeom(calculate_depth(0-e.getDistances()[-1]))
                geomList.append(Part.LineSegment(p1, p2))
                # if individualEdges:
                # name = "e%04d" % i
                # Part.show(Part.Edge(geom), name)
                #geomList.append(Part.Edge(geom))
            if geomList:
                return geomList

        def areConnected(seg1, seg2):
            '''
            Checks if two segments share an endpoint.
            returns a new linesegment if connected or original seg1 if not
            '''
            l1 = [seg1.StartPoint, seg1.EndPoint]
            l2 = [seg2.StartPoint, seg2.EndPoint]
            l3 = [v1 for v1 in l1 for v2 in l2 if PathGeom.pointsCoincide (v1, v2, error=0.01)]
            # for v1 in l1:
            #     for v2 in l2:
            #         if PathGeom.pointsCoincide(v1, v2):
            #             l3.append(v1)
            #l3 = [value for value in l1 if value in l2]
            print('l1: {} l2: {} l3: {}'.format(l1,l2,l3))
            if len(l3) == 0:  # no connection
                print('no connection')
                return seg1
            elif len(l3) == 1:  # extend chain
                print('one vert')
                p1 = l1[0] if l1[0] == l3[0] else l1[1]
                p2 = l2[0] if l2[0] == l3[0] else l2[1]
                return Part.LineSegment(p1, p2)
            else:  # loop
                print('loop')
                return None

        def chains(seglist):
            '''
            iterates through segements and builds a list of chains
            '''

            chains = []
            while len(seglist) > 0:
                cur_seg = seglist.pop(0)
                cur_chain = [cur_seg]
                remaining = []
                tempseg = cur_seg  # tempseg is a linesegment from first vertex to last in curchain
                for i, seg in enumerate(seglist):

                    ac = areConnected(tempseg, seg)
                    if ac != tempseg:
                        cur_chain.append(seg)
                    if ac is None:
                        remaining.extend(seglist[i+1:])
                        break
                    else:
                        tempseg = ac

                    #print("c: {}".format(cur_chain))

                chains.append(cur_chain)
                seglist = remaining

            return chains

        def cutWire(w):
            path = []
            p = w.Vertexes[0]
            path.append(Path.Command("G0 Z{}".format(obj.SafeHeight.Value)))
            path.append(Path.Command("G0 X{} Y{} Z{}".format(p.X, p.Y, obj.SafeHeight.Value)))
            # print('\/ \/ \/')
            # print(p.Point)
            c = Path.Command("G1 X{} Y{} Z{} F{}".format(p.X, p.Y, p.Z, obj.ToolController.HorizFeed.Value))
            # print(c)
            # print('/\ /\ /\ ')
            path.append(c)
            #path.append(Path.Command("G1 X{} Y{} Z{} F{}".format(p.X, p.Y, p.Z, obj.ToolController.HorizFeed.Value)))
            for vert in w.Vertexes[1:]:
                path.append(Path.Command("G1 X{} Y{} Z{} F{}".format(vert.X, vert.Y, vert.Z, obj.ToolController.HorizFeed.Value)))

            path.append(Path.Command("G0 Z{}".format(obj.SafeHeight.Value)))
            return path

        pathlist = []
        pathlist.append(Path.Command("(starting)"))
        for f in Faces:
            vd = Path.Voronoi()
            insert_many_wires(vd, f.Wires)

            vd.construct()
            # vd.colorExterior(1)
            # vd.colorTwins(2)

            for e in vd.Edges:
                e.Color = 0 if e.isPrimary() else 5
            vd.colorExterior(1)
            vd.colorExterior(4, lambda v: not f.isInside(v.toGeom(), 0.01, True))   # should derive tolerance from geometry
            vd.colorColinear(3)
            vd.colorTwins(2)

            edgelist = getEdges(vd)
            # for e in edgelist:
            #     Part.show(e.toShape())

            # for e in [e_ for e_ in vd.Edges if e_.Color == 2]:
            #     print(e.getDistances())
            #     p1 = e.Vertices[0].toGeom(calculate_depth(0-e.getDistances()[0]))
            #     p2 = e.Vertices[-1].toGeom(calculate_depth(0-e.getDistances()[-1]))
            #     edgelist.append(Part.makeLine(p1, p2))

                # vlist = []
                # for v, r in zip(e.Vertices, e.getDistances()):
                #     p = v.toGeom()
                #     p.z = calculate_depth(r)
                #     vlist.append(p)
                # l = Part.makeLine(vlist[0], vlist[-1])
                # edgelist.append(l)

            # for s in Part.sortEdges(edgelist):
            #     pathlist.extend(cutWire(Part.Wire(s)))

            for chain in chains(edgelist):
                print('chain length {}'.format(len(chain)))
                print(chain)
                Part.show(Part.Wire([e.toShape() for e in chain]))

            #pathlist.extend(sortedWires)  # the actual cutting g-code

        self.commandlist = pathlist

    def opExecute(self, obj):
        '''opExecute(obj) ... process engraving operation'''
        PathLog.track()
        # Openvoronoi must be installed

        if obj.ToolController.Tool.ToolType != 'Engraver':
            FreeCAD.Console.PrintError(
                translate("Path_Vcarve", "This operation requires an engraver tool.") + "\n")
            return

        if obj.ToolController.Tool.CuttingEdgeAngle >= 180.0:
            FreeCAD.Console.PrintError(
                translate("Path_Vcarve", "Engraver Cutting Edge Angle must be < 180 degrees.") + "\n")
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
            PathLog.error(translate('PathVcarve', 'The Job Base Object has no engraveable element.  Engraving operation will produce no output.'))

    def opUpdateDepths(self, obj, ignoreErrors=False):
        '''updateDepths(obj) ... engraving is always done at the top most z-value'''
        job = PathUtils.findParentJob(obj)
        self.opSetDefaultValues(obj, job)


def SetupProperties():
    return ["Discretize"]


def Create(name, obj=None):
    '''Create(name) ... Creates and returns a Vcarve operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectVcarve(obj, name)
    return obj
