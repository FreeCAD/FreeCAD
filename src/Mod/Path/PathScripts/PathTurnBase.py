# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Daniel Wood <s.d.wood.82@gmail.com>                *
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
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils
#import PathScripts.PathGeom as PathGeom

from PySide import QtCore

import math
import Draft
if FreeCAD.GuiUp:
    import FreeCADGui

from LibLathe.LLPoint import Point
from LibLathe.LLSegment import Segment

__title__ = "Path Turn Base Operation"
__author__ = "dubstar-04 (Daniel Wood)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Base class implementation for turning operations."

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


class ObjectOp(PathOp.ObjectOp):
    '''Base class for proxy objects of all turning operations.'''

    def opFeatures(self, obj):
        '''opFeatures(obj) ... returns the OR'ed list of features used and supported by the operation.'''
        return PathOp.FeatureDiameters | PathOp.FeatureTool | PathOp.FeatureDepths | PathOp.FeatureCoolant

    def initOperation(self, obj):
        '''initOperation(obj)'''

        obj.addProperty("App::PropertyLength", "StepOver", "Turn Path", translate("TurnPath", "Operation Stepover")).StepOver = 1.0 
        obj.addProperty("App::PropertyInteger", "FinishPasses", "Turn Path", translate("TurnPath", "Number of Finish Passes")).FinishPasses = 2
        obj.addProperty("App::PropertyBool", "AllowGrooving", "Turn Path", translate("TurnPath", "Minimum Diameter for Operation"))
        obj.addProperty("App::PropertyBool", "AllowFacing", "Turn Path", translate("TurnPath", "Minimum Diameter for Operation"))

    def opExecute(self, obj):
        '''opExecute(obj) ... processes all Base features
        '''
        PathLog.track()
        self.tool = None       
        self.minDia = obj.MinDiameter.Value
        self.maxDia = obj.MaxDiameter.Value
        self.startOffset = 0
        self.endOffset = 0
        self.allowGrooving = obj.AllowGrooving
        self.allowFacing = obj.AllowFacing
        self.stepOver = obj.StepOver.Value
        self.finishPasses = obj.FinishPasses

        # Clear any existing gcode
        obj.Path.Commands = []

        print("Process Geometry")     
        self.stock_silhoutte = self.get_stock_silhoutte(obj)  
        self.part_outline = self.get_part_outline()
        self.generate_gcode(obj)

    def getProps(self, obj):
        #TODO: use the start and final depths
        print('getProps - Start Depth: ', obj.OpStartDepth.Value, 'Final Depth: ', obj.OpFinalDepth.Value)

        props = {}
        props['min_dia']=self.minDia
        props['extra_dia']=self.maxDia
        props['start_offset']=self.startOffset
        props['end_offset']=self.endOffset
        props['allow_grooving']=self.allowGrooving
        props['allow_facing']=self.allowFacing
        props['step_over']=self.stepOver
        props['finish_passes']=self.finishPasses
        props['hfeed' ]=obj.ToolController.HorizFeed.Value 
        props['vfeed']=obj.ToolController.VertFeed.Value
        return props

    def get_stock_silhoutte(self, obj):
        '''
        Get Stock Silhoutte
        '''
        stockBB = self.stock.Shape.BoundBox
        stock_z_pos = stockBB.ZMax

        self.startOffset = obj.StartDepth.Value - stockBB.ZMax
        self.endOffset = stockBB.ZMin - obj.FinalDepth.Value 

        stock_plane_length =  obj.StartDepth.Value - obj.FinalDepth.Value
        stock_plane_width = stockBB.XLength/2 - self.minDia + self.maxDia 
        stock_plane = Part.makePlane(stock_plane_length, stock_plane_width, FreeCAD.Vector(-self.minDia - stock_plane_width, 0, stock_z_pos + self.startOffset ), FreeCAD.Vector(0,-1,0))
        return stock_plane

    def get_part_outline(self):
        '''
        Get Part Outline
        '''
        sections=Path.Area().add(self.model[0].Shape).makeSections(mode=0, heights=[0.0],  project=True, plane=self.stock_silhoutte)
        part_silhoutte = sections[0].setParams(Offset=0.0).getShape()
        part_bound_face = sections[0].setParams(Offset=0.1).getShape()
        path_area = self.stock_silhoutte.cut(part_silhoutte)

        part_edges = []
        part_segments = []

        for edge in path_area.Edges:
            edge_in = True
            for vertex in edge.Vertexes:
                if not part_bound_face.isInside(vertex.Point, 0.1, True):
                    edge_in = False
             
            if edge_in: 
                #if self.part_edges.__contains__(edge) == False:
                part_edges.append(edge)
                vert = edge.Vertexes
                pt1 = Point(vert[0].X, vert[0].Y,vert[0].Z)
                pt2 = Point(vert[-1].X,vert[-1].Y,vert[-1].Z)
                seg = Segment(pt1, pt2)
                
                if isinstance(edge.Curve,Part.Circle):
                    #rad = edge.Curve.Radius
                    #angle = 2 * math.asin((seg.get_length()/2) / rad)
                    line1 = Part.makeLine(edge.Curve.Location, edge.Vertexes[0].Point)
                    line2 = Part.makeLine(edge.Curve.Location, edge.Vertexes[-1].Point)
                    part_edges.append(line1)
                    part_edges.append(line2)

                    angle = edge.LastParameter - edge.FirstParameter
                    direction = edge.Curve.Axis.y
                    #print('bulge angle', direction, angle * direction)
                    #TODO: set the correct sign for the bulge +-
                    seg.set_bulge(angle * direction)

                part_segments.append(seg)
                               
        #path_profile = Part.makeCompound(part_edges)
        #Part.show(path_profile, 'Final_pass') 
        return part_segments
  
    def generate_gcode(self, obj):
        '''
        Base function to generate gcode for the OP by writing path command to self.commandlist
        Should be overwritten by subclasses. 
        '''     
        pass
