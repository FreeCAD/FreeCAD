# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2020 Schildkroet                                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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
import Path
import PathScripts.PathLog as PathLog
import PathScripts.operations.PathOp2 as PathOp
import PathScripts.PathUtils as PathUtils
import PathScripts.strategies.PathStrategyProfile as StrategyProfile
import PathScripts.PathSelectionProcessing as SelectionProcessing

from PySide import QtCore

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")


__title__ = "Path Profile Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = (
    "Path Profile operation based on entire model, selected faces or selected edges."
)


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectPerimeter(PathOp.ObjectOp):
    """Proxy object for Profile operations based on faces."""

    def opFeatures(self, obj):
        """opFeatures(obj) ... returns the base features supported by all Path.Area based operations."""
        return (
            PathOp.FeatureTool
            | PathOp.FeatureHeightsDepths
            | PathOp.FeatureStepDown
            | PathOp.FeatureStartPoint
            | PathOp.FeatureCoolant
            | PathOp.FeatureBaseFaces
            | PathOp.FeatureBasePanels
            | PathOp.FeatureBaseEdges
            | PathOp.FeatureExtensions
        )

    def initOperation(self, obj):
        """initOperation(obj) ... implement to extend class `__init__()` contructor,
        like create additional properties."""
        self.isDebug = True if PathLog.getLevel(PathLog.thisModule()) == 4 else False

    def opShapeForDepths(self, obj, job):
        """opShapeForDepths(obj) ... returns the shape used to make an initial calculation for the depths being used.
        The default implementation returns the job's Base.Shape"""
        if job:
            if job.Stock:
                PathLog.debug(
                    "job=%s base=%s shape=%s" % (job, job.Stock, job.Stock.Shape)
                )
                return job.Stock.Shape
            else:
                PathLog.warning(
                    translate("PathProfile", "job %s has no Base.") % job.Label
                )
        else:
            PathLog.warning(
                translate("PathProfile", "no job for op %s found.") % obj.Label
            )
        return None

    def opSetDefaultValues(self, obj, job):
        """opSetDefaultValues(obj) ... base implementation, do not overwrite.
        The base implementation sets the depths and heights based on the
        opShapeForDepths() return value."""
        PathLog.debug("opSetDefaultValues(%s, %s)" % (obj.Label, job.Label))

        shape = None
        try:
            shape = self.opShapeForDepths(obj, job)
        except Exception as ee:  # pylint: disable=broad-except
            PathLog.error(ee)

        # Set initial start and final depths
        if shape is None:
            PathLog.debug("shape is None")
            startDepth = 1.0
            finalDepth = 0.0
        else:
            bb = job.Stock.Shape.BoundBox
            startDepth = bb.ZMax
            finalDepth = bb.ZMin

        obj.OpStartDepth.Value = startDepth
        obj.OpFinalDepth.Value = finalDepth

    def opPropertyDefinitions(self):
        """opProperties(obj) ... returns a tuples.
        Each tuple contains property declaration information in the
        form of (prototype, name, section, tooltip)."""
        return [
            (
                "App::PropertyEnumeration",
                "CutDirection",
                "PathOptions",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The direction that the toolpath should go around the part: Climb or Conventional.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "JoinType",
                "PathOptions",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Controls how tool moves around corners. Default=Round",
                ),
            ),
            (
                "App::PropertyFloat",
                "MiterLimit",
                "PathOptions",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property", "Maximum distance before a miter join is truncated"
                ),
            ),
            (
                "App::PropertyDistance",
                "MaterialAllowance",
                "PathOptions",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Extra value to stay away from final profile- good for roughing toolpath",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "CutSide",
                "PathOptions",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property", "Side of edge that tool should cut"
                ),
            ),
            (
                "App::PropertyBool",
                "UseComp",
                "PathOptions",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property", "Make True, if using Cutter Radius Compensation"
                ),
            ),
            (
                "App::PropertyEnumeration",
                "HandleMultipleFeatures",
                "SelectionOptions",
                QtCore.QT_TRANSLATE_NOOP(
                    "PathPocket",
                    "Choose how to process multiple Base Geometry features.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "BoundaryShape",
                "SelectionOptions",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property", "Shape to use for calculating Boundary"
                ),
            ),
            (
                "App::PropertyBool",
                "ProcessHoles",
                "SelectionOptions",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property", "Profile holes as well as the outline"
                ),
            ),
            (
                "App::PropertyBool",
                "ProcessPerimeter",
                "SelectionOptions",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile the outline"),
            ),
            (
                "App::PropertyBool",
                "ProcessCircles",
                "SelectionOptions",
                QtCore.QT_TRANSLATE_NOOP("App::Property", "Profile round holes"),
            ),
            (
                "App::PropertyString",
                "AreaParams",
                "Debug",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Debug property that stores parameters passed to Path.Area() for this operation.",
                ),
            ),
            (
                "App::PropertyString",
                "PathParams",
                "Debug",
                QtCore.QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Debug property that stores parameters passed to Path.fromShapes() for this operation.",
                ),
            ),
        ]

    def opPropertyEnumerations(self):
        """opPropertyEnumerations() ... returns a dictionary of enumeration lists
        for the operation's enumeration type properties."""
        # Enumeration lists for App::PropertyEnumeration properties
        return {
            "CutDirection": [
                "Climb",
                "Conventional",
            ],  # this is the direction that the profile runs
            "HandleMultipleFeatures": ["Collectively", "Individually"],
            "BoundaryShape": ["Boundbox", "Face Region", "Perimeter", "Stock"],
            "JoinType": [
                "Round",
                "Square",
                "Miter",
            ],  # this is the direction that the Profile runs
            "CutSide": [
                "Outside",
                "Inside",
            ],  # side of profile that cutter is on in relation to direction of profile
        }

    def opPropertyDefaults(self, obj, job):
        """opPropertyDefaults(obj, job) ... returns a dictionary of default values
        for the operation's properties."""
        return {
            "CutDirection": "Conventional",
            "HandleMultipleFeatures": "Collectively",
            "BoundaryShape": "Face Region",
            "JoinType": "Round",
            "MiterLimit": 0.1,
            "MaterialAllowance": 0.0,
            "CutSide": "Outside",
            "UseComp": True,
            "ProcessCircles": False,
            "ProcessHoles": False,
            "ProcessPerimeter": True,
        }

    def opSetEditorModes(self, obj):
        """opSetEditorModes(obj, porp) ... Process operation-specific changes to properties visibility."""

        # Always hidden
        obj.setEditorMode("AreaParams", 2)  # hide
        obj.setEditorMode("PathParams", 2)  # hide
        obj.setEditorMode("JoinType", 2)
        obj.setEditorMode("MiterLimit", 2)  # ml

    def opUpdateDepths(self, obj):
        if hasattr(obj, "Base") and len(obj.Base) == 0:
            obj.OpStartDepth = obj.OpStockZMax
            obj.OpFinalDepth = obj.OpStockZMin

    def opOnDocumentRestored(self, obj):
        """opOnDocumentRestored(obj) ... implement if an op needs special handling."""
        self.isDebug = True if PathLog.getLevel(PathLog.thisModule()) == 4 else False

    def opExecute(self, obj, getsim=False):  # pylint: disable=arguments-differ
        """opExecute(obj, getsim=False) ... implementation of Path.Area ops.
        determines the parameters for _buildPathArea().
        """
        PathLog.track()

        # Instantiate class variables for operation reference
        self.endVector = None  # pylint: disable=attribute-defined-outside-init
        opFeatures = self.opFeatures(obj)
        startPoint = None
        opUseProjection = True
        opRetractTool = True

        # Initiate depthparams and calculate operation heights for operation
        finish_step = obj.FinishDepth.Value if hasattr(obj, "FinishDepth") else 0.0
        self.depthparams = PathUtils.depth_params(
            clearance_height=obj.ClearanceHeight.Value,
            safe_height=obj.SafeHeight.Value,
            start_depth=obj.StartDepth.Value,
            step_down=obj.StepDown.Value,
            z_finish_step=finish_step,
            final_depth=obj.FinalDepth.Value,
            user_depths=None,
        )

        # Set start point
        if obj.UseStartPoint:
            startPoint = obj.StartPoint

        if obj.UseComp:
            self.commandlist.append(
                Path.Command(
                    "(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")"
                )
            )
        else:
            self.commandlist.append(Path.Command("(Uncompensated Tool Path)"))

        # Identify working shapes for Profile operation
        shapes = self.getTargetShape(obj)  # pylint: disable=assignment-from-no-return

        # Adjust tuples length received from other PathWB tools/operations
        if len(shapes) > 1:
            jobs = list()
            for s in shapes:
                if s[2] == "OpenEdge":
                    shp = Part.makeCompound(s[0])
                else:
                    shp = s[0]
                jobs.append(
                    {"x": shp.BoundBox.XMax, "y": shp.BoundBox.YMax, "shape": s}
                )

            jobs = PathUtils.sort_jobs(jobs, ["x", "y"])

            shapes = [j["shape"] for j in jobs]

        sims = []
        for shape, isHole, sub in shapes:
            if sub == "OpenEdge":
                strategy = StrategyProfile.StrategyProfileOpenEdge(
                    shape,
                    startPoint,
                    self.depthparams,
                    self.horizFeed,
                    self.vertFeed,
                    self.endVector,
                    self.radius,
                    obj.SafeHeight.Value,
                    obj.ClearanceHeight.Value,
                    obj.CutDirection,
                )
            else:
                strategy = StrategyProfile.StrategyProfile(
                    shape,
                    isHole,
                    startPoint,
                    getsim,
                    self.depthparams,
                    self.horizFeed,
                    self.vertFeed,
                    self.endVector,
                    self.radius,
                    opFeatures,
                    obj.SafeHeight.Value,
                    obj.ClearanceHeight.Value,
                    obj.MaterialAllowance.Value,
                    obj.CutDirection,
                    obj.CutSide,
                    obj.UseComp,
                    obj.JoinType,
                    obj.MiterLimit,
                    opUseProjection,
                    opRetractTool,
                )

            try:
                # Generate the path commands
                strategy.generateCommands()

                if sub == "OpenEdge":
                    if obj.UseStartPoint:
                        osp = obj.StartPoint
                        self.commandlist.append(
                            Path.Command(
                                "G0", {"X": osp.x, "Y": osp.y, "F": self.horizRapid}
                            )
                        )
                self.endVector = strategy.endVector
                # Save gcode commands to object command list
                self.commandlist.extend(strategy.commandList)
                if getsim:
                    sims.append(strategy.simObj)
                obj.PathParams = strategy.pathParams  # save path parameters
                obj.AreaParams = strategy.areaParams  # save area parameters
            except Exception as e:  # pylint: disable=broad-except
                FreeCAD.Console.PrintError(e)
                FreeCAD.Console.PrintError(
                    "Something unexpected happened. Check project and tool config."
                )

            if self.endVector is not None and len(self.commandlist) > 1:
                self.endVector[2] = obj.ClearanceHeight.Value
                self.commandlist.append(
                    Path.Command(
                        "G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid}
                    )
                )

        PathLog.debug("obj.Name: " + str(obj.Name) + "\n\n")
        if shapes:
            # Save working shapes to operation's removalshape attribute
            targetShapes = list()
            for shp, __, __ in shapes:
                if isinstance(shp, list):
                    targetShapes.extend(shp)
                else:
                    targetShapes.append(shp)
            obj.TargetShape = Part.makeCompound(targetShapes)

        return sims

    def getTargetShape(self, obj, isPreview=False):
        """getTargetShape(obj) ... returns envelope for all base shapes or wires for Arch.Panels."""
        PathLog.track()

        shapes = []
        baseDataExists = True if obj.Base and len(obj.Base) > 0 else False
        offsetExtra = obj.MaterialAllowance.Value
        baseObjList = None
        extensions = None

        self._setMisingClassVariables(
            obj
        )  # declares missing class attributes for working shape previews in viewport
        self.isDebug = True if PathLog.getLevel(PathLog.thisModule()) == 4 else False

        useToolComp = False
        offsetRadius = offsetExtra
        openEdgeToolRadius = self.radius
        if obj.CutSide == "Inside":
            openEdgeToolRadius *= -1.0

        if obj.UseComp:
            useToolComp = True
            offsetRadius = self.radius + offsetExtra

        if baseDataExists:
            baseObjList = obj.Base
            extensions = PathOp.PathFeatureExtensions.getExtensions(obj)
        else:
            baseObjList = [(base, list()) for base in self.model]
            PathLog.debug("Processing entire model")

        # Process user inputs via Base Geometry and Extensions into working areas
        pac = SelectionProcessing.Working2DAreas(
            baseObjList,
            extensions,
            obj.ProcessPerimeter,
            obj.ProcessHoles,
            obj.ProcessCircles,
            obj.HandleMultipleFeatures,
            obj.BoundaryShape,
            stockShape=self.job.Stock.Shape,
            finalDepth=obj.FinalDepth.Value,
        )
        pac.isDebug = self.isDebug
        pac.showDebugShapes = obj.ShowDebugShapes
        workingAreas = pac.getWorkingAreas(avoidOverhead=True)
        workingHoles = pac.getHoleAreas()
        openEdges = pac.getOpenEdges(
            toolRadius=openEdgeToolRadius,
            offsetRadius=offsetRadius,
            useToolComp=useToolComp,
            jobTolerance=self.job.GeometryTolerance.Value,
            jobLabel=self.job.Label,
        )

        # Move working shapes to FinalDepth and create envelopes as needed
        if workingAreas:
            PathLog.debug("workingAreas count: {}".format(len(workingAreas)))
            for f in workingAreas:
                f.translate(FreeCAD.Vector(0.0, 0.0, obj.FinalDepth.Value))
                env = PathUtils.getEnvelope(f, depthparams=self.depthparams)
                shapes.append((env, False, "pathProfile"))

        if workingHoles:
            PathLog.debug("workingHoles count: {}".format(len(workingHoles)))
            for f in workingHoles:
                f.translate(FreeCAD.Vector(0.0, 0.0, obj.FinalDepth.Value))
                env = PathUtils.getEnvelope(f, depthparams=self.depthparams)
                shapes.append((env, True, "pathProfile"))

        if openEdges:
            for wire in openEdges:
                wire.translate(
                    FreeCAD.Vector(0.0, 0.0, obj.FinalDepth.Value - wire.BoundBox.ZMin)
                )
            shapes.append((openEdges, False, "OpenEdge"))

        self.removalshapes = shapes  # pylint: disable=attribute-defined-outside-init
        PathLog.debug("%d shapes" % len(shapes))

        return shapes


# Eclass


def SetupProperties():
    setup = list()
    # setup.extend(PathOp.PathFeatureExtensions.SetupProperties())
    setup.extend([tup[1] for tup in ObjectPerimeter.opPropertyDefinitions(None)])
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Profile based on faces operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectPerimeter(obj, name, parentJob)
    return obj
