# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2018 Kresimir Tusek <kresimir.tusek@gmail.com>          *
# *   Copyright (c) 2019-2021 Schildkroet                                   *
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
import PathScripts.PathUtils as PathUtils
import PathScripts.PathSelectionProcessing as SelectionProcessing

from PySide import QtCore

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")
DraftGeomUtils = LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")
PathGeom = LazyLoader("PathScripts.PathGeom", globals(), "PathScripts.PathGeom")
PathOpTools = LazyLoader(
    "PathScripts.PathOpTools", globals(), "PathScripts.PathOpTools"
)
# time = LazyLoader('time', globals(), 'time')
json = LazyLoader("json", globals(), "json")
math = LazyLoader("math", globals(), "math")
area = LazyLoader("area", globals(), "area")

if FreeCAD.GuiUp:
    coin = LazyLoader("pivy.coin", globals(), "pivy.coin")
    FreeCADGui = LazyLoader("FreeCADGui", globals(), "FreeCADGui")


__title__ = "Path Strategies"
__author__ = "Yorik van Havre; sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Path strategies available for path generation."
__contributors__ = ""


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


def getTransitionHeight(baseShape, start, end, toolDiameter):
    """getTransitionHeight(baseShape, start, end, toolDiameter)...
    Make simple circle with diameter of tool, at start point.
    Extrude it latterally along path.
    Extrude it vertically.
    Check for collision with model."""
    # Make path travel of tool as 3D solid.
    if abs(start.z - end.z) > 0.0000001:
        return None

    clearanceHeight = baseShape.BoundBox.ZMax
    layerDepth = start.z
    rad = (toolDiameter / 2.0) - 0.0005  # reduce radius by 50 thousands millimeter

    def getPerp(start, end, dist):
        toEnd = end.sub(start)
        perp = FreeCAD.Vector(-1 * toEnd.y, toEnd.x, 0.0)
        if perp.x == 0 and perp.y == 0:
            return perp
        perp.normalize()
        perp.multiply(dist)
        return perp

    # Make first cylinder
    ce1 = Part.Wire(Part.makeCircle(rad, start).Edges)
    cylinder1 = Part.Face(ce1)
    zTrans = layerDepth - cylinder1.BoundBox.ZMin
    cylinder1.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
    extFwd = clearanceHeight - layerDepth
    if extFwd <= 0.0:
        extFwd = 1.0
    extVect = FreeCAD.Vector(0.0, 0.0, extFwd)
    startShp = cylinder1.extrude(extVect)

    if end.sub(start).Length > 0:
        # Make second cylinder
        ce2 = Part.Wire(Part.makeCircle(rad, end).Edges)
        cylinder2 = Part.Face(ce2)
        zTrans = layerDepth - cylinder2.BoundBox.ZMin
        cylinder2.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
        endShp = cylinder2.extrude(extVect)

        # Make extruded rectangle to connect cylinders
        perp = getPerp(start, end, rad)
        v1 = start.add(perp)
        v2 = start.sub(perp)
        v3 = end.sub(perp)
        v4 = end.add(perp)
        e1 = Part.makeLine(v1, v2)
        e2 = Part.makeLine(v2, v3)
        e3 = Part.makeLine(v3, v4)
        e4 = Part.makeLine(v4, v1)
        edges = Part.__sortEdges__([e1, e2, e3, e4])
        rectFace = Part.Face(Part.Wire(edges))
        zTrans = layerDepth - rectFace.BoundBox.ZMin
        rectFace.translate(FreeCAD.Vector(0.0, 0.0, zTrans))
        boxShp = rectFace.extrude(extVect)

        # Fuse two cylinders and box together
        part1 = startShp.fuse(boxShp)
        pathTravel = part1.fuse(endShp).removeSplitter()
    else:
        pathTravel = startShp

    # Debugging
    # Part.show(pathTravel)
    # FreeCAD.ActiveDocument.ActiveObject.Label = 'PathTravel'
    # FreeCAD.ActiveDocument.ActiveObject.purgeTouched()

    contact = pathTravel.common(baseShape)
    if hasattr(contact, "Volume") and contact.Volume > 0.0:
        # Debugging
        # Part.show(contact)
        # FreeCAD.ActiveDocument.ActiveObject.Label = 'contact'
        # FreeCAD.ActiveDocument.ActiveObject.purgeTouched()
        return contact.BoundBox.ZMax

    return layerDepth


class StrategyClearVolume:
    """StrategyClearVolume()...
    Creates a path geometry shape from an assigned pattern for conversion to tool paths.
    Arguments:
                 callerClass: Reference to calling class - usually a `self` argument upon instantiation.
                 volumeShape: 3D volume shape to be cleared.
                 clearanceHeight: clearance height for paths
                 safeHeight: safe height for paths
                 patternCenterAt: Choice of centering options, including a `Custom` option
                 patternCenterCustom: Custom pattern center point when provided, otherwise, a feedback of calculated pattern center point.
                 cutPatternReversed: Set True to reverse the cut pattern - outside inward, or inside outward.
                 cutPatternAngle: Angle used to rotate certain cut patterns.
                 cutPattern: Choice of cut pattern: Adaptive, Circular, CircularZigZag, Grid, Line, LineOffset, Offset, Spiral, Triangle, ZigZag, ZigZagOffset.
                 cutDirection: Cut direction: Climb or Conventional
                 stepOver: Step over value.
                 materialAllowance: Material allowance, or extra offset.
                 minTravel: Boolean to enable minimum travel (disabled)
                 keepToolDown: Boolean to force keeping tool down.
                 toolController: Reference to active tool controller object.
                 startPoint: Vector start point, or None.
                 depthParams: list of depth parameters, or reference to depthParam object
                 jobTolerance: job tolerance value
    Usage:
        - Call the _generatePathGeometry() method to request the path geometry.
        - The path geometry has correctional linking applied.
    """

    def __init__(
        self,
        callerClass,
        volumeShape,
        clearanceHeight,
        safeHeight,
        patternCenterAt,
        patternCenterCustom,
        cutPatternReversed,
        cutPatternAngle,
        cutPattern,
        cutDirection,
        stepOver,
        materialAllowance,
        minTravel,
        keepToolDown,
        toolController,
        startPoint,
        depthParams,
        jobTolerance,
    ):
        """__init__(volumeShape, clearanceHeight, safeHeight, patternCenterAt,
                    patternCenterCustom, cutPatternReversed, cutPatternAngle, stepOver,
                    cutPattern, cutDirection, materialAllowance,
                    toolController, startPoint, depthParams, jobTolerance)...
        StrategyClearing class constructor method.
        """
        PathLog.debug("StrategyClearing.__init__()")

        # Debugging attributes
        self.isDebug = True if PathLog.getLevel(PathLog.thisModule()) == 4 else False
        self.showDebugShapes = False
        self.callerClass = callerClass

        self.safeBaseShape = None
        self.cutPattern = "None"
        self.face = None
        self.rawGeoList = None
        self.centerOfMass = None
        self.centerOfPattern = None
        self.halfDiag = None
        self.halfPasses = None
        self.workingPlane = Part.makeCircle(2.0)  # make circle for workplane
        self.rawPathGeometry = None
        self.linkedPathGeom = None
        self.endVectors = list()
        self.pathGeometry = list()
        self.commandList = list()
        self.useStaticCenter = True
        self.isCenterSet = False
        self.offsetDirection = -1.0  # 1.0=outside;  -1.0=inside
        self.endVector = None
        self.pathParams = ""
        self.areaParams = ""
        self.simObj = None
        self.transitionClearance = 2.0  # millimeters

        # Save argument values to class instance
        self.volumeShape = volumeShape
        self.depthParams = depthParams
        self.clearanceHeight = clearanceHeight
        self.safeHeight = safeHeight
        self.patternCenterAt = patternCenterAt
        self.patternCenterCustom = patternCenterCustom
        self.cutPattern = cutPattern
        self.cutPatternReversed = cutPatternReversed
        self.cutPatternAngle = cutPatternAngle
        self.cutDirection = cutDirection
        self.stepOver = stepOver
        self.materialAllowance = materialAllowance
        self.minTravel = minTravel
        self.keepToolDown = keepToolDown
        self.toolController = toolController
        self.jobTolerance = jobTolerance
        self.startPoint = startPoint
        self.prevDepth = safeHeight

        self.vertFeed = toolController.VertFeed.Value
        self.vertRapid = toolController.VertRapid.Value
        self.horizFeed = toolController.HorizFeed.Value
        self.horizRapid = toolController.HorizRapid.Value
        self.toolDiameter = (
            toolController.Tool.Diameter.Value
            if hasattr(toolController.Tool.Diameter, "Value")
            else float(toolController.Tool.Diameter)
        )
        self.toolRadius = self.toolDiameter / 2.0
        self.cutOut = self.toolDiameter * (self.stepOver / 100.0)
        # Setting toolDownThreshold below effective cut out (stepover * tool diameter) will keep tool down between transitions
        self.toolDownThreshold = self.toolDiameter

        # Grid and Triangle pattern requirements - paths produced by Path.Area() and Path.fromShapes()
        self.pocketMode = 6
        self.orientation = 0  # ['Conventional', 'Climb']

        # Adaptive-dependent attributes to be set by call to `setAdaptiveAttributes()` with required arguments
        self.startDepth = None
        self.finalDepth = None
        self.stepDown = None
        self.finishDepth = None
        self.operationType = None
        self.cutSide = None
        self.disableHelixEntry = None
        self.forceInsideOut = None
        self.liftDistance = None
        self.finishingProfile = None
        self.helixAngle = None
        self.helixConeAngle = None
        self.useHelixArcs = None
        self.helixDiameterLimit = None
        self.keepToolDownRatio = None
        self.tolerance = None
        self.stockObj = None
        self.viewObject = None

    def _debugMsg(self, msg, isError=False):
        """_debugMsg(msg)
        If `self.isDebug` flag is True, the provided message is printed in the Report View.
        If not, then the message is assigned a debug status.
        """
        if isError:
            FreeCAD.Console.PrintError("StrategyClearVolume: " + msg + "\n")
            return

        if self.isDebug:
            # PathLog.info(msg)
            FreeCAD.Console.PrintMessage("StrategyClearVolume: " + msg + "\n")
        else:
            PathLog.debug(msg)

    def _addDebugObject(self, objShape, objName="shape"):
        """_addDebugObject(objShape, objName='shape')
        If `self.isDebug` and `self.showDebugShapes` flags are True, the provided
        debug shape will be added to the active document with the provided name.
        """
        if self.isDebug and self.showDebugShapes:
            O = FreeCAD.ActiveDocument.addObject("Part::Feature", "debug_" + objName)
            O.Shape = objShape
            O.purgeTouched()

    def _getPathGeometry(self, face):
        """_getPathGeometry(face)... Simple switch controller for obtaining the path geometry."""
        pGG = PathGeometryGenerator(
            self,
            face,
            self.patternCenterAt,
            self.patternCenterCustom,
            self.cutPatternReversed,
            self.cutPatternAngle,
            self.cutPattern,
            self.cutDirection,
            self.stepOver,
            self.materialAllowance,
            self.minTravel,
            self.keepToolDown,
            self.toolController,
            self.jobTolerance,
        )

        if self.cutPattern == "Adaptive":
            pGG.setAdaptiveAttributes(
                self.operationType,
                self.cutSide,
                self.disableHelixEntry,
                self.forceInsideOut,
                self.liftDistance,
                self.finishingProfile,
                self.helixAngle,
                self.helixConeAngle,
                self.useHelixArcs,
                self.helixDiameterLimit,
                self.keepToolDownRatio,
                self.tolerance,
                self.stockShape,
            )

        pGG.useStaticCenter = self.useStaticCenter
        pGG.execute()
        self.centerOfPattern = pGG.centerOfPattern  # Retreive center of cut pattern
        return pGG.pathGeometry

    def _buildPaths(self, height, wireList):
        """_buildPaths(height, wireList) ... Method to convert wires into paths."""
        PathLog.debug("_buildPaths()")

        if self.cutPattern == "Offset":
            return self._buildOffsetPaths(height, wireList)

        if self.cutPattern == "Spiral":
            return self._buildSpiralPaths(height, wireList)

        if self.cutPattern in ["Line", "LineOffset"]:
            return self._buildLinePaths(height, wireList)

        if self.cutPattern in ["ZigZag", "ZigZagOffset", "CircularZigZag"]:
            return self._buildZigZagPaths(height, wireList)

        paths = []
        end_vector = None  # FreeCAD.Vector(0.0, 0.0, self.clearanceHeight)
        useStart = False
        if self.startPoint:
            useStart = True

        pathParams = {}  # pylint: disable=assignment-from-no-return
        pathParams["feedrate"] = self.horizFeed
        pathParams["feedrate_v"] = self.vertFeed
        pathParams["verbose"] = True
        pathParams["return_end"] = False  # True to return end vector
        pathParams["resume_height"] = self.safeHeight
        pathParams["retraction"] = self.clearanceHeight
        pathParams[
            "preamble"
        ] = False  # Eemitting preambles between moves breaks some dressups and prevents path optimization on some controllers

        # More work is needed on this feature before implementation
        # if self.keepToolDown:
        #    pathParams['threshold'] = self.toolDiameter * 1.001

        for wire in wireList:
            wire.translate(FreeCAD.Vector(0, 0, height))

            pathParams["shapes"] = [wire]

            vrtxs = wire.Vertexes
            if useStart:
                pathParams["start"] = FreeCAD.Vector(
                    self.startPoint.x, self.startPoint.y, self.safeHeight
                )
                useStart = False
            else:
                pathParams["start"] = FreeCAD.Vector(vrtxs[0].X, vrtxs[0].Y, vrtxs[0].Z)

            # (pp, end_vector) = Path.fromShapes(**pathParams)
            pp = Path.fromShapes(**pathParams)
            paths.extend(pp.Commands)

        self.pathParams = str(
            {key: value for key, value in pathParams.items() if key != "shapes"}
        )
        self.endVectors.append(end_vector)

        # PathLog.debug("Path with params: {} at height: {}".format(self.pathParams, height))

        return paths

    def _buildGridAndTrianglePaths(self, getsim=False):
        """_buildGridAndTrianglePaths(getsim=False) ... Generate paths for Grid and Triangle patterns."""
        PathLog.track()
        areaParams = {}
        pathParams = {}
        heights = [i for i in self.depthParams]
        PathLog.debug("depths: {}".format(heights))

        if self.cutPattern == "Triangle":
            self.pocketMode = 7
        if self.cutDirection == "Climb":
            self.orientation = 1

        areaParams["Fill"] = 0
        areaParams["Coplanar"] = 0
        areaParams["PocketMode"] = 1
        areaParams["SectionCount"] = -1
        areaParams["Angle"] = self.cutPatternAngle
        areaParams["FromCenter"] = not self.cutPatternReversed
        areaParams["PocketStepover"] = (self.toolRadius * 2) * (
            float(self.stepOver) / 100
        )
        areaParams["PocketExtraOffset"] = self.materialAllowance
        areaParams["ToolRadius"] = self.toolRadius
        # Path.Area() pattern list is ['None', 'ZigZag', 'Offset', 'Spiral', 'ZigZagOffset', 'Line', 'Grid', 'Triangle']
        areaParams[
            "PocketMode"
        ] = (
            self.pocketMode
        )  # should be a 6 or 7 to indicate the index for 'Grid' or 'Triangle'

        pathArea = Path.Area()
        pathArea.setPlane(PathUtils.makeWorkplane(Part.makeCircle(5.0)))
        pathArea.add(self.volumeShape)
        pathArea.setParams(**areaParams)

        # Save pathArea parameters
        self.areaParams = str(pathArea.getParams())
        PathLog.debug("Area with params: {}".format(pathArea.getParams()))

        # Extract layer sections from pathArea object
        sections = pathArea.makeSections(mode=0, project=False, heights=heights)
        PathLog.debug("sections = %s" % sections)
        shapelist = [sec.getShape() for sec in sections]
        PathLog.debug("shapelist = %s" % shapelist)

        # Set path parameters
        pathParams["orientation"] = self.orientation
        # if MinTravel is turned on, set path sorting to 3DSort
        # 3DSort shouldn't be used without a valid start point. Can cause
        # tool crash without it.
        #
        # ml: experimental feature, turning off for now (see https://forum.freecadweb.org/viewtopic.php?f=15&t=24422&start=30#p192458)
        # realthunder: I've fixed it with a new sorting algorithm, which I
        # tested fine, but of course need more test. Please let know if there is
        # any problem
        #
        if self.minTravel and self.startPoint:
            pathParams["sort_mode"] = 3
            pathParams["threshold"] = self.toolRadius * 2
        pathParams["shapes"] = shapelist
        pathParams["feedrate"] = self.horizFeed
        pathParams["feedrate_v"] = self.vertFeed
        pathParams["verbose"] = True
        pathParams["resume_height"] = self.safeHeight
        pathParams["retraction"] = self.clearanceHeight
        pathParams["return_end"] = True
        # Note that emitting preambles between moves breaks some dressups and prevents path optimization on some controllers
        pathParams["preamble"] = False

        if self.keepToolDown:
            pathParams["threshold"] = self.toolDiameter

        if self.endVector is not None:
            pathParams["start"] = self.endVector
        elif self.startPoint:
            pathParams["start"] = self.startPoint

        self.pathParams = str(
            {key: value for key, value in pathParams.items() if key != "shapes"}
        )
        PathLog.debug("Path with params: {}".format(self.pathParams))

        # Build paths from path parameters
        (pp, end_vector) = Path.fromShapes(**pathParams)
        PathLog.debug("pp: {}, end vector: {}".format(pp, end_vector))
        self.endVector = end_vector  # pylint: disable=attribute-defined-outside-init

        simobj = None
        if getsim:
            areaParams["Thicken"] = True
            areaParams["ToolRadius"] = self.toolRadius - self.toolRadius * 0.005
            pathArea.setParams(**areaParams)
            sec = pathArea.makeSections(mode=0, project=False, heights=heights)[
                -1
            ].getShape()
            simobj = sec.extrude(FreeCAD.Vector(0, 0, self.volumeShape.BoundBox.ZMax))

        self.commandList = pp.Commands
        self.simObj = simobj

    def _buildOffsetPaths(self, height, wireList):
        """_buildOffsetPaths(height, wireList) ... Convert Offset pattern wires to paths."""
        PathLog.debug("_buildOffsetPaths()")

        if self.keepToolDown:
            return self._buildKeepOffsetDownPaths(height, wireList)

        paths = []
        useStart = False
        if self.startPoint:
            useStart = True

        if len(self.commandList) == 0:
            paths.append(
                Path.Command("G0", {"Z": self.clearanceHeight, "F": self.vertRapid})
            )
            if useStart:
                paths.append(
                    Path.Command(
                        "G0",
                        {
                            "X": self.startPoint.x,
                            "Y": self.startPoint.y,
                            "F": self.horizRapid,
                        },
                    )
                )

        if self.cutDirection == "Climb":
            for wire in wireList:
                wire.translate(FreeCAD.Vector(0, 0, height))

                e0 = wire.Edges[len(wire.Edges) - 1]
                paths.append(
                    Path.Command(
                        "G0",
                        {
                            "X": e0.Vertexes[1].X,
                            "Y": e0.Vertexes[1].Y,
                            "F": self.horizRapid,
                        },
                    )
                )
                paths.append(
                    Path.Command("G0", {"Z": self.prevDepth + 0.1, "F": self.vertRapid})
                )
                paths.append(Path.Command("G1", {"Z": height, "F": self.vertFeed}))

                for i in range(len(wire.Edges) - 1, -1, -1):
                    e = wire.Edges[i]
                    paths.extend(
                        PathGeom.cmdsForEdge(e, flip=True, hSpeed=self.horizFeed)
                    )

                paths.append(
                    Path.Command("G0", {"Z": self.safeHeight, "F": self.vertRapid})
                )

        else:
            for wire in wireList:
                wire.translate(FreeCAD.Vector(0, 0, height))

                e0 = wire.Edges[0]
                paths.append(
                    Path.Command(
                        "G0",
                        {
                            "X": e0.Vertexes[0].X,
                            "Y": e0.Vertexes[0].Y,
                            "F": self.horizRapid,
                        },
                    )
                )
                paths.append(
                    Path.Command("G0", {"Z": self.prevDepth + 0.1, "F": self.vertRapid})
                )
                paths.append(Path.Command("G1", {"Z": height, "F": self.vertFeed}))

                for e in wire.Edges:
                    paths.extend(PathGeom.cmdsForEdge(e, hSpeed=self.horizFeed))

                paths.append(
                    Path.Command("G0", {"Z": self.safeHeight, "F": self.vertRapid})
                )

        return paths

    def _buildKeepOffsetDownPaths(self, height, wireList):
        """_buildKeepOffsetDownPaths(height, wireList) ... Convert Offset pattern wires to paths."""
        PathLog.debug("_buildKeepOffsetDownPaths()")

        paths = []
        useStart = False
        if self.startPoint:
            useStart = True

        if len(self.commandList) == 0:
            paths.append(
                Path.Command("G0", {"Z": self.clearanceHeight, "F": self.vertRapid})
            )
            if useStart:
                paths.append(
                    Path.Command(
                        "G0",
                        {
                            "X": self.startPoint.x,
                            "Y": self.startPoint.y,
                            "F": self.horizRapid,
                        },
                    )
                )

        lastPnt = None
        if self.cutDirection == "Climb":
            for wire in wireList:
                wire.translate(FreeCAD.Vector(0, 0, height))
                e0 = wire.Edges[len(wire.Edges) - 1]
                pnt0 = e0.Vertexes[1].Point

                if lastPnt:
                    # get transition height from end of last wire to start of current wire
                    transHeight = getTransitionHeight(
                        self.safeBaseShape, lastPnt, pnt0, self.toolDiameter
                    )
                    if not PathGeom.isRoughly(transHeight, height):
                        transHeight += self.transitionClearance
                        if transHeight < self.prevDepth + 1.0:
                            transHeight = self.prevDepth + 1.0
                        transHeight = min(transHeight, self.safeHeight)
                        paths.extend(
                            self._hopPath(
                                transHeight, e0.Vertexes[1].X, e0.Vertexes[1].Y, height
                            )
                        )
                    else:
                        if (
                            pnt0.sub(lastPnt).Length > self.toolDownThreshold
                        ):  # if tran dist > toolDiam, hop
                            # transHeight = min(height + 1.0, self.safeHeight)
                            # transHeight = min(self.prevDepth + 1.0, self.safeHeight)
                            transHeight = self.safeHeight
                            paths.extend(
                                self._hopPath(
                                    transHeight,
                                    e0.Vertexes[1].X,
                                    e0.Vertexes[1].Y,
                                    height,
                                )
                            )
                        else:
                            paths.append(
                                Path.Command(
                                    "G1",
                                    {
                                        "X": e0.Vertexes[1].X,
                                        "Y": e0.Vertexes[1].Y,
                                        "F": self.horizRapid,
                                    },
                                )
                            )
                else:
                    paths.append(
                        Path.Command(
                            "G0",
                            {
                                "X": e0.Vertexes[1].X,
                                "Y": e0.Vertexes[1].Y,
                                "F": self.horizRapid,
                            },
                        )
                    )
                    paths.append(
                        Path.Command(
                            "G0", {"Z": self.prevDepth + 0.1, "F": self.vertRapid}
                        )
                    )
                    paths.append(Path.Command("G1", {"Z": height, "F": self.vertFeed}))

                for i in range(len(wire.Edges) - 1, -1, -1):
                    e = wire.Edges[i]
                    paths.extend(
                        PathGeom.cmdsForEdge(e, flip=True, hSpeed=self.horizFeed)
                    )

                # Save last point
                lastPnt = wire.Edges[0].Vertexes[0].Point

        else:
            for wire in wireList:
                wire.translate(FreeCAD.Vector(0, 0, height))
                eCnt = len(wire.Edges)
                e0 = wire.Edges[0]
                pnt0 = e0.Vertexes[0].Point

                if lastPnt:
                    # get transition height from end of last wire to start of current wire
                    transHeight = getTransitionHeight(
                        self.safeBaseShape, lastPnt, pnt0, self.toolDiameter
                    )
                    if not PathGeom.isRoughly(transHeight, height):
                        transHeight += self.transitionClearance
                        if transHeight < self.prevDepth + 1.0:
                            transHeight = self.prevDepth + 1.0
                        transHeight = min(transHeight, self.safeHeight)
                        paths.extend(
                            self._hopPath(
                                transHeight, e0.Vertexes[0].X, e0.Vertexes[0].Y, height
                            )
                        )
                    else:
                        if (
                            pnt0.sub(lastPnt).Length > self.toolDownThreshold
                        ):  # if tran dist > toolDiam, hop
                            # transHeight = min(height + 1.0, self.safeHeight)
                            # transHeight = min(self.prevDepth + 1.0, self.safeHeight)
                            transHeight = self.safeHeight
                            paths.extend(
                                self._hopPath(
                                    transHeight,
                                    e0.Vertexes[0].X,
                                    e0.Vertexes[0].Y,
                                    height,
                                )
                            )
                        else:
                            paths.append(
                                Path.Command(
                                    "G1",
                                    {
                                        "X": e0.Vertexes[0].X,
                                        "Y": e0.Vertexes[0].Y,
                                        "F": self.horizRapid,
                                    },
                                )
                            )
                else:
                    paths.append(
                        Path.Command(
                            "G0",
                            {
                                "X": e0.Vertexes[0].X,
                                "Y": e0.Vertexes[0].Y,
                                "F": self.horizRapid,
                            },
                        )
                    )
                    paths.append(
                        Path.Command(
                            "G0", {"Z": self.prevDepth + 0.1, "F": self.vertRapid}
                        )
                    )
                    paths.append(Path.Command("G1", {"Z": height, "F": self.vertFeed}))

                for i in range(0, eCnt):
                    paths.extend(
                        PathGeom.cmdsForEdge(wire.Edges[i], hSpeed=self.horizFeed)
                    )

                # Save last point
                lastEdgeVertexes = wire.Edges[eCnt - 1].Vertexes
                lastPnt = lastEdgeVertexes[len(lastEdgeVertexes) - 1].Point
        # Eif

        return paths

    def _buildLinePaths(self, height, wireList):
        """_buildLinePaths(height, wireList) ... Convert Line-based wires to paths."""
        PathLog.debug("_buildLinePaths()")

        paths = []
        useStart = False
        if self.startPoint:
            useStart = True

        if len(self.commandList) == 0:
            paths.append(
                Path.Command("G0", {"Z": self.clearanceHeight, "F": self.vertRapid})
            )
            if useStart:
                paths.append(
                    Path.Command(
                        "G0",
                        {
                            "X": self.startPoint.x,
                            "Y": self.startPoint.y,
                            "F": self.horizRapid,
                        },
                    )
                )

        for wire in wireList:
            wire.translate(FreeCAD.Vector(0, 0, height))

            e0 = wire.Edges[0]
            paths.append(
                Path.Command(
                    "G0",
                    {
                        "X": e0.Vertexes[0].X,
                        "Y": e0.Vertexes[0].Y,
                        "F": self.horizRapid,
                    },
                )
            )
            paths.append(
                Path.Command("G0", {"Z": self.prevDepth + 0.1, "F": self.vertRapid})
            )
            paths.append(Path.Command("G1", {"Z": height, "F": self.vertFeed}))

            for e in wire.Edges:
                paths.extend(PathGeom.cmdsForEdge(e, hSpeed=self.horizFeed))

            paths.append(
                Path.Command("G0", {"Z": self.safeHeight, "F": self.vertRapid})
            )

        return paths

    def _buildZigZagPaths(self, height, wireList):
        """_buildZigZagPaths(height, wireList) ... Convert ZigZab-based wires to paths.
        With KeepToolDown, the assumption is that material above clearing area where transitions cross part are cleared to that depth."""
        PathLog.debug("_buildZigZagPaths()")

        if not self.keepToolDown:
            return self._buildLinePaths(height, wireList)

        # Proceed with KeepToolDown proceedure
        paths = []
        useStart = False
        if self.startPoint:
            useStart = True

        if len(self.commandList) == 0:
            paths.append(
                Path.Command("G0", {"Z": self.clearanceHeight, "F": self.vertRapid})
            )
            if useStart:
                paths.append(
                    Path.Command(
                        "G0",
                        {
                            "X": self.startPoint.x,
                            "Y": self.startPoint.y,
                            "F": self.horizRapid,
                        },
                    )
                )

        lastPnt = None
        for wire in wireList:
            wire.translate(FreeCAD.Vector(0, 0, height))
            eCnt = len(wire.Edges)
            e0 = wire.Edges[0]
            pnt0 = e0.Vertexes[0].Point

            if lastPnt:
                # get transition height from end of last wire to start of current wire
                transHeight = getTransitionHeight(
                    self.safeBaseShape, lastPnt, pnt0, self.toolDiameter
                )
                if not PathGeom.isRoughly(transHeight, height):
                    transHeight += self.transitionClearance
                    if transHeight < self.prevDepth + 1.0:
                        transHeight = self.prevDepth + 1.0
                    transHeight = min(transHeight, self.safeHeight)
                    paths.extend(
                        self._hopPath(
                            transHeight, e0.Vertexes[0].X, e0.Vertexes[0].Y, height
                        )
                    )
                else:
                    if (
                        pnt0.sub(lastPnt).Length > self.toolDownThreshold
                    ):  # if tran dist > toolDiam, hop
                        # transHeight = min(height + 1.0, self.safeHeight)
                        # transHeight = min(self.prevDepth + 1.0, self.safeHeight)
                        transHeight = self.safeHeight
                        paths.extend(
                            self._hopPath(
                                transHeight, e0.Vertexes[0].X, e0.Vertexes[0].Y, height
                            )
                        )
                    else:
                        paths.append(
                            Path.Command(
                                "G1",
                                {
                                    "X": e0.Vertexes[0].X,
                                    "Y": e0.Vertexes[0].Y,
                                    "F": self.horizRapid,
                                },
                            )
                        )
            else:
                paths.append(
                    Path.Command(
                        "G0",
                        {
                            "X": e0.Vertexes[0].X,
                            "Y": e0.Vertexes[0].Y,
                            "F": self.horizRapid,
                        },
                    )
                )
                paths.append(
                    Path.Command("G0", {"Z": self.prevDepth + 0.1, "F": self.vertRapid})
                )
                paths.append(Path.Command("G1", {"Z": height, "F": self.vertFeed}))

            for i in range(0, eCnt):
                paths.extend(PathGeom.cmdsForEdge(wire.Edges[i], hSpeed=self.horizFeed))

            # Save last point
            lastEdgeVertexes = wire.Edges[eCnt - 1].Vertexes
            lastPnt = lastEdgeVertexes[len(lastEdgeVertexes) - 1].Point
        # Efor

        return paths

    def _buildSpiralPaths(self, height, wireList):
        """_buildSpiralPaths(height, wireList) ... Convert Spiral wires to paths."""
        PathLog.debug("_buildSpiralPaths()")

        paths = []
        useStart = False
        if self.startPoint:
            useStart = True

        if len(self.commandList) == 0:
            paths.append(
                Path.Command("G0", {"Z": self.clearanceHeight, "F": self.vertRapid})
            )
            if useStart:
                paths.append(
                    Path.Command(
                        "G0",
                        {
                            "X": self.startPoint.x,
                            "Y": self.startPoint.y,
                            "F": self.horizRapid,
                        },
                    )
                )

        wIdx = 0
        for wire in wireList:
            wire.translate(FreeCAD.Vector(0, 0, height))

            e0 = wire.Edges[0]
            paths.append(
                Path.Command(
                    "G0",
                    {
                        "X": e0.Vertexes[0].X,
                        "Y": e0.Vertexes[0].Y,
                        "F": self.horizRapid,
                    },
                )
            )
            paths.append(
                Path.Command("G0", {"Z": self.prevDepth + 0.1, "F": self.vertRapid})
            )
            paths.append(Path.Command("G1", {"Z": height, "F": self.vertFeed}))

            for e in wire.Edges:
                paths.append(
                    Path.Command(
                        "G1",
                        {
                            "X": e.Vertexes[1].X,
                            "Y": e.Vertexes[1].Y,
                            "F": self.horizFeed,
                        },
                    )
                )

            paths.append(
                Path.Command("G0", {"Z": self.safeHeight, "F": self.vertRapid})
            )
            wIdx += 1

        return paths

    def _getAdaptivePaths(self):
        """_getAdaptivePaths()... Proxy method for generating Adaptive paths"""
        commandList = list()
        # Execute the Adaptive code to generate path data

        # Slice each solid at requested depth and apply Adaptive pattern to each layer
        success = False
        depIdx = 0
        startDep = self.startDepth
        for dep in self.depthParams:
            finalDep = dep
            # Slice the solid at depth
            face = SelectionProcessing.getCrossSectionOfSolid(self.volumeShape, dep)
            if face:
                faces = face.Faces

                # Execute the Adaptive code to generate path data
                strategy = StrategyAdaptive(
                    faces,
                    self.toolController,
                    self.clearanceHeight,
                    self.safeHeight,
                    startDep,
                    self.finishDepth,
                    finalDep,
                    self.operationType,
                    self.cutSide,
                    self.forceInsideOut,
                    self.materialAllowance,
                    self.stepDown,
                    self.stepOver,
                    self.liftDistance,
                    self.finishingProfile,
                    self.helixAngle,
                    self.helixConeAngle,
                    self.useHelixArcs,
                    self.helixDiameterLimit,
                    self.keepToolDownRatio,
                    self.tolerance,
                    self.stopped,
                    self.stopProcessing,
                    self.stockObj,
                    self.job,
                    self.adaptiveOutputState,
                    self.adaptiveInputState,
                    self.viewObject,
                )
                strategy.isDebug = self.isDebug  # Transfer debug status
                if self.disableHelixEntry:
                    strategy.disableHelixEntry()
                strategy.generateGeometry = False  # Set True to make geometry list available in `adaptiveGeometry` attribute
                strategy.generateCommands = (
                    True  # Set False to disable path command generation
                )

                try:
                    # Generate the path commands
                    rtn = strategy.execute()
                except Exception as e:  # pylint: disable=broad-except
                    FreeCAD.Console.PrintError(str(e) + "\n")
                    FreeCAD.Console.PrintError(
                        "Something unexpected happened. Check project and tool config. 3\n"
                    )
                else:
                    # Save path commands to operation command list
                    commandList.extend(strategy.commandList)
                    success = True

            else:
                PathLog.debug("No cross-sectional faces at {} mm.".format(dep))
                if success:
                    break
            depIdx += 1
            startDep = dep
        # Efor
        self.commandList = commandList

    def _hopPath(self, upHeight, trgtX, trgtY, downHeight):
        paths = list()
        prevDepth = self.prevDepth + 0.5  # 1/2 mm buffer
        paths.append(
            Path.Command("G0", {"Z": upHeight, "F": self.vertRapid})
        )  # Rapid retraction
        paths.append(
            Path.Command("G0", {"X": trgtX, "Y": trgtY, "F": self.horizRapid})
        )  # Rapid lateral move
        if (
            self.useStaticCenter
            and upHeight > prevDepth
            and upHeight <= self.safeHeight
        ):
            paths.append(Path.Command("G0", {"Z": prevDepth, "F": self.vertRapid}))
        paths.append(
            Path.Command("G1", {"Z": downHeight, "F": self.vertFeed})
        )  # Plunge at vertical feed rate
        return paths

    # Public methods
    def setAdaptiveAttributes(
        self,
        startDepth,
        finalDepth,
        stepDown,
        finishDepth,
        operationType,
        cutSide,
        disableHelixEntry,
        forceInsideOut,
        liftDistance,
        finishingProfile,
        helixAngle,
        helixConeAngle,
        useHelixArcs,
        helixDiameterLimit,
        keepToolDownRatio,
        stopped,
        stopProcessing,
        tolerance,
        stockObj,
        job,
        adaptiveOutputState,
        adaptiveInputState,
        viewObject,
    ):
        """setAdaptiveAttributes(startDepth,
                                 stepDown,
                                 finishDepth,
                                 operationType,
                                 cutSide,
                                 disableHelixEntry,
                                 forceInsideOut,
                                 liftDistance,
                                 finishingProfile,
                                 helixAngle,
                                 helixConeAngle,
                                 useHelixArcs,
                                 helixDiameterLimit,
                                 keepToolDownRatio,
                                 stopped,
                                 stopProcessing,
                                 tolerance,
                                 stockObj,
                                 job,
                                 adaptiveOutputState,
                                 adaptiveInputState,
                                 viewObj):
        Call to set adaptive-dependent attributes."""
        self.startDepth = startDepth
        self.finalDepth = finalDepth
        self.stepDown = stepDown
        self.finishDepth = finishDepth
        self.operationType = operationType
        self.cutSide = cutSide
        self.disableHelixEntry = disableHelixEntry
        self.forceInsideOut = forceInsideOut
        self.liftDistance = liftDistance
        self.finishingProfile = finishingProfile
        self.helixAngle = helixAngle
        self.helixConeAngle = helixConeAngle
        self.useHelixArcs = useHelixArcs
        self.helixDiameterLimit = helixDiameterLimit
        self.keepToolDownRatio = keepToolDownRatio
        self.stopped = stopped
        self.stopProcessing = stopProcessing
        self.tolerance = tolerance
        self.stockObj = stockObj
        self.job = job
        self.adaptiveOutputState = adaptiveOutputState
        self.adaptiveInputState = adaptiveInputState
        self.viewObject = viewObject

    def execute(self):
        """execute()...
        The public method for the StrategyClearing class.
        Returns a tuple containing a list of path commands and a list of shapes(wires and edges) as the path geometry.
        """
        self._debugMsg("StrategyClearing.execute()")

        # Uncomment as needed for localized class debugging
        # self.isDebug = True
        # self.showDebugShapes = True

        self.commandList = list()  # Reset list
        self.pathGeometry = list()  # Reset list
        self.isCenterSet = False
        depthParams = [i for i in self.depthParams]
        self.prevDepth = self.safeHeight

        # Exit if pattern not available
        if self.cutPattern == "None":
            return False

        if len(depthParams) == 0:
            self._debugMsg("No depth parameters", True)
            return list()

        if self.keepToolDown:
            if not self.safeBaseShape:
                PathLog.warning(
                    translate(
                        "PathStrategyClearing", "No safe base shape for Keep Tool Down."
                    )
                )
                self.keepToolDown = False

        if hasattr(self.volumeShape, "Volume") and PathGeom.isRoughly(
            self.volumeShape.Volume, 0.0
        ):
            self._debugMsg("StrategyClearing: No volume in working shape.")
            return False

        if self.cutPattern in ["Grid", "Triangle"]:
            self._buildGridAndTrianglePaths()
            return True

        if self.cutPattern == "Adaptive":
            self._getAdaptivePaths()
            return True

        # Make box to serve as cut tool, and move into position above shape
        sBB = self.volumeShape.BoundBox

        success = False
        for passDepth in depthParams:
            cutFace = PathGeom.makeBoundBoxFace(sBB, offset=5.0, zHeight=passDepth)
            workingFaces = self.volumeShape.common(cutFace)
            self._debugMsg(
                "{} faces at passDepth: {}".format(len(workingFaces.Faces), passDepth)
            )
            if workingFaces and len(workingFaces.Faces) > 0:
                for wf in workingFaces.Faces:
                    wf.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - wf.BoundBox.ZMin))
                    # self._addDebugObject(wf, 'workingFace_' + str(round(passDepth, 2)))
                    self.face = wf
                    pathGeom = self._getPathGeometry(wf)
                    # self._addDebugObject(Part.makeCompound(pathGeom), 'pathGeom_' + str(round(passDepth, 2)))
                    self.pathGeometry.extend(pathGeom)
                    pathCmds = self._buildPaths(passDepth, pathGeom)
                    if not self.keepToolDown:
                        pathCmds.append(
                            Path.Command(
                                "G0", {"Z": self.clearanceHeight, "F": self.vertRapid}
                            )
                        )
                    self.commandList.extend(pathCmds)
                    success = True
            else:
                self._debugMsg(
                    "No working faces at {} mm. Canceling lower layers.".format(
                        passDepth
                    )
                )
                if success:
                    break
            self.prevDepth = passDepth

        self._debugMsg("commandList count: {}".format(len(self.commandList)))
        self._debugMsg("Path with params: {}".format(self.pathParams))

        endVectCnt = len(self.endVectors)
        if endVectCnt > 0:
            self.endVector = self.endVectors[endVectCnt - 1]

        return True


# Eclass


class StrategyAdaptive:
    """class StrategyAdaptive
    Class and implementation of the current Adaptive operation, used as direct path generation."
    """

    sceneGraph = None
    scenePathNodes = []  # for scene cleanup aftewards
    topZ = 10

    def __init__(
        self,
        faces,
        toolController,
        clearanceHeight,
        safeHeight,
        startDepth,
        finishDepth,
        finalDepth,
        operationType,
        cutSide,
        forceInsideOut,
        materialAllowance,
        stepDown,
        stepOver,
        liftDistance,
        finishingProfile,
        helixAngle,
        helixConeAngle,
        useHelixArcs,
        helixDiameterLimit,
        keepToolDownRatio,
        tolerance,
        stopped,
        stopProcessing,
        stock,
        job,
        adaptiveOutputState,
        adaptiveInputState,
        viewObject,
    ):
        PathLog.debug("StrategyAdaptive.__init__()")

        self.isDebug = True if PathLog.getLevel(PathLog.thisModule()) == 4 else False
        self.useHelixEntry = True  # Set False to disable helix entry
        self.adaptiveGeometry = list()
        self.pathArray = list()
        self.commandList = list()

        # Apply limits to argument values
        if tolerance < 0.001:
            tolerance = 0.001

        if helixAngle < 1.0:
            helixAngle = 1.0
        if helixAngle > 89.0:
            helixAngle = 89.0

        if stepDown < 0.1:
            stepDown = 0.1

        if helixConeAngle < 0.0:
            helixConeAngle = 0.0

        self.faces = faces
        self.clearanceHeight = clearanceHeight
        self.safeHeight = safeHeight
        self.startDepth = startDepth
        self.finishDepth = finishDepth
        self.finalDepth = finalDepth
        self.operationType = operationType
        self.cutSide = cutSide
        self.forceInsideOut = forceInsideOut
        self.materialAllowance = materialAllowance
        self.stock = stock
        self.job = job
        self.stepDown = stepDown
        self.stepOver = stepOver
        self.liftDistance = liftDistance
        self.finishingProfile = finishingProfile
        self.helixAngle = helixAngle
        self.helixConeAngle = helixConeAngle
        self.useHelixArcs = useHelixArcs
        self.helixDiameterLimit = helixDiameterLimit
        self.keepToolDownRatio = keepToolDownRatio
        self.tolerance = tolerance
        self.stopped = stopped
        self.stopProcessing = stopProcessing
        self.adaptiveOutputState = adaptiveOutputState
        self.adaptiveInputState = adaptiveInputState
        self.viewObject = viewObject

        self.vertFeed = toolController.VertFeed.Value
        self.horizFeed = toolController.HorizFeed.Value
        self.toolDiameter = (
            toolController.Tool.Diameter.Value
            if hasattr(toolController.Tool.Diameter, "Value")
            else float(toolController.Tool.Diameter)
        )

        self.stockShape = stock.Shape

    # Private methods
    def _convertTo2d(self, pathArray):
        output = []
        for path in pathArray:
            pth2 = []
            for edge in path:
                for pt in edge:
                    pth2.append([pt[0], pt[1]])
            output.append(pth2)
        return output

    def _sceneDrawPath(self, path, color=(0, 0, 1)):
        coPoint = coin.SoCoordinate3()

        pts = []
        for pt in path:
            pts.append([pt[0], pt[1], self.topZ])

        coPoint.point.setValues(0, len(pts), pts)
        ma = coin.SoBaseColor()
        ma.rgb = color
        li = coin.SoLineSet()
        li.numVertices.setValue(len(pts))
        pathNode = coin.SoSeparator()
        pathNode.addChild(coPoint)
        pathNode.addChild(ma)
        pathNode.addChild(li)
        self.sceneGraph.addChild(pathNode)
        self.scenePathNodes.append(pathNode)  # for scene cleanup afterwards

    def _sceneClean(self):
        for n in self.scenePathNodes:
            self.sceneGraph.removeChild(n)

        del self.scenePathNodes[:]

    def _discretize(self, edge, flipDirection=False):
        pts = edge.discretize(Deflection=0.0001)
        if flipDirection:
            pts.reverse()

        return pts

    def _calcHelixConePoint(self, height, cur_z, radius, angle):
        x = ((height - cur_z) / height) * radius * math.cos(math.radians(angle) * cur_z)
        y = ((height - cur_z) / height) * radius * math.sin(math.radians(angle) * cur_z)
        z = cur_z

        return {"X": x, "Y": y, "Z": z}

    def _generateGCode(self, adaptiveResults):
        self.commandList = list()
        commandList = list()
        motionCutting = area.AdaptiveMotionType.Cutting
        motionLinkClear = area.AdaptiveMotionType.LinkClear
        motionLinkNotClear = area.AdaptiveMotionType.LinkNotClear

        # pylint: disable=unused-argument
        if len(adaptiveResults) == 0 or len(adaptiveResults[0]["AdaptivePaths"]) == 0:
            return

        helixRadius = 0
        for region in adaptiveResults:
            p1 = region["HelixCenterPoint"]
            p2 = region["StartPoint"]
            r = math.sqrt(
                (p1[0] - p2[0]) * (p1[0] - p2[0]) + (p1[1] - p2[1]) * (p1[1] - p2[1])
            )
            if r > helixRadius:
                helixRadius = r

        passStartDepth = self.startDepth

        length = 2 * math.pi * helixRadius

        helixAngleRad = math.pi * self.helixAngle / 180.0
        depthPerOneCircle = length * math.tan(helixAngleRad)
        # print("Helix circle depth: {}".format(depthPerOneCircle))

        stepUp = self.liftDistance
        if stepUp < 0:
            stepUp = 0

        stepDown = self.stepDown
        finish_step = self.finishDepth
        if finish_step > stepDown:
            finish_step = stepDown

        depth_params = PathUtils.depth_params(
            clearance_height=self.clearanceHeight,
            safe_height=self.safeHeight,
            start_depth=self.startDepth,
            step_down=self.stepDown,
            z_finish_step=finish_step,
            final_depth=self.finalDepth,
            user_depths=None,
        )

        # ml: this is dangerous because it'll hide all unused variables hence forward
        #     however, I don't know what lx and ly signify so I'll leave them for now
        # russ4262: I think that the `l` in `lx, ly, and lz` stands for `last`.
        # pylint: disable=unused-variable
        # lx = adaptiveResults[0]["HelixCenterPoint"][0]
        # ly = adaptiveResults[0]["HelixCenterPoint"][1]
        lz = passStartDepth  # lz is likely `last Z depth`
        step = 0

        for passEndDepth in depth_params.data:
            step = step + 1

            for region in adaptiveResults:
                startAngle = math.atan2(
                    region["StartPoint"][1] - region["HelixCenterPoint"][1],
                    region["StartPoint"][0] - region["HelixCenterPoint"][0],
                )

                # lx = region["HelixCenterPoint"][0]
                # ly = region["HelixCenterPoint"][1]

                passDepth = passStartDepth - passEndDepth

                p1 = region["HelixCenterPoint"]
                p2 = region["StartPoint"]
                helixRadius = math.sqrt(
                    (p1[0] - p2[0]) * (p1[0] - p2[0])
                    + (p1[1] - p2[1]) * (p1[1] - p2[1])
                )

                # Helix ramp
                if self.useHelixEntry and helixRadius > 0.01:
                    r = helixRadius - 0.01

                    maxfi = passDepth / depthPerOneCircle * 2 * math.pi
                    fi = 0
                    offsetFi = -maxfi + startAngle - math.pi / 16

                    helixStart = [
                        region["HelixCenterPoint"][0] + r * math.cos(offsetFi),
                        region["HelixCenterPoint"][1] + r * math.sin(offsetFi),
                    ]

                    commandList.append(
                        Path.Command("(Helix to depth: %f)" % passEndDepth)
                    )

                    if not self.useHelixArcs:
                        # rapid move to start point
                        commandList.append(
                            Path.Command("G0", {"Z": self.clearanceHeight})
                        )
                        commandList.append(
                            Path.Command(
                                "G0",
                                {
                                    "X": helixStart[0],
                                    "Y": helixStart[1],
                                    "Z": self.clearanceHeight,
                                },
                            )
                        )

                        # rapid move to safe height
                        commandList.append(
                            Path.Command(
                                "G0",
                                {
                                    "X": helixStart[0],
                                    "Y": helixStart[1],
                                    "Z": self.safeHeight,
                                },
                            )
                        )

                        # move to start depth
                        commandList.append(
                            Path.Command(
                                "G1",
                                {
                                    "X": helixStart[0],
                                    "Y": helixStart[1],
                                    "Z": passStartDepth,
                                    "F": self.vertFeed,
                                },
                            )
                        )

                        if self.helixConeAngle == 0:
                            while fi < maxfi:
                                x = region["HelixCenterPoint"][0] + r * math.cos(
                                    fi + offsetFi
                                )
                                y = region["HelixCenterPoint"][1] + r * math.sin(
                                    fi + offsetFi
                                )
                                z = passStartDepth - fi / maxfi * (
                                    passStartDepth - passEndDepth
                                )
                                commandList.append(
                                    Path.Command(
                                        "G1",
                                        {"X": x, "Y": y, "Z": z, "F": self.vertFeed},
                                    )
                                )
                                # lx = x
                                # ly = y
                                fi = fi + math.pi / 16

                            # one more circle at target depth to make sure center is cleared
                            maxfi = maxfi + 2 * math.pi
                            while fi < maxfi:
                                x = region["HelixCenterPoint"][0] + r * math.cos(
                                    fi + offsetFi
                                )
                                y = region["HelixCenterPoint"][1] + r * math.sin(
                                    fi + offsetFi
                                )
                                z = passEndDepth
                                commandList.append(
                                    Path.Command(
                                        "G1",
                                        {"X": x, "Y": y, "Z": z, "F": self.horizFeed},
                                    )
                                )
                                # lx = x
                                # ly = y
                                fi = fi + math.pi / 16

                        else:
                            # Cone
                            _HelixAngle = 360.0 - (self.helixAngle * 4.0)

                            if self.helixConeAngle > 6:
                                self.helixConeAngle = 6

                            helixRadius *= 0.9

                            # Calculate everything
                            helix_height = passStartDepth - passEndDepth
                            r_extra = helix_height * math.tan(
                                math.radians(self.helixConeAngle)
                            )
                            HelixTopRadius = helixRadius + r_extra
                            helix_full_height = HelixTopRadius * (
                                math.cos(math.radians(self.helixConeAngle))
                                / math.sin(math.radians(self.helixConeAngle))
                            )

                            # Start height
                            z = passStartDepth
                            i = 0

                            # Default step down
                            z_step = 0.05

                            # Bigger angle, smaller step down
                            if _HelixAngle > 120:
                                z_step = 0.025
                            if _HelixAngle > 240:
                                z_step = 0.015

                            p = None
                            # Calculate conical helix
                            while z >= passEndDepth:
                                if z < passEndDepth:
                                    z = passEndDepth

                                p = self._calcHelixConePoint(
                                    helix_full_height, i, HelixTopRadius, _HelixAngle
                                )
                                commandList.append(
                                    Path.Command(
                                        "G1",
                                        {
                                            "X": p["X"] + region["HelixCenterPoint"][0],
                                            "Y": p["Y"] + region["HelixCenterPoint"][1],
                                            "Z": z,
                                            "F": self.vertFeed,
                                        },
                                    )
                                )
                                z = z - z_step
                                i = i + z_step

                            # Calculate some stuff for arcs at bottom
                            p["X"] = p["X"] + region["HelixCenterPoint"][0]
                            p["Y"] = p["Y"] + region["HelixCenterPoint"][1]
                            x_m = (
                                region["HelixCenterPoint"][0]
                                - p["X"]
                                + region["HelixCenterPoint"][0]
                            )
                            y_m = (
                                region["HelixCenterPoint"][1]
                                - p["Y"]
                                + region["HelixCenterPoint"][1]
                            )
                            i_off = (x_m - p["X"]) / 2
                            j_off = (y_m - p["Y"]) / 2

                            # One more circle at target depth to make sure center is cleared
                            commandList.append(
                                Path.Command(
                                    "G3",
                                    {
                                        "X": x_m,
                                        "Y": y_m,
                                        "Z": passEndDepth,
                                        "I": i_off,
                                        "J": j_off,
                                        "F": self.horizFeed,
                                    },
                                )
                            )
                            commandList.append(
                                Path.Command(
                                    "G3",
                                    {
                                        "X": p["X"],
                                        "Y": p["Y"],
                                        "Z": passEndDepth,
                                        "I": -i_off,
                                        "J": -j_off,
                                        "F": self.horizFeed,
                                    },
                                )
                            )

                    else:
                        # Use arcs for helix - no conical shape support
                        helixStart = [
                            region["HelixCenterPoint"][0] + r,
                            region["HelixCenterPoint"][1],
                        ]

                        # rapid move to start point
                        commandList.append(
                            Path.Command("G0", {"Z": self.clearanceHeight})
                        )
                        commandList.append(
                            Path.Command(
                                "G0",
                                {
                                    "X": helixStart[0],
                                    "Y": helixStart[1],
                                    "Z": self.clearanceHeight,
                                },
                            )
                        )

                        # rapid move to safe height
                        commandList.append(
                            Path.Command(
                                "G0",
                                {
                                    "X": helixStart[0],
                                    "Y": helixStart[1],
                                    "Z": self.safeHeight,
                                },
                            )
                        )

                        # move to start depth
                        commandList.append(
                            Path.Command(
                                "G1",
                                {
                                    "X": helixStart[0],
                                    "Y": helixStart[1],
                                    "Z": passStartDepth,
                                    "F": self.vertFeed,
                                },
                            )
                        )

                        x = region["HelixCenterPoint"][0] + r
                        y = region["HelixCenterPoint"][1]

                        curDep = passStartDepth
                        while curDep > (passEndDepth + depthPerOneCircle):
                            commandList.append(
                                Path.Command(
                                    "G2",
                                    {
                                        "X": x - (2 * r),
                                        "Y": y,
                                        "Z": curDep - (depthPerOneCircle / 2),
                                        "I": -r,
                                        "F": self.vertFeed,
                                    },
                                )
                            )
                            commandList.append(
                                Path.Command(
                                    "G2",
                                    {
                                        "X": x,
                                        "Y": y,
                                        "Z": curDep - depthPerOneCircle,
                                        "I": r,
                                        "F": self.vertFeed,
                                    },
                                )
                            )
                            curDep = curDep - depthPerOneCircle

                        lastStep = curDep - passEndDepth
                        if lastStep > (depthPerOneCircle / 2):
                            commandList.append(
                                Path.Command(
                                    "G2",
                                    {
                                        "X": x - (2 * r),
                                        "Y": y,
                                        "Z": curDep - (lastStep / 2),
                                        "I": -r,
                                        "F": self.vertFeed,
                                    },
                                )
                            )
                            commandList.append(
                                Path.Command(
                                    "G2",
                                    {
                                        "X": x,
                                        "Y": y,
                                        "Z": passEndDepth,
                                        "I": r,
                                        "F": self.vertFeed,
                                    },
                                )
                            )
                        else:
                            commandList.append(
                                Path.Command(
                                    "G2",
                                    {
                                        "X": x - (2 * r),
                                        "Y": y,
                                        "Z": passEndDepth,
                                        "I": -r,
                                        "F": self.vertFeed,
                                    },
                                )
                            )
                            commandList.append(
                                Path.Command(
                                    "G1",
                                    {
                                        "X": x,
                                        "Y": y,
                                        "Z": passEndDepth,
                                        "F": self.vertFeed,
                                    },
                                )
                            )

                        # one more circle at target depth to make sure center is cleared
                        commandList.append(
                            Path.Command(
                                "G2",
                                {
                                    "X": x - (2 * r),
                                    "Y": y,
                                    "Z": passEndDepth,
                                    "I": -r,
                                    "F": self.horizFeed,
                                },
                            )
                        )
                        commandList.append(
                            Path.Command(
                                "G2",
                                {
                                    "X": x,
                                    "Y": y,
                                    "Z": passEndDepth,
                                    "I": r,
                                    "F": self.horizFeed,
                                },
                            )
                        )
                        # lx = x
                        # ly = y

                else:  # no helix entry
                    # rapid move to clearance height
                    commandList.append(Path.Command("G0", {"Z": self.clearanceHeight}))
                    commandList.append(
                        Path.Command(
                            "G0",
                            {
                                "X": region["StartPoint"][0],
                                "Y": region["StartPoint"][1],
                                "Z": self.clearanceHeight,
                            },
                        )
                    )
                    # straight plunge to target depth
                    commandList.append(
                        Path.Command(
                            "G1",
                            {
                                "X": region["StartPoint"][0],
                                "Y": region["StartPoint"][1],
                                "Z": passEndDepth,
                                "F": self.vertFeed,
                            },
                        )
                    )

                lz = passEndDepth
                z = self.clearanceHeight
                commandList.append(
                    Path.Command("(Adaptive - depth: %f)" % passEndDepth)
                )

                # add adaptive paths
                for pth in region["AdaptivePaths"]:
                    motionType = pth[0]  # [0] contains motion type

                    for pt in pth[1]:  # [1] contains list of points
                        x = pt[0]
                        y = pt[1]

                        # dist = math.sqrt((x-lx)*(x-lx) + (y-ly)*(y-ly))

                        if motionType == motionCutting:
                            z = passEndDepth
                            if z != lz:
                                commandList.append(
                                    Path.Command("G1", {"Z": z, "F": self.vertFeed})
                                )  # plunge at feed rate

                            commandList.append(
                                Path.Command(
                                    "G1", {"X": x, "Y": y, "F": self.horizFeed}
                                )
                            )  # feed to point

                        elif motionType == motionLinkClear:
                            z = passEndDepth + stepUp
                            if z != lz:
                                commandList.append(
                                    Path.Command("G0", {"Z": z})
                                )  # rapid to previous pass depth

                            commandList.append(
                                Path.Command("G0", {"X": x, "Y": y})
                            )  # rapid to point

                        elif motionType == motionLinkNotClear:
                            z = self.clearanceHeight
                            if z != lz:
                                commandList.append(
                                    Path.Command("G0", {"Z": z})
                                )  # rapid to clearance height

                            commandList.append(
                                Path.Command("G0", {"X": x, "Y": y})
                            )  # rapid to point

                        # elif motionType == area.AdaptiveMotionType.LinkClearAtPrevPass:
                        #     if lx!=x or ly!=y:
                        #         commandList.append(Path.Command("G0", { "X": lx, "Y":ly, "Z":passStartDepth+stepUp}))
                        #     commandList.append(Path.Command("G0", { "X": x, "Y":y, "Z":passStartDepth+stepUp}))

                        # rotate values: current values become last for next loop
                        # lx = x
                        # ly = y
                        lz = z

                # return to clearance height in this Z pass
                z = self.clearanceHeight
                if z != lz:
                    commandList.append(Path.Command("G0", {"Z": z}))

                lz = z

            passStartDepth = passEndDepth

            # return to safe height in this Z pass
            z = self.clearanceHeight
            if z != lz:
                commandList.append(Path.Command("G0", {"Z": z}))

            lz = z

        z = self.clearanceHeight
        if z != lz:
            commandList.append(Path.Command("G0", {"Z": z}))

        lz = z

        # Save commands
        self.commandList = commandList

    # Public methods
    def disableHelixEntry(self):
        self.useHelixEntry = False
        self.helixDiameterLimit = 0.01
        self.helixAngle = 89.0

    def execute(self):
        PathLog.debug("StrategyAdaptive.execute()")

        # PathLog.info("*** Adaptive toolpath processing started...")
        # startTime = time.time()

        for shp in self.faces:
            shp.translate(FreeCAD.Vector(0.0, 0.0, self.finalDepth - shp.BoundBox.ZMin))
            for w in shp.Wires:
                for e in w.Edges:
                    self.pathArray.append([self._discretize(e)])

        if FreeCAD.GuiUp:
            self.sceneGraph = FreeCADGui.ActiveDocument.ActiveView.getSceneGraph()

        # hide old toolpaths during recalculation
        # self.obj.Path = Path.Path("(Calculating...)")  # self.obj.Path should change to self.Path

        if FreeCAD.GuiUp:
            # store old visibility state
            oldObjVisibility = self.viewObject.Visibility
            oldJobVisibility = self.job.ViewObject.Visibility

            self.viewObject.Visibility = False
            self.job.ViewObject.Visibility = False

            FreeCADGui.updateGui()

        self.topZ = self.stockShape.BoundBox.ZMax
        self.stopped = False
        self.stopProcessing = False

        path2d = self._convertTo2d(self.pathArray)

        stockPaths = []
        if (
            hasattr(self.stock, "StockType")
            and self.stock.StockType == "CreateCylinder"
        ):
            stockPaths.append([self._discretize(self.stockShape.Edges[0])])

        else:
            stockBB = self.stockShape.BoundBox
            v = []
            v.append(FreeCAD.Vector(stockBB.XMin, stockBB.YMin, 0))
            v.append(FreeCAD.Vector(stockBB.XMax, stockBB.YMin, 0))
            v.append(FreeCAD.Vector(stockBB.XMax, stockBB.YMax, 0))
            v.append(FreeCAD.Vector(stockBB.XMin, stockBB.YMax, 0))
            v.append(FreeCAD.Vector(stockBB.XMin, stockBB.YMin, 0))
            stockPaths.append([v])

        stockPath2d = self._convertTo2d(stockPaths)

        opType = area.AdaptiveOperationType.ClearingInside
        if self.operationType == "Clearing":
            if self.cutSide == "Outside":
                opType = area.AdaptiveOperationType.ClearingOutside

            else:
                opType = area.AdaptiveOperationType.ClearingInside

        else:  # profiling
            if self.cutSide == "Outside":
                opType = area.AdaptiveOperationType.ProfilingOutside

            else:
                opType = area.AdaptiveOperationType.ProfilingInside

        # put here all properties that influence calculation of adaptive base paths,
        inputStateObject = {
            "tool": self.toolDiameter,
            "tolerance": self.tolerance,
            "geometry": path2d,
            "stockGeometry": stockPath2d,
            "stepover": self.stepOver,
            "effectiveHelixDiameter": self.helixDiameterLimit,
            "operationType": self.operationType,
            "side": self.cutSide,
            "forceInsideOut": self.forceInsideOut,
            "finishingProfile": self.finishingProfile,
            "keepToolDownRatio": self.keepToolDownRatio,
            "stockToLeave": self.materialAllowance,
        }

        inputStateChanged = False
        adaptiveResults = None

        if self.adaptiveOutputState is not None and self.adaptiveOutputState != "":
            adaptiveResults = self.adaptiveOutputState

        if json.dumps(self.adaptiveInputState) != json.dumps(inputStateObject):
            inputStateChanged = True
            adaptiveResults = None

        # progress callback fn, if return true it will stop processing
        def progressFn(tpaths):
            motionCutting = area.AdaptiveMotionType.Cutting
            if FreeCAD.GuiUp:
                for (
                    path
                ) in (
                    tpaths
                ):  # path[0] contains the MotionType, #path[1] contains list of points
                    if path[0] == motionCutting:
                        self._sceneDrawPath(path[1], (0, 0, 1))

                    else:
                        self._sceneDrawPath(path[1], (1, 0, 1))

                FreeCADGui.updateGui()

            return self.stopProcessing

        if inputStateChanged or adaptiveResults is None:
            a2d = area.Adaptive2d()
            a2d.stepOverFactor = 0.01 * self.stepOver
            a2d.toolDiameter = self.toolDiameter
            a2d.helixRampDiameter = self.helixDiameterLimit
            a2d.keepToolDownDistRatio = self.keepToolDownRatio
            a2d.stockToLeave = self.materialAllowance
            a2d.tolerance = self.tolerance
            a2d.forceInsideOut = self.forceInsideOut
            a2d.finishingProfile = self.finishingProfile
            a2d.opType = opType

        try:
            # EXECUTE
            results = a2d.Execute(stockPath2d, path2d, progressFn)

            # need to convert results to python object to be JSON serializable
            adaptiveResults = []
            for result in results:
                adaptiveResults.append(
                    {
                        "HelixCenterPoint": result.HelixCenterPoint,
                        "StartPoint": result.StartPoint,
                        "AdaptivePaths": result.AdaptivePaths,
                        "ReturnMotionType": result.ReturnMotionType,
                    }
                )

            # Generate G-Code
            self._generateGCode(adaptiveResults)

            if not self.stopProcessing:
                # PathLog.info("*** Done. Elapsed time: %f sec" % (time.time()-startTime))
                self.adaptiveOutputState = adaptiveResults
                self.adaptiveInputState = inputStateObject

            else:
                # PathLog.info("*** Processing cancelled (after: %f sec)." % (time.time()-startTime))
                pass

        finally:
            if FreeCAD.GuiUp:
                self.viewObject.Visibility = oldObjVisibility
                self.job.ViewObject.Visibility = oldJobVisibility
                self._sceneClean()

        return True

    # Functions for managing properties and their default values
    @classmethod
    def adaptivePropertyDefinitions(cls):
        """adaptivePropertyDefinitions() ... returns a list of tuples.
        Each tuple contains property declaration information in the
        form of (prototype, name, section, tooltip)."""
        return [
            (
                "App::PropertyEnumeration",
                "CutSide",
                "Adaptive",
                "Side of selected faces that tool should cut",
            ),
            (
                "App::PropertyEnumeration",
                "OperationType",
                "Adaptive",
                "Type of adaptive operation",
            ),
            (
                "App::PropertyFloat",
                "Tolerance",
                "Adaptive",
                "Influences accuracy and performance",
            ),
            (
                "App::PropertyDistance",
                "LiftDistance",
                "Adaptive",
                "Lift distance for rapid moves",
            ),
            (
                "App::PropertyDistance",
                "KeepToolDownRatio",
                "Adaptive",
                "Max length of keep tool down path compared to direct distance between points",
            ),
            (
                "App::PropertyBool",
                "ForceInsideOut",
                "Adaptive",
                "Force plunging into material inside and clearing towards the edges",
            ),
            (
                "App::PropertyBool",
                "FinishingProfile",
                "Adaptive",
                "To take a finishing profile path at the end",
            ),
            ("App::PropertyBool", "Stopped", "Adaptive", "Stop processing"),
            ("App::PropertyBool", "StopProcessing", "Adaptive", "Stop processing"),
            (
                "App::PropertyBool",
                "UseHelixArcs",
                "Adaptive",
                "Use Arcs (G2) for helix ramp",
            ),
            (
                "App::PropertyPythonObject",
                "AdaptiveInputState",
                "Adaptive",
                "Internal input state",
            ),
            (
                "App::PropertyPythonObject",
                "AdaptiveOutputState",
                "Adaptive",
                "Internal output state",
            ),
            (
                "App::PropertyAngle",
                "HelixAngle",
                "Adaptive",
                "Helix ramp entry angle (degrees)",
            ),
            (
                "App::PropertyAngle",
                "HelixConeAngle",
                "Adaptive",
                "Helix cone angle (degrees)",
            ),
            (
                "App::PropertyLength",
                "HelixDiameterLimit",
                "Adaptive",
                "Limit helix entry diameter, if limit larger than tool diameter or 0, tool diameter is used",
            ),
            (
                "App::PropertyBool",
                "DisableHelixEntry",
                "Adaptive",
                "Disable the helix entry, and use simple plunge.",
            ),
        ]

    @classmethod
    def adaptivePropertyDefaults(cls, obj, job):
        """adaptivePropertyDefaults(obj, job) ... returns a dictionary of default values
        for the strategy's properties."""
        return {
            "CutSide": "Inside",
            "OperationType": "Clearing",
            "Tolerance": 0.1,
            "LiftDistance": 0,
            "ForceInsideOut": False,
            "FinishingProfile": True,
            "Stopped": False,
            "StopProcessing": False,
            "HelixAngle": 5,
            "HelixConeAngle": 0,
            "HelixDiameterLimit": 0.0,
            "AdaptiveInputState": "",
            "AdaptiveOutputState": "",
            "KeepToolDownRatio": 3.0,
            "UseHelixArcs": False,
            "DisableHelixEntry": False,
        }

    @classmethod
    def adaptivePropertyEnumerations(cls):
        """adaptivePropertyEnumerations() ... returns a dictionary of enumeration lists
        for the operation's enumeration type properties."""
        # Enumeration lists for App::PropertyEnumeration properties
        return {
            "OperationType": ["Clearing", "Profile"],
            "CutSide": ["Outside", "Inside"],
        }

    @classmethod
    def adaptiveSetEditorModes(cls, obj, hide=False):
        """adaptiveSetEditorModes(obj) ... Set property editor modes."""
        # Always hide these properties
        obj.setEditorMode("Stopped", 2)
        obj.setEditorMode("StopProcessing", 2)
        obj.setEditorMode("AdaptiveInputState", 2)
        obj.setEditorMode("AdaptiveOutputState", 2)

        mode = 0
        if hide:
            mode = 2
        obj.setEditorMode("CutSide", mode)
        obj.setEditorMode("OperationType", mode)
        obj.setEditorMode("Tolerance", mode)
        obj.setEditorMode("LiftDistance", mode)
        obj.setEditorMode("KeepToolDownRatio", mode)
        obj.setEditorMode("ForceInsideOut", mode)
        obj.setEditorMode("FinishingProfile", mode)
        obj.setEditorMode("UseHelixArcs", mode)
        obj.setEditorMode("HelixAngle", mode)
        obj.setEditorMode("HelixConeAngle", mode)
        obj.setEditorMode("HelixDiameterLimit", mode)


# Eclass


class PathGeometryGenerator:
    """PathGeometryGenerator() class...
    Generates a path geometry shape from an assigned pattern for conversion to tool paths.
    Arguments:
        callerClass:        reference to caller class
        targetFace:         face shape to serve as base for path geometry generation
        patternCenterAt:    choice of centering options
        patternCenterCustom: custom (x, y, 0.0) center point
        cutPatternReversed: boolean to reverse cut pattern from inside-out to outside-in
        cutPatternAngle:    rotation angle applied to rotatable patterns
        cutPattern:         cut pattern choice
        cutDirection:       conventional or climb
        stepOver:           step over percentage
        materialAllowance:  positive material to allow(leave), negative additional material to remove
        minTravel:          boolean to enable minimum travel (feature not enabled at this time)
        keepToolDown:       boolean to enable keeping tool down (feature not enabled at this time)
        toolController:     instance of tool controller to be used
        jobTolerance:       job tolerance value
    Available Patterns:
        - Adaptive, Circular, CircularZigZag, Grid, Line, LineOffset, Offset, Spiral, Triangle, ZigZag, ZigZagOffset
    Usage:
        - Instantiate this class.
        - Call the `setAdaptiveAttributes()` method with required attributes if you intend to use Adaptive cut pattern.
        - Call the `execute()` method to generate the path geometry. The path geometry has correctional linking applied.
        - The path geometry in now available in the `pathGeometry` attribute.
    Notes:
        - Grid and Triangle patterns are not rotatable at this time.
    """

    # Register valid patterns here by name
    # Create a corresponding processing method below. Precede the name with an underscore(_)
    patterns = (
        "Adaptive",
        "Circular",
        "CircularZigZag",
        "Grid",
        "Line",
        "LineOffset",
        "Offset",
        "Spiral",
        "Triangle",
        "ZigZag",
        "ZigZagOffset",
    )
    rotatablePatterns = ("Line", "ZigZag", "LineOffset", "ZigZagOffset")
    curvedPatterns = ("Circular", "CircularZigZag", "Spiral")

    def __init__(
        self,
        callerClass,
        targetFace,
        patternCenterAt,
        patternCenterCustom,
        cutPatternReversed,
        cutPatternAngle,
        cutPattern,
        cutDirection,
        stepOver,
        materialAllowance,
        minTravel,
        keepToolDown,
        toolController,
        jobTolerance,
    ):
        """__init__(callerClass,
                 targetFace,
                 patternCenterAt,
                 patternCenterCustom,
                 cutPatternReversed,
                 cutPatternAngle,
                 cutPattern,
                 cutDirection,
                 stepOver,
                 materialAllowance,
                 minTravel,
                 keepToolDown,
                 toolController,
                 jobTolerance)...
        PathGeometryGenerator class constructor method.
        """
        PathLog.debug("PathGeometryGenerator.__init__()")

        # Debugging attributes
        self.isDebug = True if PathLog.getLevel(PathLog.thisModule()) == 4 else False
        self.showDebugShapes = False

        self.cutPattern = "None"
        self.face = None
        self.rawGeoList = None
        self.centerOfMass = None
        self.centerOfPattern = None
        self.halfDiag = None
        self.halfPasses = None
        self.workingPlane = Part.makeCircle(2.0)  # make circle for workplane
        self.rawPathGeometry = None
        self.linkedPathGeom = None
        self.pathGeometry = list()
        self.commandList = list()
        self.useStaticCenter = True  # Set True to use static center for all faces created by offsets and step downs.  Set False for dynamic centers based on PatternCenterAt
        self.isCenterSet = False
        self.offsetDirection = -1.0  # 1.0=outside;  -1.0=inside
        self.endVector = None
        self.pathParams = ""
        self.areaParams = ""
        self.pfsRtn = None

        # Save argument values to class instance
        self.callerClass = callerClass
        self.targetFace = targetFace
        self.patternCenterAt = patternCenterAt
        self.patternCenterCustom = patternCenterCustom
        self.cutPatternReversed = cutPatternReversed
        self.cutPatternAngle = cutPatternAngle
        self.cutDirection = cutDirection
        self.stepOver = stepOver
        self.materialAllowance = materialAllowance
        self.minTravel = minTravel
        self.keepToolDown = keepToolDown
        self.toolController = toolController
        self.jobTolerance = jobTolerance

        self.toolDiameter = (
            toolController.Tool.Diameter.Value
            if hasattr(toolController.Tool.Diameter, "Value")
            else float(toolController.Tool.Diameter)
        )
        self.toolRadius = self.toolDiameter / 2.0
        self.cutOut = self.toolDiameter * (self.stepOver / 100.0)

        if cutPattern in self.patterns:
            self.cutPattern = cutPattern
        else:
            PathLog.debug("The `{}` cut pattern is not available.".format(cutPattern))

        # Grid and Triangle pattern requirements - paths produced by Path.fromShapes()
        self.pocketMode = 6  # Grid=6, Triangle=7
        self.orientation = 0  # ['Conventional', 'Climb']

        ### Adaptive-specific attributes ###
        self.adaptiveGeometry = list()
        self.pathArray = list()
        self.operationType = None
        self.cutSide = None
        self.disableHelixEntry = None
        self.forceInsideOut = None
        self.liftDistance = None
        self.finishingProfile = None
        self.helixAngle = None
        self.helixConeAngle = None
        self.useHelixArcs = None
        self.helixDiameterLimit = None
        self.keepToolDownRatio = None
        self.tolerance = None
        self.stockObj = None

    def _addDebugShape(self, shape, name="debug"):
        if self.isDebug and self.showDebugShapes:
            do = FreeCAD.ActiveDocument.addObject("Part::Feature", "debug_" + name)
            do.Shape = shape
            do.purgeTouched()

    # Raw cut pattern geometry generation methods
    def _Line(self):
        """_Line()... Returns raw set of Line wires at Z=0.0."""
        geomList = list()
        centRot = FreeCAD.Vector(
            0.0, 0.0, 0.0
        )  # Bottom left corner of face/selection/model
        segLength = self.halfDiag
        if self.patternCenterAt in ["XminYmin", "Custom"]:
            segLength = 2.0 * self.halfDiag

        # Create end points for set of lines to intersect with cross-section face
        pntTuples = list()
        for lc in range((-1 * (self.halfPasses - 1)), self.halfPasses + 1):
            x1 = centRot.x - segLength
            x2 = centRot.x + segLength
            y1 = centRot.y + (lc * self.cutOut)
            # y2 = y1
            p1 = FreeCAD.Vector(x1, y1, 0.0)
            p2 = FreeCAD.Vector(x2, y1, 0.0)
            pntTuples.append((p1, p2))

        # Convert end points to lines

        if (self.cutDirection == "Climb" and not self.cutPatternReversed) or (
            self.cutDirection != "Climb" and self.cutPatternReversed
        ):
            for (p2, p1) in pntTuples:
                wire = Part.Wire([Part.makeLine(p1, p2)])
                geomList.append(wire)
        else:
            for (p1, p2) in pntTuples:
                wire = Part.Wire([Part.makeLine(p1, p2)])
                geomList.append(wire)

        if self.cutPatternReversed:
            geomList.reverse()

        return geomList

    def _LineOffset(self):
        """_LineOffset()... Returns raw set of Line wires at Z=0.0, with the Offset portion added later in the `_generatePathGeometry()` method."""
        return self._Line()

    def _Circular(self):
        """_Circular()... Returns raw set of Circular wires at Z=0.0."""
        geomList = list()
        radialPasses = self._getRadialPasses()
        minRad = self.toolDiameter * 0.45

        if (self.cutDirection == "Conventional" and not self.cutPatternReversed) or (
            self.cutDirection != "Conventional" and self.cutPatternReversed
        ):
            direction = FreeCAD.Vector(0.0, 0.0, 1.0)
        else:
            direction = FreeCAD.Vector(0.0, 0.0, -1.0)

        # Make small center circle to start pattern
        if self.stepOver > 50:
            circle = Part.makeCircle(minRad, self.centerOfPattern, direction)
            geomList.append(circle)

        for lc in range(1, radialPasses + 1):
            rad = lc * self.cutOut
            if rad >= minRad:
                wire = Part.Wire(
                    [Part.makeCircle(rad, self.centerOfPattern, direction)]
                )
                geomList.append(wire)

        if not self.cutPatternReversed:
            geomList.reverse()

        return geomList

    def _CircularZigZag(self):
        """_CircularZigZag()... Returns raw set of Circular ZigZag wires at Z=0.0."""
        geomList = list()
        radialPasses = self._getRadialPasses()
        minRad = self.toolDiameter * 0.45
        dirForward = FreeCAD.Vector(0, 0, 1)
        dirReverse = FreeCAD.Vector(0, 0, -1)

        if (self.cutDirection == "Climb" and self.cutPatternReversed) or (
            self.cutDirection != "Climb" and not self.cutPatternReversed
        ):
            activeDir = dirForward
            direction = 1
        else:
            activeDir = dirReverse
            direction = -1

        # Make small center circle to start pattern
        if self.stepOver > 50:
            circle = Part.makeCircle(minRad, self.centerOfPattern, activeDir)
            geomList.append(circle)
            direction *= -1  # toggle direction
            activeDir = (
                dirForward if direction > 0 else dirReverse
            )  # update active direction after toggle

        for lc in range(1, radialPasses + 1):
            rad = lc * self.cutOut
            if rad >= minRad:
                wire = Part.Wire(
                    [Part.makeCircle(rad, self.centerOfPattern, activeDir)]
                )
                geomList.append(wire)
                direction *= -1  # toggle direction
                activeDir = (
                    dirForward if direction > 0 else dirReverse
                )  # update active direction after toggle
        # Efor

        if not self.cutPatternReversed:
            geomList.reverse()

        return geomList

    def _ZigZag(self):
        """_ZigZag()... Returns raw set of ZigZag wires at Z=0.0."""
        geomList = list()
        centRot = FreeCAD.Vector(
            0.0, 0.0, 0.0
        )  # Bottom left corner of face/selection/model
        segLength = self.halfDiag
        if self.patternCenterAt == "XminYmin":
            segLength = 2.0 * self.halfDiag

        # Create end points for set of lines to intersect with cross-section face
        pntTuples = list()
        direction = 1
        for lc in range((-1 * (self.halfPasses - 1)), self.halfPasses + 1):
            x1 = centRot.x - segLength
            x2 = centRot.x + segLength
            y1 = centRot.y + (lc * self.cutOut)
            # y2 = y1
            if direction == 1:
                p1 = FreeCAD.Vector(x1, y1, 0.0)
                p2 = FreeCAD.Vector(x2, y1, 0.0)
            else:
                p1 = FreeCAD.Vector(x2, y1, 0.0)
                p2 = FreeCAD.Vector(x1, y1, 0.0)
            pntTuples.append((p1, p2))
            # swap direction
            direction *= -1

        # Convert end points to lines
        if (self.cutDirection == "Climb" and not self.cutPatternReversed) or (
            self.cutDirection != "Climb" and self.cutPatternReversed
        ):
            for (p2, p1) in pntTuples:
                wire = Part.Wire([Part.makeLine(p1, p2)])
                geomList.append(wire)
        else:
            for (p1, p2) in pntTuples:
                wire = Part.Wire([Part.makeLine(p1, p2)])
                geomList.append(wire)

        if self.cutPatternReversed:
            geomList.reverse()

        return geomList

    def _ZigZagOffset(self):
        """_ZigZagOffset()... Returns raw set of ZigZag wires at Z=0.0, with the Offset portion added later in the `_generatePathGeometry()` method."""
        return self._ZigZag()

    def _Offset(self):
        """_Offset()...
        Returns raw set of Offset wires at Z=0.0.
        Direction of cut is taken into account.
        Additional offset loop ordering is handled in the linking method.
        """
        PathLog.debug("_Offset()")
        wires = list()
        shape = self.face
        offset = (
            0.0  # Start right at the edge of cut area, reverse order later if needed
        )
        direction = 0
        loop_cnt = 0

        def _get_direction(w):
            if PathOpTools._isWireClockwise(w):
                return 1
            return -1

        def _reverse_wire(w):
            rev_list = list()
            for e in w.Edges:
                rev_list.append(PathUtils.reverseEdge(e))
            rev_list.reverse()
            return Part.Wire(Part.__sortEdges__(rev_list))

        while True:
            offsetArea = PathUtils.getOffsetArea(shape, offset, plane=self.workingPlane)
            if not offsetArea:
                # Area fully consumed
                break

            # set initial cut direction
            if direction == 0:
                first_face_wire = offsetArea.Faces[0].Wires[0]
                direction = _get_direction(first_face_wire)
                if direction == 1:
                    direction = -1

            # process each wire within face
            for f in offsetArea.Faces:
                for w in f.Wires:
                    use_direction = direction
                    if self.cutPatternReversed:
                        use_direction = -1 * direction
                    wire_direction = _get_direction(w)
                    # Process wire
                    if wire_direction == use_direction:  # direction is correct
                        wire = w
                    else:  # incorrect direction, so reverse wire
                        wire = _reverse_wire(w)
                    wires.append(wire)

            offset -= self.cutOut
            loop_cnt += 1
        # Ewhile

        return wires

    def _Spiral(self):
        """_Spiral()... Returns raw set of Spiral wires at Z=0.0."""
        geomList = list()
        allEdges = list()
        draw = True
        loopRadians = 0.0  # Used to keep track of complete loops/cycles
        sumRadians = 0.0
        loopCnt = 0
        segCnt = 0
        twoPi = 2.0 * math.pi
        maxDist = math.ceil(self.cutOut * self._getRadialPasses())  # self.halfDiag
        move = self.centerOfPattern  # Use to translate the center of the spiral
        lastPoint = self.centerOfPattern

        # Set tool properties and calculate cutout
        cutOut = self.cutOut / twoPi
        segLen = self.cutOut / 2.0  # self.sampleInterval
        stepAng = segLen / ((loopCnt + 1) * self.cutOut)
        stopRadians = maxDist / cutOut

        if self.cutPatternReversed:
            PathLog.debug("_Spiral() regular pattern")
            if self.cutDirection == "Climb":
                getPoint = self._makeRegSpiralPnt
            else:
                getPoint = self._makeOppSpiralPnt

            while draw:
                radAng = sumRadians + stepAng
                p1 = lastPoint
                p2 = getPoint(
                    move, cutOut, radAng
                )  # cutOut is 'b' in the equation r = b * radAng
                sumRadians += stepAng  # Increment sumRadians
                loopRadians += stepAng  # Increment loopRadians
                if loopRadians > twoPi:
                    loopCnt += 1
                    loopRadians -= twoPi
                    stepAng = segLen / (
                        (loopCnt + 1) * self.cutOut
                    )  # adjust stepAng with each loop/cycle
                # Create line and show in Object tree
                lineSeg = Part.makeLine(p1, p2)
                allEdges.append(lineSeg)
                # increment loop items
                segCnt += 1
                lastPoint = p2
                if sumRadians > stopRadians:
                    draw = False
            # Ewhile
        else:
            PathLog.debug("_Spiral() REVERSED pattern")
            if self.cutDirection == "Conventional":
                getPoint = self._makeOppSpiralPnt
            else:
                getPoint = self._makeRegSpiralPnt

            while draw:
                radAng = sumRadians + stepAng
                p1 = lastPoint
                p2 = getPoint(
                    move, cutOut, radAng
                )  # cutOut is 'b' in the equation r = b * radAng
                sumRadians += stepAng  # Increment sumRadians
                loopRadians += stepAng  # Increment loopRadians
                if loopRadians > twoPi:
                    loopCnt += 1
                    loopRadians -= twoPi
                    stepAng = segLen / (
                        (loopCnt + 1) * self.cutOut
                    )  # adjust stepAng with each loop/cycle
                segCnt += 1
                lastPoint = p2
                if sumRadians > stopRadians:
                    draw = False
                # Create line and show in Object tree
                lineSeg = Part.makeLine(p2, p1)
                allEdges.append(lineSeg)
            # Ewhile
            allEdges.reverse()
        # Eif

        spiral = Part.Wire(allEdges)
        geomList.append(spiral)

        return geomList

    def _Grid(self):
        """_Grid()...
        Returns raw set of Grid wires at Z=0.0 using `Path.fromShapes()` to generate gcode.
        Then a short converter algorithm is applied to the gcode to extract wires."""
        self.pocketMode = 6
        return self._extractGridAndTriangleWires()

    def _Triangle(self):
        """_Triangle()...
        Returns raw set of Triangle wires at Z=0.0 using `Path.fromShapes()` to generate gcode.
        Then a short converter algorithm is applied to the gcode to extract wires."""
        self.pocketMode = 7
        return self._extractGridAndTriangleWires()

    def _Adaptive(self):
        """_Adaptive()...
        Returns raw set of Adaptive wires at Z=0.0 using a condensed version of code from the Adaptive operation.
        Currently, no helix entry wires are included, only the clearing portion of wires.
        """
        # PathLog.info("*** Adaptive path geometry generation started...")
        # startTime = time.time()

        self.targetFace.translate(
            FreeCAD.Vector(0.0, 0.0, 0.0 - self.targetFace.BoundBox.ZMin)
        )
        for w in self.targetFace.Wires:
            for e in w.Edges:
                self.pathArray.append([self._discretize(e)])

        path2d = self._convertTo2d(self.pathArray)

        stockPaths = []
        if (
            hasattr(self.stockObj, "StockType")
            and self.stockObj.StockType == "CreateCylinder"
        ):
            stockPaths.append([self._discretize(self.stockObj.Shape.Edges[0])])

        else:
            stockBB = self.stockObj.Shape.BoundBox
            v = []
            v.append(FreeCAD.Vector(stockBB.XMin, stockBB.YMin, 0))
            v.append(FreeCAD.Vector(stockBB.XMax, stockBB.YMin, 0))
            v.append(FreeCAD.Vector(stockBB.XMax, stockBB.YMax, 0))
            v.append(FreeCAD.Vector(stockBB.XMin, stockBB.YMax, 0))
            v.append(FreeCAD.Vector(stockBB.XMin, stockBB.YMin, 0))
            stockPaths.append([v])

        stockPath2d = self._convertTo2d(stockPaths)

        opType = area.AdaptiveOperationType.ClearingInside
        if self.operationType == "Clearing":
            if self.cutSide == "Outside":
                opType = area.AdaptiveOperationType.ClearingOutside
            else:
                opType = area.AdaptiveOperationType.ClearingInside
        else:  # profile
            if self.cutSide == "Outside":
                opType = area.AdaptiveOperationType.ProfilingOutside
            else:
                opType = area.AdaptiveOperationType.ProfilingInside

        a2d = area.Adaptive2d()
        a2d.stepOverFactor = 0.01 * self.stepOver
        a2d.toolDiameter = self.toolDiameter
        a2d.helixRampDiameter = self.helixDiameterLimit
        a2d.keepToolDownDistRatio = self.keepToolDownRatio
        a2d.stockToLeave = self.materialAllowance
        a2d.tolerance = self.tolerance
        a2d.forceInsideOut = self.forceInsideOut
        a2d.finishingProfile = self.finishingProfile
        a2d.opType = opType

        def progressFn(tpaths):
            """progressFn(tpaths)... progress callback fn, if return true it will stop processing"""
            return False

        # EXECUTE
        try:
            results = a2d.Execute(stockPath2d, path2d, progressFn)
        except Exception as ee:
            FreeCAD.Console.PrintError(str(ee) + "\n")
            return list()
        else:
            # need to convert results to python object to be JSON serializable
            adaptiveResults = []
            for result in results:
                adaptiveResults.append(
                    {
                        "HelixCenterPoint": result.HelixCenterPoint,
                        "StartPoint": result.StartPoint,
                        "AdaptivePaths": result.AdaptivePaths,
                        "ReturnMotionType": result.ReturnMotionType,
                    }
                )

            # Generate geometry
            # PathLog.debug("Extracting wires from Adaptive data...")
            wires = list()
            motionCutting = area.AdaptiveMotionType.Cutting
            for region in adaptiveResults:
                for pth in region["AdaptivePaths"]:
                    motion = pth[0]  # [0] contains motion type
                    if motion == motionCutting:
                        edges = list()
                        sp = pth[1][0]
                        x = sp[0]
                        y = sp[1]
                        p1 = FreeCAD.Vector(x, y, 0.0)
                        for pt in pth[1][1:]:  # [1] contains list of points
                            xx = pt[0]
                            yy = pt[1]
                            p2 = FreeCAD.Vector(xx, yy, 0.0)
                            if not PathGeom.isRoughly(p1.sub(p2).Length, 0.0):
                                edges.append(Part.makeLine(p1, p2))
                                p1 = p2
                        wires.append(Part.Wire(Part.__sortEdges__(edges)))
            self.adaptiveGeometry = wires
            # PathLog.info("*** Done. Elapsed time: %f sec" % (time.time()-startTime))
            return self.adaptiveGeometry

    # Path linking methods
    def _Link_Line(self):
        """_Link_Line()... Apply necessary linking to resulting wire set after common between target face and raw wire set."""
        allGroups = list()
        allWires = list()

        def isOriented(direction, p0, p1):
            oriented = p1.sub(p0).normalize()
            if PathGeom.isRoughly(direction.sub(oriented).Length, 0.0):
                return True
            return False

        i = 0
        edges = self.rawPathGeometry.Edges
        limit = len(edges)

        if limit == 0:
            PathLog.debug("no edges to link")
            return allWires

        e = edges[0]
        p0 = e.Vertexes[0].Point
        p1 = e.Vertexes[1].Point
        vect = p1.sub(p0)
        targetAng = math.atan2(vect.y, vect.x)
        group = [(edges[0], vect.Length)]
        direction = p1.sub(p0).normalize()

        for i in range(1, limit):
            # get next edge
            ne = edges[i]
            np0 = ne.Vertexes[0].Point  # Next point 0
            np1 = ne.Vertexes[1].Point  # Next point 1
            diff = np1.sub(p0)
            nxtAng = math.atan2(diff.y, diff.x)

            # Check if prev and next are colinear
            angDiff = abs(nxtAng - targetAng)
            if 0.000001 > angDiff:
                if isOriented(direction, np0, np1):
                    group.append((ne, np1.sub(p0).Length))
                else:
                    # PathLog.info("flipping line")
                    line = Part.makeLine(np1, np0)  # flip line segment
                    group.append((line, np1.sub(p0).Length))
            else:
                # Save current group
                allGroups.append(group)
                # Rotate edge and point value
                e = ne
                p0 = np0
                # Create new group
                group = [(ne, np1.sub(p0).Length)]

        allGroups.append(group)

        if self.cutPattern.startswith("ZigZag") and self.keepToolDown and False:
            # The KeepToolDown feature likely needs an independent path-building method to properly keep tool down on zigs and zags
            g = allGroups.pop(0)
            if len(g) == 1:
                wires = [Part.Wire([g[0][0]])]
            else:
                g.sort(key=lambda grp: grp[1])
                wires = [Part.Wire([edg]) for edg, __ in g]
            allWires.extend(wires)
            # get last vertex
            lastWire = allWires[len(allWires) - 1]
            lastEndPoint = lastWire.Vertexes[1].Point

            for g in allGroups:
                if len(g) == 1:
                    wires = [Part.Wire([g[0][0]])]
                    lastWire = wires[0]
                else:
                    g.sort(key=lambda grp: grp[1])
                    wires = [Part.Wire([edg]) for edg, __ in g]
                    lastWire = wires[len(wires) - 1]
                startPoint = wires[0].Vertexes[0].Point
                transitionWire = Part.Wire(Part.makeLine(lastEndPoint, startPoint))
                wires.insert(0, transitionWire)
                lastEndPoint = lastWire.Vertexes[1].Point
                allWires.extend(wires)

        else:
            for g in allGroups:
                if len(g) == 1:
                    wires = [Part.Wire([g[0][0]])]
                else:
                    g.sort(key=lambda grp: grp[1])
                    wires = [Part.Wire([edg]) for edg, __ in g]
                allWires.extend(wires)

        return allWires

    def _Link_LineOffset(self):
        """_Link_Line()... Apply necessary linking to resulting wire set after common between target face and raw wire set."""
        return self._Link_Line()

    def _Link_Circular(self):
        """_Link_Circular()... Apply necessary linking to resulting wire set after common between target face and raw wire set."""
        # PathLog.debug("_Link_Circular()")

        def combineAdjacentArcs(grp):
            """combineAdjacentArcs(arcList)...
            Combine two adjacent arcs in list into single.
            The two arcs in the original list are replaced by the new single. The modified list is returned.
            """
            # PathLog.debug("combineAdjacentArcs()")

            i = 1
            limit = len(grp)
            arcs = list()
            saveLast = False

            arc = grp[0]
            aP0 = arc.Vertexes[0].Point
            aP1 = arc.Vertexes[1].Point

            while i < limit:
                nArc = grp[i]
                naP0 = nArc.Vertexes[0].Point
                naP1 = nArc.Vertexes[1].Point
                if abs(arc.Curve.AngleXU) == 0.0:
                    reversed = False
                else:
                    reversed = True
                # Check if arcs are connected
                if naP1.sub(aP0).Length < 0.00001:
                    # PathLog.debug("combining arcs")
                    # Create one continuous arc
                    cent = arc.Curve.Center
                    vect0 = aP1.sub(cent)
                    vect1 = naP0.sub(cent)
                    radius = arc.Curve.Radius
                    direct = FreeCAD.Vector(0.0, 0.0, 1.0)
                    angle0 = math.degrees(math.atan2(vect1.y, vect1.x))
                    angle1 = math.degrees(math.atan2(vect0.y, vect0.x))
                    if reversed:
                        newArc = Part.makeCircle(
                            radius,
                            cent,
                            direct.multiply(-1.0),
                            360.0 - angle0,
                            360 - angle1,
                        )  # makeCircle(radius,[pnt,dir,angle1,angle2])
                    else:
                        newArc = Part.makeCircle(
                            radius, cent, direct, angle0, angle1
                        )  # makeCircle(radius,[pnt,dir,angle1,angle2])
                    ang = aP0.sub(cent).normalize()
                    line = Part.makeLine(cent, aP0.add(ang))
                    touch = DraftGeomUtils.findIntersection(newArc, line)
                    if not touch:
                        if reversed:
                            newArc = Part.makeCircle(
                                radius,
                                cent,
                                direct.multiply(-1.0),
                                360.0 - angle1,
                                360 - angle0,
                            )  # makeCircle(radius,[pnt,dir,angle1,angle2])
                        else:
                            newArc = Part.makeCircle(
                                radius, cent, direct, angle1, angle0
                            )  # makeCircle(radius,[pnt,dir,angle1,angle2])
                    arcs.append(newArc)
                    i += 1
                    if i < limit:
                        arc = grp[i]
                        aP0 = arc.Vertexes[0].Point
                        aP1 = arc.Vertexes[1].Point
                        saveLast = True
                    else:
                        saveLast = False
                        break
                else:
                    arcs.append(arc)
                    arc = nArc
                    aP0 = arc.Vertexes[0].Point
                    aP1 = arc.Vertexes[1].Point
                    saveLast = True
                i += 1

            if saveLast:
                arcs.append(arc)

            return arcs

        allGroups = list()
        allEdges = list()
        if self.cutPatternReversed:  # inside to out
            edges = sorted(
                self.rawPathGeometry.Edges,
                key=lambda e: e.Curve.Center.sub(self.centerOfPattern).Length,
            )
        else:
            edges = sorted(
                self.rawPathGeometry.Edges,
                key=lambda e: e.Curve.Center.sub(self.centerOfPattern).Length,
                reverse=True,
            )
        limit = len(edges)

        if limit == 0:
            return allEdges

        e = edges[0]
        rad = e.Curve.Radius
        group = [e]

        if limit > 1:
            for i in range(1, limit):
                # get next edge
                ne = edges[i]
                nRad = ne.Curve.Radius

                # Check if prev and next are colinear
                if abs(nRad - rad) < 0.000001:
                    group.append(ne)
                else:
                    allGroups.append(group)
                    e = ne
                    rad = nRad
                    group = [ne]

        allGroups.append(group)

        # Process last remaining group of edges
        for g in allGroups:
            if len(g) < 2:
                allEdges.append(Part.Wire(g[0]))
            else:
                wires = [Part.Wire(arc) for arc in combineAdjacentArcs(g)]
                allEdges.extend(wires)

        return allEdges

    def _Link_CircularZigZag(self):
        """_Link_CircularZigZag()... Apply necessary linking to resulting wire set after common between target face and raw wire set."""
        return self._Link_Circular()

    def _Link_ZigZag(self):
        """_Link_ZigZag()... Apply necessary linking to resulting wire set after common between target face and raw wire set."""
        return self._Link_Line()

    def _Link_ZigZagOffset(self):
        """_Link_ZigZagOffset()... Apply necessary linking to resulting wire set after common between target face and raw wire set."""
        return self._Link_Line()

    def _Link_Offset(self):
        """_Link_Offset()... Apply necessary linking to resulting wire set after common between target face and raw wire set."""
        if self.cutPatternReversed:
            return sorted(
                self.rawPathGeometry.Wires, key=lambda wire: Part.Face(wire).Area
            )
        else:
            return sorted(
                self.rawPathGeometry.Wires,
                key=lambda wire: Part.Face(wire).Area,
                reverse=True,
            )

    def _Link_Spiral(self):
        """_Link_Spiral()... Apply necessary linking to resulting wire set after common between target face and raw wire set."""

        def sortWires0(wire):
            return wire.Edges[0].Vertexes[0].Point.sub(self.centerOfPattern).Length

        def sortWires1(wire):
            eIdx = len(wire.Edges) - 1
            return wire.Edges[eIdx].Vertexes[1].Point.sub(self.centerOfPattern).Length

        if self.cutPatternReversed:
            # Center outward
            return sorted(self.rawPathGeometry.Wires, key=sortWires0)
        else:
            # Outside inward
            return sorted(self.rawPathGeometry.Wires, key=sortWires1, reverse=True)

    def _Link_Grid(self):
        """_Link_Grid()... Apply necessary linking to resulting wire set after common between target face and raw wire set."""
        # No linking required.
        return self.rawPathGeometry.Wires

    def _Link_Triangle(self):
        """_Link_Grid()... Apply necessary linking to resulting wire set after common between target face and raw wire set."""
        # No linking required.
        return self.rawPathGeometry.Wires

    def _Link_Adaptive(self):
        """_Link_Adaptive()... Apply necessary linking to resulting wire set after common between target face and raw wire set."""
        # No linking required.
        return self.rawPathGeometry.Wires

    # Support methods
    def _prepareAttributes(self):
        """_prepareAttributes()... Prepare instance attribute values for path generation."""
        if self.isCenterSet:
            if self.useStaticCenter:
                return

        # Compute weighted center of mass of all faces combined
        if self.patternCenterAt == "CenterOfMass":
            comF = self.face.CenterOfMass
            self.centerOfMass = FreeCAD.Vector(comF.x, comF.y, 0.0)
        self.centerOfPattern = self._getPatternCenter()

        # calculate line length
        deltaC = self.targetFace.BoundBox.DiagonalLength
        lineLen = deltaC + (
            2.0 * self.toolDiameter
        )  # Line length to span boundbox diag with 2x cutter diameter extra on each end
        if self.patternCenterAt == "Custom":
            distToCent = self.face.BoundBox.Center.sub(self.centerOfPattern).Length
            lineLen += distToCent
        self.halfDiag = math.ceil(lineLen / 2.0)

        # Calculate number of passes
        cutPasses = (
            math.ceil(lineLen / self.cutOut) + 1
        )  # Number of lines(passes) required to cover boundbox diagonal
        if self.patternCenterAt == "Custom":
            self.halfPasses = math.ceil(cutPasses)
        else:
            self.halfPasses = math.ceil(cutPasses / 2.0)

        self.isCenterSet = True

    def _getPatternCenter(self):
        """_getPatternCenter()... Determine center of cut pattern and save in instance attribute."""
        centerAt = self.patternCenterAt

        if centerAt == "CenterOfMass":
            cntrPnt = FreeCAD.Vector(self.centerOfMass.x, self.centerOfMass.y, 0.0)
        elif centerAt == "CenterOfBoundBox":
            cent = self.face.BoundBox.Center
            cntrPnt = FreeCAD.Vector(cent.x, cent.y, 0.0)
        elif centerAt == "XminYmin":
            cntrPnt = FreeCAD.Vector(
                self.face.BoundBox.XMin, self.face.BoundBox.YMin, 0.0
            )
        elif centerAt == "Custom":
            cntrPnt = FreeCAD.Vector(
                self.patternCenterCustom.x, self.patternCenterCustom.y, 0.0
            )

        self.centerOfPattern = cntrPnt

        return cntrPnt

    def _getRadialPasses(self):
        """_getRadialPasses()... Return number of radial passes required for circular and spiral patterns."""
        # recalculate number of passes, if need be
        radialPasses = self.halfPasses
        if self.patternCenterAt != "CenterOfBoundBox":
            # make 4 corners of boundbox in XY plane, find which is greatest distance to new circular center
            EBB = self.face.BoundBox
            CORNERS = [
                FreeCAD.Vector(EBB.XMin, EBB.YMin, 0.0),
                FreeCAD.Vector(EBB.XMin, EBB.YMax, 0.0),
                FreeCAD.Vector(EBB.XMax, EBB.YMax, 0.0),
                FreeCAD.Vector(EBB.XMax, EBB.YMin, 0.0),
            ]
            dMax = 0.0
            for c in range(0, 4):
                dist = CORNERS[c].sub(self.centerOfPattern).Length
                if dist > dMax:
                    dMax = dist
            diag = dMax + (
                2.0 * self.toolDiameter
            )  # Line length to span boundbox diag with 2x cutter diameter extra on each end
            radialPasses = (
                math.ceil(diag / self.cutOut) + 1
            )  # Number of lines(passes) required to cover boundbox diagonal

        return radialPasses

    def _makeRegSpiralPnt(self, move, b, radAng):
        """_makeRegSpiralPnt(move, b, radAng)... Return next point on regular spiral pattern."""
        x = b * radAng * math.cos(radAng)
        y = b * radAng * math.sin(radAng)
        return FreeCAD.Vector(x, y, 0.0).add(move)

    def _makeOppSpiralPnt(self, move, b, radAng):
        """_makeOppSpiralPnt(move, b, radAng)... Return next point on opposite(reversed) spiral pattern."""
        x = b * radAng * math.cos(radAng)
        y = b * radAng * math.sin(radAng)
        return FreeCAD.Vector(-1 * x, y, 0.0).add(move)

    def _getProfileWires(self):
        """_getProfileWires()... Return set of profile wires for target face."""
        wireList = list()
        shape = self.face
        offset = 0.0
        direction = 1  # 'Conventional on outer loop of Pocket clearing
        if self.cutDirection == "Climb":
            direction = -1

        def _get_direction(w):
            if PathOpTools._isWireClockwise(w):
                return 1
            return -1

        def _reverse_wire(w):
            rev_list = list()
            for e in w.Edges:
                rev_list.append(PathUtils.reverseEdge(e))
            # rev_list.reverse()
            return Part.Wire(Part.__sortEdges__(rev_list))

        offsetArea = PathUtils.getOffsetArea(shape, offset, plane=self.workingPlane)
        if not offsetArea:
            PathLog.debug("_getProfileWires() no offsetArea")
            # Area fully consumed
            return wireList

        # process each wire within face
        for f in offsetArea.Faces:
            wCnt = 0
            for w in f.Wires:
                use_direction = direction
                wire_direction = _get_direction(w)
                if wCnt > 0:
                    use_direction = -1 * direction
                # Process wire
                if wire_direction == use_direction:  # direction is correct
                    wire = w
                else:  # incorrect direction, so reverse wire
                    wire = _reverse_wire(w)
                wireList.append(wire)
                wCnt += 1

        return wireList

    def _applyPathLinking(self):
        """_applyPathLinking()... Control method for applying linking to target wire set."""
        PathLog.debug("_applyPathLinking({})".format(self.cutPattern))

        # patterns = ('Adaptive', 'Circular', 'CircularZigZag', 'Grid', 'Line', 'LineOffset', 'Offset', 'Spiral', 'Triangle', 'ZigZag', 'ZigZagOffset')
        linkMethod = getattr(self, "_Link_" + self.cutPattern)
        self.linkedPathGeom = linkMethod()

    def _generatePathGeometry(self):
        """_generatePathGeometry()... Control function that generates path geometry wire sets."""
        PathLog.debug("_generatePathGeometry()")

        patternMethod = getattr(self, "_" + self.cutPattern)
        self.rawGeoList = patternMethod()

        # Create compound object to bind all geometry
        geomShape = Part.makeCompound(self.rawGeoList)

        self._addDebugShape(geomShape, "rawPathGeomShape")  # Debugging

        # Position and rotate the Line and ZigZag geometry
        if self.cutPattern in self.rotatablePatterns:
            if self.cutPatternAngle != 0.0:
                geomShape.Placement.Rotation = FreeCAD.Rotation(
                    FreeCAD.Vector(0, 0, 1), self.cutPatternAngle
                )
            cop = self.centerOfPattern
            geomShape.Placement.Base = FreeCAD.Vector(
                cop.x, cop.y, 0.0 - geomShape.BoundBox.ZMin
            )

        self._addDebugShape(geomShape, "tmpGeometrySet")  # Debugging

        # Return current geometry for Offset or Profile patterns
        if self.cutPattern == "Offset":
            self.rawPathGeometry = geomShape
            self._applyPathLinking()
            return self.linkedPathGeom

        # Add profile 'Offset' path after base pattern
        appendOffsetWires = False
        if self.cutPattern != "Offset" and self.cutPattern[-6:] == "Offset":
            appendOffsetWires = True

        # Identify intersection of cross-section face and lineset
        self.rawPathGeometry = self.face.common(Part.makeCompound(geomShape.Wires))

        self._addDebugShape(self.rawPathGeometry, "rawPathGeometry")  # Debugging

        self._applyPathLinking()
        if appendOffsetWires:
            self.linkedPathGeom.extend(self._getProfileWires())

        return self.linkedPathGeom

    def _extractGridAndTriangleWires(self):
        """_buildGridAndTrianglePaths() ... Returns a set of wires representng Grid or Triangle cut paths."""
        PathLog.track()
        # areaParams = {}
        pathParams = {}

        if self.cutDirection == "Climb":
            self.orientation = 1

        # Set path parameters
        pathParams["orientation"] = self.orientation
        # if MinTravel is turned on, set path sorting to 3DSort
        # 3DSort shouldn't be used without a valid start point. Can cause
        # tool crash without it.
        #
        # ml: experimental feature, turning off for now (see https://forum.freecadweb.org/viewtopic.php?f=15&t=24422&start=30#p192458)
        # realthunder: I've fixed it with a new sorting algorithm, which I
        # tested fine, but of course need more test. Please let know if there is
        # any problem
        #
        if self.minTravel and self.startPoint:
            pathParams["sort_mode"] = 3
            pathParams["threshold"] = self.toolRadius * 2
        pathParams["shapes"] = [self.targetFace]
        pathParams["feedrate"] = self.horizFeed
        pathParams["feedrate_v"] = self.vertFeed
        pathParams["verbose"] = True
        pathParams["resume_height"] = self.safeHeight
        pathParams["retraction"] = self.clearanceHeight
        pathParams["return_end"] = True
        # Note that emitting preambles between moves breaks some dressups and prevents path optimization on some controllers
        pathParams["preamble"] = False

        if self.keepToolDown:
            pathParams["threshold"] = self.toolDiameter

        if self.endVector is not None:
            pathParams["start"] = self.endVector
        elif self.startPoint:
            pathParams["start"] = self.startPoint

        self.pathParams = str(
            {key: value for key, value in pathParams.items() if key != "shapes"}
        )
        PathLog.debug("Path with params: {}".format(self.pathParams))

        # Build paths from path parameters
        (pp, end_vector) = Path.fromShapes(**pathParams)
        PathLog.debug("pp: {}, end vector: {}".format(pp, end_vector))
        self.endVector = end_vector  # pylint: disable=attribute-defined-outside-init

        self.commandList = pp.Commands

        # Use modified version of PathGeom.wiresForPath() to extract wires from paths
        wires = []
        startPoint = FreeCAD.Vector(0.0, 0.0, 0.0)
        if self.startPoint:
            startPoint = self.startPoint
        if hasattr(pp, "Commands"):
            edges = []
            for cmd in pp.Commands:
                if cmd.Name in PathGeom.CmdMove:
                    edg = PathGeom.edgeForCmd(cmd, startPoint)
                    if PathGeom.isRoughly(edg.Vertexes[0].Z, edg.Vertexes[1].Z):
                        edges.append(edg)
                    startPoint = PathGeom.commandEndPoint(cmd, startPoint)

                elif cmd.Name in PathGeom.CmdMoveRapid:
                    if len(edges) > 0:
                        wires.append(Part.Wire(edges))
                        edges = []
                    startPoint = PathGeom.commandEndPoint(cmd, startPoint)
            if edges:
                wires.append(Part.Wire(edges))
        return wires

    # Private adaptive support methods
    def _convertTo2d(self, pathArray):
        """_convertTo2d() ... Converts array of edge lists into list of point list pairs. Used for Adaptive cut pattern."""
        output = []
        for path in pathArray:
            pth2 = []
            for edge in path:
                for pt in edge:
                    pth2.append([pt[0], pt[1]])
            output.append(pth2)
        return output

    def _discretize(self, edge, flipDirection=False):
        """_discretize(edge, flipDirection=False) ... Discretizes an edge into a set of points. Used for Adaptive cut pattern."""
        pts = edge.discretize(Deflection=0.0001)
        if flipDirection:
            pts.reverse()

        return pts

    def _generateGCode(self, adaptiveResults):
        """_generateGCode(adaptiveResults) ...
        Converts raw Adaptive algorithm data into gcode.
        Not currently active.  Will be modified to extract helix data as wires.
        Will be used for Adaptive cut pattern."""
        self.commandList = list()
        commandList = list()
        motionCutting = area.AdaptiveMotionType.Cutting
        motionLinkClear = area.AdaptiveMotionType.LinkClear
        motionLinkNotClear = area.AdaptiveMotionType.LinkNotClear

        # pylint: disable=unused-argument
        if len(adaptiveResults) == 0 or len(adaptiveResults[0]["AdaptivePaths"]) == 0:
            return

        helixRadius = 0
        for region in adaptiveResults:
            p1 = region["HelixCenterPoint"]
            p2 = region["StartPoint"]
            r = math.sqrt(
                (p1[0] - p2[0]) * (p1[0] - p2[0]) + (p1[1] - p2[1]) * (p1[1] - p2[1])
            )
            if r > helixRadius:
                helixRadius = r

        passStartDepth = self.startDepth

        length = 2 * math.pi * helixRadius

        helixAngleRad = math.pi * self.helixAngle / 180.0
        depthPerOneCircle = length * math.tan(helixAngleRad)
        # print("Helix circle depth: {}".format(depthPerOneCircle))

        stepUp = self.liftDistance
        if stepUp < 0:
            stepUp = 0

        stepDown = self.stepDown
        finish_step = self.finishDepth
        if finish_step > stepDown:
            finish_step = stepDown

        depth_params = PathUtils.depth_params(
            clearance_height=self.clearanceHeight,
            safe_height=self.safeHeight,
            start_depth=self.startDepth,
            step_down=self.stepDown,
            z_finish_step=finish_step,
            final_depth=self.finalDepth,
            user_depths=None,
        )

        # ml: this is dangerous because it'll hide all unused variables hence forward
        #     however, I don't know what lx and ly signify so I'll leave them for now
        # russ4262: I think that the `l` in `lx, ly, and lz` stands for `last`.
        # pylint: disable=unused-variable
        # lx = adaptiveResults[0]["HelixCenterPoint"][0]
        # ly = adaptiveResults[0]["HelixCenterPoint"][1]
        lz = passStartDepth  # lz is likely `last Z depth`
        step = 0

        for passEndDepth in depth_params.data:
            step = step + 1

            for region in adaptiveResults:
                startAngle = math.atan2(
                    region["StartPoint"][1] - region["HelixCenterPoint"][1],
                    region["StartPoint"][0] - region["HelixCenterPoint"][0],
                )

                # lx = region["HelixCenterPoint"][0]
                # ly = region["HelixCenterPoint"][1]

                passDepth = passStartDepth - passEndDepth

                p1 = region["HelixCenterPoint"]
                p2 = region["StartPoint"]
                helixRadius = math.sqrt(
                    (p1[0] - p2[0]) * (p1[0] - p2[0])
                    + (p1[1] - p2[1]) * (p1[1] - p2[1])
                )

                # Helix ramp
                if self.useHelixEntry and helixRadius > 0.01:
                    r = helixRadius - 0.01

                    maxfi = passDepth / depthPerOneCircle * 2 * math.pi
                    fi = 0
                    offsetFi = -maxfi + startAngle - math.pi / 16

                    helixStart = [
                        region["HelixCenterPoint"][0] + r * math.cos(offsetFi),
                        region["HelixCenterPoint"][1] + r * math.sin(offsetFi),
                    ]

                    commandList.append(
                        Path.Command("(Helix to depth: %f)" % passEndDepth)
                    )

                    if not self.useHelixArcs:
                        # rapid move to start point
                        commandList.append(
                            Path.Command("G0", {"Z": self.clearanceHeight})
                        )
                        commandList.append(
                            Path.Command(
                                "G0",
                                {
                                    "X": helixStart[0],
                                    "Y": helixStart[1],
                                    "Z": self.clearanceHeight,
                                },
                            )
                        )

                        # rapid move to safe height
                        commandList.append(
                            Path.Command(
                                "G0",
                                {
                                    "X": helixStart[0],
                                    "Y": helixStart[1],
                                    "Z": self.safeHeight,
                                },
                            )
                        )

                        # move to start depth
                        commandList.append(
                            Path.Command(
                                "G1",
                                {
                                    "X": helixStart[0],
                                    "Y": helixStart[1],
                                    "Z": passStartDepth,
                                    "F": self.vertFeed,
                                },
                            )
                        )

                        if self.helixConeAngle == 0:
                            while fi < maxfi:
                                x = region["HelixCenterPoint"][0] + r * math.cos(
                                    fi + offsetFi
                                )
                                y = region["HelixCenterPoint"][1] + r * math.sin(
                                    fi + offsetFi
                                )
                                z = passStartDepth - fi / maxfi * (
                                    passStartDepth - passEndDepth
                                )
                                commandList.append(
                                    Path.Command(
                                        "G1",
                                        {"X": x, "Y": y, "Z": z, "F": self.vertFeed},
                                    )
                                )
                                # lx = x
                                # ly = y
                                fi = fi + math.pi / 16

                            # one more circle at target depth to make sure center is cleared
                            maxfi = maxfi + 2 * math.pi
                            while fi < maxfi:
                                x = region["HelixCenterPoint"][0] + r * math.cos(
                                    fi + offsetFi
                                )
                                y = region["HelixCenterPoint"][1] + r * math.sin(
                                    fi + offsetFi
                                )
                                z = passEndDepth
                                commandList.append(
                                    Path.Command(
                                        "G1",
                                        {"X": x, "Y": y, "Z": z, "F": self.horizFeed},
                                    )
                                )
                                # lx = x
                                # ly = y
                                fi = fi + math.pi / 16

                        else:
                            # Cone
                            _HelixAngle = 360.0 - (self.helixAngle * 4.0)

                            if self.helixConeAngle > 6:
                                self.helixConeAngle = 6

                            helixRadius *= 0.9

                            # Calculate everything
                            helix_height = passStartDepth - passEndDepth
                            r_extra = helix_height * math.tan(
                                math.radians(self.helixConeAngle)
                            )
                            HelixTopRadius = helixRadius + r_extra
                            helix_full_height = HelixTopRadius * (
                                math.cos(math.radians(self.helixConeAngle))
                                / math.sin(math.radians(self.helixConeAngle))
                            )

                            # Start height
                            z = passStartDepth
                            i = 0

                            # Default step down
                            z_step = 0.05

                            # Bigger angle, smaller step down
                            if _HelixAngle > 120:
                                z_step = 0.025
                            if _HelixAngle > 240:
                                z_step = 0.015

                            p = None
                            # Calculate conical helix
                            while z >= passEndDepth:
                                if z < passEndDepth:
                                    z = passEndDepth

                                p = self._calcHelixConePoint(
                                    helix_full_height, i, HelixTopRadius, _HelixAngle
                                )
                                commandList.append(
                                    Path.Command(
                                        "G1",
                                        {
                                            "X": p["X"] + region["HelixCenterPoint"][0],
                                            "Y": p["Y"] + region["HelixCenterPoint"][1],
                                            "Z": z,
                                            "F": self.vertFeed,
                                        },
                                    )
                                )
                                z = z - z_step
                                i = i + z_step

                            # Calculate some stuff for arcs at bottom
                            p["X"] = p["X"] + region["HelixCenterPoint"][0]
                            p["Y"] = p["Y"] + region["HelixCenterPoint"][1]
                            x_m = (
                                region["HelixCenterPoint"][0]
                                - p["X"]
                                + region["HelixCenterPoint"][0]
                            )
                            y_m = (
                                region["HelixCenterPoint"][1]
                                - p["Y"]
                                + region["HelixCenterPoint"][1]
                            )
                            i_off = (x_m - p["X"]) / 2
                            j_off = (y_m - p["Y"]) / 2

                            # One more circle at target depth to make sure center is cleared
                            commandList.append(
                                Path.Command(
                                    "G3",
                                    {
                                        "X": x_m,
                                        "Y": y_m,
                                        "Z": passEndDepth,
                                        "I": i_off,
                                        "J": j_off,
                                        "F": self.horizFeed,
                                    },
                                )
                            )
                            commandList.append(
                                Path.Command(
                                    "G3",
                                    {
                                        "X": p["X"],
                                        "Y": p["Y"],
                                        "Z": passEndDepth,
                                        "I": -i_off,
                                        "J": -j_off,
                                        "F": self.horizFeed,
                                    },
                                )
                            )

                    else:
                        # Use arcs for helix - no conical shape support
                        helixStart = [
                            region["HelixCenterPoint"][0] + r,
                            region["HelixCenterPoint"][1],
                        ]

                        # rapid move to start point
                        commandList.append(
                            Path.Command("G0", {"Z": self.clearanceHeight})
                        )
                        commandList.append(
                            Path.Command(
                                "G0",
                                {
                                    "X": helixStart[0],
                                    "Y": helixStart[1],
                                    "Z": self.clearanceHeight,
                                },
                            )
                        )

                        # rapid move to safe height
                        commandList.append(
                            Path.Command(
                                "G0",
                                {
                                    "X": helixStart[0],
                                    "Y": helixStart[1],
                                    "Z": self.safeHeight,
                                },
                            )
                        )

                        # move to start depth
                        commandList.append(
                            Path.Command(
                                "G1",
                                {
                                    "X": helixStart[0],
                                    "Y": helixStart[1],
                                    "Z": passStartDepth,
                                    "F": self.vertFeed,
                                },
                            )
                        )

                        x = region["HelixCenterPoint"][0] + r
                        y = region["HelixCenterPoint"][1]

                        curDep = passStartDepth
                        while curDep > (passEndDepth + depthPerOneCircle):
                            commandList.append(
                                Path.Command(
                                    "G2",
                                    {
                                        "X": x - (2 * r),
                                        "Y": y,
                                        "Z": curDep - (depthPerOneCircle / 2),
                                        "I": -r,
                                        "F": self.vertFeed,
                                    },
                                )
                            )
                            commandList.append(
                                Path.Command(
                                    "G2",
                                    {
                                        "X": x,
                                        "Y": y,
                                        "Z": curDep - depthPerOneCircle,
                                        "I": r,
                                        "F": self.vertFeed,
                                    },
                                )
                            )
                            curDep = curDep - depthPerOneCircle

                        lastStep = curDep - passEndDepth
                        if lastStep > (depthPerOneCircle / 2):
                            commandList.append(
                                Path.Command(
                                    "G2",
                                    {
                                        "X": x - (2 * r),
                                        "Y": y,
                                        "Z": curDep - (lastStep / 2),
                                        "I": -r,
                                        "F": self.vertFeed,
                                    },
                                )
                            )
                            commandList.append(
                                Path.Command(
                                    "G2",
                                    {
                                        "X": x,
                                        "Y": y,
                                        "Z": passEndDepth,
                                        "I": r,
                                        "F": self.vertFeed,
                                    },
                                )
                            )
                        else:
                            commandList.append(
                                Path.Command(
                                    "G2",
                                    {
                                        "X": x - (2 * r),
                                        "Y": y,
                                        "Z": passEndDepth,
                                        "I": -r,
                                        "F": self.vertFeed,
                                    },
                                )
                            )
                            commandList.append(
                                Path.Command(
                                    "G1",
                                    {
                                        "X": x,
                                        "Y": y,
                                        "Z": passEndDepth,
                                        "F": self.vertFeed,
                                    },
                                )
                            )

                        # one more circle at target depth to make sure center is cleared
                        commandList.append(
                            Path.Command(
                                "G2",
                                {
                                    "X": x - (2 * r),
                                    "Y": y,
                                    "Z": passEndDepth,
                                    "I": -r,
                                    "F": self.horizFeed,
                                },
                            )
                        )
                        commandList.append(
                            Path.Command(
                                "G2",
                                {
                                    "X": x,
                                    "Y": y,
                                    "Z": passEndDepth,
                                    "I": r,
                                    "F": self.horizFeed,
                                },
                            )
                        )
                        # lx = x
                        # ly = y

                else:  # no helix entry
                    # rapid move to clearance height
                    commandList.append(Path.Command("G0", {"Z": self.clearanceHeight}))
                    commandList.append(
                        Path.Command(
                            "G0",
                            {
                                "X": region["StartPoint"][0],
                                "Y": region["StartPoint"][1],
                                "Z": self.clearanceHeight,
                            },
                        )
                    )
                    # straight plunge to target depth
                    commandList.append(
                        Path.Command(
                            "G1",
                            {
                                "X": region["StartPoint"][0],
                                "Y": region["StartPoint"][1],
                                "Z": passEndDepth,
                                "F": self.vertFeed,
                            },
                        )
                    )

                lz = passEndDepth
                z = self.clearanceHeight
                commandList.append(
                    Path.Command("(Adaptive - depth: %f)" % passEndDepth)
                )

                # add adaptive paths
                for pth in region["AdaptivePaths"]:
                    motionType = pth[0]  # [0] contains motion type

                    for pt in pth[1]:  # [1] contains list of points
                        x = pt[0]
                        y = pt[1]

                        # dist = math.sqrt((x-lx)*(x-lx) + (y-ly)*(y-ly))

                        if motionType == motionCutting:
                            z = passEndDepth
                            if z != lz:
                                commandList.append(
                                    Path.Command("G1", {"Z": z, "F": self.vertFeed})
                                )  # plunge at feed rate

                            commandList.append(
                                Path.Command(
                                    "G1", {"X": x, "Y": y, "F": self.horizFeed}
                                )
                            )  # feed to point

                        elif motionType == motionLinkClear:
                            z = passEndDepth + stepUp
                            if z != lz:
                                commandList.append(
                                    Path.Command("G0", {"Z": z})
                                )  # rapid to previous pass depth

                            commandList.append(
                                Path.Command("G0", {"X": x, "Y": y})
                            )  # rapid to point

                        elif motionType == motionLinkNotClear:
                            z = self.clearanceHeight
                            if z != lz:
                                commandList.append(
                                    Path.Command("G0", {"Z": z})
                                )  # rapid to clearance height

                            commandList.append(
                                Path.Command("G0", {"X": x, "Y": y})
                            )  # rapid to point

                        # elif motionType == area.AdaptiveMotionType.LinkClearAtPrevPass:
                        #     if lx!=x or ly!=y:
                        #         commandList.append(Path.Command("G0", { "X": lx, "Y":ly, "Z":passStartDepth+stepUp}))
                        #     commandList.append(Path.Command("G0", { "X": x, "Y":y, "Z":passStartDepth+stepUp}))

                        # rotate values: current values become last for next loop
                        # lx = x
                        # ly = y
                        lz = z

                # return to clearance height in this Z pass
                z = self.clearanceHeight
                if z != lz:
                    commandList.append(Path.Command("G0", {"Z": z}))

                lz = z

            passStartDepth = passEndDepth

            # return to safe height in this Z pass
            z = self.clearanceHeight
            if z != lz:
                commandList.append(Path.Command("G0", {"Z": z}))

            lz = z

        z = self.clearanceHeight
        if z != lz:
            commandList.append(Path.Command("G0", {"Z": z}))

        lz = z

        # Save commands
        self.commandList = commandList

    # Public methods
    def setAdaptiveAttributes(
        self,
        operationType,
        cutSide,
        disableHelixEntry,
        forceInsideOut,
        liftDistance,
        finishingProfile,
        helixAngle,
        helixConeAngle,
        useHelixArcs,
        helixDiameterLimit,
        keepToolDownRatio,
        tolerance,
        stockObj,
    ):
        """setAdaptiveAttributes(operationType,
                                 cutSide,
                                 disableHelixEntry,
                                 forceInsideOut,
                                 liftDistance,
                                 finishingProfile,
                                 helixAngle,
                                 helixConeAngle,
                                 useHelixArcs,
                                 helixDiameterLimit,
                                 keepToolDownRatio,
                                 tolerance,
                                 stockObj):
        Call to set adaptive-dependent attributes prior to calling `execute()` method.
        Arguments:
            operationType:      clearing or profile
            cutSide:            inside or outside
            disableHelixEntry:  boolean to disable helix entry (feature hard coded True, with no helix geometry available at this time)
            forceInsideOut:     boolean to force cutting direction inside-out
            liftDistance:       distance to lift cutter between same-layer linking moves
            finishingProfile:   boolean to include finishing profile path
            helixAngle:         helix angle
            helixConeAngle:     helix cone angle
            useHelixArcs:       boolean to use G2/G3 arcs for helix in place of G1 line segments
            helixDiameterLimit: max diameter for helix entry
            keepToolDownRatio:  threshold value for keeping tool down
            tolerance:          tolerance versus accuracy value within set range
            stockObj:           reference to job stock object
        """
        # Apply limits to argument values
        if tolerance < 0.001:
            tolerance = 0.001

        if helixAngle < 1.0:
            helixAngle = 1.0
        if helixAngle > 89.0:
            helixAngle = 89.0

        if helixConeAngle < 0.0:
            helixConeAngle = 0.0

        self.operationType = operationType
        self.cutSide = cutSide
        self.disableHelixEntry = disableHelixEntry
        self.forceInsideOut = forceInsideOut
        self.liftDistance = liftDistance
        self.finishingProfile = finishingProfile
        self.helixAngle = helixAngle
        self.helixConeAngle = helixConeAngle
        self.useHelixArcs = useHelixArcs
        self.helixDiameterLimit = helixDiameterLimit
        self.keepToolDownRatio = keepToolDownRatio
        self.tolerance = tolerance
        self.stockObj = stockObj

        # if disableHelixEntry:
        #    self.helixDiameterLimit = 0.01
        #    self.helixAngle = 89.0

    def execute(self):
        """execute()...
        Call this method to execute the path generation code in PathGeometryGenerator class.
        Returns True on success.  Access class instance `pathGeometry` attribute for path geometry.
        """
        PathLog.debug("StrategyClearing.execute()")

        self.commandList = list()  # Reset list
        self.pathGeometry = list()  # Reset list
        self.isCenterSet = False

        # Exit if pattern not available
        if self.cutPattern == "None":
            return False

        if hasattr(self.targetFace, "Area") and PathGeom.isRoughly(
            self.targetFace.Area, 0.0
        ):
            PathLog.debug("StrategyClearing: No area in working shape.")
            return False

        self.targetFace.translate(
            FreeCAD.Vector(0.0, 0.0, 0.0 - self.targetFace.BoundBox.ZMin)
        )

        #  Apply simple radius shrinking offset for clearing pattern generation.
        ofstVal = self.offsetDirection * (
            self.toolRadius - (self.jobTolerance / 5.0) + self.materialAllowance
        )
        offsetWF = PathUtils.getOffsetArea(self.targetFace, ofstVal)
        if offsetWF and len(offsetWF.Faces) > 0:
            for f in offsetWF.Faces:
                self.face = f
                self._prepareAttributes()
                pathGeom = self._generatePathGeometry()
                self.pathGeometry.extend(pathGeom)

        PathLog.debug("Path with params: {}".format(self.pathParams))

        return True


# Eclass
