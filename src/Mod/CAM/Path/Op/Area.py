# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2017 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import Constants
import FreeCAD
import Part
import Path
from Path.Base.Generator import linking
from Path.Base.Generator.ramp_entry import RampEntry
import Path.Op.Base as PathOp
import Path.Op.Util as PathOpUtil
from PathScripts import PathUtils
from PySide.QtCore import QT_TRANSLATE_NOOP
import math

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
            | PathOp.FeatureLinking
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
        obj.addProperty("App::PropertyStringList", "AreaParams", "Path")
        obj.setEditorMode("AreaParams", 2)  # hide
        obj.addProperty("App::PropertyStringList", "PathParams", "Path")
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

    def initAfterBase(self, obj):
        if hasattr(obj, "Side"):
            obj.Side = PathOpUtil.getOpSide(obj)

    def initAreaOp(self, obj):
        """initAreaOp(obj) ... overwrite if the receiver class needs initialisation.
        Can safely be overwritten by subclasses."""

    def areaOpShapeForDepths(self, obj, job):
        """areaOpShapeForDepths(obj) ... returns the shape used to make an initial calculation for the depths being used.
        The default implementation returns the job's Base.Shape"""
        if job:
            if job.Stock:
                Path.Log.debug(f"job={job} base={job.Stock} shape={job.Stock.Shape}")
                return job.Stock.Shape
            else:
                Path.Log.warning(translate("PathAreaOp", "job %s has no Base.") % job.Label)
        else:
            Path.Log.warning(translate("PathAreaOp", "no job for operation %s found.") % obj.Label)
        return None

    def areaOpOnChanged(self, obj, prop):
        """areaOpOnChanged(obj, porp) ... overwrite to process operation specific changes to properties.
        Can safely be overwritten by subclasses."""

    def opOnChanged(self, obj, prop):
        """opOnChanged(obj, prop) ... base implementation of the notification framework - do not overwrite.
        The base implementation takes a stab at determining Heights and Depths if the operations's Base
        changes.
        Do not overwrite, overwrite areaOpOnChanged(obj, prop) instead."""
        # Path.Log.track(obj.Label, prop)
        if prop in ("AreaParams", "PathParams", "removalshape"):
            obj.setEditorMode(prop, 2)

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

    def opSetDefaultValues(self, obj, job):
        """opSetDefaultValues(obj) ... base implementation, do not overwrite.
        The base implementation sets the depths and heights based on the
        areaOpShapeForDepths() return value.
        Do not overwrite, overwrite areaOpSetDefaultValues(obj, job) instead."""
        Path.Log.debug(f"opSetDefaultValues({obj.Label}, {job.Label})")

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

    def setParamsDebug(self, obj, prop, paramsTuples):
        """setAreaParamsDebug(obj,prop, paramsTuples) ... set debug data
        Save as string for legacy PropertyString or list of string for PropertyStringList"""
        valLst = [f"{k}: {v}" for k, v in paramsTuples]
        valStr = ", ".join(valLst)
        if "list" in obj.getTypeIdOfProperty(prop).casefold():
            setattr(obj, prop, valLst)
        else:
            setattr(obj, prop, valStr)
        Path.Log.debug(f"{prop}: {valStr}")

    def areaOpSetDefaultValues(self, obj, job):
        """areaOpSetDefaultValues(obj, job) ... overwrite to set initial values of operation specific properties.
        Can safely be overwritten by subclasses."""

    def _buildPathArea(self, obj, shape, isHole, linkingArgs):
        """_buildPathArea(obj, shape, isHole, start) ... returns commands for areas."""
        Path.Log.track()

        areaParamsList = []
        # Pocket family (Pocket, PocketShape, MillFace) provides finishing areas
        pocketOp = hasattr(self, "areaOpAreaParamsFinishing")
        if pocketOp:
            # Pocket operation: split area and get order Clearing path -> Finishing pass
            if obj.ClearingPattern != "No clearing":
                areaParamsList.append(self.areaOpAreaParams(obj, isHole))  # Clearing path
            for i in range(obj.FinishingPasses):
                areaParamsList.append(self.areaOpAreaParamsFinishing(obj, isHole))  # Finishing pass
        elif obj.Proxy.__module__ == "Path.Op.Profile":
            # Profile operation: create independent area for each offset
            areaParams = self.areaOpAreaParams(obj, isHole)
            offsets = areaParams["Offset"][:]
            for offset in offsets:
                areaParams["Offset"] = offset
                areaParamsList.append(areaParams.copy())
        else:
            areaParamsList.append(self.areaOpAreaParams(obj, isHole))

        pathParams = self.areaOpPathParams(obj, isHole)
        baseOrientation = pathParams.get("orientation", 0)
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
        self.setParamsDebug(obj, "PathParams", sorted(pathParams.items()))

        commands = []
        areaParamsDebug = []
        units = []  # pass options, section shape at one height
        for areaIndex, areaParams in enumerate(areaParamsList):
            """
            Notes:
            - Finishing pass should be the last in order, no matter value 'StartAt'.
            - For helix ramp need to skip step down and use only bottom shapes.
            - For 'StartAt' at 'Center' in Pocket op StartPoint should be in the center.
            """
            rampParams = {
                "commands": None,
                "method": None,
                "angle_rad": None,
                "pitch": None,
                "tc": obj.ToolController,
                "ignoreAbove": obj.StartDepth.Value,
            }
            reverseOpenWire = False
            oneStepDown = False
            middleEdge = False
            pocketCenter = False
            orientation = baseOrientation
            finishing = areaIndex >= len(areaParamsList) - getattr(obj, "FinishingPasses", 0)
            if "Path.Op.Profile" in obj.Proxy.__module__:
                if obj.RampAngle:
                    rampParams["angle_rad"] = math.radians(obj.RampAngle.Value)
                else:
                    rampParams["pitch"] = obj.StepDown.Value

                if finishing:  # Profile finishing pass
                    if obj.FinishingOneStepDown:
                        oneStepDown = True
                    elif obj.RampMethod == "Helix":
                        rampParams["method"] = 0
                        oneStepDown = True
                elif obj.RampMethod != "None":
                    oneStepDown = obj.RampMethod == "Helix"
                    rampParams["method"] = (
                        0 if obj.RampMethod == "Helix" else int(obj.RampMethod.split()[1])
                    )

                if rampParams["method"] and not rampParams["angle_rad"]:
                    # Ramp methods 1-3 need an angle, plunge instead
                    if areaIndex == 0:
                        Path.Log.warning(
                            translate(
                                "PathAreaOp", "%s: ramp method '%s' needs a ramp angle, plunging"
                            )
                            % (obj.Label, obj.RampMethod)
                        )
                    rampParams["method"] = None

                if (
                    obj.UseLongestEdge
                    and not obj.UseStartPoint
                    and obj.HandleMultipleFeatures == "Individually"
                ):
                    middleEdge = True

            elif pocketOp:
                if finishing:  # Pocket finishing pass
                    orientation = not baseOrientation
                    if obj.FinishingOneStepDown:
                        oneStepDown = True
                    elif obj.FinishingRampHelix:
                        oneStepDown = True
                        rampParams["method"] = 0
                        rampParams["pitch"] = obj.StepDown.Value
                elif obj.ClearingPattern in ("Offset", "Helix"):  # Pocket clearing path
                    if obj.StartAt == "Center":
                        pocketCenter = True
                    if obj.ClearingPattern == "Helix":
                        oneStepDown = True
                        rampParams["method"] = 0
                        rampParams["pitch"] = obj.StepDown.Value
                if obj.CutMode == "Climb":
                    reverseOpenWire = True

            area = Path.Area()
            area.setPlane(PathUtils.makeWorkplane(shape))
            area.add(shape)
            areaParams["SectionTolerance"] = FreeCAD.Base.Precision.confusion() * 10

            heights = [i for i in self.depthparams]
            if oneStepDown:
                heights = heights[-1:]
            Path.Log.debug("depths: {}".format(heights))

            area.setParams(**areaParams)
            areaParamsDebug.append(("AREA_INDEX", areaIndex))
            areaParamsDebug.extend(sorted(area.getParams().items()))

            sections = area.makeSections(
                mode=0, project=self.areaOpUseProjection(obj), heights=heights
            )
            Path.Log.debug(f"sections = {sections}")

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

            if not sections:
                continue

            opts = (rampParams, reverseOpenWire, middleEdge, pocketCenter, orientation, finishing)
            units.extend((opts, sec.getShape()) for sec in sections)

        if obj.Proxy.__module__ == "Path.Op.Profile":
            # Cut all roughing offsets at one height before stepping down,
            # finishing passes stay last
            roughing = [u for u in units if not u[0][5]]
            roughing.sort(key=lambda u: -round(u[1].BoundBox.ZMax, 6))  # stable: keeps pass order
            units = roughing + [u for u in units if u[0][5]]

        for opts, sh in units:  # each shape is a path/wires at one height
            rampParams, reverseOpenWire, middleEdge, pocketCenter, orientation, _ = opts
            pathParams["orientation"] = orientation
            if not (wires := sh.Wires):
                continue
            sortFrom = sh.CenterOfGravity if pocketCenter else self.endVector
            while wires:
                if wires[0].isClosed():
                    v = Part.Vertex(sortFrom)
                    wire = min(wires, key=lambda w: v.distToShape(w)[0])  # nearest closed wire
                    if middleEdge:
                        # get middle point of the longest edge from wire
                        longestEdge = max(wire.Edges, key=lambda edge: edge.Length)
                        start = longestEdge.discretize(3)[1]
                    else:
                        start = self.endVector
                else:  # open wire (pocket ZigZag, Line, Grid)
                    iV = -1 if reverseOpenWire else 0
                    wire = min(wires, key=lambda w: (sortFrom - w.Vertexes[iV].Point).Length)
                    start = wire.Vertexes[iV].Point

                wires.remove(wire)
                pathParams["start"] = start
                pathParams["shapes"] = [wire]
                pp, end_vector = Path.fromShapes(**pathParams)
                Path.Log.debug("pp: {}, end vector: {}".format(pp, end_vector))

                if pp.Size:
                    doRamp = rampParams["method"] is not None
                    while pp.Commands[0].Name in Constants.GCODE_MOVE_RAPID:
                        pp.deleteCommand(0)  # remove rapid moves
                    plungeMove = pp.Commands[0]
                    p = Path.Geom.commandEndPoint(plungeMove)
                    pp.deleteCommand(0)  # remove plunge move

                    cmds = []
                    if self.initmove:
                        self.initmove = False
                        cmds.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))
                        cmds.append(Path.Command("G0", {"X": p.x, "Y": p.y}))
                        cmds.append(Path.Command("G0", {"Z": obj.SafeHeight.Value}))
                        par = {"X": p.x, "Y": p.y, "Z": p.z, "F": self.vertFeed}
                        cmds.append(Path.Command("G1", par))
                    elif obj.RetractThreshold.Value > (self.endVector - p).Length:
                        cmds.append(plungeMove)
                    else:
                        linkingArgs["start_position"] = self.endVector
                        linkingArgs["target_position"] = p
                        cmds.extend(linking.get_linking_moves(**linkingArgs))
                        zMax = max(cmd.z for cmd in cmds) if cmds else p.z
                        if rampParams["method"] == 0 and zMax < obj.SafeHeight.Value:
                            doRamp = False
                        for cmd in cmds:
                            if cmd.z < obj.SafeHeight.Value:
                                cmd.Name = "G1"
                                par = cmd.Parameters
                                par["F"] = self.vertFeed
                                cmd.Parameters = par

                        if doRamp and len(cmds) < 2:
                            c = commands[-1]
                            cmds.insert(0, Path.Command("G1", {"X": c.x, "Y": c.y, "Z": c.z}))

                    cmds.extend(pp.Commands)
                    if doRamp:  # generate ramp entry
                        rampParams["commands"] = cmds
                        cmds = RampEntry(**rampParams).generate()

                    commands.extend(cmds)
                    self.endVector = end_vector
                    sortFrom = end_vector

        self.setParamsDebug(obj, "AreaParams", areaParamsDebug)
        return commands

    def _buildProfileOpenEdges(self, obj, shape, linkingArgs):
        """_buildPathArea(obj, shape, linkingArgs) ... returns commands for open wires.
        shape: Part.Wire or Part.Compound contains wires"""
        Path.Log.track()

        commands = []
        heights = [i for i in self.depthparams]
        Path.Log.debug("depths: {}".format(heights))
        for height in heights:
            for openWire in shape.Wires:
                openWire.translate(FreeCAD.Vector(0, 0, height - openWire.BoundBox.ZMin))

                pathParams = {}
                pathParams["shapes"] = [openWire]
                pathParams["feedrate"] = self.horizFeed
                pathParams["feedrate_v"] = self.vertFeed
                pathParams["verbose"] = True
                pathParams["resume_height"] = obj.SafeHeight.Value
                pathParams["retraction"] = obj.ClearanceHeight.Value
                pathParams["return_end"] = True
                # Note that emitting preambles between moves breaks some dressups
                # and prevents path optimization on some controllers
                pathParams["preamble"] = False

                # Always manually setting pathParams["start"] to the first or
                # last vertex of the wire (depending on obj.Direction) ensures
                # the edge is always milled in the correct direction. Using
                # self.endVector would allow Path.fromShapes to reverse the
                # direction if that would shorten the travel move and thus cause
                # the edges being milled in seemingly random directions.

                verts = openWire.Vertexes
                pathParams["start"] = verts[0].Point if obj.Direction == "CW" else verts[-1].Point

                self.setParamsDebug(obj, "PathParams", sorted(pathParams.items()))

                pp, end_vector = Path.fromShapes(**pathParams)
                if pp.Size:
                    p = pathParams["start"]
                    while pp.Commands[0].Name in Constants.GCODE_MOVE_RAPID:
                        pp.deleteCommand(0)  # remove rapid moves
                    pp.deleteCommand(0)  # remove plunge move

                    if self.initmove:
                        self.initmove = False
                        commands.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))
                        commands.append(Path.Command("G0", {"X": p.x, "Y": p.y}))
                        commands.append(Path.Command("G0", {"Z": obj.SafeHeight.Value}))
                        par = {"X": p.x, "Y": p.y, "Z": p.z, "F": self.vertFeed}
                        commands.append(Path.Command("G1", par))
                    else:
                        linkingArgs["start_position"] = self.endVector
                        linkingArgs["target_position"] = p
                        linkingMoves = linking.get_linking_moves(**linkingArgs)
                        for cmd in linkingMoves:
                            if cmd.z < obj.SafeHeight.Value:
                                cmd.Name = "G1"
                                par = cmd.Parameters
                                par["F"] = self.vertFeed
                                cmd.Parameters = par
                        commands.extend(linkingMoves)

                    commands.extend(pp.Commands)
                    self.endVector = end_vector
                    Path.Log.debug("pp: {}, end vector: {}".format(pp, end_vector))

        return commands

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

        # Init start point
        if PathOp.FeatureStartPoint & self.opFeatures(obj) and obj.UseStartPoint:
            self.endVector = obj.StartPoint
        else:
            self.endVector = FreeCAD.Vector()

        # Initiate depthparams and calculate operation heights for operation
        self.depthparams = self._customDepthParams(obj, obj.StartDepth.Value, obj.FinalDepth.Value)

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
            keys = {(iH, desc) for _, iH, desc in shapes}
            collectively = []
            for key in keys:
                combine = []
                for sh, iH, desc in shapes:
                    if (iH, desc) == key:
                        combine.append(sh)
                fc = Part.makeCompound(combine)
                collectively.append((fc, key[0], key[1]))

            shapes = collectively

        # Build linking kwargs for collision-aware between-feature transitions
        self.initmove = True
        linkingArgs = None
        if PathOp.FeatureLinking & self.opFeatures(obj):
            solids = []
            if self.job and hasattr(self.job, "Model"):
                solids = [b.Shape for b in self.job.Model.Group if hasattr(b, "Shape")]
            linkingArgs = {
                "start_position": None,
                "target_position": None,
                "heights_clearance": (obj.SafeHeight.Value, obj.ClearanceHeight.Value),
                "solids": None,
                "tool_shape": None,
                "tool_diameter": None,
                "collision_clearance": obj.CollisionClearance.Value,
                "retract_height_offset": None,
                "split_plunge_height": obj.SafeHeight.Value,
            }
            if obj.CollisionAvoidanceStrategy == "Clearance Height":
                linkingArgs["heights_clearance"] = obj.ClearanceHeight.Value
            elif obj.CollisionAvoidanceStrategy == "Retract Height":
                pass
            elif obj.CollisionAvoidanceStrategy == "Line of Sight":
                linkingArgs["retract_height_offset"] = obj.CollisionClearance.Value
                linkingArgs["solids"] = solids
            elif obj.CollisionAvoidanceStrategy == "Tool Diameter":
                linkingArgs["retract_height_offset"] = obj.CollisionClearance.Value
                linkingArgs["solids"] = solids
                linkingArgs["tool_diameter"] = obj.ToolController.Tool.Diameter.Value
            elif obj.CollisionAvoidanceStrategy == "Tool Shape":
                linkingArgs["retract_height_offset"] = obj.CollisionClearance.Value
                linkingArgs["solids"] = solids
                linkingArgs["tool_shape"] = obj.ToolController.Tool.BitBody.Shape

        for shape, isHole, sub in shapes:
            try:
                if sub == "OpenEdge":
                    ppCmds = self._buildProfileOpenEdges(obj, shape, linkingArgs)
                else:
                    ppCmds = self._buildPathArea(obj, shape, isHole, linkingArgs)
            except Exception as e:
                FreeCAD.Console.PrintError(e)
                FreeCAD.Console.PrintError(
                    "Something unexpected happened. Check project and tool config."
                )
                raise

            if ppCmds:
                self.commandlist.extend(ppCmds)

        Path.Log.debug("obj.Name: " + str(obj.Name) + "\n\n")

    def areaOpAreaParams(self, obj, isHole):
        """areaOpAreaParams(obj, isHole) ... return operation specific area parameters in a dictionary.
        Note that the resulting parameters are stored in the property AreaParams.
        Must be overwritten by subclasses."""

    def areaOpPathParams(self, obj, isHole):
        """areaOpPathParams(obj, isHole) ... return operation specific path parameters in a dictionary.
        Note that the resulting parameters are stored in the property PathParams.
        Must be overwritten by subclasses."""

    def areaOpShapes(self, obj):
        """areaOpShapes(obj) ... return all shapes to be processed by Path.Area for this op.
        Must be overwritten by subclasses."""

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
