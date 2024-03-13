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
import Path.Op.Base as PathOp
import PathScripts.PathUtils as PathUtils


# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Draft = LazyLoader("Draft", globals(), "Draft")
Part = LazyLoader("Part", globals(), "Part")


__title__ = "Base class for PathArea based operations."
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Base class and properties for Path.Area based operations."
__contributors__ = "russ4262 (Russell Johnson) davidgilkaufman (David Kaufman)"


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

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
        Path.Log.track()

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
                Path.Log.debug(
                    "job=%s base=%s shape=%s" % (job, job.Stock, job.Stock.Shape)
                )
                return job.Stock.Shape
            else:
                Path.Log.warning(
                    translate("PathAreaOp", "job %s has no Base.") % job.Label
                )
        else:
            Path.Log.warning(
                translate("PathAreaOp", "no job for operation %s found.") % obj.Label
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
        # Path.Log.track(obj.Label, prop)
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
        Path.Log.track()
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
        Path.Log.debug("opSetDefaultValues(%s, %s)" % (obj.Label, job.Label))

        if PathOp.FeatureDepths & self.opFeatures(obj):
            try:
                shape = self.areaOpShapeForDepths(obj, job)
            except Exception as ee:
                Path.Log.error(ee)
                shape = None

            # Set initial start and final depths
            if shape is None:
                Path.Log.debug("shape is None")
                startDepth = 1.0
                finalDepth = 0.0
            else:
                bb = job.Stock.Shape.BoundBox
                startDepth = bb.ZMax
                finalDepth = bb.ZMin

            obj.OpStartDepth.Value = startDepth
            obj.OpFinalDepth.Value = finalDepth

            Path.Log.debug(
                "Default OpDepths are Start: {}, and Final: {}".format(
                    obj.OpStartDepth.Value, obj.OpFinalDepth.Value
                )
            )
            Path.Log.debug(
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
        Path.Log.track()
        area = Path.Area()
        area.setPlane(PathUtils.makeWorkplane(baseobject))
        area.add(baseobject)

        areaParams = self.areaOpAreaParams(obj, isHole)
        areaParams["SectionTolerance"] = FreeCAD.Base.Precision.confusion() * 10 # basically 1e-06

        heights = [i for i in self.depthparams]
        Path.Log.debug("depths: {}".format(heights))
        area.setParams(**areaParams)
        obj.AreaParams = str(area.getParams())

        Path.Log.debug("Area with params: {}".format(area.getParams()))

        sections = area.makeSections(
            mode=0, project=self.areaOpUseProjection(obj), heights=heights
        )
        Path.Log.debug("sections = %s" % sections)

        # Rest machining
        self.sectionShapes = self.sectionShapes + [section.toTopoShape() for section in sections]
        if hasattr(obj, "UseRestMachining") and obj.UseRestMachining:
            restSections = []
            for section in sections:
                bbox = section.getShape().BoundBox
                z = bbox.ZMin
                sectionClearedAreas = []
                for op in self.job.Operations.Group:
                    if self in [x.Proxy for x in [op] + op.OutListRecursive if hasattr(x, "Proxy")]:
                        break
                    if hasattr(op, "Active") and op.Active and op.Path:
                        tool = op.Proxy.tool if hasattr(op.Proxy, "tool") else op.ToolController.Proxy.getTool(op.ToolController)
                        diameter = tool.Diameter.getValueAs("mm")
                        dz = 0 if not hasattr(tool, "TipAngle") else -PathUtils.drillTipLength(tool)  # for drills, dz translates to the full width part of the tool
                        sectionClearedAreas.append(section.getClearedArea(op.Path, diameter, z+dz+self.job.GeometryTolerance.getValueAs("mm"), bbox))
                restSection = section.getRestArea(sectionClearedAreas, self.tool.Diameter.getValueAs("mm"))
                if (restSection is not None):
                    restSections.append(restSection)
            sections = restSections

        shapelist = [sec.getShape() for sec in sections]
        Path.Log.debug("shapelist = %s" % shapelist)

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

        # disable path sorting for offset and zigzag-offset paths
        if hasattr(obj, "OffsetPattern") and obj.OffsetPattern in ["ZigZagOffset", "Offset"] and hasattr(obj, "MinTravel") and not obj.MinTravel:
            pathParams["sort_mode"] = 0

        if not self.areaOpRetractTool(obj):
            pathParams["threshold"] = 2.001 * self.radius

        if self.endVector is not None:
            pathParams["start"] = self.endVector
        elif PathOp.FeatureStartPoint & self.opFeatures(obj) and obj.UseStartPoint:
            pathParams["start"] = obj.StartPoint

        obj.PathParams = str(
            {key: value for key, value in pathParams.items() if key != "shapes"}
        )
        Path.Log.debug("Path with params: {}".format(obj.PathParams))

        (pp, end_vector) = Path.fromShapes(**pathParams)
        Path.Log.debug("pp: {}, end vector: {}".format(pp, end_vector))

        # Keep track of this segment's end only if it has movement (otherwise end_vector is 0,0,0 and the next segment will unnecessarily start there)
        if pp.Size > 0:
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
        Path.Log.track()

        paths = []
        heights = [i for i in self.depthparams]
        Path.Log.debug("depths: {}".format(heights))
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

                # Always manually setting pathParams["start"] to the first or
                # last vertex of the wire (depending on obj.Direction) ensures
                # the edge is always milled in the correct direction. Using
                # self.endVector would allow Path.fromShapes to reverse the
                # direction if that would shorten the travel move and thus cause
                # the edges being milled in seemingly random directions.

                verts = hWire.Wires[0].Vertexes
                idx = 0
                if obj.Direction == "CCW":
                    idx = len(verts) - 1
                x = verts[idx].X
                y = verts[idx].Y
                # Zero start value adjustments for Path.fromShapes() bug
                if Path.Geom.isRoughly(x, 0.0):
                    x = 0.00001
                if Path.Geom.isRoughly(y, 0.0):
                    y = 0.00001
                pathParams["start"] = FreeCAD.Vector(x, y, verts[0].Z)

                obj.PathParams = str(
                    {key: value for key, value in pathParams.items() if key != "shapes"}
                )
                Path.Log.debug("Path with params: {}".format(obj.PathParams))

                (pp, end_vector) = Path.fromShapes(**pathParams)
                paths.extend(pp.Commands)
                Path.Log.debug("pp: {}, end vector: {}".format(pp, end_vector))

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
        Path.Log.track()

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

        # Adjust tuples length received from other PathWB tools/operations
        shapes = []
        for shp in self.areaOpShapes(obj):
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
        self.sectionShapes = []
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
                ppCmds = pp if profileEdgesIsOpen else pp.Commands

                self.commandlist.extend(ppCmds)
                sims.append(sim)

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

        Path.Log.debug("obj.Name: " + str(obj.Name) + "\n\n")
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
        return PathUtils.depth_params(
            clearance_height=obj.ClearanceHeight.Value,
            safe_height=obj.SafeHeight.Value,
            start_depth=strDep,
            step_down=obj.StepDown.Value,
            z_finish_step=finish_step,
            final_depth=finDep,
            user_depths=None,
        )


def SetupProperties():
    setup = []
    return setup
