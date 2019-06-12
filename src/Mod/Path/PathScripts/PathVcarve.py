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
import FreeCAD
import Part
import Path
import PathScripts.PathEngraveBase as PathEngraveBase
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils
import traceback
import time
import PathScripts.PathGeom as pg
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
        return PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureHeights | PathOp.FeatureStepDown | PathOp.FeatureBaseFaces;

    def setupAdditionalProperties(self, obj):
        if not hasattr(obj, 'BaseShapes'):
            obj.addProperty("App::PropertyLinkList", "BaseShapes", "Path", QtCore.QT_TRANSLATE_NOOP("PathVcarve", "Additional base objects to be engraved"))
        obj.setEditorMode('BaseShapes', 2) # hide
        if not hasattr(obj, 'BaseObject'):
            obj.addProperty("App::PropertyLink", "BaseObject", "Path", QtCore.QT_TRANSLATE_NOOP("PathVcarve", "Additional base objects to be engraved"))
        obj.setEditorMode('BaseObject', 2) # hide

    def initOperation(self, obj):
        '''initOperation(obj) ... create vcarve specific properties.'''
        obj.addProperty("App::PropertyFloat", "Discretize", "Path", QtCore.QT_TRANSLATE_NOOP("PathVcarve", "The deflection value for discretizing arcs"))
        self.setupAdditionalProperties(obj)

    def opOnDocumentRestored(self, obj):
        # upgrade ...
        self.setupAdditionalProperties(obj)


    def buildPathMedial(self, obj, Faces, zDepths, unitcircle):
        '''constructs a medial axis path using openvoronoi'''
        import openvoronoi as ovd

        def insert_wire_points(vd, wire):
            pts=[]
            for p in wire.Vertexes:
                pts.append( ovd.Point( p.X, p.Y ) )
                print('p1 = FreeCAD.Vector(X:{} Y:{}'.format(p.X, p.Y))
            id_list = []
            print("inserting ",len(pts)," point-sites:")
            for p in pts:
                id_list.append( vd.addVertexSite( p ) )
            return id_list

        def insert_wire_segments(vd,id_list):
            print('insert_polygon-segments')
            print('inserting {} segments'.format(len(id_list)))
            for n in range(len(id_list)):
                n_nxt = n+1
                if n==(len(id_list)-1):
                    n_nxt=0
                vd.addLineSite( id_list[n], id_list[n_nxt])

        def insert_many_wires(vd, wires):
            # print('inserting {} wires'.format(len(obj.Wires)))
            polygon_ids =[]
            t_before = time.time()
            for idx, wire in enumerate(wires):
                print('discretize: {}'.format(obj.Discretize))
                pointList = wire.discretize(Deflection=obj.Discretize)
                segwire = Part.Wire([Part.makeLine(p[0],p[1]) for p in zip(pointList, pointList[1:] )])

                if idx == 0:
                    segwire = orientWire(segwire, forward=False)
                else:
                    segwire = orientWire(segwire, forward=True)

                poly_id = insert_wire_points(vd,segwire)
                polygon_ids.append(poly_id)
            t_after = time.time()
            pt_time = t_after-t_before

            t_before = time.time()
            for ids in polygon_ids:
                insert_wire_segments(vd,ids)
            t_after = time.time()
            seg_time = t_after-t_before
            return [pt_time, seg_time]

        def calculate_depth(MIC):
            # given a maximum inscribed circle (MIC) and tool angle,
            # return depth of cut.
            toolangle = obj.ToolController.Tool.CuttingEdgeAngle
            return MIC / math.tan(math.radians(toolangle/2))

        def buildMedial(vd):
            safeheight = obj.SafeHeight.Value
            path = []
            maw = ovd.MedialAxisWalk(  vd.getGraph() )
            toolpath = maw.walk()
            for chain in toolpath:
                path.append(Path.Command("G0 Z{}".format(safeheight)))
                p = chain[0][0][0]
                z = -(chain[0][0][1])

                path.append(Path.Command("G0 X{} Y{} Z{}".format(p.x, p.y, safeheight)))

                for step in chain:
                    for point in step:
                        p = point[0]
                        z = calculate_depth(-(point[1]))
                        path.append(Path.Command("G1 X{} Y{} Z{} F{}".format(p.x, p.y, z, obj.ToolController.HorizFeed.Value)))

            path.append(Path.Command("G0 Z{}".format(safeheight)))

            return path

        pathlist = []
        bins = 120 # int bins = number of bins for grid-search (affects performance, should not affect correctness)
        for f in Faces:
            #unitcircle = f.BoundBox.DiagonalLength/2
            print('unitcircle: {}'.format(unitcircle))
            vd = ovd.VoronoiDiagram(200, bins)
            vd.set_silent(True) # suppress Warnings!
            wires = f.Wires
            insert_many_wires(vd, wires)
            pi = ovd.PolygonInterior(  True )
            vd.filter_graph(pi)
            ma = ovd.MedialAxis()
            vd.filter_graph(ma)
            pathlist.extend(buildMedial( vd )) # the actual cutting g-code

        self.commandlist = pathlist



    def opExecute(self, obj):
        '''opExecute(obj) ... process engraving operation'''
        PathLog.track()
        # Openvoronoi must be installed
        try:
            import openvoronoi as ovd
        except:
            FreeCAD.Console.PrintError(
                translate("Path_Vcarve", "This operation requires OpenVoronoi to be installed.") + "\n")
            return


        job = PathUtils.findParentJob(obj)

        jobshapes = []
        zValues = self.getZValues(obj)

        if obj.ToolController.Tool.ToolType != 'Engraver':
            FreeCAD.Console.PrintError(
                translate("Path_Vcarve", "This operation requires an engraver tool.") + "\n")
            return

        if obj.ToolController.Tool.CuttingEdgeAngle >= 180.0:
            FreeCAD.Console.PrintError(
                translate("Path_Vcarve", "Engraver Cutting Edge Angle must be < 180 degrees.") + "\n")
            return
        try:
            if len(self.model) == 1 and self.model[0].isDerivedFrom('Sketcher::SketchObject') or \
                    self.model[0].isDerivedFrom('Part::Part2DObject'):
                PathLog.track()

                # self.commandlist.append(Path.Command('G0', {'Z': obj.ClearanceHeight.Value, 'F': self.vertRapid}))

                # we only consider the outer wire if this is a Face
                modelshape = self.model[0].Shape
                modelshape.tessellate(0.01)
                self.buildPathMedial(obj, modelshape.Faces, zValues, modelshape.BoundBox.DiagonalLength/2)
                # self.wires = wires

        #     elif obj.Base:
        #         PathLog.track()
        #         wires = []
        #         for base, subs in obj.Base:
        #             edges = []
        #             basewires = []
        #             for feature in subs:
        #                 sub = base.Shape.getElement(feature)
        #                 if type(sub) == Part.Edge:
        #                     edges.append(sub)
        #                 elif sub.Wires:
        #                     basewires.extend(sub.Wires)
        #                 else:
        #                     basewires.append(Part.Wire(sub.Edges))

        #             for edgelist in Part.sortEdges(edges):
        #                 basewires.append(Part.Wire(edgelist))

        #             wires.extend(basewires)
        #         self.buildpathocc(obj, wires, zValues)
        #         self.wires = wires
        #     elif not obj.BaseShapes:
        #         PathLog.track()
        #         if not obj.Base and not obj.BaseShapes:
        #             for base in self.model:
        #                 PathLog.track(base.Label)
        #                 if base.isDerivedFrom('Part::Part2DObject'):
        #                     jobshapes.append(base)

        #         if not jobshapes:
        #             raise ValueError(translate('PathVcarve', "Unknown baseobject type for engraving (%s)") % (obj.Base))

        #     if obj.BaseShapes or jobshapes:
        #         PathLog.track()
        #         wires = []
        #         for shape in obj.BaseShapes + jobshapes:
        #             PathLog.track(shape.Label)
        #             shapeWires = shape.Shape.Wires
        #             self.buildpathocc(obj, shapeWires, zValues)
        #             wires.extend(shapeWires)
        #         self.wires = wires
        #     # the last command is a move to clearance, which is automatically added by PathOp
        #     if self.commandlist:
        #         self.commandlist.pop()

        except Exception as e:
            PathLog.error(e)
            traceback.print_exc()
            PathLog.error(translate('PathVcarve', 'The Job Base Object has no engraveable element.  Engraving operation will produce no output.'))

    def opUpdateDepths(self, obj, ignoreErrors=False):
        '''updateDepths(obj) ... engraving is always done at the top most z-value'''
        job = PathUtils.findParentJob(obj)
        self.opSetDefaultValues(obj, job)

def SetupProperties():
    return [ "Discretize" ]

def Create(name, obj = None):
    '''Create(name) ... Creates and returns a Vcarve operation.'''
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectVcarve(obj, name)
    return obj

