# -*- coding: utf-8 -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import FreeCADGui
import Part
import Path
import Path.Op.Base as OpBase
import Path.Op.Util as PathOpUtil
import PathScripts.PathUtils as PathUtils

from PySide.QtCore import QT_TRANSLATE_NOOP


__title__ = "CAM Path from Shape with Tool Controller"
__author__ = "tarman3"
__inspirer__ = "Russ4262"
__url__ = "https://forum.freecad.org/viewtopic.php?t=93896"
__doc__ = ""


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class ObjectPathShape:
    def __init__(self, obj):
        self.Type = "PathShapeObject"
        self.obj = obj
        obj.Proxy = self
        obj.addProperty(
            "App::PropertyBool",
            "Active",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "Make False, to prevent operation from generating code"
            ),
            locked=True,
        )
        obj.addProperty(
            "App::PropertyString",
            "Comment",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "An optional comment for this operation"),
            locked=True,
        )
        obj.addProperty(
            "App::PropertyString",
            "UserLabel",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "User assigned label"),
            locked=True,
        )
        obj.addProperty(
            "App::PropertyLink",
            "ToolController",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The tool controller that will be used to calculate the path"
            ),
            locked=True,
        )
        obj.addProperty(
            "App::PropertyString",
            "CycleTime",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Operations cycle time estimation"),
            locked=True,
        )
        obj.addProperty(
            "App::PropertyBool",
            "UseMakeOffset2D",
            "Offset",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Use Part.Wire.makeOffset2D() directly instead of Path.Op.Util.offsetWire()",
            ),
        )
        obj.addProperty(
            "App::PropertyBool",
            "UseComp",
            "Offset",
            QT_TRANSLATE_NOOP("App::Property", "Use tool radius compensation"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "InvertSide",
            "Offset",
            QT_TRANSLATE_NOOP("App::Property", "Invert offset drection"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "OffsetExtra",
            "Offset",
            QT_TRANSLATE_NOOP("App::Property", "Offset from shape"),
        )
        obj.Active = True
        obj.setEditorMode("CycleTime", 1)  # Set property read-only
        self.addToolController(obj)
        self.setSafetyZ(obj)

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onDelete(self, obj, args):
        return True

    def onChanged(self, obj, prop):
        return None

    def onDocumentRestored(self, obj):
        return None

    def execute(self, obj):
        offset = obj.OffsetExtra.Value
        if obj.UseComp:
            offset += obj.ToolController.Tool.Diameter.Value / 2
        if obj.InvertSide:
            offset = -offset

        if offset:
            shape = []
            if obj.UseMakeOffset2D:
                for source in obj.Sources:
                    for wire in source.Shape.Wires:
                        shape.append(wire.makeOffset2D(offset, join=1, openResult=True))
            else:
                job = PathUtils.findParentJob(obj)
                base = job.Model.Group[0].Shape
                for source in obj.Sources:
                    for wire in source.Shape.Wires:
                        shape.append(PathOpUtil.offsetWire(wire, base, offset, forward=True))

        else:
            shape = [so.Shape for so in obj.Sources]

        params = {}
        params["shapes"] = shape
        if obj.UseStartPoint:
            params["start"] = obj.StartPoint
        params["return_end"] = False
        params["arc_plane"] = obj.getEnumerationsOfProperty("ArcPlane").index(obj.ArcPlane)
        params["sort_mode"] = obj.getEnumerationsOfProperty("SortMode").index(obj.SortMode)
        params["min_dist"] = obj.MinDistance
        params["abscissa"] = obj.SortAbscissa
        params["nearest_k"] = obj.NearestK
        params["orientation"] = obj.getEnumerationsOfProperty("Orientation").index(obj.Orientation)
        params["direction"] = obj.getEnumerationsOfProperty("Direction").index(obj.Direction)
        params["threshold"] = obj.RetractThreshold
        params["retract_axis"] = obj.getEnumerationsOfProperty("RetractAxis").index(obj.RetractAxis)
        params["retraction"] = obj.Retraction
        params["resume_height"] = obj.ResumeHeight
        params["segmentation"] = obj.Segmentation
        params["feedrate"] = obj.FeedRate / 60
        params["feedrate_v"] = obj.FeedRateVertical / 60
        params["verbose"] = obj.Verbose
        params["abs_center"] = obj.AbsoluteArcCenter
        params["preamble"] = obj.EmitPreamble
        params["deflection"] = obj.Deflection

        obj.Path = Path.fromShapes(**params)

        obj.CycleTime = OpBase.getCycleTimeEstimate(obj)

    # This method must return True and needed for PathUtils.findToolController()
    def isToolSupported(self, obj, tool=None):
        return True

    def addToolController(self, obj):
        toolController = PathUtils.findToolController(obj, None)
        if toolController:
            obj.ToolController = toolController
            obj.FeedRate = obj.ToolController.HorizFeed.Value * 60
            obj.FeedRateVertical = obj.ToolController.VertFeed.Value * 60
        else:
            raise OpBase.PathNoTCException()

    # Set safety height parameters
    def setSafetyZ(self, obj):
        job = PathUtils.findParentJob(obj)
        if job:
            safetyZ = job.Stock.Shape.BoundBox.ZMax + 10
            obj.Retraction = safetyZ
            obj.ResumeHeight = safetyZ


# Geometry for selected shapes
class ObjectPartShape:
    def __init__(self, obj, base):
        self.Type = "PartShapeObject"
        self.obj = obj
        obj.addProperty(
            "App::PropertyLinkSubListGlobal",
            "Base",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "The base geometry for this operation"),
        )
        obj.Base = base

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onDelete(self, obj, args):
        return True

    def onDocumentRestored(self, obj):
        self.obj = obj

    def onChanged(self, obj, prop):
        """onChanged(obj, prop) ... method called when objECT is changed,
        with source propERTY of the change."""
        if "Restore" in obj.State:
            pass

    def execute(self, obj):
        wires = []
        for base in obj.Base:
            edges = []
            (baseObj, subNames) = base
            if not subNames or subNames == ("",):
                subNames = [f"Edge{i[0]+1}" for i in enumerate(baseObj.Shape.Edges)]
            edges.extend(
                [baseObj.Shape.getElement(sub).copy() for sub in subNames if sub.startswith("Edge")]
            )
            for sortedEdges in Part.sortEdges(edges):
                wires.append(Part.Wire(sortedEdges))
        obj.Shape = Part.makeCompound(wires)


class CommandPathShapeTC:
    def GetResources(self):
        return {
            "Pixmap": "CAM_ShapeTC",
            "MenuText": QT_TRANSLATE_NOOP("CAM_PathShapeTC", "Path from Shape TC"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_PathShapeTC", "Creates path from selected shapes with tool controller"
            ),
        }

    def IsActive(self):
        isJob = False
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name.startswith("Job"):
                    isJob = True
                    break
        if isJob:
            selection = FreeCADGui.Selection.getSelectionEx()
            if selection:
                baseObj = selection[0].Object
                subNames = selection[0].SubElementNames
                if subNames and [edge for edge in subNames if "Edge" in edge]:
                    return True
                elif (
                    hasattr(baseObj, "Shape")
                    and hasattr(baseObj.Shape, "Edges")
                    and baseObj.Shape.Edges
                ):
                    return True
        return False

    def Activated(self):
        doc = FreeCAD.ActiveDocument
        selection = FreeCADGui.Selection.getSelectionEx()
        base = []
        for sel in selection:
            baseObj = sel.Object
            subNames = sel.SubElementNames if sel.SubElementNames else ("",)
            base.append([baseObj, subNames])
        shapeObj = doc.addObject("Part::FeaturePython", "PartShape")
        shapeObj.ViewObject.Proxy = 0
        shapeObj.Visibility = False
        shapeObj.Proxy = ObjectPartShape(shapeObj, base)

        pathShapeObj = doc.addObject("Path::FeatureShapePython", "PathShape")
        pathShapeObj.Sources = [shapeObj]
        PathUtils.addToJob(pathShapeObj)
        ObjectPathShape(pathShapeObj)
        doc.recompute()


if FreeCAD.GuiUp:
    # Register the FreeCAD command
    FreeCADGui.addCommand("CAM_PathShapeTC", CommandPathShapeTC())
