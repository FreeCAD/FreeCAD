# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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

from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils


# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Draft = LazyLoader("Draft", globals(), "Draft")
Part = LazyLoader("Part", globals(), "Part")
PathGeom = LazyLoader("PathScripts.PathGeom", globals(), "PathScripts.PathGeom")


__title__ = "Base class for PathArea based operations."
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Base class and properties for Path.Area based operations."
__contributors__ = "russ4262 (Russell Johnson)"


if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

translate = FreeCAD.Qt.translate


class ObjectOp(PathOp.ObjectOp):
    """Base class for all Path.Area based operations.
    Provides standard features including debugging properties AreaParams,
    PathParams and removalshape, all hidden.
    The main reason for existence is to implement the standard interface
    to Path.Area so subclasses only have to provide the shapes for the
    operations."""

    def opFeatures(self, obj):
        """opFeatures(obj) ... returns the base features supported by all Path.Area based operations.
        The standard feature list is OR'ed with the return value of areaOpFeatures().
        Do not overwrite, implement areaOpFeatures(obj) instead."""
        return (
            PathOp.FeatureTool
            | PathOp.FeatureDepths
            | PathOp.FeatureStepDown
            | PathOp.FeatureHeights
            | PathOp.FeatureStartPoint
            | self.areaOpFeatures(obj)
            | PathOp.FeatureCoolant
        )

    def areaOpFeatures(self, obj):
        """areaOpFeatures(obj) ... overwrite to add operation specific features.
        Can safely be overwritten by subclasses."""
        return 0

    def initOperation(self, obj):
        """initOperation(obj) ... sets up standard Path.Area properties and calls initAreaOp().
        Do not overwrite, overwrite initAreaOp(obj) instead."""
        PathLog.track()

        # Debugging
        obj.addProperty("App::PropertyString", "AreaParams", "Path")
        obj.setEditorMode("AreaParams", 2)  # hide
        obj.addProperty("App::PropertyString", "PathParams", "Path")
        obj.setEditorMode("PathParams", 2)  # hide
        obj.addProperty("Part::PropertyPartShape", "removalshape", "Path")
        obj.setEditorMode("removalshape", 2)  # hide

        obj.addProperty(
            "App::PropertyBool",
            "SplitArcs",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Split Arcs into discrete segments"),
        )

        # obj.Proxy = self

        self.initAreaOp(obj)

    def initAreaOp(self, obj):
        """initAreaOp(obj) ... overwrite if the receiver class needs initialisation.
        Can safely be overwritten by subclasses."""
        pass

    def areaOpShapeForDepths(self, obj, job):
        """areaOpShapeForDepths(obj) ... returns the shape used to make an initial calculation for the depths being used.
        The default implementation returns the job's Base.Shape"""
        if job:
            if job.Stock:
                PathLog.debug(
                    "job=%s base=%s shape=%s" % (job, job.Stock, job.Stock.Shape)
                )
                return job.Stock.Shape
            else:
                PathLog.warning(
                    translate("PathAreaOp", "job %s has no Base.") % job.Label
                )
        else:
            PathLog.warning(
                translate("PathAreaOp", "no job for op %s found.") % obj.Label
            )
        return None

    def areaOpOnChanged(self, obj, prop):
        """areaOpOnChanged(obj, porp) ... overwrite to process operation specific changes to properties.
        Can safely be overwritten by subclasses."""
        pass

    def opOnChanged(self, obj, prop):
        """opOnChanged(obj, prop) ... base implementation of the notification framework - do not overwrite.
        The base implementation takes a stab at determining Heights and Depths if the operations's Base
        changes.
        Do not overwrite, overwrite areaOpOnChanged(obj, prop) instead."""
        # PathLog.track(obj.Label, prop)
        if prop in ["AreaParams", "PathParams", "removalshape"]:
            obj.setEditorMode(prop, 2)

        if prop == "Base" and len(obj.Base) == 1:
            (base, sub) = obj.Base[0]
            bb = base.Shape.BoundBox  # parent boundbox
            subobj = base.Shape.getElement(sub[0])
            fbb = subobj.BoundBox  # feature boundbox

            if hasattr(obj, "Side"):
                if bb.XLength == fbb.XLength and bb.YLength == fbb.YLength:
                    obj.Side = "Outside"
                else:
                    obj.Side = "Inside"

        self.areaOpOnChanged(obj, prop)

    def opOnDocumentRestored(self, obj):
        PathLog.track()
        for prop in ["AreaParams", "PathParams", "removalshape"]:
            if hasattr(obj, prop):
                obj.setEditorMode(prop, 2)
        if not hasattr(obj, "SplitArcs"):
            obj.addProperty(
                "App::PropertyBool",
                "SplitArcs",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Split Arcs into discrete segments"),
            )

        self.areaOpOnDocumentRestored(obj)

    def areaOpOnDocumentRestored(self, obj):
        """areaOpOnDocumentRestored(obj) ... overwrite to fully restore receiver"""
        pass

    def opSetDefaultValues(self, obj, job):
        """opSetDefaultValues(obj) ... base implementation, do not overwrite.
        The base implementation sets the depths and heights based on the
        areaOpShapeForDepths() return value.
        Do not overwrite, overwrite areaOpSetDefaultValues(obj, job) instead."""
        PathLog.debug("opSetDefaultValues(%s, %s)" % (obj.Label, job.Label))

        if PathOp.FeatureDepths & self.opFeatures(obj):
            try:
                shape = self.areaOpShapeForDepths(obj, job)
            except Exception as ee:
                PathLog.error(ee)
                shape = None

            # Set initial start and final depths
            if shape is None:
                PathLog.debug("shape is None")
                startDepth = 1.0
                finalDepth = 0.0
            else:
                bb = job.Stock.Shape.BoundBox
                startDepth = bb.ZMax
                finalDepth = bb.ZMin

            # obj.StartDepth.Value = startDepth
            # obj.FinalDepth.Value = finalDepth
            obj.OpStartDepth.Value = startDepth
            obj.OpFinalDepth.Value = finalDepth

            PathLog.debug(
                "Default OpDepths are Start: {}, and Final: {}".format(
                    obj.OpStartDepth.Value, obj.OpFinalDepth.Value
                )
            )
            PathLog.debug(
                "Default Depths are Start: {}, and Final: {}".format(
                    startDepth, finalDepth
                )
            )

        self.areaOpSetDefaultValues(obj, job)

    def areaOpSetDefaultValues(self, obj, job):
        """areaOpSetDefaultValues(obj, job) ... overwrite to set initial values of operation specific properties.
        Can safely be overwritten by subclasses."""
        pass

    def _buildPathArea(self, obj, baseobject, isHole, start, getsim):
        """_buildPathArea(obj, baseobject, isHole, start, getsim) ... internal function."""
        PathLog.track()
        area = Path.Area()
        area.setPlane(PathUtils.makeWorkplane(baseobject))
        area.add(baseobject)

        areaParams = self.areaOpAreaParams(obj, isHole)

        heights = [i for i in self.depthparams]
        PathLog.debug("depths: {}".format(heights))
        area.setParams(**areaParams)
        obj.AreaParams = str(area.getParams())

        PathLog.debug("Area with params: {}".format(area.getParams()))

        sections = area.makeSections(
            mode=0, project=self.areaOpUseProjection(obj), heights=heights
        )
        PathLog.debug("sections = %s" % sections)
        shapelist = [sec.getShape() for sec in sections]
        PathLog.debug("shapelist = %s" % shapelist)

        pathParams = self.areaOpPathParams(obj, isHole)
        pathParams["shapes"] = shapelist
        pathParams["feedrate"] = self.horizFeed
        pathParams["feedrate_v"] = self.vertFeed
        pathParams["verbose"] = True
        pathParams["resume_height"] = obj.SafeHeight.Value
        pathParams["retraction"] = obj.ClearanceHeight.Value
        pathParams["return_end"] = True
        # Note that emitting preambles between moves breaks some dressups and prevents path optimization on some controllers
        pathParams["preamble"] = False

        if not self.areaOpRetractTool(obj):
            pathParams["threshold"] = 2.001 * self.radius

        if self.endVector is not None:
            pathParams["start"] = self.endVector
        elif PathOp.FeatureStartPoint & self.opFeatures(obj) and obj.UseStartPoint:
            pathParams["start"] = obj.StartPoint

        obj.PathParams = str(
            {key: value for key, value in pathParams.items() if key != "shapes"}
        )
        PathLog.debug("Path with params: {}".format(obj.PathParams))

        (pp, end_vector) = Path.fromShapes(**pathParams)
        PathLog.debug("pp: {}, end vector: {}".format(pp, end_vector))
        self.endVector = end_vector

        simobj = None
        if getsim:
            areaParams["Thicken"] = True
            areaParams["ToolRadius"] = self.radius - self.radius * 0.005
            area.setParams(**areaParams)
            sec = area.makeSections(mode=0, project=False, heights=heights)[
                -1
            ].getShape()
            simobj = sec.extrude(FreeCAD.Vector(0, 0, baseobject.BoundBox.ZMax))

        return pp, simobj

    def _buildProfileOpenEdges(self, obj, edgeList, isHole, start, getsim):
        """_buildPathArea(obj, edgeList, isHole, start, getsim) ... internal function."""
        PathLog.track()

        paths = []
        heights = [i for i in self.depthparams]
        PathLog.debug("depths: {}".format(heights))
        for i in range(0, len(heights)):
            for baseShape in edgeList:
                hWire = Part.Wire(Part.__sortEdges__(baseShape.Edges))
                hWire.translate(FreeCAD.Vector(0, 0, heights[i] - hWire.BoundBox.ZMin))

                pathParams = {}
                pathParams["shapes"] = [hWire]
                pathParams["feedrate"] = self.horizFeed
                pathParams["feedrate_v"] = self.vertFeed
                pathParams["verbose"] = True
                pathParams["resume_height"] = obj.SafeHeight.Value
                pathParams["retraction"] = obj.ClearanceHeight.Value
                pathParams["return_end"] = True
                # Note that emitting preambles between moves breaks some dressups and prevents path optimization on some controllers
                pathParams["preamble"] = False

                if self.endVector is None:
                    verts = hWire.Wires[0].Vertexes
                    idx = 0
                    if obj.Direction == "CCW":
                        idx = len(verts) - 1
                    x = verts[idx].X
                    y = verts[idx].Y
                    # Zero start value adjustments for Path.fromShapes() bug
                    if PathGeom.isRoughly(x, 0.0):
                        x = 0.00001
                    if PathGeom.isRoughly(y, 0.0):
                        y = 0.00001
                    pathParams["start"] = FreeCAD.Vector(x, y, verts[0].Z)
                else:
                    pathParams["start"] = self.endVector

                obj.PathParams = str(
                    {key: value for key, value in pathParams.items() if key != "shapes"}
                )
                PathLog.debug("Path with params: {}".format(obj.PathParams))

                (pp, end_vector) = Path.fromShapes(**pathParams)
                paths.extend(pp.Commands)
                PathLog.debug("pp: {}, end vector: {}".format(pp, end_vector))

        self.endVector = end_vector
        simobj = None

        return paths, simobj

    def opExecute(self, obj, getsim=False):
        """opExecute(obj, getsim=False) ... implementation of Path.Area ops.
        determines the parameters for _buildPathArea().
        Do not overwrite, implement
            areaOpAreaParams(obj, isHole) ... op specific area param dictionary
            areaOpPathParams(obj, isHole) ... op specific path param dictionary
            areaOpShapes(obj)             ... the shape for path area to process
            areaOpUseProjection(obj)      ... return true if operation can use projection
        instead."""
        PathLog.track()

        # Instantiate class variables for operation reference
        self.endVector = None
        self.leadIn = 2.0

        # Initiate depthparams and calculate operation heights for operation
        self.depthparams = self._customDepthParams(
            obj, obj.StartDepth.Value, obj.FinalDepth.Value
        )

        # Set start point
        if PathOp.FeatureStartPoint & self.opFeatures(obj) and obj.UseStartPoint:
            start = obj.StartPoint
        else:
            start = None

        aOS = self.areaOpShapes(obj)

        # Adjust tuples length received from other PathWB tools/operations
        shapes = []
        for shp in aOS:
            if len(shp) == 2:
                (fc, iH) = shp
                #     fc, iH,  sub or description
                tup = fc, iH, "otherOp"
                shapes.append(tup)
            else:
                shapes.append(shp)

        if len(shapes) > 1:
            locations = []
            for s in shapes:
                if s[2] == "OpenEdge":
                    shp = Part.makeCompound(s[0])
                else:
                    shp = s[0]
                locations.append(
                    {"x": shp.BoundBox.XMax, "y": shp.BoundBox.YMax, "shape": s}
                )

            locations = PathUtils.sort_locations(locations, ["x", "y"])

            shapes = [j["shape"] for j in locations]

        sims = []
        for shape, isHole, sub in shapes:
            profileEdgesIsOpen = False

            if sub == "OpenEdge":
                profileEdgesIsOpen = True
                if (
                    PathOp.FeatureStartPoint & self.opFeatures(obj)
                    and obj.UseStartPoint
                ):
                    osp = obj.StartPoint
                    self.commandlist.append(
                        Path.Command(
                            "G0", {"X": osp.x, "Y": osp.y, "F": self.horizRapid}
                        )
                    )

            try:
                if profileEdgesIsOpen:
                    (pp, sim) = self._buildProfileOpenEdges(
                        obj, shape, isHole, start, getsim
                    )
                else:
                    (pp, sim) = self._buildPathArea(obj, shape, isHole, start, getsim)
            except Exception as e:
                FreeCAD.Console.PrintError(e)
                FreeCAD.Console.PrintError(
                    "Something unexpected happened. Check project and tool config."
                )
                raise e
            else:
                if profileEdgesIsOpen:
                    ppCmds = pp
                else:
                    ppCmds = pp.Commands

                # Save gcode commands to object command list
                self.commandlist.extend(ppCmds)
                sims.append(sim)
            # Eif

            if (
                self.areaOpRetractTool(obj)
                and self.endVector is not None
                and len(self.commandlist) > 1
            ):
                self.endVector[2] = obj.ClearanceHeight.Value
                self.commandlist.append(
                    Path.Command(
                        "G0", {"Z": obj.ClearanceHeight.Value, "F": self.vertRapid}
                    )
                )

        PathLog.debug("obj.Name: " + str(obj.Name) + "\n\n")
        return sims

    def areaOpRetractTool(self, obj):
        """areaOpRetractTool(obj) ... return False to keep the tool at current level between shapes. Default is True."""
        return True

    def areaOpAreaParams(self, obj, isHole):
        """areaOpAreaParams(obj, isHole) ... return operation specific area parameters in a dictionary.
        Note that the resulting parameters are stored in the property AreaParams.
        Must be overwritten by subclasses."""
        pass

    def areaOpPathParams(self, obj, isHole):
        """areaOpPathParams(obj, isHole) ... return operation specific path parameters in a dictionary.
        Note that the resulting parameters are stored in the property PathParams.
        Must be overwritten by subclasses."""
        pass

    def areaOpShapes(self, obj):
        """areaOpShapes(obj) ... return all shapes to be processed by Path.Area for this op.
        Must be overwritten by subclasses."""
        pass

    def areaOpUseProjection(self, obj):
        """areaOpUseProcjection(obj) ... return True if the operation can use procjection, defaults to False.
        Can safely be overwritten by subclasses."""
        return False

    # Support methods
    def _customDepthParams(self, obj, strDep, finDep):
        finish_step = obj.FinishDepth.Value if hasattr(obj, "FinishDepth") else 0.0
        cdp = PathUtils.depth_params(
            clearance_height=obj.ClearanceHeight.Value,
            safe_height=obj.SafeHeight.Value,
            start_depth=strDep,
            step_down=obj.StepDown.Value,
            z_finish_step=finish_step,
            final_depth=finDep,
            user_depths=None,
        )
        return cdp


# Eclass


def SetupProperties():
    setup = []
    return setup
