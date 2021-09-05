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

from PySide import QtCore

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")
PathUtils = LazyLoader("PathScripts.PathUtils", globals(), "PathScripts.PathUtils")
# area = LazyLoader('area', globals(), 'area')


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


class StrategyProfile:
    """class StrategyProfile"""

    def __init__(
        self,
        shape,
        isHole,
        startPoint,
        getsim,
        depthparams,
        horizFeed,
        vertFeed,
        endVector,
        radius,
        opFeatures,
        safeHeight,
        clearanceHeight,
        stockAllowance,
        direction,
        cutSide,
        useToolCompensation,
        joinType,
        miterLimit,
        opUseProjection,
        opRetractTool,
    ):
        PathLog.debug("StrategyProfile.__init__()")

        self.opUseProjection = opUseProjection
        self.opRetractTool = opRetractTool
        self.shape = shape
        self.isHole = isHole
        self.startPoint = startPoint
        self.getsim = getsim
        self.resume_height = safeHeight  # obj.SafeHeight.Value
        self.retraction = clearanceHeight  # obj.ClearanceHeight.Value
        self.depthparams = depthparams
        self.horizFeed = horizFeed
        self.vertFeed = vertFeed
        self.endVector = endVector
        self.radius = radius
        self.opFeatures = opFeatures
        self.stockAllowance = stockAllowance
        self.direction = direction
        self.cutSide = cutSide
        self.useToolCompensation = useToolCompensation
        self.joinType = joinType
        self.miterLimit = miterLimit
        self.simObj = None
        self.pathParams = ""
        self.areaParams = ""
        self.commandList = list()

        """
        @Sliptonic summation per chat on 7 June 2021
        def generateProfile (
                face,
                toolController,
                startPoint=None,
                endPoint=None,
                materialAllowance=0,
                direction = 'Climb',
                cutSide = 'INSIDE',
                useToolCompensation,
                joinType=somedefault,
                miterLimit=somedefault,
                opRetractTool  # research / rename
                ):
        """

    # Private methods
    def _getAreaParams(self):
        """_getAreaParams() ... returns dictionary with area parameters."""
        params = {}
        params["Fill"] = 0
        params["Coplanar"] = 0
        params["SectionCount"] = -1

        offset = self.stockAllowance  # 0.0
        if self.useToolCompensation:
            offset = self.radius + self.stockAllowance
        if self.cutSide == "Inside":
            offset = 0 - offset
        if self.isHole:
            offset = 0 - offset
        params["Offset"] = offset

        jointype = ["Round", "Square", "Miter"]
        params["JoinType"] = jointype.index(self.joinType)

        if self.joinType == "Miter":
            params["MiterLimit"] = self.miterLimit

        return params

    def _getPathParams(self):
        """_getPathParams() ... returns dictionary with path parameters."""
        params = {}

        # Reverse the direction for holes
        if self.isHole:
            direction = "Climb" if self.direction == "Conventional" else "Conventional"
        else:
            direction = self.direction

        orientation = ["Conventional", "Climb"]
        params["orientation"] = orientation.index(self.direction)

        if not self.useToolCompensation:
            if direction == "Conventional":
                params["orientation"] = 1
            else:
                params["orientation"] = 0

        return params

    # Public methods
    def generateCommands(self):
        """generateCommands() ... public function to generate gcode for path area shape."""
        PathLog.debug("StrategyProfile.generateCommands()")

        area = Path.Area()
        area.setPlane(PathUtils.makeWorkplane(self.shape))
        area.add(self.shape)

        areaParams = self._getAreaParams()  # pylint: disable=assignment-from-no-return

        heights = [i for i in self.depthparams]
        PathLog.debug("depths: {}".format(heights))
        area.setParams(**areaParams)
        self.areaParams = str(area.getParams())

        PathLog.debug("Area with params: {}".format(area.getParams()))

        sections = area.makeSections(
            mode=0, project=self.opUseProjection, heights=heights
        )
        PathLog.debug("sections = %s" % sections)
        shapelist = [sec.getShape() for sec in sections]
        PathLog.debug("shapelist = %s" % shapelist)

        pathParams = self._getPathParams()  # pylint: disable=assignment-from-no-return
        pathParams["shapes"] = shapelist
        pathParams["feedrate"] = self.horizFeed
        pathParams["feedrate_v"] = self.vertFeed
        pathParams["verbose"] = True
        pathParams["resume_height"] = self.resume_height
        pathParams["retraction"] = self.retraction
        pathParams["return_end"] = True
        # Note that emitting preambles between moves breaks some dressups and prevents path optimization on some controllers
        pathParams["preamble"] = False

        # if not self.opRetractTool:
        #    pathParams['threshold'] = 2.001 * self.radius

        if self.endVector is not None:
            pathParams["start"] = self.endVector
        elif self.startPoint:
            pathParams["start"] = self.startPoint

        self.pathParams = str(
            {key: value for key, value in pathParams.items() if key != "shapes"}
        )
        PathLog.debug("Path with params: {}".format(self.pathParams))

        (pp, end_vector) = Path.fromShapes(**pathParams)
        PathLog.debug("pp: {}, end vector: {}".format(pp, end_vector))
        self.endVector = end_vector  # pylint: disable=attribute-defined-outside-init

        if self.getsim:
            areaParams["Thicken"] = True
            areaParams["ToolRadius"] = self.radius - self.radius * 0.005
            area.setParams(**areaParams)
            sec = area.makeSections(mode=0, project=False, heights=heights)[
                -1
            ].getShape()
            self.simObj = sec.extrude(FreeCAD.Vector(0, 0, self.shape.BoundBox.ZMax))

        self.commandList = pp.Commands

    def getSimObj(self):
        return self.simObj


# Eclass


class StrategyProfileOpenEdge:
    """class StrategyProfileOpenEdge"""

    def __init__(
        self,
        edgeList,
        startPoint,
        depthparams,
        horizFeed,
        vertFeed,
        endVector,
        radius,
        safeHeight,
        clearanceHeight,
        direction,
    ):
        PathLog.debug("StrategyProfileOpenEdge.__init__()")

        self.edgeList = edgeList
        self.startPoint = startPoint
        self.resume_height = safeHeight  # obj.SafeHeight.Value
        self.retraction = clearanceHeight  # obj.ClearanceHeight.Value
        self.depthparams = depthparams
        self.horizFeed = horizFeed
        self.vertFeed = vertFeed
        self.endVector = endVector
        self.radius = radius
        self.direction = direction
        self.simObj = None
        self.pathParams = ""
        self.areaParams = ""
        self.commandList = list()

    # Public methods
    def generateCommands(self):
        """generateCommands() ... internal function."""
        PathLog.debug("StrategyProfileOpenEdge.generateCommands()")

        commands = []
        heights = [i for i in self.depthparams]
        PathLog.debug("depths: {}".format(heights))
        end_vector = None
        cntHeights = len(heights)

        for i in range(0, cntHeights):
            end_vector = None
            start = None
            for wire in self.edgeList:
                # Create new wire from edges and translate to height
                hWire = Part.Wire(Part.__sortEdges__(wire.Edges))
                hWire.translate(FreeCAD.Vector(0, 0, heights[i] - hWire.BoundBox.ZMin))

                pathParams = {}  # pylint: disable=assignment-from-no-return
                pathParams["shapes"] = [hWire]
                pathParams["feedrate"] = self.horizFeed
                pathParams["feedrate_v"] = self.vertFeed
                pathParams["verbose"] = True
                pathParams["resume_height"] = self.resume_height
                pathParams["retraction"] = self.retraction
                pathParams["return_end"] = True
                # Note that emitting preambles between moves breaks some dressups and prevents path optimization on some controllers
                pathParams["preamble"] = False

                if True or self.endVector is None:
                    vrtxs = hWire.Wires[0].Vertexes
                    lv = len(vrtxs) - 1
                    pathParams["start"] = FreeCAD.Vector(
                        vrtxs[0].X, vrtxs[0].Y, vrtxs[0].Z
                    )
                    if self.direction == "Conventional":
                        pathParams["start"] = FreeCAD.Vector(
                            vrtxs[lv].X, vrtxs[lv].Y, vrtxs[lv].Z
                        )
                    if not start:
                        start = pathParams["start"]
                else:
                    pathParams["start"] = self.endVector

                self.pathParams = str(
                    {key: value for key, value in pathParams.items() if key != "shapes"}
                )
                PathLog.debug("Path with params: {}".format(self.pathParams))

                (pp, end_vector) = Path.fromShapes(**pathParams)
                commands.extend(pp.Commands)
                commands.append(Path.Command("G0", {"Z": self.resume_height}))
                PathLog.debug("pp: {}, end vector: {}".format(pp, end_vector))
            # Efor
            if i < cntHeights - 1:
                commands.append(Path.Command("G0", {"X": start.x, "Y": start.y}))

        self.endVector = end_vector

        self.commandList = commands


# Eclass
