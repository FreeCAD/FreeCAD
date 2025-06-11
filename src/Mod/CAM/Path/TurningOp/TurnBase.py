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
import Path.Log as PathLog
import Path.Op.Base as PathOp
import PathScripts.PathUtils as PathUtils

from PySide import QtCore

from liblathe.base.point import Point
from liblathe.base.segment import Segment
from liblathe.tool.tool import Tool

if FreeCAD.GuiUp:
    import FreeCADGui

__title__ = "CAM Turning Base Operation"
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
    """Base class for proxy objects of all turning operations."""

    def opFeatures(self, obj):
        """opFeatures(obj) ... returns the OR'ed list of features used and supported by the operation."""
        return (
            PathOp.FeatureDiameters
            | PathOp.FeatureTool
            | PathOp.FeatureDepths
            | PathOp.FeatureCoolant
        )

    def initOperation(self, obj):
        """initOperation(obj)"""

        obj.addProperty(
            "App::PropertyLength",
            "StepOver",
            "Turn Path",
            translate("TurnPath", "Operation Stepover"),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "FinishPasses",
            "Turn Path",
            translate("TurnPath", "Number of Finish Passes"),
        )
        obj.addProperty(
            "App::PropertyFloat",
            "StockToLeave",
            "Turn Path",
            translate("TurnPath", "Distance for stock to leave uncut"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "AllowGrooving",
            "Turn Path",
            translate("TurnPath", "Minimum Diameter for Operation"),
        )

        obj.StepOver = FreeCAD.Units.Quantity(1.0, FreeCAD.Units.Length)
        obj.FinishPasses = 2
        obj.StockToLeave = 0

        # hide properties that are not required for turning operations
        for op in ["OpStartDepth", "OpFinalDepth", "OpToolDiameter", "OpStockZMax", "OpStockZMin"]:
            if hasattr(obj, op):
                obj.setEditorMode(op, 2)  # hide property

    def opExecute(self, obj):
        """opExecute(obj) ... processes all Base features"""
        PathLog.track()
        self.tool = None
        self.minDia = obj.MinDiameter.Value
        self.maxDia = obj.MaxDiameter.Value
        self.startDepth = obj.StartDepth.Value
        self.finalDepth = obj.FinalDepth.Value

        if self.minDia >= self.maxDia:
            raise RuntimeError(
                translate("Turn", "Minimum diameter is equal or greater than maximum diameter")
            )

        if self.minDia < 0 or self.maxDia < 0:
            raise RuntimeError(translate("Turn", "Diamater values must be positive"))

        if obj.StartDepth.Value <= obj.FinalDepth.Value:
            raise RuntimeError(translate("Turn", "Start depth is equal or less than final depth"))

        self.endOffset = 0
        self.allowGrooving = obj.AllowGrooving
        self.stepOver = obj.StepOver.Value
        self.finishPasses = obj.FinishPasses
        self.stockToLeave = obj.StockToLeave

        # Clear any existing gcode
        obj.Path.Commands = []

        print("Process Geometry")
        self.stockPlane = self.getStockPlane()
        self.partOutline = self.getPartOutline()
        self.generateGCode(obj)

    def opSetDefaultValues(self, obj, job):
        obj.OpStartDepth = job.Stock.Shape.BoundBox.ZMax
        obj.OpFinalDepth = job.Stock.Shape.BoundBox.ZMin
        print("opSetDefaultValues:", obj.OpStartDepth.Value, obj.OpFinalDepth.Value)

    def opUpdateDepths(self, obj):
        obj.OpStartDepth = obj.OpStockZMax
        obj.OpFinalDepth = obj.OpStockZMin
        print("opUpdateDepths:", obj.OpStartDepth.Value, obj.OpFinalDepth.Value)

    def getProps(self, obj):
        # print('getProps - Start Depth: ', obj.OpStartDepth.Value, 'Final Depth: ', obj.OpFinalDepth.Value)
        parentJob = PathUtils.findParentJob(obj)

        props = {}
        props["allow_grooving"] = self.allowGrooving
        props["step_over"] = self.stepOver
        props["finish_passes"] = self.finishPasses
        props["stock_to_leave"] = self.stockToLeave
        props["hfeed"] = obj.ToolController.HorizFeed.Value
        props["vfeed"] = obj.ToolController.VertFeed.Value
        props["clearance"] = parentJob.SetupSheet.SafeHeightOffset.Value
        return props

    def getStockPlane(self):
        """
        Get Stock Silhoutte
        """
        stockPlaneLength = self.startDepth - self.finalDepth
        stockPlaneWidth = (self.maxDia - self.minDia) / 2
        stockPlane = Part.makePlane(
            stockPlaneLength,
            stockPlaneWidth,
            FreeCAD.Vector(self.minDia * 0.5, 0, self.finalDepth),
            FreeCAD.Vector(0, 1, 0),
        )
        return stockPlane

    def getPartOutline(self):
        """
        Get Part Outline
        """
        # TODO: Revisit the edge extraction and find a more elegant method
        model = self.model[0].Shape
        # get a section through the part origin on the XZ Plane
        sections = (
            Path.Area()
            .add(model)
            .makeSections(mode=0, heights=[0.0], project=True, plane=self.stockPlane)
        )
        partSection = sections[0].setParams(Offset=0.0).getShape()
        # get an offset section larger than the part section
        partBoundFace = sections[0].setParams(Offset=0.1).getShape()

        # ensure the cutplane is larger than the part or segments will be missed
        modelBB = model.BoundBox
        partPlaneLength = modelBB.ZLength * 1.5
        partPlaneWidth = (modelBB.XLength / 2) * 1.5
        zRef = modelBB.ZMax + (partPlaneLength - modelBB.ZLength) / 2

        # create a plane larger than the part
        cutPlane = Part.makePlane(
            partPlaneLength, partPlaneWidth, FreeCAD.Vector(0, 0, zRef), FreeCAD.Vector(0, -1, 0)
        )
        # Part.show(cutPlane, 'cutPlane')
        # Cut the part section from the cut plane
        partArea = cutPlane.cut(partSection)

        partEdges = []

        # iterate through the edges and check if each is inside the bound_face
        for edge in partArea.Edges:
            for vertex in edge.Vertexes:
                if partBoundFace.isInside(vertex.Point, 0.1, True):
                    partEdges.append(edge)

        # path_profile = Part.makeCompound(partEdges)
        # Part.show(path_profile, 'Final_pass')
        return self.getSegmentsFromEdges(partEdges)

    def getToolShape(self, obj):
        """
        Get Tool Shape
        """
        opTool = obj.ToolController.Tool

        toolShape = opTool.Shape
        toolBB = toolShape.BoundBox
        toolPlane = Part.makePlane(
            toolBB.ZLength,
            toolBB.XLength,
            FreeCAD.Vector(toolBB.XMin, toolBB.Center.y, toolBB.ZMin),
            FreeCAD.Vector(0, -1, 0),
        )
        # get a section through the tool origin on the XZ Plane
        sections = (
            Path.Area()
            .add(toolShape)
            .makeSections(mode=0, heights=[0.0], project=True, plane=toolPlane)
        )
        toolShape = sections[0].setParams(Offset=0.0).getShape()
        toolEdges = toolShape.Edges

        # tool_profile = Part.makeCompound(toolEdges)
        # Part.show(tool_profile, 'Tool_2d')
        return self.getSegmentsFromEdges(toolEdges, True)

    def getSegmentsFromEdges(self, edges, allowOnXAxis=False):
        """Convert part edges to liblathe segments"""
        segments = []

        for edge in edges:
            vert = edge.Vertexes
            # skip edges that are on the X axis
            if allowOnXAxis is False:
                startX = round(vert[0].X)
                endX = round(vert[-1].X)
                if startX == 0 and endX == 0:
                    continue

            pt1 = Point(vert[0].X, vert[0].Z)
            pt2 = Point(vert[-1].X, vert[-1].Z)
            seg = Segment(pt1, pt2)

            if isinstance(edge.Curve, Part.Circle):
                angle = edge.LastParameter - edge.FirstParameter
                direction = edge.Curve.Axis.y
                # print('bulge angle', direction, angle * direction)
                # TODO: set the correct sign for the bulge +-
                seg.setBulge(angle * direction)

            segments.append(seg)

        return segments

    def generateGCode(self, obj):
        """
        Base function to generate gcode for the OP by writing path command to self.commandlist
        Calls operations opGenerateGCode.
        """

        # create a liblathe tool and assign the toolbit segments representing the shape
        turnTool = Tool()
        turnTool.set_tool_from_segments(self.getToolShape(obj))

        self.opGenerateGCode(obj, turnTool)

    def opGenerateGCode(self, obj, turnTool):
        """opGenerateGCode(obj) ... overwrite to set initial default values.
        Called after the receiver has been fully created with all properties.
        Should be overwritten by subclasses."""
        pass  # pylint: disable=unnecessary-pass
