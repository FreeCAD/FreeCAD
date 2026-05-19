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
import Path.Op.Util as PathOpUtil
import PathScripts.PathUtils as PathUtils
from Path.Base.Generator.ramp_entry_helix import HelixRamp
from Path.Geom import isRoughly
import Constants

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

        if (
            getattr(self, "init", False)
            and hasattr(obj, "Side")
            and prop == "FinalDepth"
            and obj.Base
        ):
            # Offer side only while creating new operation
            self.init = False
            self.opSetDefaultSide(obj)

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
        self.init = True  # using for offer 'Side' while creating new operation

    def areaOpSetDefaultValues(self, obj, job):
        """areaOpSetDefaultValues(obj, job) ... overwrite to set initial values of operation specific properties.
        Can safely be overwritten by subclasses."""
        pass

    def opSetDefaultSide(self, obj):
        """setDefaltSide(obj) ...  offer side while creating new operation"""
        base, subNames = obj.Base[0]

        # find parent boundbox
        if isinstance(base.Shape, Part.Compound):
            bbs = [shape.BoundBox for shape in base.Shape.SubShapes]
        else:
            bbs = [base.Shape.BoundBox]

        subBb = None
        if "Face" in subNames[0]:
            faces = [base.Shape.getElement(sub) for sub in subNames if sub.startswith("Face")]
            vFaces = [f for f in faces if not Path.Geom.isHorizontal(f)]
            if vFaces:
                # check if vertical faces creates a closed area
                fzMin = min(e.BoundBox.ZMin for f in vFaces for e in f.Edges)
                bottomEdges = [
                    e for f in vFaces for e in f.Edges if isRoughly(e.BoundBox.ZMax, fzMin)
                ]
                wire = Part.Wire(Part.__sortEdges__(bottomEdges))
                if not wire.isClosed():
                    # for open area always offer 'Outside'
                    obj.Side = "Outside"
                    return
            shape = Part.Compound(faces)
            subBb = shape.BoundBox
        elif "Edge" in subNames[0]:
            edges = [base.Shape.getElement(sub) for sub in subNames if sub.startswith("Edge")]
            wire = Part.Wire(Part.__sortEdges__(edges))
            if not wire.isClosed():
                # for open wire always offer 'Outside'
                obj.Side = "Outside"
                return
            else:
                subBb = wire.BoundBox

        if subBb:
            for bb in bbs:
                if not bb.isInside(subBb):
                    continue
                if isRoughly(bb.XLength, subBb.XLength) and isRoughly(bb.YLength, subBb.YLength):
                    obj.Side = "Outside"
                else:
                    obj.Side = "Inside"
                return

    def getMiddlePointLongestEdge(self, shape):
        """getMiddlePointLongestEdge(shape) ... return middle point of longest edge from shape."""
        candidate = shape.Edges[0]
        for edge in shape.Edges:
            if edge.Length > candidate.Length:
                candidate = edge

        return candidate.discretize(3)[1]

    def getStartIndex(self, cmds1, cmds2, threshold):
        # get index of command from which continue path to skip retract
        index = None
        lastPoint = lastX = lastY = None
        nextPoint = nextX = nextY = None
        for cmd in reversed(cmds1):  # define previous path last point
            if cmd.Name in Constants.GCODE_MOVE_MILL:
                lastX = cmd.x if cmd.x is not None and lastX is None else lastX
                lastY = cmd.y if cmd.y is not None and lastY is None else lastY
                if lastX is not None and lastY is not None:
                    lastPoint = FreeCAD.Vector(lastX, lastY, 0)
                    break
        for i, cmd in enumerate(cmds2):  # define next path first point
            if cmd.Name in Constants.GCODE_MOVE_STRAIGHT:
                nextX = cmd.Parameters.get("X", nextX)
                nextY = cmd.Parameters.get("Y", nextY)
                if nextX is not None and nextY is not None:
                    index = i
                    nextPoint = FreeCAD.Vector(nextX, nextY, 0)
                    break
        if (
            index is not None
            and lastPoint is not None
            and nextPoint is not None
            and lastPoint.distanceToPoint(nextPoint) <= threshold
        ):
            return index
        else:
            return 0

    def getCenterPoint(self, shape):
        center = shape.CenterOfGravity
        candidate = shape.Vertexes[0].Point
        minDist = candidate.distanceToPoint(center)
        for v in shape.Vertexes:
            dist = center.distanceToPoint(v.Point)
            if dist < minDist:
                minDist = dist
                candidate = v.Point

        return candidate

    def _buildPathArea(self, obj, baseobject, isHole, start):
        """_buildPathArea(obj, baseobject, isHole, start) ... internal function."""
        Path.Log.track()

        areaParamsList = []
        if "Path.Op.Pocket" in obj.Proxy.__module__:
            # Pocket operation: split area and get order Clearing path -> Finishing pass
            if obj.ClearingPattern != "No clearing":
                areaParamsList.append(self.areaOpAreaParams(obj, isHole))  # Clearing path
            for i in range(obj.FinishingPasses):
                areaParamsList.append(self.areaOpAreaParamsFinishing(obj, isHole))  # Finishing pass
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
        for indexArea, areaParams in enumerate(areaParamsList):
            """
            Notes:
            - Finishing pass should be the last in order, no matter value 'StartAt'.
            - For helix ramp need to skip step down and use only bottom shapes.
            - For 'StartAt' at 'Center' in Pocket op StartPoint should be in the center.
            """

            oneStepDown = False
            helixRamp = False
            middleEdge = False
            isPocketFinishingPass = False
            pocketCenter = False
            if "Path.Op.Profile" in obj.Proxy.__module__:
                if indexArea >= len(areaParamsList) - obj.FinishingPasses:  # Profile finishing pass
                    if obj.FinishingOneStepDown:
                        oneStepDown = True
                elif obj.HelixRamp:
                    helixRamp = True
                if (
                    obj.UseLongestEdge
                    and not obj.UseStartPoint
                    and obj.HandleMultipleFeatures == "Individually"
                ):
                    middleEdge = True

            elif "Path.Op.Pocket" in obj.Proxy.__module__:
                if indexArea >= len(areaParamsList) - obj.FinishingPasses:  # Pocket finishing pass
                    isPocketFinishingPass = True
                    if obj.FinishingOneStepDown:
                        oneStepDown = True
                    elif obj.FinishingRampHelix:
                        helixRamp = True
                elif obj.ClearingPattern in ("Offset", "Helix"):  # Pocket clearing path
                    if obj.StartAt == "Center":
                        pocketCenter = True
                    if obj.ClearingPattern == "Helix":
                        oneStepDown = True
                        helixRamp = True

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
            if getattr(obj, "UseRestMachining", False):
                restSections = []
                for section in sections:
                    bbox = section.getShape().BoundBox
                    sectionClearedAreas = PathOpUtil.getClearedAreas(obj, bbox)
                    restSection = section.getRestArea(
                        sectionClearedAreas, self.tool.Diameter.getValueAs("mm")
                    )
                    if restSection is not None:
                        restSections.append(restSection)
                sections = restSections

            shapelist = []
            for sec in sections:
                shape = sec.getShape()
                if shape.Wires:  # skip empty shape
                    shapelist.append(shape)

            if not shapelist:
                continue

            Path.Log.debug("shapelist = %s" % shapelist)

            pathParams = self.areaOpPathParams(obj, isHole)
            if isPocketFinishingPass:
                # invert orientation for finishing pass in Pocket operation
                pathParams["orientation"] = not pathParams["orientation"]

            pathParams["shapes"] = shapelist[-1] if oneStepDown else shapelist
            pathParams["feedrate"] = self.horizFeed
            pathParams["feedrate_v"] = self.vertFeed
            pathParams["verbose"] = True
            pathParams["resume_height"] = obj.SafeHeight.Value
            pathParams["retraction"] = obj.ClearanceHeight.Value
            pathParams["return_end"] = True
            # Note that emitting preambles between moves breaks some dressups
            # and prevents path optimization on some controllers
            pathParams["preamble"] = False

            pathParams["sort_mode"] = 1  # 2D5 sorting mode
            if isPocketFinishingPass or pocketCenter:
                if obj.RetractThreshold:
                    pathParams["sort_mode"] = 3  # Greedy sorting mode

            if getattr(obj, "SortMode", -1) != -1:
                # user can forcing sort_mode if add int property 'SortMode'
                pathParams["sort_mode"] = obj.SortMode

            if hasattr(obj, "RetractThreshold"):
                pathParams["threshold"] = obj.RetractThreshold.Value

            if middleEdge:
                pathParams["start"] = self.getMiddlePointLongestEdge(shapelist[0])
            elif pocketCenter:
                pathParams["start"] = self.getCenterPoint(shapelist[0])
            elif self.endVector is not None:
                if self.endVector[:2] != (0, 0):
                    pathParams["start"] = self.endVector
            elif PathOp.FeatureStartPoint & self.opFeatures(obj) and obj.UseStartPoint:
                pathParams["start"] = obj.StartPoint

            obj.PathParams = str(
                {key: value for key, value in pathParams.items() if key != "shapes"}
            )
            Path.Log.debug("Path with params: {}".format(obj.PathParams))

            pp, end_vector = Path.fromShapes(**pathParams)
            Path.Log.debug("pp: {}, end vector: {}".format(pp, end_vector))

            if pp.Size:
                self.endVector = end_vector
                commands = pp.Commands

                if helixRamp:  # create helix ramp entry
                    commands = HelixRamp(
                        commands,
                        maxStepDown=obj.StepDown.Value,
                        tc=obj.ToolController,
                        ignoreAbove=obj.StartDepth.Value,
                    ).generate()

                # remove retract between previous and next path
                if obj.RetractThreshold and cmds and (not helixRamp or oneStepDown):
                    startIndex = self.getStartIndex(cmds, commands, obj.RetractThreshold.Value)
                    cmds.extend(commands[startIndex:])
                else:
                    cmds.extend(commands)

        return Path.Path(cmds)

    def _buildProfileOpenEdges(self, obj, openWire, start):
        """_buildPathArea(obj, openWire, start) ... internal function."""
        Path.Log.track()

        paths = []
        heights = [i for i in self.depthparams]
        Path.Log.debug("depths: {}".format(heights))
        for i in range(0, len(heights)):
            openWire.translate(FreeCAD.Vector(0, 0, heights[i] - openWire.BoundBox.ZMin))

            pathParams = {}
            pathParams["shapes"] = [openWire]
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

            verts = openWire.Wires[0].Vertexes
            idx = 0
            if obj.Direction == "CCW":
                idx = len(verts) - 1
            x = verts[idx].X
            y = verts[idx].Y
            # Zero start value adjustments for Path.fromShapes() bug
            if isRoughly(x, 0.0):
                x = 0.00001
            if isRoughly(y, 0.0):
                y = 0.00001
            pathParams["start"] = FreeCAD.Vector(x, y, verts[0].Z)

            obj.PathParams = str(
                {key: value for key, value in pathParams.items() if key != "shapes"}
            )
            Path.Log.debug("Path with params: {}".format(obj.PathParams))

            pp, end_vector = Path.fromShapes(**pathParams)
            paths.extend(pp.Commands)
            Path.Log.debug("pp: {}, end vector: {}".format(pp, end_vector))

        self.endVector = end_vector

        return paths

    def opExecute(self, obj):
        """opExecute(obj) ... implementation of Path.Area ops.
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

        # Initiate depthparams and calculate operation heights for operation
        self.depthparams = self._customDepthParams(obj, obj.StartDepth.Value, obj.FinalDepth.Value)

        # Set start point
        if PathOp.FeatureStartPoint & self.opFeatures(obj) and obj.UseStartPoint:
            start = obj.StartPoint
        else:
            start = None

        shapes = self.areaOpShapes(obj)

        # Sorting shapes
        if (
            len(shapes) > 1
            and getattr(obj, "SortingMode", None) != "Manual"
            and getattr(obj, "HandleMultipleFeatures", False) != "Collectively"
        ):
            locations = []
            for s in shapes:
                shp = s[0]
                locations.append({"x": shp.BoundBox.XMax, "y": shp.BoundBox.YMax, "shape": s})
            locations = PathUtils.sort_locations(locations, ["x", "y"])
            shapes = [j["shape"] for j in locations]

        # Adjust tuples length received from other PathWB tools/operations
        rshapes = []
        for shp in shapes:
            if len(shp) == 2:
                rshapes.append((shp[0], shp[1], "otherOp"))
            elif len(shp) == 3 and isinstance(shp[1], list):  # unpack open wires
                rshapes.extend((s, False, "OpenEdge") for s in shp[1])
            else:
                rshapes.append(shp)

        shapes = rshapes

        # Combine similar tasks for Collectively HandleMultipleFeatures
        if (
            obj.Proxy.__module__ == "Path.Op.Profile"
            and len(shapes) > 1
            and getattr(obj, "HandleMultipleFeatures", False) == "Collectively"
        ):
            keys = set((iH, desc) for _, iH, desc in shapes)
            collectively = []
            for key in keys:
                combine = []
                for sh, iH, desc in shapes:
                    if (iH, desc) == key:
                        combine.append(sh)
                fc = Part.makeCompound(combine)
                collectively.append((fc, key[0], key[1]))

            shapes = collectively

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
                    pp = self._buildProfileOpenEdges(obj, shape, start)
                else:
                    pp = self._buildPathArea(obj, shape, isHole, start)
            except Exception as e:
                FreeCAD.Console.PrintError(e)
                FreeCAD.Console.PrintError(
                    "Something unexpected happened. Check project and tool config."
                )
                raise e
            else:
                ppCmds = pp if profileEdgesIsOpen else pp.Commands
                # ppCmds = Path.Geom.filterArcs(ppCmds, self.job.GeometryTolerance.Value)

                self.commandlist.extend(ppCmds)

            if self.endVector is not None and len(self.commandlist) > 1:
                z = getattr(obj, "ClearanceHeightOut", obj.ClearanceHeight).Value
                self.endVector[2] = z
                self.commandlist.append(Path.Command("G0", {"Z": z, "F": self.vertRapid}))

        Path.Log.debug("obj.Name: " + str(obj.Name) + "\n\n")

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
