# SPDX-License-Identifier: LGPL-2.1-or-later
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
from Path.Base.Drillable import isDrillableFace
import Path.Op.Base as PathOp
import Path.Op.EngraveBase as PathEngraveBase
import Path.Op.Util as PathOpUtil
from PathScripts import PathUtils
from Path.Base.Generator import linking
import math
import tsp_solver

from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "CAM Deburring Operation"
__url__ = "https://www.freecad.org"
__doc__ = "Deburring operation."


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class ObjectDeburring(PathEngraveBase.ObjectOp):
    """Proxy class for Deburring operation."""

    def opFeatures(self, obj):
        return (
            PathOp.FeatureTool
            | PathOp.FeatureHeights
            | PathOp.FeatureStepDown
            | PathOp.FeatureBaseEdges
            | PathOp.FeatureBaseFaces
            | PathOp.FeatureCoolant
            | PathOp.FeatureBaseGeometry
            | PathOp.FeatureLinking
        )

    def initOperation(self, obj):
        """initOperation(obj) ... Initialize the operation by
        managing property creation and property editor status."""
        Path.Log.track(obj.Label)
        self.propertiesReady = False
        self.initOpProperties(obj)  # Initialize operation-specific properties

    def initOpProperties(self, obj, warn=False):
        """initOpProperties(obj) ... create operation specific properties"""
        Path.Log.track()
        self.addNewProps = []

        for prtyp, nm, grp, tt in self.opPropertyDefinitions():
            if not hasattr(obj, nm):
                obj.addProperty(prtyp, nm, grp, tt)
                self.addNewProps.append(nm)

        # Set enumeration lists for enumeration properties
        if len(self.addNewProps) > 0:
            ENUMS = self.propertyEnumerations()
            for n in ENUMS:
                if n[0] in self.addNewProps:
                    setattr(obj, n[0], n[1])
            if warn:
                newPropMsg = translate("CAM_Deburring", "New property added to")
                newPropMsg += ' "{}": {}'.format(obj.Label, self.addNewProps) + ". "
                newPropMsg += translate("CAM_Deburring", "Check default value(s).")
                FreeCAD.Console.PrintWarning(newPropMsg + "\n")

        self.propertiesReady = True

    def opPropertyDefinitions(self):
        """opPropertyDefinitions(obj) ... Store operation specific properties"""
        return [
            (
                "App::PropertyDistance",
                "Width",
                "Deburring",
                QT_TRANSLATE_NOOP("App::Property", "Chamfer width or fillet radius"),
            ),
            (
                "App::PropertyAngle",
                "Angle",
                "Deburring",
                QT_TRANSLATE_NOOP("App::Property", "Chamfer angle"),
            ),
            (
                "App::PropertyDistance",
                "RadialStockToLeave",
                "Deburring",
                QT_TRANSLATE_NOOP("App::Property", "Radial offset"),
            ),
            (
                "App::PropertyDistance",
                "ExtraDepth",
                "Deburring",
                QT_TRANSLATE_NOOP("App::Property", "Additional depth"),
            ),
            (
                "App::PropertyEnumeration",
                "Direction",
                "Deburring",
                QT_TRANSLATE_NOOP("App::Property", "Direction of toolpath"),
            ),
            (
                "App::PropertyEnumeration",
                "Side",
                "Deburring",
                QT_TRANSLATE_NOOP("App::Property", "Side of base object"),
            ),
            (
                "App::PropertyEnumeration",
                "Strategy",
                "Deburring",
                QT_TRANSLATE_NOOP("App::Property", "Type of the profile, chamfer or fillet"),
            ),
            (
                "App::PropertyBool",
                "ProcessCircles",
                "Deburring",
                QT_TRANSLATE_NOOP("App::Property", "Process round holes of horizontal faces"),
            ),
            (
                "App::PropertyBool",
                "ProcessHoles",
                "Deburring",
                QT_TRANSLATE_NOOP("App::Property", "Process holes of horizontal faces"),
            ),
            (
                "App::PropertyBool",
                "ProcessPerimeter",
                "Deburring",
                QT_TRANSLATE_NOOP("App::Property", "Process the outline of horizontal faces"),
            ),
            (
                "App::PropertyBool",
                "StartFromBottom",
                "Deburring",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Start mill from bottom to top.\nOnly for Endmill, Ballend and Bullnose.",
                ),
            ),
            (
                "App::PropertyBool",
                "SplitArcs",
                "Deburring",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Split arcs into discrete segments."
                    "\nCan be useful with bad edges concidence in wire.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "SortingMode",
                "Sorting",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Order processing of the wires\n"
                    "\nManual - Using order from selection without sorting"
                    "\nAutomatic - Sorting wires by the nearest neighbour method, further improved with 2-opt",
                ),
            ),
            (
                "App::PropertyVectorDistance",
                "StartPoint",
                "Sorting",
                QT_TRANSLATE_NOOP("App::Property", "The start point for sorting"),
            ),
            (
                "App::PropertyVectorDistance",
                "EndPoint",
                "Sorting",
                QT_TRANSLATE_NOOP("App::Property", "The end point for sorting"),
            ),
            (
                "App::PropertyBool",
                "UseEndPoint",
                "Sorting",
                QT_TRANSLATE_NOOP("App::Property", "Use end point for sorting"),
            ),
        ]

    @classmethod
    def propertyEnumerations(cls, dataType="data"):
        """opPropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        # Enumeration lists for App::PropertyEnumeration properties
        enums = {
            "Direction": [
                (translate("CAM_Deburring", "CW"), "CW"),
                (translate("CAM_Deburring", "CCW"), "CCW"),
            ],  # direction that the profile runs
            "Side": [
                (translate("CAM_Deburring", "Outside"), "Outside"),
                (translate("CAM_Deburring", "Inside"), "Inside"),
            ],  # side of profile on which create Path
            "SortingMode": [
                (translate("CAM_Deburring", "Automatic"), "Automatic"),
                (translate("CAM_Deburring", "Manual"), "Manual"),
            ],  # sorting wires
            "Strategy": [
                (translate("CAM_Deburring", "Chamfer"), "Chamfer"),
                (translate("CAM_Deburring", "Fillet"), "Fillet"),
            ],  # type of profile
        }

        if dataType == "raw":
            return enums

        data = []
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            # data[k] = [tup[idx] for tup in v]
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def opPropertyDefaults(self, obj, job):
        """opPropertyDefaults(obj, job) ... returns a dictionary of default values
        for the operation's properties."""
        defaults = {
            "Angle": 45,
            "Direction": "CCW",
            "ExtraDepth": 0,
            "ProcessPerimeter": True,
            "Side": "Outside",
            "SortingMode": "Automatic",
            "Strategy": "Chamfer",
            "Width": 1,
        }

        return defaults

    def opApplyPropertyDefaults(self, obj, job, propList):
        # Set standard property defaults
        PROP_DFLTS = self.opPropertyDefaults(obj, job)
        for name in PROP_DFLTS:
            if name in propList:
                obj.clearExpression(name)
                val = PROP_DFLTS[name]
                setattr(obj, name, val)

    def opSetDefaultValues(self, obj, job):
        if self.addNewProps and self.addNewProps.__len__() > 0:
            self.opApplyPropertyDefaults(obj, job, self.addNewProps)

    def setOpEditorProperties(self, obj):
        obj.setEditorMode("Side", 0)

        tools = ("ballend", "bullnose", "endmill")
        if obj.ToolController:
            tool = obj.ToolController.Tool.ShapeID.casefold()
            chamferMode = 0 if obj.Strategy == "Chamfer" and tool in tools else 2
            bottomMode = 0 if tool in tools else 2
            obj.setEditorMode("Angle", chamferMode)
            obj.setEditorMode("StartFromBottom", bottomMode)

        sortingMode = 0 if obj.SortingMode == "Automatic" else 2
        obj.setEditorMode("StartPoint", sortingMode)
        obj.setEditorMode("EndPoint", sortingMode)
        obj.setEditorMode("UseEndPoint", sortingMode)

    def opOnDocumentRestored(self, obj):
        if obj.StepDown == 0:
            obj.clearExpression("StepDown")
            obj.StepDown = 999

        self.propertiesReady = False
        self.initOpProperties(obj, warn=True)
        self.opSetDefaultValues(obj, PathUtils.findParentJob(obj))
        self.setOpEditorProperties(obj)

    def opOnChanged(self, obj, prop):
        """opOnChanged(obj, prop) ... Called when a property changes"""
        if getattr(self, "propertiesReady", False):
            self.setOpEditorProperties(obj)

        super().opOnChanged(obj, prop)

    def opExecute(self, obj):
        if not obj.Base:
            return

        if obj.Angle > 89:
            obj.Angle = 89
        if obj.Angle < 1:
            obj.Angle = 1

        if obj.Width < 0.01:
            obj.Width = 0.01

        depths, offsets = self.toolDepthAndOffset(obj)
        if not depths:
            Path.Log.warning(
                translate("CAM_Deburring", "%s: Depths and offsets is empty. Check step down value")
                % obj.Label
            )
            return

        if obj.StartFromBottom:
            offsets = offsets[::-1]
            depths = depths[::-1]

        tol = self.job.GeometryTolerance.Value or 0.01
        solids = [base.Shape for base in self.model if base.Shape.Faces]

        edges = []
        faces = []
        wireTups = []
        for base, subsList in self.baseShapes(obj):
            for subName in subsList:
                sub = getattr(base.Shape, subName, None)
                if isinstance(sub, Part.Edge):
                    edges.append(sub)
                elif isinstance(sub, Part.Face):
                    faces.append(sub)

        for face in faces:
            if Path.Geom.isHorizontal(face):
                outerWire, innerHoles, innerCircles = self.separateFaceWires(face, max(offsets))
                if obj.ProcessPerimeter:
                    wireTups.append((outerWire, False))
                if obj.ProcessHoles:
                    wireTups.extend((w, True) for w in innerHoles)
                if obj.ProcessCircles:
                    wireTups.extend((w, True) for w in innerCircles)
            else:  # angled face
                fbb = face.BoundBox
                bottom_edges = [
                    e
                    for e in face.Edges
                    if Path.Geom.isHorizontal(e) and Path.Geom.isRoughly(e.BoundBox.ZMax, fbb.ZMin)
                ]
                for edge in bottom_edges:
                    edge.translate(FreeCAD.Vector(0, 0, fbb.ZMax - bottom_edges[0].BoundBox.ZMax))
                edges.extend(bottom_edges)

        wireTups.extend((Part.Wire(se), False) for se in Part.sortEdges(edges))

        if not wireTups:
            return

        if obj.SplitArcs:
            wireTups = [(PathOpUtil.discretizeWire(w), isHole) for w, isHole in wireTups]

        if obj.SortingMode == "Automatic" and len(wireTups) > 1:
            wireTups = self.getSortedTups(obj, wireTups)

        owireTups = []
        for tup in wireTups:
            wire = tup[0]

            if Path.Geom.isRoughly(wire.BoundBox.ZLength, 0):  # flat wire in XY plane
                isHole = tup[1]
                index = obj.Side == "Inside"
                index = not index if isHole else index
                last_candidates = None
                for i in range(len(offsets)):
                    if last_candidates and offsets[i] == offsets[i - 1]:
                        # do not create offset, if it was already created before
                        candidates = last_candidates
                    else:
                        candidates = PathOpUtil.offsetWire(wire, solids, offsets[i], tol)
                        last_candidates = candidates
                    for w in candidates[index]:
                        owireTups.append((w.translated(FreeCAD.Vector(0, 0, depths[i])), isHole))

            else:  # 3d wire
                last_owire = None
                for i in range(len(offsets)):
                    offset = offsets[i] if obj.Side == "Outside" else -offsets[i]
                    if last_owire and offsets[i] == offsets[i - 1]:
                        owire = last_owire
                    else:
                        wire = PathOpUtil.orientWire(wire, True)
                        wall = wire.extrude(FreeCAD.Vector(0, 0, 10))
                        owall = wall.makeOffsetShape(offset, tolerance=tol, join=2)
                        edges = [e for e in owall.Edges if not Path.Geom.isVertical(e)]
                        owire = Part.Wire(Part.__sortEdges__(edges))
                        last_owire = owire
                    diffz = wire.BoundBox.ZMax - owire.BoundBox.ZMax + depths[i]
                    owireTups.append((owire.translated(FreeCAD.Vector(0, 0, diffz)), False))
                    # Part.show(owires[-1], "owire_translated")

        pathParams = {
            "shapes": None,
            "start": None,
            "sort_mode": 1,
            "orientation": 0,
            "retraction": obj.ClearanceHeight.Value,
            "resume_height": obj.SafeHeight.Value,
            "feedrate": self.horizFeed,
            "feedrate_v": self.vertFeed,
            "verbose": True,
            "preamble": False,
            "min_dist": 0,
        }

        startPoint = self.startPoint(obj)
        linkingArgs = linking.get_linking_args(obj, self.job)

        for i, (wire, isHole) in enumerate(owireTups):
            pathParams["orientation"] = self.getPathOrientation(obj, isHole)
            pathParams["shapes"] = [wire]
            pathParams["start"] = startPoint
            pp = Path.fromShapes(**pathParams)
            if not pp.Size:
                continue

            while pp.Commands[0].Name in Constants.GCODE_MOVE_RAPID:
                pp.deleteCommand(0)  # remove rapid moves
            plungeMove = pp.Commands[0]
            p = Path.Geom.commandEndPoint(plungeMove)
            pp.deleteCommand(0)  # remove plunge move

            if i == 0:  # first rapid moves at clearance height
                self.commandlist.append(Path.Command("G0", {"Z": obj.ClearanceHeight.Value}))
                self.commandlist.append(Path.Command("G0", {"X": p.x, "Y": p.y}))
                self.commandlist.append(Path.Command("G0", {"Z": obj.SafeHeight.Value}))
                par = {"X": p.x, "Y": p.y, "Z": p.z, "F": self.vertFeed}
                self.commandlist.append(Path.Command("G1", par))
            else:  # linking moves
                linkingArgs["start_position"] = startPoint
                linkingArgs["target_position"] = p
                linkingMoves = linking.get_linking_moves(**linkingArgs)
                for cmd in linkingMoves:
                    if cmd.z < obj.SafeHeight.Value:
                        cmd.Name = "G1"
                        par = cmd.Parameters
                        par["F"] = self.vertFeed
                        cmd.Parameters = par
                self.commandlist.extend(linkingMoves)

            self.commandlist.extend(pp.Commands)
            startPoint = Path.Geom.commandEndPoint(self.commandlist[-1])

    def getSortedTups(self, obj, wireTups):
        tunnels = []
        for tup in wireTups:
            wire = tup[0]
            indexEnd = -1 if not wire.isClosed() else 0
            tunnels.append(
                {
                    "startX": wire.Vertexes[0].X,
                    "startY": wire.Vertexes[0].Y,
                    "endX": wire.Vertexes[indexEnd].X,
                    "endY": wire.Vertexes[indexEnd].Y,
                }
            )
        sortedIndexes = tsp_solver.solveTunnels(
            tunnels,
            allowFlipping=False,
            routeStartPoint=self.toFrame(obj.StartPoint),
            routeEndPoint=self.toFrame(obj.EndPoint) if obj.UseEndPoint else None,
        )
        return [wireTups[t["index"]] for t in sortedIndexes]

    def getPathOrientation(self, obj, isHole):
        orientation = obj.Direction == "CW"
        orientation = not orientation if isHole else orientation
        return int(orientation)

    def toolDepthAndOffset(self, obj):
        """getOffset(width, extraDepth, tool) ... returns list of offsets and depths"""

        tools = ["ballend", "bullnose", "endmill"]
        chamfer_tools = tools + ["chamfer", "v-bit"]
        fillet_tools = tools + ["radius"]

        if (obj.Strategy == "Chamfer" and self.tool.ShapeID.casefold() not in chamfer_tools) or (
            obj.Strategy == "Fillet" and self.tool.ShapeID not in fillet_tools
        ):
            Path.Log.warning(
                translate("CAM_Deburring", "%s: not appropriate tool (%s) for %s")
                % (obj.Label, self.tool.ShapeID, obj.Strategy)
            )
            return [], []

        offsets = []
        depths = []
        step_down = obj.StepDown.Value
        radial_offset = obj.RadialStockToLeave.Value
        extra_depth = obj.ExtraDepth.Value
        width = obj.Width.Value

        if obj.Strategy == "Chamfer" and self.tool.ShapeID.casefold() in ("chamfer", "v-bit"):
            tool_offset = self.tool.TipDiameter.Value / 2
            angle = self.tool.CuttingEdgeAngle.Value
            tan = math.tan(math.radians(angle / 2))
            extra_offset = extra_depth * tan
            offset = tool_offset + extra_offset
            tool_depth = -width / tan
            top_z = 0
            bottom_z = tool_depth - extra_depth
            steps = Path.Geom.ceil((top_z - bottom_z) / step_down)
            increment_z = (top_z - bottom_z) / steps
            for i in range(steps):
                z = top_z - (i + 1) * increment_z
                depths.append(z)
                offsets.append(offset + radial_offset)
            return depths, offsets

        tool_radius = self.tool.Diameter.Value / 2
        chamfer_angle = math.radians(obj.Angle.Value)
        tan = math.tan(chamfer_angle)

        if obj.Strategy == "Chamfer" and self.tool.ShapeID.casefold() == "endmill":
            top_z = 0
            bottom_z = -tan * width
            steps = Path.Geom.ceil((top_z - bottom_z) / step_down)
            increment_z = (top_z - bottom_z) / steps
            for i in range(steps - 1):
                z = top_z - (i + 1) * increment_z
                depths.append(z - extra_depth)
                offset = -width - z / tan + tool_radius
                offsets.append(offset + radial_offset)
            return depths, offsets

        if obj.Strategy == "Chamfer" and self.tool.ShapeID.casefold() == "ballend":
            dz = tool_radius - math.cos(chamfer_angle) * tool_radius
            top_z = dz
            bottom_z = -tan * width
            steps = Path.Geom.ceil((top_z - bottom_z) / step_down)
            increment_z = (top_z - bottom_z) / steps
            for i in range(steps):
                z = top_z - (i + 1) * increment_z
                depths.append(z - dz - extra_depth)
                offset = -width - z / tan + math.sin(chamfer_angle) * tool_radius
                offsets.append(offset + radial_offset)
            return depths, offsets

        if obj.Strategy == "Chamfer" and self.tool.ShapeID.casefold() == "bullnose":
            corner_radius = self.tool.CornerRadius.Value
            dz = corner_radius * (1 - math.cos(chamfer_angle))
            top_z = dz
            bottom_z = -tan * width
            steps = Path.Geom.ceil((top_z - bottom_z) / step_down)
            increment_z = (top_z - bottom_z) / steps
            for i in range(steps):
                z = top_z - (i + 1) * increment_z
                depths.append(z - dz - extra_depth)
                offset = (
                    -width
                    - z / tan
                    + math.sin(chamfer_angle) * corner_radius
                    + (tool_radius - corner_radius)
                )
                offsets.append(offset + radial_offset)
            return depths, offsets

        fillet_radius = obj.Width.Value

        if obj.Strategy == "Fillet" and self.tool.ShapeID.casefold() == "radius":
            cutting_radius = self.tool.CuttingRadius.Value
            tip_radius = self.tool.TipDiameter.Value / 2
            tool_offset = cutting_radius - cutting_radius * math.cos(math.pi / 4) + tip_radius
            top_z = -cutting_radius * math.sin(math.pi / 4)

            chord_length = fillet_radius * math.cos(math.pi / 4) * 2
            chord_angle = math.asin(chord_length / 2 / cutting_radius)
            math.degrees(chord_angle)
            mid_ordinate = cutting_radius - cutting_radius * math.cos(chord_angle)
            bottom_z = top_z - fillet_radius / 2 + mid_ordinate * math.cos(math.pi / 4)

            steps = Path.Geom.ceil((top_z - bottom_z) / step_down)
            increment = (top_z - bottom_z) / steps
            for i in range(steps):
                z = top_z - (i + 1) * increment
                depths.append(z - extra_depth)
                offset = tool_offset - (i + 1) * increment
                offsets.append(offset + radial_offset)
            return depths, offsets

        step_angle = step_down / fillet_radius  # use step down as length of arc
        steps = Path.Geom.ceil(math.pi / 2 / step_angle)
        increment_angle = math.pi / 2 / steps

        if obj.Strategy == "Fillet" and self.tool.ShapeID.casefold() == "endmill":
            for i in range(steps):
                angle = (i + 1) * increment_angle
                z = -(fillet_radius - fillet_radius * math.cos(angle))
                depths.append(z - extra_depth)
                x = -(fillet_radius - fillet_radius * math.sin(angle)) + tool_radius
                offsets.append(x + radial_offset)
            return depths, offsets

        if obj.Strategy == "Fillet" and self.tool.ShapeID.casefold() == "ballend":
            for i in range(steps):
                angle = (i + 1) * increment_angle
                z_ball_center = -fillet_radius + (fillet_radius + tool_radius) * math.cos(angle)
                z = z_ball_center - tool_radius
                depths.append(z - extra_depth)
                x = (fillet_radius + tool_radius) * math.sin(angle) - fillet_radius
                offsets.append(x + radial_offset)
            return depths, offsets

        if obj.Strategy == "Fillet" and self.tool.ShapeID.casefold() == "bullnose":
            corner_radius = self.tool.CornerRadius.Value
            for i in range(steps):
                angle = (i + 1) * increment_angle
                z_ball_center = -fillet_radius + (fillet_radius + corner_radius) * math.cos(angle)
                z = z_ball_center - corner_radius
                depths.append(z - extra_depth)
                x = (
                    (fillet_radius + corner_radius) * math.sin(angle)
                    - fillet_radius
                    + tool_radius
                    - corner_radius
                )
                offsets.append(x + radial_offset)
            return depths, offsets

        return depths, offsets

    def separateFaceWires(self, face, offset):
        """separateFaceWires(face) ... return outerWire, innerHoles and innerCircles of face"""
        outerWire = face.OuterWire
        outerIndex = [w.hashCode() for w in face.Wires].index(outerWire.hashCode())

        innerWires = face.Wires
        del innerWires[outerIndex]

        innerHoles = []
        innerCircles = []
        for w in innerWires:
            f = Part.makeFace(w, "Part::FaceMakerSimple")
            if isDrillableFace(f, tooldiameter=2 * offset, vector=None):
                innerCircles.append(w)
            else:
                innerHoles.append(w)

        return outerWire, innerHoles, innerCircles


def SetupProperties():
    setup = []
    setup.append("Angle")
    setup.append("Direction")
    setup.append("ExtraDepth")
    setup.append("ProcessHoles")
    setup.append("ProcessPerimeter")
    setup.append("RadialStockToLeave")
    setup.append("Side")
    setup.append("SplitArcs")
    setup.append("SortingMode")
    setup.append("StartFromBottom")
    setup.append("Strategy")
    setup.append("Width")
    setup.append("UseEndPoint")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Deburring operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectDeburring(obj, name, parentJob)
    return obj
