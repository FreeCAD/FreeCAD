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
import PathScripts.PathUtils as PathUtils

from PySide.QtCore import QT_TRANSLATE_NOOP

import math
import time

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
        self.Type = "ShapeObject"
        self.obj = obj
        obj.Proxy = self
        obj.addProperty(
            "App::PropertyBool",
            "Active",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "Make False, to prevent operation from generating code"
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "Comment",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "An optional comment for this Operation"),
        )
        obj.addProperty(
            "App::PropertyString",
            "UserLabel",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "User Assigned Label"),
        )
        obj.addProperty(
            "App::PropertyLink",
            "ToolController",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The tool controller that will be used to calculate the path"
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "CycleTime",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Operations Cycle Time Estimation"),
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
        shapes = [so.Shape for so in obj.Sources]
        start = obj.StartPoint
        return_end = False
        arc_plane = obj.getEnumerationsOfProperty("ArcPlane").index(obj.ArcPlane)
        sort_mode = obj.getEnumerationsOfProperty("SortMode").index(obj.SortMode)
        min_dist = obj.MinDistance
        abscissa = obj.SortAbscissa
        nearest_k = obj.NearestK
        orientation = obj.getEnumerationsOfProperty("Orientation").index(obj.Orientation)
        direction = obj.getEnumerationsOfProperty("Direction").index(obj.Direction)
        threshold = obj.RetractThreshold
        retract_axis = obj.getEnumerationsOfProperty("RetractAxis").index(obj.RetractAxis)
        retraction = obj.Retraction
        resume_height = obj.ResumeHeight
        segmentation = obj.Segmentation
        feedrate = obj.FeedRate
        feedrate_v = obj.FeedRateVertical
        verbose = obj.Verbose
        abs_center = obj.AbsoluteArcCenter
        preamble = obj.EmitPreamble
        deflection = obj.Deflection
        if obj.UseStartPoint:
            obj.Path = Path.fromShapes(
                shapes,
                start=start,
                return_end=return_end,
                sort_mode=sort_mode,
                min_dist=min_dist,
                abscissa=abscissa,
                nearest_k=nearest_k,
                orientation=orientation,
                direction=direction,
                threshold=threshold,
                retract_axis=retract_axis,
                retraction=retraction,
                resume_height=resume_height,
                segmentation=segmentation,
                feedrate=feedrate,
                feedrate_v=feedrate_v,
                verbose=verbose,
                abs_center=abs_center,
                preamble=preamble,
                deflection=deflection,
            )
        else:
            obj.Path = Path.fromShapes(
                shapes,
                return_end=return_end,
                sort_mode=sort_mode,
                min_dist=min_dist,
                abscissa=abscissa,
                nearest_k=nearest_k,
                orientation=orientation,
                direction=direction,
                threshold=threshold,
                retract_axis=retract_axis,
                retraction=retraction,
                resume_height=resume_height,
                segmentation=segmentation,
                feedrate=feedrate,
                feedrate_v=feedrate_v,
                verbose=verbose,
                abs_center=abs_center,
                preamble=preamble,
                deflection=deflection,
            )
        obj.CycleTime = OpBase.getCycleTimeEstimate(obj)

    # This method must return True and needed for PathUtils.findToolController()
    def isToolSupported(self, obj, tool=None):
        return True

    def addToolController(self, obj):
        obj.ToolController = PathUtils.findToolController(obj, None)
        if not obj.ToolController:
            raise OpBase.PathNoTCException()
        obj.FeedRate = obj.ToolController.HorizFeed.Value
        obj.FeedRateVertical = obj.ToolController.VertFeed.Value

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
        self.obj = obj
        obj.addProperty(
            "App::PropertyLinkSubListGlobal",
            "Base",
            "Path",
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
        (baseObj, subNames) = obj.Base[0]
        edges = [baseObj.Shape.getElement(sub).copy() for sub in subNames if sub.startswith("Edge")]
        obj.Shape = Part.Wire(Part.__sortEdges__(edges))


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
                subBase = selection[0].SubElementNames
                if subBase and [edge for edge in subBase if "Edge" in edge]:
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
        if selection:
            baseObj = selection[0].Object
            subBase = selection[0].SubElementNames
            if not subBase:
                subBase = [f"Edge{i[0]+1}" for i in enumerate(baseObj.Shape.Edges)]
            base = [baseObj, subBase]
            shapeObj = doc.addObject("Part::FeaturePython", "PartShape")
            shapeObj.ViewObject.Proxy = 0
            shapeObj.Visibility = False
            selection[0].Object.Visibility = False
            shapeObj.Proxy = ObjectPartShape(shapeObj, base)

        pathShapeObj = doc.addObject("Path::FeatureShapePython", "PathShape")
        pathShapeObj.Sources = [shapeObj]
        PathUtils.addToJob(pathShapeObj)
        ObjectPathShape(pathShapeObj)
        doc.recompute()


if FreeCAD.GuiUp:
    # Register the FreeCAD command
    FreeCADGui.addCommand("CAM_PathShapeTC", CommandPathShapeTC())
