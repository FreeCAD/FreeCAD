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

import Draft
import FreeCAD
import FreeCADGui
import Part
import Path
import Path.Op.Base as OpBase
import PathScripts.PathUtils as PathUtils

from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "CAM Path from Shape with Tool Controller"
__author__ = ""
__inspirer__ = "Russ4262"
__url__ = "https://forum.freecad.org/viewtopic.php?t=93896"
__doc__ = ""


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


# Add base set of operation properties
def _addBaseProperties(obj):
    obj.addProperty(
        "App::PropertyBool",
        "Active",
        "Path",
        QT_TRANSLATE_NOOP("App::Property", "Make False, to prevent operation from generating code"),
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
        "App::PropertyString",
        "CycleTime",
        "Path",
        QT_TRANSLATE_NOOP("App::Property", "Operations cycle time estimation"),
        locked=True,
    )
    obj.setEditorMode("CycleTime", 1)  # Set property read-only
    obj.Active = True


# Add ToolController properties
def _addToolController(obj):
    obj.addProperty(
        "App::PropertyLink",
        "ToolController",
        "Path",
        QT_TRANSLATE_NOOP(
            "App::Property",
            "The tool controller that will be used to calculate the path",
        ),
    )
    obj.addProperty(
        "App::PropertyDistance",
        "OpToolDiameter",
        "Op Values",
        QT_TRANSLATE_NOOP("App::Property", "Holds the diameter of the tool"),
    )
    obj.setEditorMode("OpToolDiameter", 1)  # Set property read-only
    obj.ToolController = PathUtils.findToolController(obj, None)
    if not obj.ToolController:
        raise OpBase.PathNoTCException()
    obj.OpToolDiameter = obj.ToolController.Tool.Diameter

    obj.FeedRate = obj.ToolController.HorizFeed.Value
    obj.FeedRateVertical = obj.ToolController.VertFeed.Value


# Get list of tool controllers
def _getToolControllers(obj, proxy=None):
    # Modified getToolControllers() from PathScripts.PathUtils
    # for Path object without Proxy
    job = PathUtils.findParentJob(obj)
    if job:
        return [tc for tc in job.Tools.Group]
    else:
        return []


# Set safety height parameters for Path operation
def _setSafetyZ(obj):
    job = PathUtils.findParentJob(obj)
    if job:
        safetyZ = job.Stock.Shape.BoundBox.ZMax + 10
        obj.RetractThreshold = safetyZ
        obj.Retraction = safetyZ
        obj.ResumeHeight = safetyZ


# Geometry for selected shapes
class ObjectPartShape:
    def __init__(self, obj, base):
        # Path.Log.info("ObjectPartShape.__init__()")
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
        edges = []
        if obj.Base:
            (base, subNames) = obj.Base[0]
            edges = [
                base.Shape.getElement(sub).copy() for sub in subNames if sub.startswith("Edge")
            ]

        if edges:
            obj.Shape = Part.Wire(Part.__sortEdges__(edges))
        else:
            obj.Shape = Part.Shape()


class CommandPathShapeTC:
    def GetResources(self):
        return {
            "Pixmap": "CAM_ShapeTC",
            "MenuText": QT_TRANSLATE_NOOP("CAM_PathShapeTC", "Path From Shape TC"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_PathShapeTC",
                "Creates a path from the selected shapes with the tool controller",
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
                base = selection[0].Object
                subBase = selection[0].SubElementNames
                if subBase and [edge for edge in subBase if "Edge" in edge]:
                    return True
                elif base.Shape.ShapeType in ["Wire", "Edge"]:
                    return True
        return False

    def Activated(self):
        print("Create PathShape object with Tool Controller")
        doc = FreeCAD.ActiveDocument
        selection = FreeCADGui.Selection.getSelectionEx()
        shapeObj = None
        if selection:
            base = selection[0].Object
            subBase = selection[0].SubElementNames
            if subBase:
                subEdges = [edge for edge in subBase if "Edge" in edge]
                shapeObj = doc.addObject("Part::FeaturePython", "PartShape")
                shapeObj.ViewObject.Proxy = 0
                shapeObj.Visibility = False
                shapeObj.Proxy = ObjectPartShape(shapeObj, [(base, subEdges)])
            elif base.Shape.ShapeType in ["Wire", "Edge"]:
                shapeObj = Draft.make_clone(base)

        pathObj = doc.addObject("Path::FeatureShape", "PathShape")
        pathObj.Sources = [shapeObj]

        # Overwrite getToolControllers() function with modified version
        PathUtils.getToolControllers = _getToolControllers

        PathUtils.addToJob(pathObj)
        _addBaseProperties(pathObj)
        _addToolController(pathObj)
        _setSafetyZ(pathObj)
        doc.recompute()


if FreeCAD.GuiUp:
    # Register the FreeCAD command
    FreeCADGui.addCommand("CAM_PathShapeTC", CommandPathShapeTC())
