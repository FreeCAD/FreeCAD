# SPDX-License-Identifier: LGPL-2.1-or-later

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
import Path.Dressup.Utils as PathDressup

import math

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
                Path.Log.debug("job=%s base=%s shape=%s" % (job, job.Stock, job.Stock.Shape))
                return job.Stock.Shape
            else:
                Path.Log.warning(translate("PathAreaOp", "job %s has no Base.") % job.Label)
        else:
            Path.Log.warning(translate("PathAreaOp", "no job for operation %s found.") % obj.Label)
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
        if prop in ("AreaParams", "PathParams", "removalshape"):
            obj.setEditorMode(prop, 2)

        # Offer side while creating new operation
        if (
            getattr(self, "init", False)
            and hasattr(obj, "Side")
            and prop == "FinalDepth"
            and len(obj.Base) == 1
        ):
            """Offer side only while creating new operation
            if prop is 'Base', only 1 shape saved to 'obj.Base'
            if prop is 'FinalDepth', all selected shapes saved to 'obj.Base'"""
            self.init = False

            (base, subNames) = obj.Base[0]

            # find parent boundbox
            # shape can be compound, so use list
            if isinstance(base.Shape, Part.Compound):
                bbs = [Part.Shape(shape).BoundBox for shape in base.Shape.SubShapes]
            else:
                bbs = [base.Shape.BoundBox]

            if "Face" in subNames[0]:
                faces = [base.Shape.getElement(sub) for sub in subNames if sub.startswith("Face")]
                shape = Part.Compound(faces)
                fbb = shape.BoundBox
                for bb in bbs:
                    if bb.isInside(fbb):
                        if bb.XLength == fbb.XLength and bb.YLength == fbb.YLength:
                            obj.Side = "Outside"
                        else:
                            obj.Side = "Inside"
                        break

            elif "Edge" in subNames[0]:
                edges = [base.Shape.getElement(sub) for sub in subNames if sub.startswith("Edge")]
                wire = Part.Wire(Part.__sortEdges__(edges))
                wbb = wire.BoundBox
                for bb in bbs:
                    if bb.isInside(wbb):
                        if not wire.isClosed() or (
                            bb.XLength == wbb.XLength and bb.YLength == wbb.YLength
                        ):
                            # for open wire always offer Outside
                            obj.Side = "Outside"
                        else:
                            obj.Side = "Inside"
                        break

        self.areaOpOnChanged(obj, prop)

    def opOnDocumentRestored(self, obj):
        Path.Log.track()
        for prop in ("AreaParams", "PathParams", "removalshape"):
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
                "Default Depths are Start: {}, and Final: {}".format(startDepth, finalDepth)
            )

        self.areaOpSetDefaultValues(obj, job)
        self.init = True  # using for offer 'Side' while creating 'Profile' operation

    def areaOpSetDefaultValues(self, obj, job):
        """areaOpSetDefaultValues(obj, job) ... overwrite to set initial values of operation specific properties.
        Can safely be overwritten by subclasses."""
        pass

    def getCornerPoint(self, shape):
        """Use point connection of the edges and two points by distance 1 mm from connection
        to determine angle between edges and find corner with angle near 90 degrees"""

        candidate = shape.Edges[0].Vertexes[0].Point
        lastAngle = math.pi  # worst angle
        for i in range(len(shape.Edges)):
            p1, p2, p3 = None, None, None
            e1 = shape.Edges[i]
            if i < len(shape.Edges) - 1:
                e2 = shape.Edges[i + 1]
            else:
                # compare last edge with first
                e2 = shape.Edges[0]

            if Path.Geom.pointsCoincide(e1.Vertexes[-1].Point, e2.Vertexes[0].Point):
                p2, p1 = e1.discretize(Distance=1)[-2:]
                p3 = e2.discretize(Distance=1)[1]
            elif Path.Geom.pointsCoincide(e1.Vertexes[0].Point, e2.Vertexes[-1].Point):
                p1, p2 = e1.discretize(Distance=1)[:2]
                p3 = e2.discretize(Distance=1)[-2]
            if p1 and p2 and p3:
                pa = p2 - p1
                pb = p3 - p1
                angle = pa.getAngle(pb)
                if abs(angle - math.pi / 2) < abs(lastAngle - math.pi / 2):
                    lastAngle = angle
                    candidate = p1

        return candidate

    def isStraightEdge(self, edge):
        if edge.Length < 1:
            # exclude too short edge
            return False
        if isinstance(edge.Curve, Part.Line):
            # line is always straight
            return True
        # check other edge types
        p1 = edge.Vertexes[0].Point
        p2 = edge.Vertexes[-1].Point
        tol = self.job.GeometryTolerance.Value
        if Path.Geom.isRoughly(edge.Length, p1.distanceToPoint(p2), tol):
            return True

        return False

    def getMiddlePoint(self, shape, longest, straight):
        candidate = None
        for edge in shape.Edges:
            if straight and not self.isStraightEdge(edge):
                # skip not straight edge
                continue
            if (
                candidate is None
                or (longest and edge.Length > candidate.Length)
                or (not longest and edge.Length < candidate.Length)
            ):
                candidate = edge

        if candidate:
            return candidate.discretize(3)[1]
        else:
            return shape.Edges[0].Vertexes[0].Point

    def getStartIndex(self, cmds1, cmds2, threshold):
        # get index of command from which continue path to skip retract
        lastPoint = lastX = lastY = None
        nextPoint = nextX = nextY = None
        # determine last point in previous path
        for cmd in reversed(cmds1):
            if cmd.Name in Path.Geom.CmdMoveMill:
                lastX = cmd.x if cmd.x is not None and lastX is None else lastX
                lastY = cmd.y if cmd.y is not None and lastY is None else lastY
                if lastX is not None and lastY is not None:
                    lastPoint = FreeCAD.Vector(lastX, lastY, 0)
                    break
        # determine first point in new path
        indexG1 = None
        for i, cmd in enumerate(cmds2):
            if cmd.Name in Path.Geom.CmdMoveStraight:
                indexG1 = i if indexG1 is None else indexG1
                nextX = cmd.x if cmd.x is not None and nextX is None else nextX
                nextY = cmd.y if cmd.y is not None and nextY is None else nextY
                if nextX is not None and nextY is not None:
                    nextPoint = FreeCAD.Vector(nextX, nextY, 0)
                    break
        if (
            indexG1 is not None
            and lastPoint is not None
            and nextPoint is not None
            and lastPoint.distanceToPoint(nextPoint) <= threshold
        ):
            return indexG1
        else:
            return 0

    def getEndVector(self, cmds):
        # get index of command from which continue path to skip retract
        endVector = FreeCAD.Vector()
        lastX = lastY = lastZ = None
        for cmd in reversed(cmds):
            if cmd.Name in Path.Geom.CmdMoveMill:
                lastX = cmd.x if cmd.x is not None and lastX is None else lastX
                lastY = cmd.y if cmd.y is not None and lastY is None else lastY
                lastZ = cmd.z if cmd.z is not None and lastZ is None else lastZ
                if lastX is not None and lastY is not None and lastZ is not None:
                    endVector = FreeCAD.Vector(lastX, lastY, lastZ)

        return endVector

    def _buildPathArea(self, obj, baseobject, isHole, start, getsim):
        """_buildPathArea(obj, baseobject, isHole, start, getsim) ... internal function."""
        Path.Log.track()

        areaParamsList = []
        if "Path.Op.Pocket" in obj.Proxy.__module__:
            # Pocket operation: split area and get order 'Clearing' path -> 'Finish' pass
            if obj.ClearingPattern != "No clearing":
                areaParamsList.append(self.areaOpAreaParams(obj, isHole))  # 'Clearing' path
            if getattr(obj, "FinishOffset", None):
                areaParamsList.append(self.areaOpAreaParamsOffset(obj, isHole))  # 'Finish' pass
        elif obj.Proxy.__module__ == "Path.Op.Profile":
            # Profile operation: create independent area for each offset
            areaParams = self.areaOpAreaParams(obj, isHole)
            offsets = areaParams["Offset"]
            for offset in offsets:
                areaParams["Offset"] = offset
                areaParamsList.append(areaParams.copy())
        else:
            areaParamsList.append(self.areaOpAreaParams(obj, isHole))

        cmds = []
        sims = []
        for index, areaParams in enumerate(areaParamsList):
            """
            Notes:
            'Finish' pass mill last, no metter value 'StartAt'
            For 'StartAt' 'Center' in Pocket op need to set sort_mode = 0 to get correct order
            """

            oneStepDown = False
            if obj.Proxy.__module__ == "Path.Op.Profile":
                if (
                    getattr(obj, "OffsetFinish", None)
                    and getattr(obj, "NumPasses", 1) > 1
                    and index == len(areaParamsList) - 1
                    and getattr(obj, "FinishOneStepDown", False)
                ):
                    oneStepDown = True

            isPocketFinishPass = False
            sortMode_0 = False
            if "Path.Op.Pocket" in obj.Proxy.__module__:
                if "JoinType" in areaParams:  # Pocket finish pass
                    isPocketFinishPass = True
                    if getattr(obj, "FinishOneStepDown", False):
                        oneStepDown = True
                elif (
                    getattr(obj, "ClearingPattern", None) == "Offset"
                    and getattr(obj, "StartAt", None) == "Center"
                ):  # Pocket clearing path
                    sortMode_0 = True

            area = Path.Area()
            area.setPlane(PathUtils.makeWorkplane(baseobject))
            area.add(baseobject)
            areaParams["SectionTolerance"] = FreeCAD.Base.Precision.confusion() * 10

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
            self.sectionShapes = self.sectionShapes + [
                section.toTopoShape() for section in sections
            ]
            if getattr(obj, "UseRestMachining", False):
                restSections = []
                for section in sections:
                    bbox = section.getShape().BoundBox
                    z = bbox.ZMin
                    sectionClearedAreas = []
                    for op in self.job.Operations.Group:
                        baseOp = PathDressup.baseOp(op)
                        if baseOp.Name == obj.Name:
                            break
                        if not getattr(op, "ApplyToRestMachining", None):
                            op = baseOp
                        if getattr(baseOp, "Active", None) and op.Path:
                            tool = baseOp.ToolController.Tool
                            diameter = tool.Diameter.getValueAs("mm")
                            dz = (
                                0
                                if not hasattr(tool, "TipAngle")
                                else -PathUtils.drillTipLength(tool)
                            )  # for drills, dz translates to the full width part of the tool
                            sectionClearedAreas.append(
                                section.getClearedArea(
                                    op.Path,
                                    diameter,
                                    z + dz + self.job.GeometryTolerance.getValueAs("mm"),
                                    bbox,
                                )
                            )
                    restSection = section.getRestArea(
                        sectionClearedAreas, self.tool.Diameter.getValueAs("mm")
                    )
                    if restSection is not None:
                        restSections.append(restSection)
                sections = restSections

            shapelist = []
            for sec in sections:
                shape = sec.getShape()
                if shape.Wires:
                    # skip empty shape
                    shapelist.append(shape)

            Path.Log.debug("shapelist = %s" % shapelist)

            pathParams = self.areaOpPathParams(obj, isHole)

            if isPocketFinishPass:
                # invert orientation for finish Offset pass in Pocket operation
                pathParams["orientation"] = not pathParams["orientation"]

            if oneStepDown:
                pathParams["shapes"] = shapelist[-1]
            else:
                pathParams["shapes"] = shapelist

            pathParams["feedrate"] = self.horizFeed
            pathParams["feedrate_v"] = self.vertFeed
            pathParams["verbose"] = True
            pathParams["resume_height"] = obj.SafeHeight.Value
            pathParams["retraction"] = obj.ClearanceHeight.Value
            pathParams["return_end"] = True
            # Note that emitting preambles between moves breaks some dressups
            # and prevents path optimization on some controllers
            pathParams["preamble"] = False

            if sortMode_0:
                pathParams["sort_mode"] = 0
            elif isPocketFinishPass:
                pathParams["sort_mode"] = 3

            if hasattr(obj, "SortMode"):
                pathParams["sort_mode"] = obj.SortMode

            if hasattr(obj, "RetractThreshold"):
                pathParams["threshold"] = obj.RetractThreshold.Value

            if (
                not obj.UseStartPoint
                and getattr(obj, "HandleMultipleFeatures", None) == "Individually"
                and hasattr(obj, "StartPointOverride")
                and obj.StartPointOverride != "No"
            ):
                if obj.StartPointOverride == "Corner":
                    pathParams["start"] = self.getCornerPoint(shapelist[0])
                elif obj.StartPointOverride == "Middle-Long":
                    pathParams["start"] = self.getMiddlePoint(shapelist[0], True, False)
                elif obj.StartPointOverride == "Middle-Long-Straight":
                    pathParams["start"] = self.getMiddlePoint(shapelist[0], True, True)
                elif obj.StartPointOverride == "Middle-Short":
                    pathParams["start"] = self.getMiddlePoint(shapelist[0], False, False)
                elif obj.StartPointOverride == "Middle-Short-Straight":
                    pathParams["start"] = self.getMiddlePoint(shapelist[0], False, True)
            elif self.endVector is not None:
                if self.endVector[:2] != (0, 0):
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
            if pp.Size:
                self.endVector = end_vector
                if end_vector[:2] == (0, 0):
                    # need for finish pass after pocket offset pattern
                    self.endVector = self.getEndVector(pp.Commands)

            simobj = None
            if getsim:
                areaParams["Thicken"] = True
                areaParams["ToolRadius"] = self.radius - self.radius * 0.005
                area.setParams(**areaParams)
                sec = area.makeSections(mode=0, project=False, heights=heights)[-1].getShape()
                simobj = sec.extrude(FreeCAD.Vector(0, 0, baseobject.BoundBox.ZMax))

            sims.append(simobj)

            commands = pp.Commands
            # remove retract between "main" area and Offset
            if obj.RetractThreshold and cmds and commands:
                startIndex = self.getStartIndex(cmds, commands, obj.RetractThreshold.Value)
                cmds.extend(commands[startIndex:])
            else:
                cmds.extend(commands)

        return Path.Path(cmds), sims

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
        self.depthparams = self._customDepthParams(obj, obj.StartDepth.Value, obj.FinalDepth.Value)

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
                locations.append({"x": shp.BoundBox.XMax, "y": shp.BoundBox.YMax, "shape": s})
            locations = PathUtils.sort_locations(locations, ["x", "y"])

            shapes = [j["shape"] for j in locations]

        sims = []
        self.sectionShapes = []
        for shape, isHole, sub in shapes:
            profileEdgesIsOpen = False

            if sub == "OpenEdge":
                profileEdgesIsOpen = True
                if PathOp.FeatureStartPoint & self.opFeatures(obj) and obj.UseStartPoint:
                    osp = obj.StartPoint
                    self.commandlist.append(
                        Path.Command("G0", {"X": osp.x, "Y": osp.y, "F": self.horizRapid})
                    )

            try:
                if profileEdgesIsOpen:
                    (pp, sim) = self._buildProfileOpenEdges(obj, shape, isHole, start, getsim)
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
                if isinstance(sim, list):
                    sims.extend(sim)
                else:
                    sims.append(sim)

            if self.endVector is not None and len(self.commandlist) > 1:
                self.endVector[2] = obj.ClearanceHeight.Value
                self.commandlist.append(
                    Path.Command("G0", {"Z": obj.ClearanceHeightOut.Value, "F": self.vertRapid})
                )

        Path.Log.debug("obj.Name: " + str(obj.Name) + "\n\n")
        return sims

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
